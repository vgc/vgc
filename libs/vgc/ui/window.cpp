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

#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QOpenGLFunctions>

#include <QScreen>

#include <vgc/core/os.h>
#include <vgc/core/paths.h>
#include <vgc/core/profile.h>
#include <vgc/geometry/camera2d.h>
#include <vgc/graphics/d3d11/d3d11engine.h>
#include <vgc/graphics/text.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/qtutil.h>

#include <vgc/ui/detail/qopenglengine.h>

namespace vgc::ui {

namespace {

inline constexpr QEvent::Type presentCalledEvent =
    static_cast<QEvent::Type>(core::toUnderlying(QEvent::User) + 1000);

std::string debugTime(const core::Stopwatch& stopwatch) {
    std::string res;
    Int64 us = stopwatch.elapsedMicroseconds();
    Int64 ms = us / 1000;
    us = us - 1000 * ms;
    Int64 s = ms / 1000;
    ms = ms - 1000 * s;
    return core::format("{:>3}s {:0>3}ms {:0>3}us", s, ms, us);
}

template<bool debug>
class WindowDebug;

template<>
class WindowDebug<false> {
public:
    WindowDebug(Int&, const core::Stopwatch&) {
    }
};

template<>
class WindowDebug<true> {
public:
    WindowDebug(Int& indent, const core::Stopwatch& stopwatch)
        : indent_(indent)
        , stopwatch_(stopwatch) {

        ++indent_;
    }

    ~WindowDebug() {
        --indent_;
        VGC_DEBUG(
            LogVgcUi, "[Window] {} {:>{}} END ", debugTime(stopwatch_), "", indent_ * 2);
    }

private:
    Int& indent_;
    const core::Stopwatch& stopwatch_;
};

} // namespace

} // namespace vgc::ui

#define VGC_WINDOW_DEBUG(fmt, ...)                                                       \
    if (debugEvents) {                                                                   \
        VGC_DEBUG(                                                                       \
            LogVgcUi,                                                                    \
            "[Window] {} {:>{}} BEGIN " fmt,                                             \
            debugTime(debugStopwatch_),                                                  \
            "",                                                                          \
            debugIndent_ * 2,                                                            \
            __VA_ARGS__);                                                                \
    }                                                                                    \
    WindowDebug<debugEvents> debug_(debugIndent_, debugStopwatch_)

#define VGC_WINDOW_DEBUG_NOARGS(fmt)                                                     \
    if (debugEvents) {                                                                   \
        VGC_DEBUG(                                                                       \
            LogVgcUi,                                                                    \
            "[Window] {} {:>{}} BEGIN " fmt,                                             \
            debugTime(debugStopwatch_),                                                  \
            "",                                                                          \
            debugIndent_ * 2);                                                           \
    }                                                                                    \
    WindowDebug<debugEvents> debug_(debugIndent_, debugStopwatch_)

namespace vgc::ui {

static constexpr bool debugEvents = false;

//#define VGC_DISABLE_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX

#ifdef VGC_CORE_OS_WINDOWS
#    ifndef VGC_DISABLE_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX
#        define VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX
#    endif
#endif

Window::Window(CreateKey key, const WidgetPtr& widget)
    : Object(key)
    , QWindow()

