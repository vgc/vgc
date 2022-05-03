// Copyright 2021 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vgc/widgets/openglviewer.h>

#include <cassert>
#include <cmath>

#include <QBitmap>
#include <QMouseEvent>
#include <QPainter>

#include <vgc/core/assert.h>
#include <vgc/core/os.h>
#include <vgc/core/paths.h>
#include <vgc/core/stopwatch.h>
#include <vgc/core/vec2f.h>
#include <vgc/core/vec2farray.h>
#include <vgc/geometry/curve.h>
#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {


namespace {

// Returns the file path of a shader file as a QString
QString shaderPath_(const std::string& name) {
    std::string path = core::resourcePath("graphics/opengl/" + name);
    return toQt(path);
}

QMatrix4x4 toQtMatrix(const core::Mat4d& m) {
    return QMatrix4x4(
               static_cast<float>(m(0,0)), static_cast<float>(m(0,1)), static_cast<float>(m(0,2)), static_cast<float>(m(0,3)),
               static_cast<float>(m(1,0)), static_cast<float>(m(1,1)), static_cast<float>(m(1,2)), static_cast<float>(m(1,3)),
               static_cast<float>(m(2,0)), static_cast<float>(m(2,1)), static_cast<float>(m(2,2)), static_cast<float>(m(2,3)),
               static_cast<float>(m(3,0)), static_cast<float>(m(3,1)), static_cast<float>(m(3,2)), static_cast<float>(m(3,3)));
}

double width_(const PointingDeviceEvent& event) {
    const double defaultWidth = 6.0;
    return event.hasPressure() ?
               2 * event.pressure() * defaultWidth :
               defaultWidth;
}

core::StringId PATH("path");
core::StringId POSITIONS("positions");
core::StringId WIDTHS("widths");
core::StringId COLOR("color");

void drawCrossCursor(QPainter& painter) {
    painter.setPen(QPen(Qt::color1, 1.0));
    painter.drawLine(16, 0, 16, 10);
    painter.drawLine(16, 22, 16, 32);
    painter.drawLine(0, 16, 10, 16);
    painter.drawLine(22, 16, 32, 16);
    painter.drawPoint(16, 16);
}

QCursor crossCursor() {

    // Draw bitmap
    QBitmap bitmap(32, 32);
    QPainter bitmapPainter(&bitmap);
    bitmapPainter.fillRect(0, 0, 32, 32, QBrush(Qt::color0));
    drawCrossCursor(bitmapPainter);

    // Draw mask
    QBitmap mask(32, 32);
    QPainter maskPainter(&mask);
    maskPainter.fillRect(0, 0, 32, 32, QBrush(Qt::color0));
#ifndef VGC_CORE_OS_WINDOWS
    // Make the cursor color XOR'd on Windows, black on other platforms. Ideally,
    // we'd prefer XOR'd on all platforms, but it's only supported on Windows.
    // See Qt doc for QCursor(const QBitmap &bitmap, const QBitmap &mask).
    drawCrossCursor(maskPainter);
#endif

    // Create and return cursor
    return QCursor(bitmap, mask);
}

} // namespace

void OpenGLViewer::init()
{
    // Note:
    //
    // Performance seems to be significantly impacted by multisample
    // antialiasing (MSAA), which is controlled by format.setSamples(n).
    //
    // Ideally, we may want to implement antialiasing either via FXAA/MLAA
    // (i.e. as a post-processing step), or since we are 2D, by generating
    // special thin "blurry" geometry at the boundary of objects, which may
    // provide better performance.
    //
    // In the meantime, since none of the above is currently implemented, we do
    // use MSAA which is trivial to enable/implement. Not using antialiasing at
    // all (or with too few samples like 2 or 4) makes the lines quite ugly,
    // but more importantly, makes the text almost unreadable (since it's
    // currently rendered as triangles rather than textured quads or distance
    // fields). Using setSamples(8) seems like a good trade-off for now.
    //
    // Note that to disable MSAA, you need to call setSamples(0). Calling
    // setSamples(1) instead does NOT disable MSAA, but surprisingly gives the
    // same result as setSamples(2).

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(8);
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
}

