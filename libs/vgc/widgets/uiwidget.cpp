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

namespace vgc {
namespace widgets {

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
    widget_->repaintRequested().connect([this]() { this->onRepaintRequested(); });
    widget_->focusRequested().connect([this]() { this->onFocusRequested(); });

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

void UiWidget::mouseMoveEvent(QMouseEvent* event) {
    ui::MouseEventPtr e = ui::fromQt(event);
    event->setAccepted(widget_->onMouseMove(e.get()));
}

void UiWidget::mousePressEvent(QMouseEvent* event) {
    ui::MouseEventPtr e = ui::fromQt(event);
    event->setAccepted(widget_->onMousePress(e.get()));
}

void UiWidget::mouseReleaseEvent(QMouseEvent* event) {
    ui::MouseEventPtr e = ui::fromQt(event);
    event->setAccepted(widget_->onMouseRelease(e.get()));
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void UiWidget::enterEvent(QEvent* event)
#else
void UiWidget::enterEvent(QEnterEvent* event)
#endif
{
    event->setAccepted(widget_->onMouseEnter());
}

void UiWidget::leaveEvent(QEvent* event) {
    event->setAccepted(widget_->onMouseLeave());
}

void UiWidget::focusInEvent(QFocusEvent* /*event*/) {
    widget_->setTreeActive(true);
}

void UiWidget::focusOutEvent(QFocusEvent* /*event*/) {
    widget_->setTreeActive(false);
}

void UiWidget::keyPressEvent(QKeyEvent* event) {
    bool accepted = widget_->onKeyPress(event);
    event->setAccepted(accepted);
}

void UiWidget::keyReleaseEvent(QKeyEvent* event) {
    event->setAccepted(widget_->onKeyRelease(event));
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
        engine_ = vgc::ui::internal::QglEngine::create(createInfo, context());
    }

    QSurface* surface = context()->surface();
    graphics::SwapChainPtr swapChain = engine_->createSwapChainFromSurface(surface);
    engine_->setSwapChain(swapChain);

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
    geometry::Camera2d c;
    c.setViewportSize(w, h);
    proj_ = toMat4f(c.projectionMatrix());

    // Set new widget geometry. Note: if w or h is > 16777216 (=2^24), then static_cast
    // silently rounds to the nearest integer representable as a float. See:
    //   https://stackoverflow.com/a/60339495/1951907
    // Should we issue a warning in these cases?
    widget()->setGeometry(0, 0, static_cast<float>(w), static_cast<float>(h));

    // Note: paintGL will automatically be called after this
}

void UiWidget::paintGL() {
    if (!engine_) {
        throw core::LogicError("engine_ is null.");
    }

    // setViewport & present is done by Qt
    GLint vp[4];
    engine_->api()->glGetIntegerv(GL_VIEWPORT, vp);

    engine_->setRasterizerState(rasterizerState_);
    engine_->setBlendState(blendState_, geometry::Vec4f{0.f, 0.f, 0.f, 0.f});
    engine_->setViewport(vp[0], vp[1], vp[2], vp[3]);
    engine_->beginFrame(true);
    // XXX split to beginFrame() and qopenglengine-only beginInlineFrame

    //engine_->clear(core::Color(0., 0., 0.));
    engine_->clear(core::Color(0.337, 0.345, 0.353));
    engine_->setProgram(graphics::BuiltinProgram::Simple);
    engine_->setProjectionMatrix(proj_);
    engine_->setViewMatrix(geometry::Mat4f::identity);
    widget_->paint(engine_.get());

    engine_->endFrame();
    engine_->finish();

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

void UiWidget::onRepaintRequested() {
    update();
}

void UiWidget::onFocusRequested() {
    setFocus();
    // Note: Under the hood, this calls setFocus(Qt::OtherFocusReason)
}

} // namespace widgets
} // namespace vgc