    , widget_(widget)
    , proj_(geometry::Mat4f::identity) {

    connect((QWindow*)this, &QWindow::activeChanged, this, &Window::onActiveChanged_);

    //setMouseTracking(true);
    widget_->repaintRequested().connect(onRepaintRequestedSlot_());
    widget_->focusSet().connect(onFocusSetOrClearedSlot_());
    widget_->focusCleared().connect(onFocusSetOrClearedSlot_());
    widget_->mouseCaptureStarted().connect(onMouseCaptureStartedSlot_());
    widget_->mouseCaptureStopped().connect(onMouseCaptureStoppedSlot_());
    widget_->keyboardCaptureStarted().connect(onKeyboardCaptureStartedSlot_());
    widget_->keyboardCaptureStopped().connect(onKeyboardCaptureStoppedSlot_());
    widget_->widgetAddedToTree().connect(onWidgetAddedToTreeSlot_());
    widget_->widgetRemovedFromTree().connect(onWidgetRemovedFromTreeSlot_());
    widget_->window_ = this;

    initEngine_();
    addShortcuts_(widget_.get());

    // Handle dead keys and complex input methods.
    //
    onTextInputReceiverChanged_();

    // Sets this Window as an application-wide filter to listen to events not
    // redirected to Window by default (e.g., TabletProximity).
    //
    QGuiApplication::instance()->installEventFilter(this);
}

void Window::onDestroyed() {
    // Destroying the engine will stop it.
    engine_ = nullptr;
}

WindowPtr Window::create(const WidgetPtr& widget) {
    return core::createObject<Window>(widget);
}

float Window::globalToWindowScale() const {
    return static_cast<float>(this->QWindow::devicePixelRatio());
}

geometry::Vec2f Window::mapFromGlobal(const geometry::Vec2f& globalPosition) const {
    // Note: In Qt5, mapFromGlobal uses QPoint. In Qt6, QPointF overloads were added.
    QPointF qGlobalPosition = toQt(globalPosition);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF qPosition = this->QWindow::mapFromGlobal(qGlobalPosition);
#else
    QPoint qPosition = this->QWindow::mapFromGlobal(qGlobalPosition.toPoint());
#endif
    geometry::Vec2f position = fromQtf(qPosition);
    position *= globalToWindowScale();
    return position;
}

geometry::Vec2f Window::mapToGlobal(const geometry::Vec2f& position) const {
    // Note: In Qt5, mapFromGlobal uses QPoint. In Qt6, QPointF overloads were added.
    QPointF qPosition = toQt(position);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF qGlobalPosition = this->QWindow::mapToGlobal(qPosition);
#else
    QPoint qGlobalPosition = this->QWindow::mapToGlobal(qPosition.toPoint());
#endif
    geometry::Vec2f globalPosition = fromQtf(qGlobalPosition);
    globalPosition /= globalToWindowScale();
    return globalPosition;
}

void Window::setBackgroundPainted(bool isPainted) {
    isBackgroundPainted_ = isPainted;
    requestUpdate();
}

void Window::setBackgroundColor(const core::Color& color) {
    backgroundColor_ = color;
    requestUpdate();
}

void Window::enterEvent(QEvent* event) {
    entered_ = true;
    if (isLeaveDeferred_) {
        isLeaveDeferred_ = false;
    }
    else {
        if (widget_ && !widget_->mouseCaptor()) {
            event->setAccepted(widget_->setHovered(true));
        }
    }
}

void Window::leaveEvent(QEvent* event) {
    entered_ = false;
    bool hasMouseCaptor = widget_ && widget_->mouseCaptor();
    if (pressedMouseButtons_ == MouseButton::None && !hasMouseCaptor) {
        if (widget_) { // no need to check for !hasMouseCaptor: already done
            event->setAccepted(widget_->setHovered(false));
        }
    }
    else {
        isLeaveDeferred_ = true;
        event->setAccepted(true);
    }
}

namespace {

void prepareMouseEvent(Widget* root, MouseEvent* event, const Window* window) {

    // Apply scaling between Qt coordinates our coordinates
    geometry::Vec2f position = event->position();
    position *= window->globalToWindowScale();
    event->setPosition(position);

    // Handle mouse captor
    Widget* mouseCaptor = root->mouseCaptor();
    if (mouseCaptor) {
        position = root->mapTo(mouseCaptor, position);
        event->setPosition(position);
    }
}

} // namespace

void Window::mouseMoveEvent(QMouseEvent* event) {
    if (pressedTabletButtons_) {
        return;
    }
    MouseMoveEventPtr vgcEvent = MouseMoveEvent::create();
    fromQt(event, vgcEvent.get());
    prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
    event->setAccepted(mouseMoveEvent_(vgcEvent.get()));
}

void Window::mousePressEvent(QMouseEvent* event) {
    // Reset scroll accumulator
    accumulatedScrollDelta_ = {};
    // Process event
    MousePressEventPtr vgcEvent = MousePressEvent::create();
    fromQt(event, vgcEvent.get());
    MouseButton button = vgcEvent->button();
    if (pressedMouseButtons_.has(button)) {
        // Already pressed on mouse: ignore event.
        event->setAccepted(true);
        return;
    }
    if (pressedTabletButtons_.has(button) || isTabletInUse_()) {
        // Already hovered/pressed on tablet: ignore event.
        event->setAccepted(true);
        return;
    }
    pressedMouseButtons_.set(button);
    prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
    event->setAccepted(mousePressEvent_(vgcEvent.get()));
}

void Window::mouseReleaseEvent(QMouseEvent* event) {
    // Reset scroll accumulator
    accumulatedScrollDelta_ = {};
    // Process event
    MouseReleaseEventPtr vgcEvent = MouseReleaseEvent::create();
    fromQt(event, vgcEvent.get());
    MouseButton button = vgcEvent->button();
    if (!pressedMouseButtons_.has(button)) {
        // Not pressed on mouse: ignore event.
        event->setAccepted(true);
        return;
    }
    pressedMouseButtons_.unset(button);
    prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
    event->setAccepted(mouseReleaseEvent_(vgcEvent.get()));
    if (isLeaveDeferred_ && pressedMouseButtons_ == MouseButton::None) {
        isLeaveDeferred_ = false;
        if (widget_ && !widget_->mouseCaptor()) {
            event->setAccepted(widget_->setHovered(false));
        }
    }
}

void Window::tabletEvent(QTabletEvent* event) {
    // Reset scroll accumulator
    accumulatedScrollDelta_ = {};
    // Process event
    switch (event->type()) {
    case QEvent::TabletMove: {
        MouseMoveEventPtr vgcEvent = MouseMoveEvent::create();
        fromQt(event, vgcEvent.get());
        prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
        mouseMoveEvent_(vgcEvent.get());
        // Always accept to prevent Qt from retrying as a mouse event.
        event->setAccepted(true);
        timeSinceLastTableEvent_.restart();
        break;
    }
    case QEvent::TabletPress: {
        MousePressEventPtr vgcEvent = MousePressEvent::create();
        fromQt(event, vgcEvent.get());
        MouseButton button = vgcEvent->button();
        if (pressedTabletButtons_.has(button)) {
            // Already pressed on tablet: ignore event.
            event->setAccepted(true);
            break;
        }
        if (pressedMouseButtons_.has(button)) {
            // Already pressed on mouse: ignore event.
            event->setAccepted(true);
            break;
        }
        pressedTabletButtons_.set(button);
        prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
        mousePressEvent_(vgcEvent.get());
        // Always accept to prevent Qt from retrying as a mouse event.
        event->setAccepted(true);
        timeSinceLastTableEvent_.restart();
        break;
    }
    case QEvent::TabletRelease: {
        MouseReleaseEventPtr vgcEvent = MouseReleaseEvent::create();
        fromQt(event, vgcEvent.get());
        MouseButton button = vgcEvent->button();
        if (!pressedTabletButtons_.has(button)) {
            // Not pressed on tablet: ignore event.
            event->setAccepted(true);
            return;
        }
        pressedTabletButtons_.unset(button);
        prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
        mouseReleaseEvent_(vgcEvent.get());
        // Always accept to prevent Qt from retrying as a mouse event.
        event->setAccepted(true);
        timeSinceLastTableEvent_.restart();
        break;
    }
    case QEvent::TabletEnterProximity: {
        timeSinceLastTableEvent_.restart();
        tabletInProximity_ = true;
        break;
    }
    case QEvent::TabletLeaveProximity: {
        timeSinceLastTableEvent_.restart();
        tabletInProximity_ = false;
        // XXX should we do this ?
        // pressedTabletButtons_.clear();
        break;
    }
    default:
        // nothing
        break;
    }
}

void Window::wheelEvent(QWheelEvent* event) {
    ScrollEventPtr vgcEvent = ScrollEvent::create();
    fromQt(event, vgcEvent.get());
    geometry::Vec2f delta = vgcEvent->scrollDelta();
    std::array<Int, 2> scrollSteps = {};
    for (Int i = 0; i < 2; ++i) {
        float acc = accumulatedScrollDelta_[i];
        float d = delta[i];
        if (d != 0) {
            if (acc == 0) {
                acc = d;
            }
            else if (std::signbit(acc) != std::signbit(d)) {
                // If the scroll direction change we restart the accumulation from zero
                acc = d;
            }
            else {
                acc += d;
            }
            float integralPart = 0;
            accumulatedScrollDelta_[i] = std::modf(acc, &integralPart);
            scrollSteps[i] = static_cast<Int>(integralPart);
        }
    }
    vgcEvent->setHorizontalSteps(scrollSteps[0]);
    vgcEvent->setVerticalSteps(scrollSteps[1]);
    prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
    event->setAccepted(mouseScrollEvent_(vgcEvent.get()));
}

// Qt tablet event handling is buggy.
// Tablet proximity events may not happen in some cases according to some Qt user reports.
// Also, the documentation says accepting a tablet event prevents it from being resent
// through as a mouse event (fallback mechanism for apps not handling tablet events).
// However we tested this and still had an especially bad result on windows:
// Real input:
//    - tablet press
//    - tablet release
// Received events:
//    - tablet press
//    - tablet release
//    - mouse press
//    - mouse release
// To filter those spurious events out we use an additional timer.
//
bool Window::isTabletInUse_() const {
    return pressedTabletButtons_ //
           || tabletInProximity_ //
           || timeSinceLastTableEvent_.elapsed() < tabletIdleDuration_;
}

bool Window::mouseMoveEvent_(MouseMoveEvent* event) {
    bool isHandled = false;
    Widget* mouseCaptor = widget_->mouseCaptor();
    if (!mouseCaptor && !widget_->isHovered()) {
        if (widget_->geometry().contains(event->position())) {
            widget_->setHovered(true);
            entered_ = true;
        }
        else {
            return false;
        }
    }
    if (mouseCaptor) {
        isHandled = mouseCaptor->onMouseMove(event);
    }
    else {
        isHandled = widget_->mouseMove(event);
    }
    return isHandled;
}

bool Window::mousePressEvent_(MousePressEvent* event) {
    bool isHandled = false;
    Widget* mouseCaptor = widget_->mouseCaptor();
    if (mouseCaptor) {
        isHandled = mouseCaptor->onMousePress(event);
    }
    else {
        isHandled = widget_->mousePress(event);
    }
    return isHandled;
}

bool Window::mouseReleaseEvent_(MouseReleaseEvent* event) {
    bool isHandled = false;
    Widget* mouseCaptor = widget_->mouseCaptor();
    if (mouseCaptor) {
        isHandled = mouseCaptor->onMouseRelease(event);
    }
    else {
        isHandled = widget_->mouseRelease(event);
    }
    return isHandled;
}

bool Window::mouseScrollEvent_(ScrollEvent* event) {
    bool isHandled = false;
    Widget* mouseCaptor = widget_->mouseCaptor();
    if (mouseCaptor) {
        isHandled = mouseCaptor->onMouseScroll(event);
    }
    else {
        isHandled = widget_->mouseScroll(event);
    }
    return isHandled;
}

void Window::focusInEvent(QFocusEvent* event) {
    FocusReason reason = static_cast<FocusReason>(event->reason());
    widget_->setTreeActive(true, reason);
}

void Window::focusOutEvent(QFocusEvent* event) {
    FocusReason reason = static_cast<FocusReason>(event->reason());
    widget_->setTreeActive(false, reason);
}

void Window::keyPressEvent(QKeyEvent* event) {

    KeyPressEventPtr vgcEvent = KeyPressEvent::create();
    fromQt(event, vgcEvent.get());
    Widget* keyboardCaptor = widget_->keyboardCaptor();
    bool isHandled = false;
    if (keyboardCaptor) {
        isHandled = keyboardCaptor->onKeyPress(vgcEvent.get());
    }
    else {
        isHandled = widget_->keyPress(vgcEvent.get());
    }

    // Handle window-wide shortcuts
    if (!isHandled) {
        Key key = vgcEvent->key();
        if (key != Key::None) {
            Shortcut shortcut(vgcEvent->modifierKeys(), key);
            auto it = shortcutMap_.find(shortcut);
            if (it != shortcutMap_.end()) {

                // Found matching shortcut => trigger action
                Action* action = it->second;
                isHandled = true;
                action->trigger();
            }
        }
    }

    event->setAccepted(isHandled);
}

void Window::keyReleaseEvent(QKeyEvent* event) {
    KeyReleaseEventPtr vgcEvent = KeyReleaseEvent::create();
    fromQt(event, vgcEvent.get());
    Widget* keyboardCaptor = widget_->keyboardCaptor();
    bool isHandled = false;
    if (keyboardCaptor) {
        isHandled = keyboardCaptor->onKeyRelease(vgcEvent.get());
    }
    else {
        isHandled = widget_->keyRelease(vgcEvent.get());
    }
    event->setAccepted(isHandled);
}

void Window::inputMethodEvent(QInputMethodEvent* event) {

    // For now, we only use a very simple implementation that, at the very
    // least, correctly handle dead keys. See:
    //
    // https://stackoverflow.com/questions/28793356/qt-and-dead-keys-in-a-custom-widget
    //
    if (!event->commitString().isEmpty()) {
        // XXX Shouldn't we pass more appropriate modifiers?
        QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier, event->commitString());
        keyPressEvent(&keyEvent);
    }
}