OpenGLViewer::OpenGLViewer(dom::Document* document, QWidget* parent) :
    QOpenGLWidget(parent),
    document_(document),
    isSketching_(false),
    isPanning_(false),
    isRotating_(false),
    isZooming_(false),
    mousePressed_(false),
    tabletPressed_(false),
    polygonMode_(2),
    showControlPoints_(false),
    requestedTesselationMode_(2),
    currentTesselationMode_(2),
    renderTask_("Render"),
    updateTask_("Update"),
    drawTask_("Draw")
{
    // Set ClickFocus policy to be able to accept keyboard events (default
    // policy is NoFocus).
    setFocusPolicy(Qt::ClickFocus);

    // Set cursor. For now we assume that we are in a drawing tool, and
    // therefore use a cross cursor. In the future, each tool should specify
    // which cursor should be drawn in the viewer.
    setCursor(crossCursor());
}

void OpenGLViewer::setDocument(dom::Document* document)
{
    makeCurrent();
    cleanupGL();
    doneCurrent();

    document_ = document;
    update();
}

OpenGLViewer::~OpenGLViewer()
{
    // Make OpenGL context current and call cleanupGL()
    makeCurrent();
    cleanupGL();
    doneCurrent();
}

QSize OpenGLViewer::minimumSizeHint() const
{
    return QSize(160, 120);
}

void OpenGLViewer::startLoggingUnder(core::PerformanceLog* parent)
{
    core::PerformanceLog* renderLog = renderTask_.startLoggingUnder(parent);
    updateTask_.startLoggingUnder(renderLog);
    drawTask_.startLoggingUnder(renderLog);
}

void OpenGLViewer::stopLoggingUnder(core::PerformanceLog* parent)
{
    core::PerformanceLogPtr renderLog = renderTask_.stopLoggingUnder(parent);
    updateTask_.stopLoggingUnder(renderLog.get());
    drawTask_.stopLoggingUnder(renderLog.get());
}

void OpenGLViewer::mousePressEvent(QMouseEvent* event)
{
    if (mousePressed_ || tabletPressed_) {
        return;
    }
    mousePressed_ = true;
    pointingDeviceButtonAtPress_ = event->button();
    pointingDevicePress(PointingDeviceEvent(event));
}

void OpenGLViewer::mouseMoveEvent(QMouseEvent* event)
{
    if (!mousePressed_) {
        return;
    }
    pointingDeviceMove(PointingDeviceEvent(event));
}

void OpenGLViewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (!mousePressed_ || pointingDeviceButtonAtPress_ != event->button()) {
        return;
    }
    pointingDeviceRelease(PointingDeviceEvent(event));
    mousePressed_ = false;
}

void OpenGLViewer::tabletEvent(QTabletEvent* event)
{
    switch(event->type()) {
    case QEvent::TabletPress:
        if (mousePressed_ || tabletPressed_) {
            return;
        }
        tabletPressed_ = true;
        pointingDeviceButtonAtPress_ = event->button();
        pointingDevicePress(PointingDeviceEvent(event));
        break;

    case QEvent::TabletMove:
        if (!tabletPressed_) {
            return;
        }
        pointingDeviceMove(PointingDeviceEvent(event));
        break;

    case QEvent::TabletRelease:
        if (!tabletPressed_ || pointingDeviceButtonAtPress_ != event->button()) {
            return;
        }
        pointingDeviceRelease(PointingDeviceEvent(event));
        tabletPressed_ = false;
        break;

    default:
        // nothing
        break;
    }
}

