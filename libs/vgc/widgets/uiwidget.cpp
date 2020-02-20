// Copyright 2020 The VGC Developers
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

#include <vgc/widgets/uiwidget.h>

#include <QMouseEvent>

#include <vgc/core/paths.h>
#include <vgc/geometry/camera2d.h>
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
               (float)m(0,0), (float)m(0,1), (float)m(0,2), (float)m(0,3),
               (float)m(1,0), (float)m(1,1), (float)m(1,2), (float)m(1,3),
               (float)m(2,0), (float)m(2,1), (float)m(2,2), (float)m(2,3),
               (float)m(3,0), (float)m(3,1), (float)m(3,2), (float)m(3,3));
}

struct XYRGBVertex {
    float x, y, r, g, b;
};

} // namespace

UiWidget::UiWidget(ui::WidgetSharedPtr widget, QWidget* parent) :
    QOpenGLWidget(parent),
    widget_(widget),
    engine_(UiWidgetEngine::create())
{
    widget_->repaintRequested.connect(std::bind(
        &UiWidget::onRepaintRequested, this));
}

UiWidget::~UiWidget()
{
    makeCurrent();
    cleanupGL();
    doneCurrent();
}

namespace {

ui::MouseEventSharedPtr convertEvent(QMouseEvent* /*event*/)
{
    return ui::MouseEvent::create();
}

} // namespace

void UiWidget::mouseMoveEvent(QMouseEvent *event)
{
    ui::MouseEventSharedPtr e = convertEvent(event);
    event->setAccepted(widget_->onMouseMove(e.get()));
}

void UiWidget::mousePressEvent(QMouseEvent *event)
{
    ui::MouseEventSharedPtr e = convertEvent(event);
    event->setAccepted(widget_->onMousePress(e.get()));
}

void UiWidget::mouseReleaseEvent(QMouseEvent *event)
{
    ui::MouseEventSharedPtr e = convertEvent(event);
    event->setAccepted(widget_->onMouseRelease(e.get()));
}

UiWidget::OpenGLFunctions* UiWidget::openGLFunctions() const
{
    return context()->versionFunctions<OpenGLFunctions>();
}

void UiWidget::initializeGL()
{
    // Initialize shader program
    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Vertex, shaderPath_("iv4pos_iv4col_um4proj_um4view_ov4fcol.v.glsl"));
    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Fragment, shaderPath_("iv4fcol.f.glsl"));
    shaderProgram_.link();

    // Get shader locations
    shaderProgram_.bind();
    posLoc_  = shaderProgram_.attributeLocation("pos");
    colLoc_  = shaderProgram_.attributeLocation("col");
    projLoc_ = shaderProgram_.uniformLocation("proj");
    viewLoc_ = shaderProgram_.uniformLocation("view");
    shaderProgram_.release();

    // Initialize engine
    OpenGLFunctions* f = openGLFunctions();
    engine_->initialize(f, posLoc_, colLoc_);

    // Initialize widget
    widget_->initialize(engine_.get());
}

void UiWidget::resizeGL(int w, int h)
{
    geometry::Camera2d c;
    c.setViewportSize(w, h);
    proj_ = toQtMatrix(c.projectionMatrix());
    view_ = QMatrix4x4(); // identity
    widget_->resize(engine_.get(), int_cast<Int>(w), int_cast<Int>(h));
}

void UiWidget::paintGL()
{
    shaderProgram_.bind();
    shaderProgram_.setUniformValue(projLoc_, proj_);
    shaderProgram_.setUniformValue(viewLoc_, view_);
    widget_->paint(engine_.get());
    shaderProgram_.release();
}

void UiWidget::cleanupGL()
{
    widget_->cleanup(engine_.get());
}

void UiWidget::onRepaintRequested()
{
    update();
}

UiWidgetEngine::UiWidgetEngine(const ConstructorKey&) :
    graphics::Engine()
{

}

/* static */
UiWidgetEngineSharedPtr UiWidgetEngine::create()
{
    return std::make_shared<UiWidgetEngine>(ConstructorKey());
}

void UiWidgetEngine::initialize(UiWidget::OpenGLFunctions* functions, int posLoc, int colLoc)
{
    openGLFunctions_ = functions;
    posLoc_ = posLoc;
    colLoc_ = colLoc;
}

void UiWidgetEngine::clear(const core::Color& color)
{
    openGLFunctions_->glClearColor(
        static_cast<float>(color.r()),
        static_cast<float>(color.g()),
        static_cast<float>(color.b()),
        static_cast<float>(color.a()));
    openGLFunctions_->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

Int UiWidgetEngine::createTriangles()
{
    // Get or create TrianglesBuffer
    Int id = trianglesIdGenerator_.generate();
    if (id >= trianglesBuffers_.length()) {
        trianglesBuffers_.append(TrianglesBuffer()); // use Array growth policy
    }
    if (id >= trianglesBuffers_.length()) {
        trianglesBuffers_.resize(id+1);              // bypass Array growth policy
    }
    TrianglesBuffer& r = trianglesBuffers_[id];

    // Create VBO/VAO for rendering triangles
    r.vboTriangles.create();
    r.vaoTriangles = new QOpenGLVertexArrayObject();
    r.vaoTriangles->create();
    GLsizei stride  = sizeof(XYRGBVertex);
    GLvoid* posPointer = reinterpret_cast<void*>(offsetof(XYRGBVertex, x));
    GLvoid* colPointer = reinterpret_cast<void*>(offsetof(XYRGBVertex, r));
    GLboolean normalized = GL_FALSE;
    r.vaoTriangles->bind();
    r.vboTriangles.bind();
    openGLFunctions_->glEnableVertexAttribArray(posLoc_);
    openGLFunctions_->glEnableVertexAttribArray(colLoc_);
    openGLFunctions_->glVertexAttribPointer(posLoc_, 2, GL_FLOAT, normalized, stride, posPointer);
    openGLFunctions_->glVertexAttribPointer(colLoc_, 3, GL_FLOAT, normalized, stride, colPointer);
    r.vboTriangles.release();
    r.vaoTriangles->release();

    return id;
}

void UiWidgetEngine::loadTriangles(Int id, const float* data, Int length)
{
    if (length < 0) {
        throw core::NegativeIntegerError(core::format(
            "Negative length ({}) provided to loadTriangles()", length));
    }
    TrianglesBuffer& r = trianglesBuffers_[id];
    r.numVertices = length / 5;
    r.vboTriangles.bind();
    r.vboTriangles.allocate(data, r.numVertices * sizeof(XYRGBVertex));
    r.vboTriangles.release();
}

void UiWidgetEngine::drawTriangles(Int id)
{
    TrianglesBuffer& r = trianglesBuffers_[id];
    openGLFunctions_->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    r.vaoTriangles->bind();
    openGLFunctions_->glDrawArrays(GL_TRIANGLES, 0, r.numVertices);
    r.vaoTriangles->release();
}

void UiWidgetEngine::destroyTriangles(Int id)
{
    TrianglesBuffer& r = trianglesBuffers_[id];
    r.vaoTriangles->destroy();
    delete r.vaoTriangles;
    r.vboTriangles.destroy();
    trianglesIdGenerator_.release(id);
}

} // namespace widgets
} // namespace vgc