void Window::inputMethodQueryEvent(QInputMethodQueryEvent* event) {
    Qt::InputMethodQueries queries = event->queries();
    for (UInt32 i = 0; i < 32; ++i) {
        Qt::InputMethodQuery query = static_cast<Qt::InputMethodQuery>(1 << i);
        if (queries.testFlag(query)) {
            QVariant value = inputMethodQuery(query);
            event->setValue(query, value);
        }
    }
    event->accept();
}

QVariant Window::inputMethodQuery(Qt::InputMethodQuery query) {

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
    // For now, we simply return something relevant for the `Enabled` query (to
    // ensure that we receive further queries and input method events), and
    // return an empty QVariant for all other queries. Most likely, this means
    // that many (most?) IME won't work with our app, but at least dead keys
    // work. Fixing this is left for future work.
    //
    // Also see:
    //
    // - https://stackoverflow.com/questions/43078567/qt-inputmethodevent-get-the-keyboard-key-that-was-pressed
    // - https://stackoverflow.com/questions/3287180/putting-ime-in-a-custom-text-box-derived-from-control
    // - https://stackoverflow.com/questions/434048/how-do-you-use-ime
    //
    if (query == Qt::ImEnabled) {
        bool res = false;
        if (focusedWidget_ && focusedWidget_->isTextInputReceiver()) {
            res = true;
        }
        return QVariant(res);
    }
    else {
        // TODO: handle other queries more appropriately.
        return QVariant();
    }
}

