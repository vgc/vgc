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

#include <cmath>
#include <QMouseEvent>
#include <vgc/core/resources.h>
#include <vgc/scene/scene.h>
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
    isTabletEvent_(false),
    tabletPressure_(0.0),
    polygonMode_(2),
    showControlPoints_(false),
    tesselationMode_(2)
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
    return isTabletEvent_
           ? 2 * tabletPressure_ * defaultWidth
           : defaultWidth;
}

void OpenGLViewer::mousePressEvent(QMouseEvent* event)
{
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
        isTabletEvent_ = true;
        break;
    case QEvent::TabletMove:
        // nothing
        break;
    case QEvent::TabletRelease:
        isTabletEvent_ = false;
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
        tesselationMode_ = 0;
        update();
        break;
    case Qt::Key_U:
        tesselationMode_ = 1;
        update();
        break;
    case Qt::Key_A:
        tesselationMode_ = 2;
        update();
        break;
    case Qt::Key_C:
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
    colorLoc_      = shaderProgram_.uniformLocation("ucolor");
    shaderProgram_.release();

    // Create VBO
    vbo_.create();

    // Create VAO
    vao_.create();
    GLsizei  stride  = sizeof(GLVertex);
    GLvoid* pointer = reinterpret_cast<void*>(offsetof(GLVertex, x));
    vao_.bind();
    vbo_.bind();
    f->glEnableVertexAttribArray(vertexLoc_);
    f->glVertexAttribPointer(
                vertexLoc_, // index of the generic vertex attribute
                2,          // number of components   (x and y components)
                GL_FLOAT,   // type of each component
                GL_FALSE,   // should it be normalized
                stride,     // byte offset between consecutive vertex attributes
                pointer);   // byte offset between the first attribute and the pointer given to allocate()
    vbo_.release();
    vao_.release();

    // Setup VBO/VAO for displaying control points
    controlPointsVbo_.create();
    controlPointsVao_.create();
    controlPointsVao_.bind();
    controlPointsVbo_.bind();
    f->glEnableVertexAttribArray(vertexLoc_);
    f->glVertexAttribPointer(
                vertexLoc_, // index of the generic vertex attribute
                2,          // number of components   (x and y components)
                GL_FLOAT,   // type of each component
                GL_FALSE,   // should it be normalized
                stride,     // byte offset between consecutive vertex attributes
                pointer);   // byte offset between the first attribute and the pointer given to allocate()
    controlPointsVbo_.release();
    controlPointsVao_.release();

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

    // Update VBO of curves geometry
    // Note: for simplicity, we perform this at each paintGL. In an actual app,
    // it would be much better to do this only once after each mouse event.
    computeGLVertices_();
    vbo_.bind();
    vbo_.allocate(glVertices_.data(), glVertices_.size() * sizeof(GLVertex));
    vbo_.release();

    // Update VBO of control points geometry
    if (showControlPoints_) {
        computeControlPointsGLVertices_();
        controlPointsVbo_.bind();
        controlPointsVbo_.allocate(controlPointsGlVertices_.data(), controlPointsGlVertices_.size() * sizeof(GLVertex));
        controlPointsVbo_.release();
    }

    // Clear color and depth buffer
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind shader program
    shaderProgram_.bind();

    // Set uniform values
    shaderProgram_.setUniformValue(projMatrixLoc_, toQtMatrix(camera_.projectionMatrix()));
    shaderProgram_.setUniformValue(viewMatrixLoc_, toQtMatrix(camera_.viewMatrix()));

    // Draw triangles
    if (polygonMode_ > 0) {
        shaderProgram_.setUniformValue(colorLoc_, 0.0f, 0.0f, 0.0f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, (polygonMode_ == 1) ? GL_LINE : GL_FILL);
        vao_.bind();
        int firstIndex = 0;
        for (int n: glVerticesChunkSizes_) {
            f->glDrawArrays(GL_TRIANGLE_STRIP, firstIndex, n);
            firstIndex += n;
        }
        vao_.release();
    }

    // Draw control points
    if (showControlPoints_) {
        shaderProgram_.setUniformValue(colorLoc_, 1.0f, 0.0f, 0.0f, 1.0f);
        glPointSize(10.0);
        controlPointsVao_.bind();
        f->glDrawArrays(GL_POINTS, 0, controlPointsGlVertices_.size());
        controlPointsVao_.release();
    }

    // Release shader program
    shaderProgram_.release();
}

void OpenGLViewer::cleanupGL()
{
    // Destroy VAOs
    vao_.destroy();
    controlPointsVao_.destroy();

    // Destroy VBOs
    vbo_.destroy();
    controlPointsVbo_.destroy();
}

void OpenGLViewer::computeGLVertices_()
{
    glVertices_.clear();
    glVerticesChunkSizes_.clear();
    for (const geometry::CurveSharedPtr& curvePtr: scene_->curves()) {
        const geometry::Curve& curve = *curvePtr;
        double maxAngle = 0.05;
        int minQuads = 1;
        int maxQuads = 64;
        if (tesselationMode_ == 0) {
            maxQuads = 1;
        }
        else if (tesselationMode_ == 1) {
            minQuads = 10;
            maxQuads = 10;
        }
        std::vector<geometry::Vec2d> triangulation = curve.triangulate(maxAngle, minQuads, maxQuads);
        int n = triangulation.size();
        if (n > 2) {
            for(const geometry::Vec2d& v: triangulation) {
                glVertices_.emplace_back((float)v[0], (float)v[1]);
            }
            glVerticesChunkSizes_.push_back(n);
        }
    }
}

void OpenGLViewer::computeControlPointsGLVertices_()
{
    controlPointsGlVertices_.clear();
    for (const geometry::CurveSharedPtr& curvePtr: scene_->curves()) {
        const geometry::Curve& curve = *curvePtr;
        const auto& d = curve.positionData();
        int n = d.size() / 2;
        for (int i = 0; i < n; ++i) {
            controlPointsGlVertices_.emplace_back(
                        (float)d[2*i], (float)d[2*i+1]);
        }
    }
}

} // namespace widgets
} // namespace vgc
