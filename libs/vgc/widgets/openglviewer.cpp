// Copyright 2018 The VGC Developers
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

#include <QMouseEvent>

#include <vgc/core/assert.h>
#include <vgc/core/resources.h>
#include <vgc/core/stopwatch.h>
#include <vgc/geometry/curve.h>
#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {

namespace {

// Returns the file path of a shader file as a QString
QString shaderPath_(const std::string& name) {
    std::string path = core::resourcePath("opengl/shaders/" + name);
    return toQt(path);
}

QMatrix4x4 toQtMatrix(const core::Mat4d& m) {
    return QMatrix4x4(
               (float)m(0,0), (float)m(0,1), (float)m(0,2), (float)m(0,3),
               (float)m(1,0), (float)m(1,1), (float)m(1,2), (float)m(1,3),
               (float)m(2,0), (float)m(2,1), (float)m(2,2), (float)m(2,3),
               (float)m(3,0), (float)m(3,1), (float)m(3,2), (float)m(3,3));
}

// XXX TODO implement and use core::Vec2f instead.
struct GLVertex {
    float x, y;
    GLVertex(float x, float y) : x(x), y(y) {}
};

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

} // namespace

void OpenGLViewer::init()
{
    // Note:
    // Performance seems to be significantly impacted by format.setSamples().
    // For now I keep setSamples(1). May change to 4 or 16 after investigation.
    // Should probably be a user preference.

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(1);
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

void OpenGLViewer::startLoggingUnder(core::PerformanceLog* parent)
{
    core::PerformanceLog* renderLog = renderTask_.startLoggingUnder(parent);
    updateTask_.startLoggingUnder(renderLog);
    drawTask_.startLoggingUnder(renderLog);
}

void OpenGLViewer::stopLoggingUnder(core::PerformanceLog* parent)
{
    core::PerformanceLogSharedPtr renderLog = renderTask_.stopLoggingUnder(parent);
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
        core::Vec2d worldCoords = camera_.viewMatrix().inverse() * viewCoords;
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
             event.button() == Qt::MidButton)
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
        core::Vec2d worldCoords = camera_.viewMatrix().inverse() * viewCoords;
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
        core::Vec2d pivotWorldCoords = cameraAtPress_.viewMatrix().inverse() * pivotViewCoords;
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
        core::Vec2d pivotWorldCoords = cameraAtPress_.viewMatrix().inverse() * pivotViewCoords;
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

#undef near
#undef far

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
                        (float)r.trianglesColor.r(),
                        (float)r.trianglesColor.g(),
                        (float)r.trianglesColor.b(),
                        (float)r.trianglesColor.a());
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
    int nCurvesInGpu = curveGLResources_.size();
    for (int i = nCurvesInGpu - 1; i >= 0; --i) {
        destroyCurveGLResources_(i);
    }
}

void OpenGLViewer::updateGLResources_()
{
    // Create new GPU resources for new curves
    // XXX CLEAN + don't assume all elements are paths
    // + make constant-time complexity
    dom::Element* root = document_->rootElement();
    int nCurvesInCpu = 0;
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
    int nCurvesInGpu = curveGLResources_.size();
    for (int i = nCurvesInGpu; i < nCurvesInCpu; ++i) {
        createCurveGLResources_(i);
    }

    // Destroy GPU resources for deleted curves
    // XXX why the decreasing loop index?
    for (int i = nCurvesInGpu - 1; i >= nCurvesInCpu; --i) {
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
        for (int i = 0; i < nCurvesInGpu; ++i) {
            updateCurveGLResources_(i);
        }
    }
}