bool Window::updateScreenScaleRatio_() {

    // Update DPI scaling info. Examples of hiDpi configurations:
    //
    //                     macOS     Windows    Kubuntu/X11
    //                    (Retina)   at 125%     at 125%
    //
    // logicalDotsPerInch   72         120         120      (Note: 120 = 96 * 1.25)
    //
    // devicePixelRatio     2           1           1
    //
    // Note: on Kubuntu 22.04 (X11) at 100%, with Qt 5.15, the function
    // screen()->logicalDotsPerInch() returns 96.26847 instead of exactly 96
    // (at 125%, it returns exaxtly 120). So we round it in order to have a
    // screenScaleRatio of exactly 1.0.
    //
    if (screen()) {
        logicalDotsPerInch_ = static_cast<float>(screen()->logicalDotsPerInch());
        if (std::abs(logicalDotsPerInch_ - detail::baseLogicalDpi) < 5) {
            logicalDotsPerInch_ = detail::baseLogicalDpi;
        }
    }
    devicePixelRatio_ = static_cast<float>(devicePixelRatio());

    // Compute suitable screen scale ratio based on queried info
    float s = logicalDotsPerInch_ * devicePixelRatio_ / detail::baseLogicalDpi;

    // Update style metrics if changed
    if (screenScaleRatio_ != s) {
        screenScaleRatio_ = s;
        if (widget_) {
            style::Metrics metrics(screenScaleRatio_);
            widget_->setStyleMetrics(metrics);
        }
        return true;
    }
    else {
        return false;
    }
}

