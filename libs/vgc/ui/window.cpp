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

static constexpr bool debugEvents = false;

//#define VGC_DISABLE_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX

#ifdef VGC_CORE_OS_WINDOWS
#    ifndef VGC_DISABLE_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX
#        define VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX
#    endif
#endif

Window::Window(WidgetPtr widget)
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

    graphics::SwapChainCreateInfo scd = {};

    graphics::EngineCreateInfo engineConfig = {};
    graphics::WindowSwapChainFormat& windowSwapChainFormat =
        engineConfig.windowSwapChainFormat();
    windowSwapChainFormat.setNumBuffers(2);
    windowSwapChainFormat.setNumSamples(8);
    engineConfig.setMultithreadingEnabled(true);

#if defined(VGC_CORE_OS_WINDOWS) && TRUE
    // RasterSurface looks ok since Qt seems to not automatically create a backing store.
    setSurfaceType(QWindow::RasterSurface);
    QWindow::create();
    engine_ = graphics::D3d11Engine::create(engineConfig);

    HWND hwnd = (HWND)QWindow::winId();
    hwnd_ = hwnd;
    scd.setWindowNativeHandle(hwnd, graphics::WindowNativeHandleType::Win32);

    //WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, Window::WndProc, 0L, 20 * sizeof(LONG_PTR), ::GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Win32 Window", NULL };
    //::RegisterClassExW(&wc);
    //HWND hwnd = ::CreateWindowExW(0L, wc.lpszClassName, L"Win32 Window", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
    //::SetWindowLongPtrW(hwnd, 19 * sizeof(LONG_PTR), (LONG_PTR)(Window*)this);
    //scd.setWindowNativeHandle(hwnd, graphics::WindowNativeHandleType::Win32);
    //::ShowWindow(hwnd, SW_SHOWDEFAULT);
    //::UpdateWindow(hwnd);

    // get window class info

    WINDOWINFO wi = {sizeof(WINDOWINFO)};
    GetWindowInfo(hwnd, &wi);
    wchar_t className[400];
    GetClassNameW(hwnd, className, 400);
    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
    GetClassInfoExW(hinst, className, &wcex);
    std::wstring classNameW(className);
    std::string classNameA(classNameW.begin(), classNameW.end());
    //VGC_INFO(LogVgcUi, "Window class name: {}", classNameA);
#else
    engine_ = detail::QglEngine::create(engineConfig);
    scd.setWindowNativeHandle(
        static_cast<QWindow*>(this), graphics::WindowNativeHandleType::QOpenGLWindow);
#endif

    swapChain_ = engine_->createSwapChain(scd);

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

    // Handle dead keys and complex input methods.
    //
    // XXX TODO: We should enable/disable this property based on whether the
    // current keyboard-focused widget is a text-editing widget, similarly to
    // what Qt::WA_InputMethodEnabled is used for.
    //
    QGuiApplication::inputMethod()->update(Qt::ImEnabled);
}

void Window::onDestroyed() {
    // Destroying the engine will stop it.
    engine_ = nullptr;
}

WindowPtr Window::create(WidgetPtr widget) {
    return WindowPtr(new Window(widget));
}

namespace {

Widget* prepareMouseEvent(Widget* root, MouseEvent* event, const Window* window) {

    // Apply device pixel ratio
    geometry::Vec2f position = event->position();
    position *= static_cast<float>(window->devicePixelRatio());
    event->setPosition(position);

    // Handle mouse captor
    Widget* mouseCaptor = root->mouseCaptor();
    if (mouseCaptor) {
        position = root->mapTo(mouseCaptor, position);
        event->setPosition(position);
        return mouseCaptor;
    }
    else {
        return root;
    }
}

} // namespace

void Window::mouseMoveEvent(QMouseEvent* event) {
    if (pressedTabletButtons_) {
        return;
    }
    MouseEventPtr vgcEvent = fromQt(event);
    Widget* receiver = prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
    event->setAccepted(mouseMoveEvent_(receiver, vgcEvent.get()));
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
    Widget* receiver = prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
    event->setAccepted(mousePressEvent_(receiver, vgcEvent.get()));
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
    Widget* receiver = prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
    event->setAccepted(mouseReleaseEvent_(receiver, vgcEvent.get()));
}

