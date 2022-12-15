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
#include <vgc/geometry/camera2d.h>
#include <vgc/graphics/d3d11/d3d11engine.h>
#include <vgc/graphics/text.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/qtutil.h>

#include <vgc/ui/detail/qopenglengine.h>

namespace vgc::ui {

namespace {

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

Window::Window(const WidgetPtr& widget)
    : QWindow()
    , widget_(widget)
    , proj_(geometry::Mat4f::identity)
    , clearColor_(0.251f, 0.259f, 0.267f, 1.f) {

    connect((QWindow*)this, &QWindow::activeChanged, this, &Window::onActiveChanged_);

    //setMouseTracking(true);
    widget_->repaintRequested().connect(onRepaintRequestedSlot_());
    widget_->mouseCaptureStarted().connect(onMouseCaptureStartedSlot_());
    widget_->mouseCaptureStopped().connect(onMouseCaptureStoppedSlot_());
    widget_->keyboardCaptureStarted().connect(onKeyboardCaptureStartedSlot_());
    widget_->keyboardCaptureStopped().connect(onKeyboardCaptureStoppedSlot_());
    //widget_->focusRequested().connect([this](){ this->onFocusRequested(); });
    widget_->widgetAddedToTree().connect(onWidgetAddedToTreeSlot_());
    widget_->widgetRemovedFromTree().connect(onWidgetRemovedFromTreeSlot_());

    initEngine_();
    addShortcuts_(widget_.get());

    // Handle dead keys and complex input methods.
    //
    // XXX TODO: We should enable/disable this property based on whether the
    // current keyboard-focused widget is a text-editing widget, similarly to
    // what Qt::WA_InputMethodEnabled is used for.
    //
    QGuiApplication::inputMethod()->update(Qt::ImEnabled);

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
    return WindowPtr(new Window(widget));
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

    // Apply device pixel ratio
    geometry::Vec2f position = event->position();
    position *= static_cast<float>(window->devicePixelRatio());
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
    MouseEventPtr vgcEvent = fromQt(event);
    prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
    event->setAccepted(mouseMoveEvent_(vgcEvent.get()));
}

void Window::mousePressEvent(QMouseEvent* event) {
    MouseEventPtr vgcEvent = fromQt(event);
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
    MouseEventPtr vgcEvent = fromQt(event);
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
    switch (event->type()) {
    case QEvent::TabletMove: {
        MouseEventPtr vgcEvent = fromQt(event);
        prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
        mouseMoveEvent_(vgcEvent.get());
        // Always accept to prevent Qt from retrying as a mouse event.
        event->setAccepted(true);
        timeSinceLastTableEvent_.restart();
        break;
    }
    case QEvent::TabletPress: {
        MouseEventPtr vgcEvent = fromQt(event);
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
        MouseEventPtr vgcEvent = fromQt(event);
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

bool Window::mouseMoveEvent_(MouseEvent* event) {
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

bool Window::mousePressEvent_(MouseEvent* event) {
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

bool Window::mouseReleaseEvent_(MouseEvent* event) {
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

void Window::focusInEvent(QFocusEvent* event) {
    FocusReason reason = static_cast<FocusReason>(event->reason());
    widget_->setTreeActive(true, reason);
}

void Window::focusOutEvent(QFocusEvent* event) {
    FocusReason reason = static_cast<FocusReason>(event->reason());
    widget_->setTreeActive(false, reason);
}

void Window::keyPressEvent(QKeyEvent* event) {

    KeyEventPtr vgcEvent = fromQt(event);
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
    KeyEventPtr vgcEvent = fromQt(event);
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
    // For now, we simply return `true` for the `Enabled` query (to ensure that
    // we receive further queries and input method events), and return an empty
    // QVariant for all other queries. Most likely, this means that many
    // (most?) IME won't work with our app, but at least dead keys work. Fixing
    // this is left for future work.
    //
    // Also see:
    //
    // - https://stackoverflow.com/questions/43078567/qt-inputmethodevent-get-the-keyboard-key-that-was-pressed
    // - https://stackoverflow.com/questions/3287180/putting-ime-in-a-custom-text-box-derived-from-control
    // - https://stackoverflow.com/questions/434048/how-do-you-use-ime
    //
    if (query == Qt::ImEnabled) {
        // TODO: return true only if the current focus widget accepts text input.
        return QVariant(true);
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
    updateDeferred_ = false;

    engine_->beginFrame(swapChain_, graphics::FrameKind::Window);

    engine_->setRasterizerState(rasterizerState_);
    engine_->setBlendState(blendState_, geometry::Vec4f());
    engine_->setViewport(0, 0, width_, height_);
    engine_->setScissorRect(rect());
    engine_->clear(clearColor_);
    engine_->setProgram(graphics::BuiltinProgram::Simple);
    engine_->setProjectionMatrix(proj_);
    engine_->setViewMatrix(geometry::Mat4f::identity);

    if (widget_->isGeometryUpdateRequested()) {
        widget_->updateGeometry();
    }

    widget_->paint(engine_.get());

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

    // XXX make it endInlineFrame in QglEngine and copy its code into Engine::present()
    engine_->endFrame(sync ? 1 : 0 + 0);
}

bool Window::event(QEvent* event) {
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
    default:
        break;
    }
    return QWindow::event(event);
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
        if (updateDeferred_) {
            QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest), 0);
        }
    });
}

namespace {

bool isWindowShorctut_(Action* action) {

    // For now, we store both application-wide and window-wide shortcuts in the
    // Window's shortcut map. Later, we may want to store application-wide
    // shortcuts elsewhere.
    //
    return action->shortcutContext() == ShortcutContext::Application
           || action->shortcutContext() == ShortcutContext::Window;
}

} // namespace

void Window::addShortcuts_(Widget* widget) {
    for (Action* action : widget->actions()) {
        addShorctut_(action);
    }
    for (Widget* child : widget->children()) {
        addShortcuts_(child);
    }
}

void Window::removeShortcuts_(Widget* widget) {
    for (Action* action : widget->actions()) {
        removeShortcut_(action);
    }
    for (Widget* child : widget->children()) {
        removeShortcuts_(child);
    }
}

void Window::addShorctut_(Action* action) {
    if (!isWindowShorctut_(action)) {
        return;
    }
    Shortcut shortcut = action->shortcut();
    if (shortcut.key() == Key::None) {
        return;
    }
    auto [it, inserted] = shortcutMap_.insert({shortcut, action});
    if (!inserted) {
        Action* otherAction = it->second;
        VGC_WARNING(
            LogVgcUi,
            "Shortcut [{}] for action \"{}\" ignored, "
            "as it conflicts with action \"{}\".",
            shortcut,
            action->text(),
            otherAction->text());
    }
    else {
        action->aboutToBeDestroyed().connect(onActionAboutToBeDestroyedSlot_());
    }
}

void Window::removeShortcut_(Action* action) {
    if (!isWindowShorctut_(action)) {
        return;
    }
    Shortcut shortcut = action->shortcut();
    if (shortcut.key() == Key::None) {
        return;
    }
    auto it = shortcutMap_.find(shortcut);
    if (it != shortcutMap_.end()) {
        Action* otherAction = it->second;
        if (otherAction == action) {
            shortcutMap_.erase(it);
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

void Window::onWidgetAddedToTree_(Widget* widget) {
    addShortcuts_(widget);
}

void Window::onWidgetRemovedFromTree_(Widget* widget) {
    if (!widget->hasReachedStage(core::ObjectStage::AboutToBeDestroyed)) {
        removeShortcuts_(widget);
    }
}

void Window::onActionAboutToBeDestroyed_(Object* obj) {
    Action* action = static_cast<Action*>(obj);
    removeShortcut_(action);
}

} // namespace vgc::ui