bool Window::updateScreenScaleRatioAndWindowSize1_(
    Int unscaledWidth,
    Int unscaledHeight) {

    // Update screen scale ratio
    bool screenScaleRatioChanged = updateScreenScaleRatio_();

    // Update window size
    float w = static_cast<float>(unscaledWidth);
    float h = static_cast<float>(unscaledHeight);
    width_ = static_cast<Int>(std::round(w * devicePixelRatio_));
    height_ = static_cast<Int>(std::round(h * devicePixelRatio_));

    // Redraw when switching from two monitors with different DPI scaling on Windows.
    //
    // Under most circumstances, there is no need to explicitly call paint() in
    // this function, since updateScreenScaleRatio_() calls
    // widget_->setStyleMetrics() which calls requestUpdate(). However, when
    // when activeSizemove_ is true, update requests are ignored, so we have to
    // call paint() explicitly for a repaint to actually happen.
    //
    return screenScaleRatioChanged && activeSizemove_;
}

void Window::updateScreenScaleRatioAndWindowSize2_(bool shouldRepaint) {
    if (shouldRepaint) {
        VGC_WINDOW_DEBUG_NOARGS("Note: repainting because screenScaleRatio changed");
        paint(true);
    }
}

void Window::updateViewportSize_() {

    float w = static_cast<float>(width_);
    float h = static_cast<float>(height_);

    VGC_WINDOW_DEBUG("updateViewportSize_({}, {})", width_, height_);

    // Update projection matrix
    geometry::Camera2d c;
    c.setViewportSize(w, h);
    proj_ = detail::toMat4f(c.projectionMatrix());

    // Update size of root widget
    if (widget_) {
        widget_->updateGeometry(0, 0, w, h);
    }

    // Update size of GPU resources: render targets, framebuffers, etc.
    //
    // Note: this can be quite slow with MSAA on. It will probably be better
    // when we have a compositor.
    //
    if (engine_ && swapChain_) {
        engine_->onWindowResize(swapChain_, width_, height_);
    }
}

void Window::exposeEvent(QExposeEvent*) {
    if (isExposed()) {
        if (activeSizemove_) {
            // On Windows, Expose events happen on both WM_PAINT and WM_ERASEBKGND
            // but in the case of a resize we already redraw properly.
            VGC_WINDOW_DEBUG(
                "exposeEvent(({}, {}), activeSizemove={})",
                width_,
                height_,
                activeSizemove_);
            requestUpdate();
        }
        else {
            // On macOS, moving a window between monitors with different devicePixelRatios
            // calls exposeEvent() but doesn't call resize(). So we need to fake a resize
            // here if the size in px of the window change, even though the "QWindow size"
            // (in device-independent scale) doesn't change.
            //
            Int oldScaledWidth = width_;
            Int oldScaledHeight = height_;
            Int unscaledWidth_ = width();
            Int unscaledHeight_ = height();
            bool b =
                updateScreenScaleRatioAndWindowSize1_(unscaledWidth_, unscaledHeight_);
            VGC_WINDOW_DEBUG(
                "exposeEvent(({}, {}), activeSizemove={})",
                width_,
                height_,
                activeSizemove_);
            updateScreenScaleRatioAndWindowSize2_(b);
            if (oldScaledWidth != width_ || oldScaledHeight != height_) {
                updateViewportSize_();
            }
            paint(true);
        }
    }
}

void Window::resizeEvent(QResizeEvent* event) {

    // Remember old size
    [[maybe_unused]] Int oldWidth = width_;
    [[maybe_unused]] Int oldHeight = height_;

    // Get new unscaled size
    QSize size = event->size();
    Int unscaledWidth = size.width();
    Int unscaledHeight = size.height();

    // Compute and set new scale ratio and scaled size
    bool b = updateScreenScaleRatioAndWindowSize1_(unscaledWidth, unscaledHeight);
    VGC_WINDOW_DEBUG("resizeEvent({}, {})", width_, height_);
    updateScreenScaleRatioAndWindowSize2_(b);

#if defined(VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX)
    // Wait until WM_SIZE native event to actually set new window size
    width_ = oldWidth;
    height_ = oldHeight;
#else
    deferredResize_ = true;
    updateViewportSize_();
    requestUpdate();
#endif
}

void Window::updateRequestEvent(QEvent*) {
    if (!activeSizemove_) {
        VGC_WINDOW_DEBUG(
            "updateRequestEvent({}, {}) deferredResize={}",
            width_,
            height_,
            deferredResize_);
#if !defined(VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX)
        if (deferredResize_) {
            deferredResize_ = false;
            updateViewportSize_();
        }
#endif
        paint(true);
    }
}