void OpenGLViewer::createCurveGLResources_(int)
{
    curveGLResources_.push_back(CurveGLResources());
    CurveGLResources& r = curveGLResources_.back();

    OpenGLFunctions* f = openGLFunctions();

    // Create VBO/VAO for rendering triangles
    r.vboTriangles.create();
    r.vaoTriangles = new QOpenGLVertexArrayObject();
    r.vaoTriangles->create();
    GLsizei stride  = sizeof(GLVertex);
    GLvoid* pointer = reinterpret_cast<void*>(offsetof(GLVertex, x));
    r.vaoTriangles->bind();
    r.vboTriangles.bind();
    f->glEnableVertexAttribArray(vertexLoc_);
    f->glVertexAttribPointer(
                vertexLoc_, // index of the generic vertex attribute
                2,          // number of components (x and y components)
                GL_FLOAT,   // type of each component
                GL_FALSE,   // should it be normalized
                stride,     // byte offset between consecutive vertex attributes
                pointer);   // byte offset between the first attribute and the pointer given to allocate()
    r.vboTriangles.release();
    r.vaoTriangles->release();

    // Setup VBO/VAO for rendering control points
    r.vboControlPoints.create();
    r.vaoControlPoints = new QOpenGLVertexArrayObject();
    r.vaoControlPoints->create();
    r.vaoControlPoints->bind();
    r.vboControlPoints.bind();
    f->glEnableVertexAttribArray(vertexLoc_);
    f->glVertexAttribPointer(
                vertexLoc_, // index of the generic vertex attribute
                2,          // number of components   (x and y components)
                GL_FLOAT,   // type of each component
                GL_FALSE,   // should it be normalized
                stride,     // byte offset between consecutive vertex attributes
                pointer);   // byte offset between the first attribute and the pointer given to allocate()
    r.vboControlPoints.release();
    r.vaoControlPoints->release();
}

void OpenGLViewer::updateCurveGLResources_(int i)
{
    assert(i >= 0); // TODO convert all asserts to VGC_CORE_ASSERT
    assert(i < curveGLResources_.size());
    //assert(i < scene_->curves().size());
    CurveGLResources& r = curveGLResources_[i];

    // Convert the dom::Path to a geometry::Curve
    // XXX move this logic to dom::Path
    dom::Element* path = paths_[i];
    core::Vec2dArray positions = path->getAttribute(POSITIONS).getVec2dArray();
    core::DoubleArray widths = path->getAttribute(WIDTHS).getDoubleArray();
    core::Color color = path->getAttribute(COLOR).getColor();
    VGC_CORE_ASSERT(positions.size() == widths.size());
    int nControlPoints = positions.size();
    geometry::Curve curve;
    curve.setColor(color);
    for (int i = 0; i < nControlPoints; ++i) {
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
    std::vector<core::Vec2d> triangulation =
            curve.triangulate(maxAngle, minQuads, maxQuads);

    // Convert triangles to single-precision and transfer to GPU
    r.numVerticesTriangles = triangulation.size();
    std::vector<GLVertex> glVerticesTriangles;
    for(const core::Vec2d& v: triangulation) {
        glVerticesTriangles.emplace_back((float)v[0], (float)v[1]);
    }
    r.vboTriangles.bind();
    r.vboTriangles.allocate(glVerticesTriangles.data(), r.numVerticesTriangles * sizeof(GLVertex));
    r.vboTriangles.release();

    // Transfer control points vertex data to GPU
    std::vector<GLVertex> glVerticesControlPoints;
    const auto& d = curve.positionData();
    r.numVerticesControlPoints = d.size() / 2;
    for (int i = 0; i < r.numVerticesControlPoints; ++i) {
        glVerticesControlPoints.emplace_back((float)d[2*i], (float)d[2*i+1]);
    }
    r.vboControlPoints.bind();
    r.vboControlPoints.allocate(glVerticesControlPoints.data(), r.numVerticesControlPoints * sizeof(GLVertex));
    r.vboControlPoints.release();

    // Set color
    r.trianglesColor = curve.color();
}

void OpenGLViewer::destroyCurveGLResources_(int)
{
    assert(curveGLResources_.size() > 0);
    CurveGLResources& r = curveGLResources_.back();

    r.vaoTriangles->destroy();
    delete r.vaoTriangles;
    r.vboTriangles.destroy();

    r.vaoControlPoints->destroy();
    delete r.vaoControlPoints;
    r.vboControlPoints.destroy();

    curveGLResources_.pop_back();
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
