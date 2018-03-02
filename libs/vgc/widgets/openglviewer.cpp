// Copyright 2017 The VGC Developers
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

#include <vgc/core/resources.h>
#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {

namespace {

// Returns the file path of a shader file as a QString
QString shaderPath_(const std::string& name) {
    std::string path = core::resourcePath("opengl/shaders/" + name);
    return toQt(path);
}

geometry::Vec2d toVec2d_(QMouseEvent* event) {
    return geometry::Vec2d(event->x(), event->y());
}

QMatrix4x4 toQtMatrix(const geometry::Mat4d& m) {
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

OpenGLViewer::OpenGLViewer(scene::Scene* scene, QWidget* parent) :
    QOpenGLWidget(parent),
    scene_(scene),
    isSketching_(false),
    isPanning_(false),
    isRotating_(false),
    isZooming_(false),
    tabletPressure_(0.0),
    mouseEventInProgress_(false),
    tabletEventInProgress_(false),
    polygonMode_(2),
    showControlPoints_(false),
    requestedTesselationMode_(2),
    currentTesselationMode_(2)
{
    // Set ClickFocus policy to be able to accept keyboard events (default
    // policy is NoFocus).
    setFocusPolicy(Qt::ClickFocus);
}

OpenGLViewer::~OpenGLViewer()
{
    // Make OpenGL context current and call cleanupGL()
    makeCurrent();
    cleanupGL();
    doneCurrent();
}

double OpenGLViewer::width_() const
{
    const double defaultWidth = 6.0;
    return mouseEventInProgress_
           ? defaultWidth
           : 2 * tabletPressure_ * defaultWidth;

    // Note: when drawing with a Wacom tablet, this may be called
    // in a MouseMove just after TabletRelease. In which case,
    // we have mouseEventInProgress_ = false but also tabletEventInProgress = false
    // The one we should trust is mouseEventInProgress. See bug #9.
}

void OpenGLViewer::mousePressEvent(QMouseEvent* event)
{
    if (tabletEventInProgress_) {
        mouseEventInProgress_ = false;
    }
    else {
        mouseEventInProgress_ = true;
    }

    if (isSketching_ || isPanning_ || isRotating_ || isZooming_) {
        return;
    }

    if (event->modifiers() == Qt::NoModifier &&
        event->button() == Qt::LeftButton)
    {
        isSketching_ = true;
        // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
        // and should be cached), but let's keep it like this for now for testing.
        geometry::Vec2d viewCoords = toVec2d_(event);
        geometry::Vec2d worldCoords = camera_.viewMatrix().inverse() * viewCoords;
        scene_->startCurve(worldCoords, width_());
    }
    else if (event->modifiers() == Qt::AltModifier &&
             event->button() == Qt::LeftButton)
    {
        isRotating_ = true;
        mousePosAtPress_ = toVec2d_(event);
        cameraAtPress_ = camera_;
    }
    else if (event->modifiers() == Qt::AltModifier &&
             event->button() == Qt::MidButton)
    {
        isPanning_ = true;
        mousePosAtPress_ = toVec2d_(event);
        cameraAtPress_ = camera_;
    }
    else if (event->modifiers() == Qt::AltModifier &&
             event->button() == Qt::RightButton)
    {
        isZooming_ = true;
        mousePosAtPress_ = toVec2d_(event);
        cameraAtPress_ = camera_;
    }
}

void OpenGLViewer::mouseMoveEvent(QMouseEvent* event)
{
    // Note: event-button() is always NoButton for mouseMoveEvent. This is why
    // we use the variable isPanning_ and isSketching_ to remember the current
    // mouse action. In the future, we'll abstract this mechanism in a separate
    // class.

    if (isSketching_) {
        // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
        // and should be cached), but let's keep it like this for now for testing.
        geometry::Vec2d viewCoords = toVec2d_(event);
        geometry::Vec2d worldCoords = camera_.viewMatrix().inverse() * viewCoords;
        scene_->continueCurve(worldCoords, width_());
    }
    else if (isPanning_) {
        geometry::Vec2d mousePos = toVec2d_(event);
        geometry::Vec2d delta = mousePosAtPress_ - mousePos;
        camera_.setCenter(cameraAtPress_.center() + delta);
        update();
    }
    else if (isRotating_) {
        // Set new camera rotation
        // XXX rotateViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double rotateViewSensitivity = 0.01;
        geometry::Vec2d mousePos = toVec2d_(event);
        geometry::Vec2d deltaPos = mousePosAtPress_ - mousePos;
        double deltaRotation = rotateViewSensitivity * (deltaPos.x() - deltaPos.y());
        camera_.setRotation(cameraAtPress_.rotation() + deltaRotation);

        // Set new camera center so that rotation center = mouse pos at press
        geometry::Vec2d pivotViewCoords = mousePosAtPress_;
        geometry::Vec2d pivotWorldCoords = cameraAtPress_.viewMatrix().inverse() * pivotViewCoords;
        geometry::Vec2d pivotViewCoordsNow = camera_.viewMatrix() * pivotWorldCoords;
        camera_.setCenter(camera_.center() - pivotViewCoords + pivotViewCoordsNow);

        update();
    }
    else if (isZooming_) {
        // Set new camera zoom
        // XXX zoomViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double zoomViewSensitivity = 0.005;
        geometry::Vec2d mousePos = toVec2d_(event);
        geometry::Vec2d deltaPos = mousePosAtPress_ - mousePos;
        const double s = std::exp(zoomViewSensitivity * (deltaPos.y() - deltaPos.x()));
        camera_.setZoom(cameraAtPress_.zoom() * s);

        // Set new camera center so that zoom center = mouse pos at press
        geometry::Vec2d pivotViewCoords = mousePosAtPress_;
        geometry::Vec2d pivotWorldCoords = cameraAtPress_.viewMatrix().inverse() * pivotViewCoords;
        geometry::Vec2d pivotViewCoordsNow = camera_.viewMatrix() * pivotWorldCoords;
        camera_.setCenter(camera_.center() - pivotViewCoords + pivotViewCoordsNow);

        update();
    }
}

void OpenGLViewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (isSketching_ && event->button() == Qt::LeftButton) {
        isSketching_ = false;
    }
    if (isRotating_ && event->button() == Qt::LeftButton) {
        isRotating_ = false;
    }
    else if (isPanning_ && event->button() == Qt::MidButton) {
        isPanning_ = false;
    }
    else if (isZooming_ && event->button() == Qt::RightButton) {
        isZooming_ = false;
    }

    mouseEventInProgress_ = false;
}

void OpenGLViewer::tabletEvent(QTabletEvent* event)
{
    // We store the pressure, and handle the event in:
    // - mousePressEvent()
    // - mouseMoveEvent()
    // - mouseReleaseEvent()
    //
    // This is because Qt 5.6, at least on some plateform, generates
    // the mouse event anyway even if we accept the tablet event, causing
    // duplicates. The implementation here is the most reliable way I found to
    // properly handle tablet events.
    //
    // Relevant: https://bugreports.qt.io/browse/QTBUG-47007

    // Remember state
    tabletPressure_ = event->pressure();
    switch(event->type()) {
    case QEvent::TabletPress:
        tabletEventInProgress_ = true;
        break;
    case QEvent::TabletMove:
        // nothing
        break;
    case QEvent::TabletRelease:
        tabletEventInProgress_ = false;
        break;
    default:
        // nothing
        break;
    }

    // Ignore event to ensure that Qt generates a mouse event.
    // Note: on some (but not all) systems, Qt generate the mouse event even
    // when this event is accepted.
    event->ignore();
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
    OpenGLFunctions* f = openGLFunctions();

    // Transfer to GPU any data out-of-sync with CPU
    updateGLResources_();

    // Clear color and depth buffer
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind shader program
    shaderProgram_.bind();

    // Set uniform values
    shaderProgram_.setUniformValue(projMatrixLoc_, toQtMatrix(camera_.projectionMatrix()));
    shaderProgram_.setUniformValue(viewMatrixLoc_, toQtMatrix(camera_.viewMatrix()));

    // Draw triangles
    if (polygonMode_ > 0) {
        glPolygonMode(GL_FRONT_AND_BACK, (polygonMode_ == 1) ? GL_LINE : GL_FILL);
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
        glPointSize(10.0);
        for (CurveGLResources& r: curveGLResources_) {
            r.vaoControlPoints->bind();
            f->glDrawArrays(GL_POINTS, 0, r.numVerticesControlPoints);
            r.vaoControlPoints->release();
        }
    }

    // Release shader program
    shaderProgram_.release();
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
    int nCurvesInCpu = scene_->curves().size();
    int nCurvesInGpu = curveGLResources_.size();
    for (int i = nCurvesInGpu; i < nCurvesInCpu; ++i) {
        createCurveGLResources_(i);
    }

    // Destroy GPU resources for deleted curves
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
    assert(i >= 0);
    assert(i < curveGLResources_.size());
    assert(i < scene_->curves().size());
    CurveGLResources& r = curveGLResources_[i];
    const geometry::Curve& curve = *scene_->curves()[i];

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
    std::vector<geometry::Vec2d> triangulation =
            curve.triangulate(maxAngle, minQuads, maxQuads);

    // Convert triangles to single-precision and transfer to GPU
    r.numVerticesTriangles = triangulation.size();
    std::vector<GLVertex> glVerticesTriangles;
    for(const geometry::Vec2d& v: triangulation) {
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

} // namespace widgets
} // namespace vgc
