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

#include <vgc/widgets/uiwidget.h>

#include <QInputMethodEvent>
#include <QMouseEvent>

#include <vgc/core/paths.h>
#include <vgc/geometry/camera2d.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/widget.h>
#include <vgc/widgets/toolbar.h>

namespace vgc::widgets {

namespace {

geometry::Mat4f toMat4f(const geometry::Mat4d& m) {
    // TODO: implement Mat4d to Mat4f conversion directly in Mat4x classes
    // clang-format off
    return geometry::Mat4f(
        (float)m(0,0), (float)m(0,1), (float)m(0,2), (float)m(0,3),
        (float)m(1,0), (float)m(1,1), (float)m(1,2), (float)m(1,3),
        (float)m(2,0), (float)m(2,1), (float)m(2,2), (float)m(2,3),
        (float)m(3,0), (float)m(3,1), (float)m(3,2), (float)m(3,3));
    // clang-format on
}

} // namespace

UiWidget::UiWidget(ui::WidgetPtr widget, QWidget* parent)
    : QOpenGLWidget(parent)
    , widget_(widget)
    , engine_()
    , isInitialized_(false) {

    setMouseTracking(true);

    widget_->geometryUpdateRequested().connect(
        [this]() { this->onGeometryUpdateRequested(); });

    widget_->repaintRequested().connect( //
        [this]() { this->onRepaintRequested(); });

    widget_->mouseCaptureStarted().connect( //
        [this]() { this->onMouseCaptureStarted(); });

    widget_->mouseCaptureStopped().connect( //
        [this]() { this->onMouseCaptureStopped(); });

    widget_->keyboardCaptureStarted().connect( //
        [this]() { this->onKeyboardCaptureStarted(); });

    widget_->keyboardCaptureStopped().connect( //
        [this]() { this->onKeyboardCaptureStopped(); });

    widget_->focusSet().connect(
        [this](ui::FocusReason reason) { this->onFocusSet(reason); });

    widget_->focusCleared().connect(
        [this](ui::FocusReason reason) { this->onFocusCleared(reason); });

    widget_->addStyleClass(ui::strings::root);

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
    setAttribute(Qt::WA_InputMethodEnabled, true);
}

UiWidget::~UiWidget() {
    makeCurrent();
    cleanupGL();
    doneCurrent();
}

QSize UiWidget::sizeHint() const {
    geometry::Vec2f s = widget_->preferredSize();
    return QSize(core::ifloor<int>(s[0]), core::ifloor<int>(s[1]));
}

bool UiWidget::hasHeightForWidth() const {
    return true;
}

int UiWidget::heightForWidth(int w) const {
    float width = static_cast<float>(w);
    float height = widget_->preferredHeightForWidth(width);
    return core::ifloor<int>(height);
}

namespace {

std::pair<ui::Widget*, ui::MouseEventPtr>
prepareMouseEvent(ui::Widget* root, QMouseEvent* event) {
    ui::MouseEventPtr vgcEvent = ui::fromQt(event);
    ui::Widget* mouseCaptor = root->mouseCaptor();
    if (mouseCaptor) {
        geometry::Vec2f position = root->mapTo(mouseCaptor, vgcEvent->position());
        vgcEvent->setPosition(position);
        return {mouseCaptor, vgcEvent};
    }
    else {
        return {root, vgcEvent};
    }
}

} // namespace

void UiWidget::mouseMoveEvent(QMouseEvent* event) {
    auto [receiver, vgcEvent] = prepareMouseEvent(widget_.get(), event);
    if (receiver != widget_.get()) {
        // mouse captor
        event->setAccepted(receiver->onMouseMove(vgcEvent.get()));
    }
    else {
        event->setAccepted(receiver->mouseMove(vgcEvent.get()));
    }
}

void UiWidget::mousePressEvent(QMouseEvent* event) {
    auto [receiver, vgcEvent] = prepareMouseEvent(widget_.get(), event);
    if (receiver != widget_.get()) {
        // mouse captor
        event->setAccepted(receiver->onMousePress(vgcEvent.get()));
    }
    else {
        event->setAccepted(receiver->mousePress(vgcEvent.get()));
    }
}

void UiWidget::mouseReleaseEvent(QMouseEvent* event) {
    auto [receiver, vgcEvent] = prepareMouseEvent(widget_.get(), event);
    if (receiver != widget_.get()) {
        // mouse captor
        event->setAccepted(receiver->onMouseRelease(vgcEvent.get()));
    }
    else {
        event->setAccepted(receiver->mouseRelease(vgcEvent.get()));
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void UiWidget::enterEvent(QEvent* event)
#else
void UiWidget::enterEvent(QEnterEvent* event)
#endif
{
    event->setAccepted(widget_->setHovered(true));
}

void UiWidget::leaveEvent(QEvent* event) {
    event->setAccepted(widget_->setHovered(false));
}

void UiWidget::focusInEvent(QFocusEvent* event) {
    ui::FocusReason reason = static_cast<ui::FocusReason>(event->reason());
    widget_->setTreeActive(true, reason);
}

void UiWidget::focusOutEvent(QFocusEvent* event) {
    ui::FocusReason reason = static_cast<ui::FocusReason>(event->reason());
    widget_->setTreeActive(false, reason);
}

namespace {

std::pair<ui::Widget*, ui::KeyEventPtr>
prepareKeyboardEvent(ui::Widget* root, QKeyEvent* event) {
    ui::KeyEventPtr vgcEvent = ui::fromQt(event);
    ui::Widget* keyboardCaptor = root->keyboardCaptor();
    if (keyboardCaptor) {
        return {keyboardCaptor, vgcEvent};
    }
    else {
        return {root, vgcEvent};
    }
}

} // namespace

void UiWidget::keyPressEvent(QKeyEvent* event) {
    auto [receiver, vgcEvent] = prepareKeyboardEvent(widget_.get(), event);
    event->setAccepted(receiver->keyPress(vgcEvent.get()));
}

void UiWidget::keyReleaseEvent(QKeyEvent* event) {
    auto [receiver, vgcEvent] = prepareKeyboardEvent(widget_.get(), event);
    event->setAccepted(receiver->keyRelease(vgcEvent.get()));
}

QVariant UiWidget::inputMethodQuery(Qt::InputMethodQuery) const {
    // This function allows the input method editor (commonly abbreviated IME)
    // to query useful info about the widget state that it needs to operate.
    // For more info on IME in general, see:
    //
    // https://en.wikipedia.org/wiki/Input_method
    //
    // For inspiration on how to implement this function, see QLineEdit:
    //
    // https://github.com/qt/qtbase/blob/ec7ff5cede92412b929ff30207b0eeafce93ee3b/src/widgets/widgets/qlineedit.cpp#L1849
    //
    // For now, we simply return an empty QVariant. Most likely, this means
    // that many (most?) IME won't work with our app. Fixing this is left for
    // future work.
    //
    // Also see:
    //
    // - https://stackoverflow.com/questions/43078567/qt-inputmethodevent-get-the-keyboard-key-that-was-pressed
    // - https://stackoverflow.com/questions/3287180/putting-ime-in-a-custom-text-box-derived-from-control
    // - https://stackoverflow.com/questions/434048/how-do-you-use-ime
    //
    return QVariant();
}

void UiWidget::inputMethodEvent(QInputMethodEvent* event) {
    // For now, we only use a very simple implementation that, at the very
    // least, correctly handle dead keys. See:
    //
    // https://stackoverflow.com/questions/28793356/qt-and-dead-keys-in-a-custom-widget
    //
    // Most likely, complex IME still won't work correctly, see comment in the
    // implementation of inputMethodQuery().
    //
    if (!event->commitString().isEmpty()) {
        // XXX Shouldn't we pass more appropriate modifiers?
        QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier, event->commitString());
        keyPressEvent(&keyEvent);
    }
}

void UiWidget::showEvent(QShowEvent*) {

    // In UiWidget::onRepaintRequested(), we call update().
    //
    // Usually, this causes paintGL() to be called, which calls
    // ui::Widget::paint().
    //
    // However, if the UiWidget isn't visible, then Qt will in fact not call
    // paintGL(), not even when the widget becomes visible again (i.e., Qt
    // "forgets" about the update()).
    //
    // By default, this causes the ui::Widget to never be repainted again, because
    // by design, ui::Widget never emits another repaintRequested() as long as
    // paint() isn't called. So the widget appears broken / frozen: clicking
    // on it doesn't repaint the widget, because its requestRepaint() doesn't
    // propagate to the root.
    //
    // The solution is to manually call update() again when the UiWidget
    // becomes visible, if there is a pending repaint request. This will cause
    // paintGL() to be called, which calls ui::Widget::paint(), clearing the
    // dirty flags, so any further user interactions will cause
    // repaintRequested() to be emitted again.
    //
    if (isRepaintRequested_) {
        update();
    }
}

bool UiWidget::event(QEvent* e) {
    if (e->type() == QEvent::ShortcutOverride) {
        e->accept();
    }
    return QWidget::event(e);
}

void UiWidget::initializeGL() {
    {
        graphics::EngineCreateInfo createInfo = {};
        createInfo.setMultithreadingEnabled(false);
        engine_ = vgc::ui::detail::QglEngine::create(createInfo, context());
    }

    QSurface* surface = context()->surface();
    swapChain_ = engine_->createSwapChainFromSurface(surface);

    {
        graphics::RasterizerStateCreateInfo createInfo = {};
        rasterizerState_ = engine_->createRasterizerState(createInfo);
    }

    {
        graphics::BlendStateCreateInfo createInfo = {};
        createInfo.setEnabled(true);
        createInfo.setEquationRGB(
            graphics::BlendOp::Add,
            graphics::BlendFactor::SourceAlpha,
            graphics::BlendFactor::OneMinusSourceAlpha);
        createInfo.setEquationAlpha(
            graphics::BlendOp::Add,
            graphics::BlendFactor::One,
            graphics::BlendFactor::OneMinusSourceAlpha);
        createInfo.setWriteMask(graphics::BlendWriteMaskBit::All);
        blendState_ = engine_->createBlendState(createInfo);
    }

    // Initialize widget for painting.
    // Note that initializedGL() is never called if the widget is never visible.
    // Therefore it's important to keep track whether it has been called, so that
    // we don't call onPaintDestroy() without first calling onPaintCreate()
    isInitialized_ = true;

    //widget_->onPaintCreate(engine_.get());
}

void UiWidget::resizeGL(int w, int h) {

    // Compute new projection matrix
    geometry::Camera2d c;
    c.setViewportSize(w, h);
    proj_ = toMat4f(c.projectionMatrix());

    // Set new widget geometry
    widget()->updateGeometry(0, 0, static_cast<float>(w), static_cast<float>(h));

    // Note: paintGL will automatically be called after this
    if (engine_) {
        engine_->onWindowResize(swapChain_, width(), height());
    }
}

void UiWidget::paintGL() {
    if (!engine_) {
        throw core::LogicError("engine_ is null.");
    }

    // setViewport & present is done by Qt

    //engine_->onWindowResize(swapChain_, width(), height());
    engine_->beginFrame(swapChain_, graphics::FrameKind::QWidget);

    engine_->setRasterizerState(rasterizerState_);
    engine_->setBlendState(blendState_, geometry::Vec4f());

    // XXX split to beginFrame() and qopenglengine-only beginInlineFrame

    //engine_->clear(core::Color(0.f, 0.f, 0.f));

    // Note: clear calls syncState_, and since it's the first time it is
    // called, all parameters are dirty, so setScissorRect_() is called
    // with the top of the scissorRectStackto
    // engine->
    engine_->clear(core::Color(0.251f, 0.259f, 0.267f));
    engine_->setProgram(graphics::BuiltinProgram::Simple);
    engine_->setProjectionMatrix(proj_);
    engine_->setViewMatrix(geometry::Mat4f::identity);
    engine_->setScissorRect(widget()->rect());
    widget_->paint(engine_.get());
    isRepaintRequested_ = false;
    engine_->endFrame();

    // make current in current thread again, engine has no immediate mode yet
    context()->makeCurrent(context()->surface());

    //engine_->releasePaintShader();
    // XXX opengl only.. we need to add a flush/finish to submit ?
}

void UiWidget::cleanupGL() {
    if (isInitialized_) {
        blendState_.reset();
        rasterizerState_.reset();
        engine_ = nullptr;
        isInitialized_ = false;
    }
}

void UiWidget::onGeometryUpdateRequested() {
    updateGeometry();
    QWidget* p = parentWidget();
    if (p && qobject_cast<Toolbar*>(p)) {
        // Hack to force a parent->resizeEvent() if the parent is a Toolbar.
        // This is necessary because unlike other layouts, QToolBarLayout
        // ignores the heightForWidth() of its children. As a workaround, we
        // reimplemented QToolbar::resizeEvent() in our Toolbar subclass, which
        // set the heightForWidth() of its children as minimum height.
        QRect oldGeometry = parentWidget()->geometry();
        QRect tempGeometry = oldGeometry.adjusted(0, 0, 0, 1);
        parentWidget()->setGeometry(tempGeometry);
        parentWidget()->setGeometry(oldGeometry);
    }
}

void UiWidget::onRepaintRequested() {
    isRepaintRequested_ = true;
    update();
}

void UiWidget::onMouseCaptureStarted() {
    grabMouse();
}

void UiWidget::onMouseCaptureStopped() {
    releaseMouse();
}

void UiWidget::onKeyboardCaptureStarted() {
    grabKeyboard();
}

void UiWidget::onKeyboardCaptureStopped() {
    releaseKeyboard();
}

void UiWidget::onFocusSet(ui::FocusReason reason) {
    Qt::FocusReason qreason = static_cast<Qt::FocusReason>(reason);
    setFocus(qreason);
}

void UiWidget::onFocusCleared(ui::FocusReason) {
    clearFocus();
}

} // namespace vgc::widgets