void OpenGLViewer::pointingDevicePress(const PointingDeviceEvent& event)
{
    if (isSketching_ || isPanning_ || isRotating_ || isZooming_) {
        return;
    }

    if (event.modifiers() == Qt::NoModifier &&
        event.button() == Qt::LeftButton)
    {
        isSketching_ = true;
        // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
        // and should be cached), but let's keep it like this for now for testing.
        core::Vec2d viewCoords = event.pos();
        core::Vec2d worldCoords = camera_.viewMatrix().inverted() * viewCoords;
        startCurve_(worldCoords, width_(event));
    }
    else if (event.modifiers() == Qt::AltModifier &&
             event.button() == Qt::LeftButton)
    {
        isRotating_ = true;
        pointingDevicePosAtPress_ = event.pos();
        cameraAtPress_ = camera_;
    }
    else if (event.modifiers() == Qt::AltModifier &&
             event.button() == Qt::MiddleButton)
    {
        isPanning_ = true;
        pointingDevicePosAtPress_ = event.pos();
        cameraAtPress_ = camera_;
    }
    else if (event.modifiers() == Qt::AltModifier &&
             event.button() == Qt::RightButton)
    {
        isZooming_ = true;
        pointingDevicePosAtPress_ = event.pos();
        cameraAtPress_ = camera_;
    }
}

void OpenGLViewer::pointingDeviceMove(const PointingDeviceEvent& event)
{
    // Note: event.button() is always NoButton for move events. This is why
    // we use the variable isPanning_ and isSketching_ to remember the current
    // mouse action. In the future, we'll abstract this mechanism in a separate
    // class.

    if (isSketching_) {
        // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
        // and should be cached), but let's keep it like this for now for testing.
        core::Vec2d viewCoords = event.pos();
        core::Vec2d worldCoords = camera_.viewMatrix().inverted() * viewCoords;
        continueCurve_(worldCoords, width_(event));
    }
    else if (isPanning_) {
        core::Vec2d mousePos = event.pos();
        core::Vec2d delta = pointingDevicePosAtPress_ - mousePos;
        camera_.setCenter(cameraAtPress_.center() + delta);
        update();
    }
    else if (isRotating_) {
        // Set new camera rotation
        // XXX rotateViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double rotateViewSensitivity = 0.01;
        core::Vec2d mousePos = event.pos();
        core::Vec2d deltaPos = pointingDevicePosAtPress_ - mousePos;
        double deltaRotation = rotateViewSensitivity * (deltaPos.x() - deltaPos.y());
        camera_.setRotation(cameraAtPress_.rotation() + deltaRotation);

        // Set new camera center so that rotation center = mouse pos at press
        core::Vec2d pivotViewCoords = pointingDevicePosAtPress_;
        core::Vec2d pivotWorldCoords = cameraAtPress_.viewMatrix().inverted() * pivotViewCoords;
        core::Vec2d pivotViewCoordsNow = camera_.viewMatrix() * pivotWorldCoords;
        camera_.setCenter(camera_.center() - pivotViewCoords + pivotViewCoordsNow);

        update();
    }
    else if (isZooming_) {
        // Set new camera zoom
        // XXX zoomViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double zoomViewSensitivity = 0.005;
        core::Vec2d mousePos = event.pos();
        core::Vec2d deltaPos = pointingDevicePosAtPress_ - mousePos;
        const double s = std::exp(zoomViewSensitivity * (deltaPos.y() - deltaPos.x()));
        camera_.setZoom(cameraAtPress_.zoom() * s);

        // Set new camera center so that zoom center = mouse pos at press
        core::Vec2d pivotViewCoords = pointingDevicePosAtPress_;
        core::Vec2d pivotWorldCoords = cameraAtPress_.viewMatrix().inverted() * pivotViewCoords;
        core::Vec2d pivotViewCoordsNow = camera_.viewMatrix() * pivotWorldCoords;
        camera_.setCenter(camera_.center() - pivotViewCoords + pivotViewCoordsNow);

        update();
    }
}

void OpenGLViewer::pointingDeviceRelease(const PointingDeviceEvent&)
{
    isSketching_ = false;
    isRotating_ = false;
    isPanning_ = false;
    isZooming_ = false;
}

void OpenGLViewer::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_N:
        polygonMode_ = 0;
        update();
        break;
    case Qt::Key_T:
        polygonMode_ = 1;
        update();
        break;
    case Qt::Key_F:
        polygonMode_ = 2;
        update();
        break;
    case Qt::Key_I:
        requestedTesselationMode_ = 0;
        update();
        break;
    case Qt::Key_U:
        requestedTesselationMode_ = 1;
        update();
        break;
    case Qt::Key_A:
        requestedTesselationMode_ = 2;
        update();
        break;
    case Qt::Key_P:
        showControlPoints_ = !showControlPoints_;
        update();
        break;
    default:
        break;
    }

    // Don't factor out "update()" here, to avoid unnecessary redraws for keys
    // not handled here, including modifiers.
}