void Window::paint(bool sync) {

    VGC_WINDOW_DEBUG("paint(({}, {}), sync={})", width_, height_, sync);

    if (updateDeferred_) {
        return;
    }

    if (!isExposed()) {
        return;
    }

    if (!engine_) {
        throw LogicError("engine_ is null.");
    }

    if (!swapChain_) {
        throw LogicError("swapChain_ is null.");
    }

    if (swapChain_->numPendingPresents() > 0 && !sync) {
        // race condition possible but unlikely here.
        updateDeferred_ = true;
        return;
    }

    engine_->beginFrame(swapChain_, graphics::FrameKind::Window);

    engine_->setRasterizerState(rasterizerState_);
    engine_->setBlendState(blendState_, geometry::Vec4f());
    engine_->setViewport(0, 0, width_, height_);
    engine_->setScissorRect(rect());
    if (isBackgroundPainted()) {
        engine_->clear(backgroundColor());
    }
    engine_->setProgram(graphics::BuiltinProgram::Simple);
    engine_->setProjectionMatrix(proj_);
    engine_->setViewMatrix(geometry::Mat4f::identity);

    if (widget_->isGeometryUpdateRequested()) {
        widget_->updateGeometry();
    }

    {
        //VGC_PROFILE_SCOPE("Window:MainWidgetPaint");
        widget_->paint(engine_.get());
    }

#if defined(VGC_QOPENGL_EXPERIMENT)
    static int frameIdx = 0;
    auto fmt = format();
    OutputDebugString(
        core::format("Window swap behavior: {}\n", (int)fmt.swapBehavior()).c_str());
    OutputDebugString(
        core::format("Window swap interval: {}\n", fmt.swapInterval()).c_str());
    OutputDebugString(core::format("frameIdx: {}\n", frameIdx).c_str());
    frameIdx++;
#endif

    {
        //VGC_PROFILE_SCOPE("Window:EndFrame");
        // XXX make it endInlineFrame in QglEngine and copy its code into Engine::present()
        engine_->endFrame(sync ? 1 : 0 + 0);
    }
}

bool Window::event(QEvent* event) {
    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(4063) // invalid switch value (custom event types)
    VGC_WARNING_GCC_DISABLE(switch)
    switch (event->type()) {
    case QEvent::InputMethodQuery:
        inputMethodQueryEvent(static_cast<QInputMethodQueryEvent*>(event));
        return true;
    case QEvent::InputMethod:
        inputMethodEvent(static_cast<QInputMethodEvent*>(event));
        return true;
    case QEvent::Enter:
        enterEvent(event);
        return true;
    case QEvent::Leave:
        leaveEvent(event);
        return true;
    case QEvent::UpdateRequest:
        updateRequestEvent(event);
        return true;
    case QEvent::ShortcutOverride:
        event->accept();
        break;
    // custom event types
    case presentCalledEvent:
        if (updateDeferred_) {
            updateDeferred_ = false;
            updateRequestEvent(event);
        }
        break;
    default:
        break;
    }
    return QWindow::event(event);
    VGC_WARNING_POP
}

// These events may not exist on some Qt versions and OSs.
// Also, see isTabletInUse_() for the workaround.
//
bool Window::eventFilter(QObject* obj, QEvent* event) {
    switch (event->type()) {
    case QEvent::TabletEnterProximity:
        tabletEvent(static_cast<QTabletEvent*>(event));
        break;
    case QEvent::TabletLeaveProximity:
        tabletEvent(static_cast<QTabletEvent*>(event));
        break;
    default:
        break;
    }
    return QWindow::eventFilter(obj, event);
}

bool Window::nativeEvent(
    [[maybe_unused]] const QByteArray& eventType,
    [[maybe_unused]] void* message,
    [[maybe_unused]] NativeEventResult* result) {

#if defined(VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX)

    if (eventType == "windows_generic_MSG") {
        *result = 0;
        MSG* msg = reinterpret_cast<MSG*>(message);
        switch (msg->message) {
        case WM_SIZE: {

            // Get new unscaled size
            Int unscaledWidth = static_cast<Int>(LOWORD(msg->lParam));
            Int unscaledHeight = static_cast<Int>(HIWORD(msg->lParam));

            // Compute and set new scale ratio and scaled size
            bool b = updateScreenScaleRatioAndWindowSize1_(unscaledWidth, unscaledHeight);
            VGC_WINDOW_DEBUG("WM_SIZE({}, {})", width_, height_);
            updateScreenScaleRatioAndWindowSize2_(b);

            updateViewportSize_();
            paint(true);

            return false;
        }
        /*case WM_MOVING: {
            VGC_WINDOW_DEBUG_NOARGS("WM_MOVING");
            return false;
        }
        case WM_MOVE: {
            VGC_WINDOW_DEBUG_NOARGS("WM_MOVE");
            return false;
        }
        case WM_WINDOWPOSCHANGING: {
            VGC_WINDOW_DEBUG_NOARGS("WM_WINDOWPOSCHANGING");
            return false;
        }
        case WM_WINDOWPOSCHANGED: {
            VGC_WINDOW_DEBUG_NOARGS("WM_WINDOWPOSCHANGED");
            return false;
        }
        case WM_GETMINMAXINFO: {
            VGC_WINDOW_DEBUG_NOARGS("WM_GETMINMAXINFO");
            return false;
        }*/
        case WM_ENTERSIZEMOVE: {
            activeSizemove_ = true;
            return false;
        }
        case WM_EXITSIZEMOVE: {
            activeSizemove_ = false;
            // If the last render was downgraded for performance during WM_MOVE
            // we should ask for a redraw now.
            requestUpdate();
            return false;
        }
        case WM_ERASEBKGND: {
            static int i = 0;
            VGC_WINDOW_DEBUG("WM_ERASEBKGND {}", ++i);
            //if (activeSizemove_) {
            //    paint(true);
            //}

            // Try using GetDC()?
            // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc

            //*result = 1;
            //return true;
            return false;
        }
        case WM_PAINT: {
            static int i = 0;
            if (debugEvents) {
                VGC_WINDOW_DEBUG("WM_PAINT {}", ++i);
            }
            if (activeSizemove_) {
                paint(true);
            }
            return false;
        }
        default:
            break;
        }
    }
    //*result = ::DefWindowProcW(msg->hwnd, msg->message, msg->wParam, msg->lParam);
    //return true;

#endif

    return false;
}

