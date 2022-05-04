// Copyright 2022 The VGC Developers
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

#include <vgc/ui/window.h>

#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QOpenGLFunctions>

#include <vgc/core/paths.h>
#include <vgc/geometry/camera2d.h>
#include <vgc/ui/widget.h>
#include <vgc/widgets/qtutil.h>

namespace vgc::ui {


/*
DESIGN PSEUDOCODE:
    Context {
        int id;
    }
    Engine {
        Context* ctx;
    }

    Foo = Engine or Context

    Widget {
        Foo ref;
        rid resourceId;

        paintDrawInternal(Engine* drawingEngine) {
            if (drawingEngine.ref != ref) {
                onPaintDestroy(ref);
                ctx = drawingEngine->ctx;
                onPaintCreate(drawingEngine);
                ctx->onDestroy().connect(this->onCtxDestroyed());
            }

            paintDraw(drawingEngine);
        }

        void onCtxDestroyed_(Context*) {
            onPaintDestroy(ctx);
        }
        VGC_SLOT(onCtxDestroyed, onCtxDestroyed_);

        virtual onPaintCreate(Engine*);
        virtual onPaintDraw(Engine*);
        virtual onPaintDestroy(Foo*);
    }

*/











namespace {

QString toQt(const std::string& s)
{
    int size = vgc::core::int_cast<int>(s.size());
    return QString::fromUtf8(s.data(), size);
}

std::string fromQt(const QString& s)
{
    QByteArray a = s.toUtf8();
    size_t size = vgc::core::int_cast<size_t>(s.size());
    return std::string(a.data(), size);
}

QPointF toQt(const core::Vec2d& v)
{
    return QPointF(v[0], v[1]);
}

QPointF toQt(const core::Vec2f& v)
{
    return QPointF(v[0], v[1]);
}

core::Vec2d fromQtd(const QPointF& v)
{
    return core::Vec2d(v.x(), v.y());
}

core::Vec2f fromQtf(const QPointF& v)
{
    return core::Vec2f(v.x(), v.y());
}

// Returns the file path of a shader file as a QString
QString shaderPath_(const std::string& name) {
    std::string path = core::resourcePath("graphics/opengl/" + name);
    return toQt(path);
}

core::Mat4f toMat4f(const core::Mat4d& m) {
    // TODO: implement Mat4d to Mat4f conversion directly in Mat4x classes
    return core::Mat4f(
        (float)m(0,0), (float)m(0,1), (float)m(0,2), (float)m(0,3),
        (float)m(1,0), (float)m(1,1), (float)m(1,2), (float)m(1,3),
        (float)m(2,0), (float)m(2,1), (float)m(2,2), (float)m(2,3),
        (float)m(3,0), (float)m(3,1), (float)m(3,2), (float)m(3,3));
}

QMatrix4x4 toQtMatrix(const core::Mat4f& m) {
    return QMatrix4x4(
        m(0,0), m(0,1), m(0,2), m(0,3),
        m(1,0), m(1,1), m(1,2), m(1,3),
        m(2,0), m(2,1), m(2,2), m(2,3),
        m(3,0), m(3,1), m(3,2), m(3,3));
}

struct XYRGBVertex {
    float x, y, r, g, b;
};

} // namespace

QWindowEngine::QWindowEngine() :
    graphics::Engine(),
    projectionMatrices_({core::Mat4f::identity}),
    viewMatrices_({core::Mat4f::identity}),
    ctx_(QOpenGLContext::globalShareContext()) {

}

/* static */
QWindowEnginePtr QWindowEngine::create()
{
    return QWindowEnginePtr(new QWindowEngine());
}

void QWindowEngine::clear(const core::Color& color)
{
    api_->glClearColor(
        static_cast<float>(color.r()),
        static_cast<float>(color.g()),
        static_cast<float>(color.b()),
        static_cast<float>(color.a()));
    api_->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

core::Mat4f QWindowEngine::projectionMatrix()
{
    return projectionMatrices_.last();
}

void QWindowEngine::setProjectionMatrix(const core::Mat4f& m)
{
    projectionMatrices_.last() = m;
    shaderProgram_.setUniformValue(projLoc_, toQtMatrix(m));
}

void QWindowEngine::pushProjectionMatrix()
{
    // Note: we need a temporary copy by value, since the address
    // of the current projection matrix might change during "append" in
    // case of memory re-allocation.
    core::Mat4f m = projectionMatrix();
    projectionMatrices_.append(m);
}

void QWindowEngine::popProjectionMatrix()
{
    projectionMatrices_.removeLast();
    shaderProgram_.setUniformValue(projLoc_, toQtMatrix(projectionMatrices_.last()));
}

core::Mat4f QWindowEngine::viewMatrix()
{
    return viewMatrices_.last();
}

void QWindowEngine::setViewMatrix(const core::Mat4f& m)
{
    viewMatrices_.last() = m;
    shaderProgram_.setUniformValue(viewLoc_, toQtMatrix(m));
}

void QWindowEngine::pushViewMatrix()
{
    // Note: we need a temporary copy by value, since the address
    // of the current view matrix might change during "append" in
    // case of memory re-allocation.
    core::Mat4f m = viewMatrix();
    viewMatrices_.append(m);
}

void QWindowEngine::popViewMatrix()
{
    viewMatrices_.removeLast();
    shaderProgram_.setUniformValue(viewLoc_, toQtMatrix(viewMatrices_.last()));
}

Int QWindowEngine::createTriangles()
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
    api_->glEnableVertexAttribArray(posLoc_);
    api_->glEnableVertexAttribArray(colLoc_);
    api_->glVertexAttribPointer(posLoc_, 2, GL_FLOAT, normalized, stride, posPointer);
    api_->glVertexAttribPointer(colLoc_, 3, GL_FLOAT, normalized, stride, colPointer);
    r.vboTriangles.release();
    r.vaoTriangles->release();

    return id;
}

void QWindowEngine::loadTriangles(Int id, const float* data, Int length)
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

void QWindowEngine::drawTriangles(Int id)
{
    TrianglesBuffer& r = trianglesBuffers_[id];
    r.vaoTriangles->bind();
    api_->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    api_->glDrawArrays(GL_TRIANGLES, 0, r.numVertices);
    r.vaoTriangles->release();
}

void QWindowEngine::destroyTriangles(Int id)
{
    TrianglesBuffer& r = trianglesBuffers_[id];
    r.vaoTriangles->destroy();
    delete r.vaoTriangles;
    r.vboTriangles.destroy();
    trianglesIdGenerator_.release(id);
}

bool QWindowEngine::setViewport(Int x, Int y, Int width, Int height)
{
    api_->glViewport(0, 0, width, height);
}

bool QWindowEngine::initContext()
{
    if (ctx_ == nullptr) {
        throw LogicError("ctx_ is null.");
    }

    //m_context->setFormat(requestedFormat());
    //ctx_->create();
    //ctx_->makeCurrent(this);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(8);
    format.setSwapInterval(0);
    ctx_->setFormat(format);

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
    api_ = ctx_->versionFunctions<QOpenGLFunctions_3_2_Core>();
    api_->initializeOpenGLFunctions();

    // Initialize widget for painting.
    // Note that initializedGL() is never called if the widget is never visible.
    // Therefore it's important to keep track whether it has been called, so that
    // we don't call onPaintDestroy() without first calling onPaintCreate()

    /*ctx_->makeCurrent(this);

    oglf_->glViewport(0, 0, width(), height());

    oglf_->glClearColor(1.f, 0, 0, 1.f);
    oglf_->glClear(GL_COLOR_BUFFER_BIT);

    m_context->swapBuffers(this);

    widget_->onPaintCreate(engine_.get());*/
}

Window::Window(ui::WidgetPtr widget) :
    widget_(widget),
    engine_(),
    isInitialized_(false)
{
    //setMouseTracking(true);
    widget_->repaintRequested().connect([this](){ this->onRepaintRequested(); });
    //widget_->focusRequested().connect([this](){ this->onFocusRequested(); });

    setSurfaceType(QWindow::OpenGLSurface);

    // Handle dead keys and complex input methods.
    //
    // Also see:
    // - inputMethodQuery(Qt::InputMethodQuery)
    // - inputMethodEvent(QInputMethodEvent*)
    //
    // XXX Shouldn't we enable/disable this property dynamically at runtime,
    // based on which ui::Widget has the focus? Is it even possible? Indeed, we
    // probably want to prevent an IME to popup if the focused widget doesn't
    // accept text input.
    //
    //setAttribute(Qt::WA_InputMethodEnabled, true);

}

Window::~Window() noexcept
{
}

WindowPtr Window::create(ui::WidgetPtr widget)
{
    return WindowPtr(new Window(widget));
}

void Window::setupDefaultEngine() {

    // Initialize widget for painting.
    // Note that initializedGL() is never called if the widget is never visible.
    // Therefore it's important to keep track whether it has been called, so that
    // we don't call onPaintDestroy() without first calling onPaintCreate()
    isInitialized_ = true;
    if (widget_) {
        //m_context->makeCurrent(this);

        engine_->glViewport(0, 0, width(), height());

        oglf_->glClearColor(1.f, 0, 0, 1.f);
        oglf_->glClear(GL_COLOR_BUFFER_BIT);

        m_context->swapBuffers(this);
        widget_->onPaintCreate(engine_.get());
    }

}


//QSize Window::sizeHint() const
//{
//    core::Vec2f s = widget_->preferredSize();
//    return QSize(core::ifloor<int>(s[0]), core::ifloor<int>(s[1]));
//}

namespace {

ui::MouseEventPtr convertEvent(QMouseEvent* event)
{
    const QPointF& p = event->localPos();
    return ui::MouseEvent::create(fromQtf(p));
}

} // namespace

void Window::mouseMoveEvent(QMouseEvent *event)
{
    ui::MouseEventPtr e = convertEvent(event);
    event->setAccepted(widget_->onMouseMove(e.get()));
}

void Window::mousePressEvent(QMouseEvent *event)
{
    ui::MouseEventPtr e = convertEvent(event);
    event->setAccepted(widget_->onMousePress(e.get()));
}

void Window::mouseReleaseEvent(QMouseEvent *event)
{
    ui::MouseEventPtr e = convertEvent(event);
    event->setAccepted(widget_->onMouseRelease(e.get()));
}

//void Window::enterEvent(QEvent* event)
//{
//    event->setAccepted(widget_->onMouseEnter());
//}
//
//void Window::leaveEvent(QEvent* event)
//{
//    event->setAccepted(widget_->onMouseLeave());
//}

void Window::focusInEvent(QFocusEvent* /*event*/)
{
    widget_->setTreeActive(true);
}

void Window::focusOutEvent(QFocusEvent* /*event*/)
{
    widget_->setTreeActive(false);
}

void Window::resizeEvent(QResizeEvent *ev)
{
    geometry::Camera2d c;
    c.setViewportSize(width(), height());
    proj_ = toMat4f(c.projectionMatrix());

    // Set new widget geometry. Note: if w or h is > 16777216 (=2^24), then static_cast
    // silently rounds to the nearest integer representable as a float. See:
    //   https://stackoverflow.com/a/60339495/1951907
    // Should we issue a warning in these cases?
    widget_->setGeometry(0, 0, static_cast<float>(width()), static_cast<float>(height()));
    // Note: paintGL will automatically be called after this
    paint();
}

void Window::keyPressEvent(QKeyEvent* event)
{
    bool accepted = widget_->onKeyPress(event);
    event->setAccepted(accepted);
}

void Window::keyReleaseEvent(QKeyEvent* event)
{
    event->setAccepted(widget_->onKeyRelease(event));
}

//QVariant Window::inputMethodQuery(Qt::InputMethodQuery) const
//{
//    // This function allows the input method editor (commonly abbreviated IME)
//    // to query useful info about the widget state that it needs to operate.
//    // For more info on IME in general, see:
//    //
//    // https://en.wikipedia.org/wiki/Input_method
//    //
//    // For inspiration on how to implement this function, see QLineEdit:
//    //
//    // https://github.com/qt/qtbase/blob/ec7ff5cede92412b929ff30207b0eeafce93ee3b/src/widgets/widgets/qlineedit.cpp#L1849
//    //
//    // For now, we simply return an empty QVariant. Most likely, this means
//    // that many (most?) IME won't work with our app. Fixing this is left for
//    // future work.
//    //
//    // Also see:
//    //
//    // - https://stackoverflow.com/questions/43078567/qt-inputmethodevent-get-the-keyboard-key-that-was-pressed
//    // - https://stackoverflow.com/questions/3287180/putting-ime-in-a-custom-text-box-derived-from-control
//    // - https://stackoverflow.com/questions/434048/how-do-you-use-ime
//    //
//    return QVariant();
//}

//void Window::inputMethodEvent(QInputMethodEvent* event)
//{
//    // For now, we only use a very simple implementation that, at the very
//    // least, correctly handle dead keys. See:
//    //
//    // https://stackoverflow.com/questions/28793356/qt-and-dead-keys-in-a-custom-widget
//    //
//    // Most likely, complex IME still won't work correctly, see comment in the
//    // implementation of inputMethodQuery().
//    //
//    if (!event->commitString().isEmpty()) {
//        // XXX Shouldn't we pass more appropriate modifiers?
//        QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier, event->commitString());
//        keyPressEvent(&keyEvent);
//    }
//}

void Window::paint() {
    if (!m_context) {
        setupDefaultEngine();
    }
    m_context->makeCurrent(this);
    oglf_->glViewport(0, 0, width(), height());
    oglf_->glClearColor(1.f, 0, 0, 1.f);
    oglf_->glClear(GL_COLOR_BUFFER_BIT);
    
    shaderProgram_.bind();
    engine_->setProjectionMatrix(proj_);
    engine_->setViewMatrix(core::Mat4f::identity);
    engine_->clear(core::Color(0.337, 0.345, 0.353));
    widget_->onPaintDraw(engine_.get());
    shaderProgram_.release();

    m_context->swapBuffers(this);
}

bool Window::event(QEvent* e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        paint();
        return true;
    case QEvent::ShortcutOverride:
        e->accept();
        break;
    default:
        break;
    }
    return QWindow::event(e);
}

void Window::cleanup()
{
    if (isInitialized_) {
        widget_->onPaintDestroy(engine_.get());
        isInitialized_ = false;
    }
}

void Window::onRepaintRequested()
{
    requestUpdate();
}

} // namespace vgc::ui