OpenGLViewer::OpenGLFunctions* OpenGLViewer::openGLFunctions() const
{
    return context()->versionFunctions<OpenGLFunctions>();
}

void OpenGLViewer::initializeGL()
{
    OpenGLFunctions* f = openGLFunctions();

    // Initialize shader program
    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Vertex, shaderPath_("shader.v.glsl"));
    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Fragment, shaderPath_("shader.f.glsl"));
    shaderProgram_.link();

    // Get shader locations
    shaderProgram_.bind();
    vertexLoc_     = shaderProgram_.attributeLocation("vertex");
    projMatrixLoc_ = shaderProgram_.uniformLocation("projMatrix");
    viewMatrixLoc_ = shaderProgram_.uniformLocation("viewMatrix");
    colorLoc_      = shaderProgram_.uniformLocation("color");
    shaderProgram_.release();

    // Set clear color
    f->glClearColor(1, 1, 1, 1);
}

void OpenGLViewer::resizeGL(int w, int h)
{
    camera_.setViewportSize(w, h);
}

void OpenGLViewer::paintGL()
{
    // Measure rendering time
    renderTask_.start();

    OpenGLFunctions* f = openGLFunctions();

    // Transfer to GPU any data out-of-sync with CPU
    updateTask_.start();
    updateGLResources_();
    updateTask_.stop();

    drawTask_.start();

    // Clear color and depth buffer
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind shader program
    shaderProgram_.bind();

    // Set uniform values
    shaderProgram_.setUniformValue(projMatrixLoc_, toQtMatrix(camera_.projectionMatrix()));
    shaderProgram_.setUniformValue(viewMatrixLoc_, toQtMatrix(camera_.viewMatrix()));

    // Draw triangles
    if (polygonMode_ > 0) {
        f->glPolygonMode(GL_FRONT_AND_BACK, (polygonMode_ == 1) ? GL_LINE : GL_FILL);
        for (CurveGLResources& r: curveGLResources_) {
            shaderProgram_.setUniformValue(
                        colorLoc_,
                        static_cast<float>(r.trianglesColor.r()),
                        static_cast<float>(r.trianglesColor.g()),
                        static_cast<float>(r.trianglesColor.b()),
                        static_cast<float>(r.trianglesColor.a()));
            r.vaoTriangles->bind();
            f->glDrawArrays(GL_TRIANGLE_STRIP, 0, r.numVerticesTriangles);
            r.vaoTriangles->release();
        }
    }

    // Draw control points
    if (showControlPoints_) {
        shaderProgram_.setUniformValue(colorLoc_, 1.0f, 0.0f, 0.0f, 1.0f);
        f->glPointSize(10.0);
        for (CurveGLResources& r: curveGLResources_) {
            r.vaoControlPoints->bind();
            f->glDrawArrays(GL_POINTS, 0, r.numVerticesControlPoints);
            r.vaoControlPoints->release();
        }
    }

    // Release shader program
    shaderProgram_.release();
    drawTask_.stop();

    // Complete measure of rendering time
    renderTask_.stop();

    // Inform that the render is completed
    Q_EMIT renderCompleted();
}

void OpenGLViewer::cleanupGL()
{
    Int nCurvesInGpu = curveGLResources_.length();
    for (Int i = nCurvesInGpu - 1; i >= 0; --i) {
        destroyCurveGLResources_(i);
    }
}