#if defined(VGC_CORE_OS_WINDOWS)

LRESULT WINAPI Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* w = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, 19 * sizeof(LONG_PTR)));
    NativeEventResult res = {};
    MSG mmsg = {};
    mmsg.message = msg;
    mmsg.hwnd = hwnd;
    mmsg.wParam = wParam;
    mmsg.lParam = lParam;
    switch (msg) {
    case WM_SIZE:
        w->nativeEvent(QByteArray("windows_generic_MSG"), &mmsg, &res);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

#endif

void Window::initEngine_() {

    graphics::SwapChainCreateInfo swapChainCreateInfo = {};

    graphics::EngineCreateInfo engineCreateInfo = {};
    graphics::WindowSwapChainFormat& windowSwapChainFormat =
        engineCreateInfo.windowSwapChainFormat();
    windowSwapChainFormat.setNumBuffers(2);
    windowSwapChainFormat.setNumSamples(8);

#if defined(VGC_CORE_OS_MACOS)
    engineCreateInfo.setMultithreadingEnabled(false);
#else
    engineCreateInfo.setMultithreadingEnabled(true);
#endif

#if defined(VGC_CORE_OS_WINDOWS) && TRUE

    // Set WindowSwapChainFormat to RGBA_8_UNORM, that is, linear RGB.
    //
    // This is a counter-intuitive hack made necessary by our current pipeline:
    // - We write sRGB values to the vertex buffer
    // - Our vertex shaders and pixel shaders process the colors as sRGB
    // - The pixel shader outputs an sRGB color
    // - We want the framebuffer to store an sRGB color
    //
    // The problem is that Direct3D *assumes* that the shaders are processing
    // and outputting linear RGB colors. So if we set WindowSwapChainFormat to
    // RGBA_8_UNORM_SRGB (which makes more sense intuitively, since we want the
    // final color to be stored as SRGB), it would apply a linear-to-sRGB
    // conversion between the output of the pixel shader and what is stored in
    // the framebuffer, producing in our case an incorrect color (the gamma
    // correction is applied twice).
    //
    // By using RGBA_8_UNORM instead, Direct3D assumes that the pixel shader is
    // in linear RGB, but since we ask it to store it as linear RGB in the
    // framebuffer, Direct3D does not perform any conversion, and store the
    // values as is (that is, in practice in our case, sRGB values are stored,
    // but Direct3D thinks it is linear).
    //
    // Later, these values are *interepreted* as sRGB anyway by Windows.
    //
    // TODO: A proper fix would to send linear RGB as input to our shaders, and
    // process everything in our shaders in linear RGB. We could then set
    // WindowSwapChainFormat to RGBA_8_UNORM_SRGB, which would convert those to
    // sRGB to store them in the framebuffer, which are then interpreted by
    // Windows as sRGB.
    //
    windowSwapChainFormat.setPixelFormat(graphics::WindowPixelFormat::RGBA_8_UNORM);

    // RasterSurface looks ok since Qt seems to not automatically create a backing store.
    setSurfaceType(QWindow::RasterSurface);
    QWindow::create();
    engine_ = graphics::D3d11Engine::create(engineCreateInfo);

    HWND hwnd = (HWND)QWindow::winId();
    hwnd_ = hwnd;
    swapChainCreateInfo.setWindowNativeHandle(
        hwnd, graphics::WindowNativeHandleType::Win32);

    //WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, Window::WndProc, 0L, 20 * sizeof(LONG_PTR), ::GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Win32 Window", NULL };
    //::RegisterClassExW(&wc);
    //HWND hwnd = ::CreateWindowExW(0L, wc.lpszClassName, L"Win32 Window", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
    //::SetWindowLongPtrW(hwnd, 19 * sizeof(LONG_PTR), (LONG_PTR)(Window*)this);
    //scd.setWindowNativeHandle(hwnd, graphics::WindowNativeHandleType::Win32);
    //::ShowWindow(hwnd, SW_SHOWDEFAULT);
    //::UpdateWindow(hwnd);

    // get window class info

    WINDOWINFO windowInfo = {};
    windowInfo.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(hwnd, &windowInfo);

    wchar_t className[400];
    GetClassNameW(hwnd, className, 400);

    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    WNDCLASSEXW wndClassExW = {};
    wndClassExW.cbSize = sizeof(WNDCLASSEXW);
    GetClassInfoExW(hInstance, className, &wndClassExW);

    std::wstring classNameW(className);
    std::string classNameA(classNameW.begin(), classNameW.end());
    //VGC_INFO(LogVgcUi, "Window class name: {}", classNameA);
#else
    engine_ = detail::QglEngine::create(engineCreateInfo);
    swapChainCreateInfo.setWindowNativeHandle(
        static_cast<QWindow*>(this), graphics::WindowNativeHandleType::QOpenGLWindow);
#endif

    swapChain_ = engine_->createSwapChain(swapChainCreateInfo);

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

    engine_->setPresentCallback([=](UInt64) {
        QCoreApplication::postEvent(this, new QEvent(presentCalledEvent), 0);
    });
}

namespace {

bool isWindowShortcut_(Action* action) {

    // For now, we store both application-wide and window-wide shortcuts in the
    // Window's shortcut map. Later, we may want to store application-wide
    // shortcuts elsewhere.
    //
    return action->shortcutContext() == ShortcutContext::Application
           || action->shortcutContext() == ShortcutContext::Window;
}

} // namespace

void Window::addShortcuts_(Widget* widget) {
    widget->actionAdded().connect(onActionAddedSlot_());
    widget->actionRemoved().connect(onActionRemovedSlot_());
    for (Action* action : widget->actions()) {
        addShortcut_(action);
    }
    for (Widget* child : widget->children()) {
        addShortcuts_(child);
    }
}

void Window::removeShortcuts_(Widget* widget) {
    widget->actionAdded().disconnect(onActionAddedSlot_());
    widget->actionRemoved().disconnect(onActionRemovedSlot_());
    for (Action* action : widget->actions()) {
        removeShortcut_(action);
    }
    for (Widget* child : widget->children()) {
        removeShortcuts_(child);
    }
}

void Window::addShortcut_(Action* action) {
    if (!isWindowShortcut_(action)) {
        return;
    }
    bool anyShortcutInserted = false;
    for (const Shortcut& shortcut : action->userShortcuts()) {
        if (shortcut.key() == Key::None) {
            break;
        }
        auto [it, inserted] = shortcutMap_.insert({shortcut, action});
        if (inserted) {
            anyShortcutInserted = true;
        }
        else {
            Action* otherAction = it->second;
            VGC_WARNING(
                LogVgcUi,
                "Shortcut [{}] for action \"{}\" ignored, "
                "as it conflicts with action \"{}\".",
                shortcut,
                action->text(),
                otherAction->text());
        }
    }
    if (anyShortcutInserted) {
        action->aboutToBeDestroyed().connect(onActionAboutToBeDestroyedSlot_());
    }
}

void Window::removeShortcut_(Action* action) {
    if (!isWindowShortcut_(action)) {
        return;
    }
    for (const Shortcut& shortcut : action->userShortcuts()) {
        if (shortcut.key() == Key::None) {
            break;
        }
        auto it = shortcutMap_.find(shortcut);
        if (it != shortcutMap_.end()) {
            Action* otherAction = it->second;
            if (otherAction == action) {
                shortcutMap_.erase(it);
            }
        }
    }
}

void Window::onActiveChanged_() {
    bool active = isActive();
    widget_->setTreeActive(active, FocusReason::Window);
}

void Window::onRepaintRequested_() {
    if (engine_) {
        requestUpdate();
    }
}

void Window::onMouseCaptureStarted_() {
    setMouseGrabEnabled(true);
}

void Window::onMouseCaptureStopped_() {
    setMouseGrabEnabled(false);
}

void Window::onKeyboardCaptureStarted_() {
    setKeyboardGrabEnabled(true);
}

void Window::onKeyboardCaptureStopped_() {
    setKeyboardGrabEnabled(false);
}

void Window::onFocusSetOrCleared_() {

    // Fast return if the focused widget hasn't actually changed.
    //
    // Note that if there was several widget trees (e.g., mixing QtWidgets and
    // vgc::ui), then we shoul not fast return, but instead set as active the
    // tree that emits `focusSet()` (as long as the window itself is
    // active). Indeed, receiving `focusSet()` from a tree typically means that
    // the user clicked on a widget in the tree, so this tree should now become
    // active, regardless if the user clicked on the focused widget or not.
    //
    if (focusedWidget_.get() == widget_->focusedWidget()) {
        return;
    }

    // Otherwise, update connections and InputMethod handling.
    //
    if (focusedWidget_) {
        focusedWidget_->textInputReceiverChanged().disconnect(
            onTextInputReceiverChangedSlot_());
    }
    focusedWidget_ = widget_->focusedWidget();
    if (focusedWidget_) {
        focusedWidget_->textInputReceiverChanged().connect(
            onTextInputReceiverChangedSlot_());
    }
    onTextInputReceiverChanged_();
}

void Window::onTextInputReceiverChanged_() {
    QGuiApplication::inputMethod()->update(Qt::ImEnabled);
}

void Window::onWidgetAddedToTree_(Widget* widget) {
    addShortcuts_(widget);
}

void Window::onWidgetRemovedFromTree_(Widget* widget) {
    if (!widget->hasReachedStage(core::ObjectStage::AboutToBeDestroyed)) {
        removeShortcuts_(widget);
    }
}

void Window::onActionAdded_(Action* action) {
    addShortcut_(action);
}

void Window::onActionRemoved_(Action* action) {
    removeShortcut_(action);
}

void Window::onActionAboutToBeDestroyed_(Object* obj) {
    Action* action = static_cast<Action*>(obj);
    removeShortcut_(action);
}

} // namespace vgc::ui