void Window::tabletEvent(QTabletEvent* event) {
    switch (event->type()) {
    case QEvent::TabletMove: {
        MouseEventPtr vgcEvent = fromQt(event);
        Widget* receiver = prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
        mouseMoveEvent_(receiver, vgcEvent.get());
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
        Widget* receiver = prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
        mousePressEvent_(receiver, vgcEvent.get());
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
        Widget* receiver = prepareMouseEvent(widget_.get(), vgcEvent.get(), this);
        mouseReleaseEvent_(receiver, vgcEvent.get());
        // Always accept to prevent Qt from retrying as a mouse event.
        event->setAccepted(true);
        timeSinceLastTableEvent_.restart();
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
    return pressedTabletButtons_ || tabletInProximity_
           || timeSinceLastTableEvent_.elapsed() < 1.0;
}

bool Window::mouseMoveEvent_(Widget* receiver, MouseEvent* event) {
    if (!widget_->isHovered()) {
        if (widget_->geometry().contains(event->position())) {
            widget_->setHovered(true);
            entered_ = true;
        }
        else {
            return false;
        }
    }
    bool handled = false;
    if (!receiver->isRoot()) {
        // mouse captor
        handled = receiver->onMouseMove(event);
    }
    else {
        handled = receiver->mouseMove(event);
    }
    return handled;
}

bool Window::mousePressEvent_(Widget* receiver, MouseEvent* event) {
    bool handled = false;
    if (!receiver->isRoot()) {
        // mouse captor
        handled = receiver->onMousePress(event);
    }
    else {
        handled = receiver->mousePress(event);
    }
    return handled;
}

bool Window::mouseReleaseEvent_(Widget* receiver, MouseEvent* event) {
    bool handled = false;
    if (!receiver->isRoot()) {
        // mouse captor
        handled = receiver->onMouseRelease(event);
    }
    else {
        handled = receiver->mouseRelease(event);
    }
    return handled;
}

// Note: enterEvent() and leaveEvent() are part of QWidget, but not of QWindow.
// Therefore, the methods below are not overriden from QWindow, but are instead
// custom methods that we explicitly call from Window::event().

void Window::enterEvent(QEvent* event) {
    entered_ = true;
    event->setAccepted(widget_->setHovered(true));
}

void Window::leaveEvent(QEvent* event) {
    entered_ = false;
    event->setAccepted(widget_->setHovered(false));
}

void Window::focusInEvent(QFocusEvent* event) {
    FocusReason reason = static_cast<FocusReason>(event->reason());
    widget_->setTreeActive(true, reason);
}

void Window::focusOutEvent(QFocusEvent* event) {
    FocusReason reason = static_cast<FocusReason>(event->reason());
    widget_->setTreeActive(false, reason);
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
    updateScreenScaleRatioAndWindowSize_(unscaledWidth, unscaledHeight);

    if (debugEvents) {
        VGC_DEBUG(
            LogVgcUi,
            "resizeEvent({:04d}, {:04d}) -> ({}, {})",
            unscaledWidth,
            unscaledHeight,
            width_,
            height_);
    }

#if defined(VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX)
    // Wait until WM_SIZE native event to actually set new window size
    width_ = oldWidth;
    height_ = oldHeight;
#else
    deferredResize_ = true;
    requestUpdate();
#endif
}

namespace {

std::pair<Widget*, QKeyEvent*> prepareKeyboardEvent(Widget* root, QKeyEvent* event) {
    Widget* keyboardCaptor = root->keyboardCaptor();
    if (keyboardCaptor) {
        return {keyboardCaptor, event};
    }
    else {
        return {root, event};
    }
}

} // namespace

void Window::keyPressEvent(QKeyEvent* event) {
    auto [receiver, vgcEvent] = prepareKeyboardEvent(widget_.get(), event);
    event->setAccepted(receiver->onKeyPress(vgcEvent));
}

void Window::keyReleaseEvent(QKeyEvent* event) {
    auto [receiver, vgcEvent] = prepareKeyboardEvent(widget_.get(), event);
    event->setAccepted(receiver->onKeyRelease(vgcEvent));
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

void Window::paint(bool sync) {
    if (debugEvents) {
        VGC_DEBUG(LogVgcUi, "paint({})", sync);
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
    updateDeferred_ = false;

    engine_->beginFrame(swapChain_, graphics::FrameKind::Window);

    engine_->setRasterizerState(rasterizerState_);
    engine_->setBlendState(blendState_, geometry::Vec4f());
    engine_->setViewport(0, 0, width_, height_);
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
    engine_->endFrame(sync ? 1 : 0);
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
        if (!activeSizemove_) {
            if (debugEvents) {
                VGC_DEBUG(LogVgcUi, "paint from UpdateRequest");
            }
#if !defined(VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX)
            if (deferredResize_) {
                deferredResize_ = false;
                updateViewportSize_();
            }
#endif
            paint(true);
        }
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
        timeSinceLastTableEvent_.restart();
        tabletInProximity_ = true;
        break;
    case QEvent::TabletLeaveProximity:
        timeSinceLastTableEvent_.restart();
        tabletInProximity_ = false;
        // XXX should we do this ?
        // pressedTabletButtons_.clear();
        break;
    default:
        break;
    }
    return QWindow::eventFilter(obj, event);
}

#if defined(VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX)

LRESULT WINAPI Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* w = (Window*)::GetWindowLongPtr(hwnd, 19 * sizeof(LONG_PTR));
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

bool Window::nativeEvent(
    const QByteArray& eventType,
    void* message,
    NativeEventResult* result) {

    if (eventType == "windows_generic_MSG") {
        *result = 0;
        MSG* msg = reinterpret_cast<MSG*>(message);
        switch (msg->message) {
        case WM_SIZE: {

            // Get new unscaled size
            Int unscaledWidth = static_cast<Int>(LOWORD(msg->lParam));
            Int unscaledHeight = static_cast<Int>(HIWORD(msg->lParam));

            // Compute and set new scale ratio and scaled size
            updateScreenScaleRatioAndWindowSize_(unscaledWidth, unscaledHeight);

            if (debugEvents) {
                VGC_DEBUG(
                    LogVgcUi,
                    "WM_SIZE({:04d}, {:04d}) -> ({}, {})",
                    unscaledWidth,
                    unscaledHeight,
                    width_,
                    height_);
            }

            updateViewportSize_();
            paint(true);

            return false;
        }
        /*case WM_MOVING: {
            VGC_DEBUG(LogVgcUi, "WM_MOVING");
            return false;
        }
        case WM_MOVE: {
            VGC_DEBUG(LogVgcUi, "WM_MOVE");
            return false;
        }
        case WM_WINDOWPOSCHANGING: {
            VGC_DEBUG(LogVgcUi, "WM_WINDOWPOSCHANGING");
            return false;
        }
        case WM_WINDOWPOSCHANGED: {
            VGC_DEBUG(LogVgcUi, "WM_WINDOWPOSCHANGED");
            return false;
        }
        case WM_GETMINMAXINFO: {
            VGC_DEBUG(LogVgcUi, "WM_GETMINMAXINFO");
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
            if (debugEvents) {
                VGC_DEBUG(LogVgcUi, "WM_ERASEBKGND {}", ++i);
            }
            //if (activeSizemove_) {
            //    paint(true);
            //}

            // test get DC

            //*result = 1;
            //return true;
            return false;
        }
        case WM_PAINT: {
            static int i = 0;
            if (debugEvents) {
                VGC_DEBUG(LogVgcUi, "WM_PAINT {}", ++i);
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
    return false;
}
#else
bool Window::nativeEvent(
    const QByteArray& /*eventType*/,
    void* /*message*/,
    NativeEventResult* /*result*/) {

    return false;
}
#endif

void Window::exposeEvent(QExposeEvent*) {
    if (isExposed()) {
        if (activeSizemove_) {
            // On Windows, Expose events happen on both WM_PAINT and WM_ERASEBKGND
            // but in the case of a resize we already redraw properly.
            requestUpdate();
        }
        else {
            if (debugEvents) {
                VGC_DEBUG(LogVgcUi, "paint from exposeEvent");
            }
            // On macOS, moving a window between monitors with different devicePixelRatios
            // calls exposeEvent() but doesn't call resize(). So we need to fake a resize
            // here if the size in px of the window change, even though the "QWindow size"
            // (in device-independent scale) doesn't change.
            //
            Int oldScaledWidth = width_;
            Int oldScaledHeight = height_;
            Int unscaledWidth_ = width();
            Int unscaledHeight_ = height();
            updateScreenScaleRatioAndWindowSize_(unscaledWidth_, unscaledHeight_);
            if (oldScaledWidth != width_ || oldScaledHeight != height_) {
                updateViewportSize_();
            }
            paint(true);
        }
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
    if (screen()) {
        logicalDotsPerInch_ = static_cast<float>(screen()->logicalDotsPerInch());
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

void Window::updateScreenScaleRatioAndWindowSize_(Int unscaledWidth, Int unscaledHeight) {

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
    if (screenScaleRatioChanged && activeSizemove_) {
        paint(true);
    }
}

void Window::updateViewportSize_() {

    float w = static_cast<float>(width_);
    float h = static_cast<float>(height_);

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

} // namespace vgc::ui