void OpenGLViewer::updateGLResources_()
{
    // Create new GPU resources for new curves
    // XXX CLEAN + don't assume all elements are paths
    // + make constant-time complexity
    dom::Element* root = document_->rootElement();
    Int nCurvesInCpu = 0;
    for (dom::Node* node : root->children()) {
        if (dom::Element::cast(node)) {
            ++nCurvesInCpu;
        }
    }
    paths_.resize(nCurvesInCpu);
    int i = 0;
    for (dom::Node* node : root->children()) {
        if (dom::Element* path = dom::Element::cast(node)) {
            paths_[i] = path;
            ++i;
        }
    }
    Int nCurvesInGpu = curveGLResources_.length();
    for (Int i = nCurvesInGpu; i < nCurvesInCpu; ++i) {
        createCurveGLResources_(i);
    }

    // Destroy GPU resources for deleted curves
    // XXX why the decreasing loop index?
    for (Int i = nCurvesInGpu - 1; i >= nCurvesInCpu; --i) {
        destroyCurveGLResources_(i);
    }

    // Now the number of curve in GPU and CPU is the same
    nCurvesInGpu = nCurvesInCpu;

    // Retesselate all existing curves. This is overkill but safe.
    //
    // XXX TODO Avoid retesselating unchanged curves by querying the scene
    // about which curves have changed.
    //
    bool dontKnowWhichCurvesHaveChanged = true;
    bool tesselationModeChanged = requestedTesselationMode_ != currentTesselationMode_;
    if (dontKnowWhichCurvesHaveChanged || tesselationModeChanged) {
        currentTesselationMode_ = requestedTesselationMode_;
        for (Int i = 0; i < nCurvesInGpu; ++i) {
            updateCurveGLResources_(i);
        }
    }
}

void OpenGLViewer::createCurveGLResources_(Int)
{
    curveGLResources_.append(CurveGLResources());
    CurveGLResources& r = curveGLResources_.last();

    OpenGLFunctions* f = openGLFunctions();
    GLuint vertexLoc = static_cast<GLuint>(vertexLoc_);

    // Create VBO/VAO for rendering triangles
    r.vboTriangles.create();
    r.vaoTriangles = new QOpenGLVertexArrayObject();
    r.vaoTriangles->create();
    GLsizei stride  = sizeof(core::Vec2f);
    GLvoid* pointer = nullptr;
    r.vaoTriangles->bind();
    r.vboTriangles.bind();
    f->glEnableVertexAttribArray(vertexLoc);
    f->glVertexAttribPointer(
                vertexLoc, // index of the generic vertex attribute
                2,         // number of components (x and y components)
                GL_FLOAT,  // type of each component
                GL_FALSE,  // should it be normalized
                stride,    // byte offset between consecutive vertex attributes
                pointer);  // byte offset between the first attribute and the pointer given to allocate()
    r.vboTriangles.release();
    r.vaoTriangles->release();

    // Setup VBO/VAO for rendering control points
    r.vboControlPoints.create();
    r.vaoControlPoints = new QOpenGLVertexArrayObject();
    r.vaoControlPoints->create();
    r.vaoControlPoints->bind();
    r.vboControlPoints.bind();
    f->glEnableVertexAttribArray(vertexLoc);
    f->glVertexAttribPointer(
                vertexLoc, // index of the generic vertex attribute
                2,         // number of components (x and y components)
                GL_FLOAT,  // type of each component
                GL_FALSE,  // should it be normalized
                stride,    // byte offset between consecutive vertex attributes
                pointer);  // byte offset between the first attribute and the pointer given to allocate()
    r.vboControlPoints.release();
    r.vaoControlPoints->release();
}

