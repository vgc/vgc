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

Window::Window(ui::WidgetPtr widget) :
    QWindow(),
    widget_(widget),
    engine_(internal::QOpenglEngine::create()),
    proj_(geometry::Mat4f::identity),
    clearColor_(0.337f, 0.345f, 0.353f, 1.f)
{
    setSurfaceType(QWindow::OpenGLSurface);

    connect(this, &QWindow::activeChanged, this, &Window::onActiveChanged_);

    //setMouseTracking(true);
    widget_->repaintRequested().connect(onRepaintRequested());
    //widget_->focusRequested().connect([this](){ this->onFocusRequested(); });

    // useful ?
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(8);
    format.setSwapInterval(0);
    setFormat(format);

    QWindow::create();

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

void Window::onDestroyed()
{
    if (engine_) {
        // Resources are going to be released so we setup the correct context.
        engine_->setTarget(this);
        engine_ = nullptr;
    }
}

WindowPtr Window::create(ui::WidgetPtr widget)
{
    return WindowPtr(new Window(widget));
}

//void Window::setEngine() {
//
//    // Initialize widget for painting.
//    // Note that initializedGL() is never called if the widget is never visible.
//    // Therefore it's important to keep track whether it has been called, so that
//    // we don't call onPaintDestroy() without first calling onPaintCreate()
//    isInitialized_ = true;
//    if (widget_) {
//        //m_context->makeCurrent(this);
//
//        engine_->glViewport(0, 0, width(), height());
//
//        oglf_->glClearColor(1.f, 0, 0, 1.f);
//        oglf_->glClear(GL_COLOR_BUFFER_BIT);
//
//        m_context->swapBuffers(this);
//        widget_->onPaintCreate(engine_.get());
//    }
//
//}

//QSize Window::sizeHint() const
//{
//    geometry::Vec2f s = widget_->preferredSize();
//    return QSize(core::ifloor<int>(s[0]), core::ifloor<int>(s[1]));
//}

namespace {

ui::MouseEventPtr convertEvent(QMouseEvent* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QPointF& p = event->localPos();
#else
    const QPointF& p = event->position();
#endif
    return ui::MouseEvent::create(internal::fromQtf(p));
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

void Window::resizeEvent(QResizeEvent*)
{
    geometry::Camera2d c;
    c.setViewportSize(width(), height());
    proj_ = internal::toMat4f(c.projectionMatrix());

    // Set new widget geometry. Note: if w or h is > 16777216 (=2^24), then static_cast
    // silently rounds to the nearest integer representable as a float. See:
    //   https://stackoverflow.com/a/60339495/1951907
    // Should we issue a warning in these cases?
    widget_->setGeometry(0, 0, static_cast<float>(width()), static_cast<float>(height()));

    // Ask for redraw. This often improves how smooth resizing the window looks like,
    // since Qt doesn't always immediately post an update during resize.
    requestUpdate();
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
    if (!isExposed()) {
        return;
    }

    if (!engine_) {
        throw LogicError("engine_ is null.");
    }

    engine_->setTarget(this);
    engine_->setViewport(0, 0, width(), height());
    engine_->clear(clearColor_);

    engine_->bindPaintShader();
    engine_->setProjectionMatrix(proj_);
    engine_->setViewMatrix(geometry::Mat4f::identity);
    widget_->paint(engine_.get());
    engine_->releasePaintShader();

#ifdef VGC_QOPENGL_EXPERIMENT
    static int frameIdx = 0;
    auto fmt = format();
    OutputDebugString(core::format("Window swap behavior: {}\n", (int)fmt.swapBehavior()).c_str());
    OutputDebugString(core::format("Window swap interval: {}\n", fmt.swapInterval()).c_str());
    OutputDebugString(core::format("frameIdx: {}\n", frameIdx).c_str());
    frameIdx++;
#endif

    engine_->present();
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

void Window::exposeEvent(QExposeEvent*)
{
    if (isExposed()) {
        paint();
        //onRepaintRequested_();
    }
}

void Window::cleanup()
{
    // XXX ?
}

void Window::onActiveChanged_()
{
    bool active = isActive();
    widget_->setTreeActive(active);
}

void Window::onRepaintRequested_()
{
    if (engine_) {
        engine_->setTarget(this);
        requestUpdate();
    }
}

} // namespace vgc::ui