void OpenGLViewer::updateCurveGLResources_(Int i)
{
    assert(i >= 0); // TODO convert all asserts to VGC_CORE_ASSERT
    assert(i < curveGLResources_.length());
    //assert(i < scene_->curves().size());
    CurveGLResources& r = curveGLResources_[i];

    // Convert the dom::Path to a geometry::Curve
    // XXX move this logic to dom::Path
    dom::Element* path = paths_[i];
    core::Vec2dArray positions = path->getAttribute(POSITIONS).getVec2dArray();
    core::DoubleArray widths = path->getAttribute(WIDTHS).getDoubleArray();
    core::Color color = path->getAttribute(COLOR).getColor();
    VGC_CORE_ASSERT(positions.size() == widths.size());
    Int nControlPoints = positions.length();
    geometry::Curve curve;
    curve.setColor(color);
    for (Int i = 0; i < nControlPoints; ++i) {
        curve.addControlPoint(positions[i], widths[i]);
    }

    // Triangulate the curve
    double maxAngle = 0.05;
    int minQuads = 1;
    int maxQuads = 64;
    if (requestedTesselationMode_ == 0) {
        maxQuads = 1;
    }
    else if (requestedTesselationMode_ == 1) {
        minQuads = 10;
        maxQuads = 10;
    }
    core::Vec2dArray triangulation =
            curve.triangulate(maxAngle, minQuads, maxQuads);

    // Convert triangles to single-precision and transfer to GPU
    //
    // XXX For the doubles to floats, we should either:
    //     - have a public helper function to do this
    //     - directly compute the triangulation using floats (although
    //       using doubles is more precise for intersection tests)
    //
    r.numVerticesTriangles = core::int_cast<GLsizei>(triangulation.length());
    core::Vec2fArray glVerticesTriangles;
    for(const core::Vec2d& v: triangulation) {
        glVerticesTriangles.append(core::Vec2f(static_cast<float>(v[0]),
                                               static_cast<float>(v[1])));
    }
    r.vboTriangles.bind();
    int count = core::int_cast<int>(r.numVerticesTriangles) * static_cast<int>(sizeof(core::Vec2f));
    r.vboTriangles.allocate(glVerticesTriangles.data(), count);
    r.vboTriangles.release();

    // Transfer control points vertex data to GPU
    core::Vec2fArray glVerticesControlPoints;
    const core::DoubleArray& d = curve.positionData();
    r.numVerticesControlPoints = core::int_cast<GLsizei>(d.length() / 2);
    for (Int i = 0; i < r.numVerticesControlPoints; ++i) {
        glVerticesControlPoints.append(core::Vec2f(static_cast<float>(d[2*i]),
                                                   static_cast<float>(d[2*i+1])));
    }
    r.vboControlPoints.bind();
    count = core::int_cast<int>(r.numVerticesControlPoints) * static_cast<int>(sizeof(core::Vec2f));
    r.vboControlPoints.allocate(glVerticesControlPoints.data(), count);
    r.vboControlPoints.release();

    // Set color
    r.trianglesColor = curve.color();
}

void OpenGLViewer::destroyCurveGLResources_(Int)
{
    assert(curveGLResources_.size() > 0);
    CurveGLResources& r = curveGLResources_.last();

    r.vaoTriangles->destroy();
    delete r.vaoTriangles;
    r.vboTriangles.destroy();

    r.vaoControlPoints->destroy();
    delete r.vaoControlPoints;
    r.vboControlPoints.destroy();

    curveGLResources_.removeLast();
}

void OpenGLViewer::startCurve_(const core::Vec2d& p, double width)
{
    // XXX CLEAN

    dom::Element* root = document_->rootElement();
    dom::Element* path = dom::Element::create(root, PATH);

    path->setAttribute(POSITIONS, core::Vec2dArray());
    path->setAttribute(WIDTHS, core::DoubleArray());
    path->setAttribute(COLOR, currentColor_);

    continueCurve_(p, width);
}

void OpenGLViewer::continueCurve_(const core::Vec2d& p, double width)
{
    // XXX CLEAN

    dom::Element* root = document_->rootElement();
    dom::Element* path = dom::Element::cast(root->lastChild()); // I really need the casted version like lastChildElement()

    if (path) {
        // Should I make this more efficient? If so, we have a few choices:
        // duplicate the API of arrays within value and provide fine-grain
        // "changed" signals. And/or allow to pass a lambda that modifies the
        // underlying value. The dom::Value will call the lambda to mutate the
        // value, then emit a generic changed signal. I could also let clients
        // freely mutate the value and trusteing them in sending a changed
        // signal themselves.

        core::Vec2dArray positions = path->getAttribute(POSITIONS).getVec2dArray();
        core::DoubleArray widths = path->getAttribute(WIDTHS).getDoubleArray();

        positions.append(p);
        widths.append(width);

        path->setAttribute(POSITIONS, positions);
        path->setAttribute(WIDTHS, widths);

        update();
    }
}


} // namespace widgets
} // namespace vgc
