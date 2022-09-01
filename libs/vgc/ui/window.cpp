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

#include <QCoreApplication>
#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QOpenGLFunctions>

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

Window::Window(ui::WidgetPtr widget)
    : QWindow()
    , widget_(widget)
    , proj_(geometry::Mat4f::identity)
    , clearColor_(0.251f, 0.259f, 0.267f, 1.f) {

    connect((QWindow*)this, &QWindow::activeChanged, this, &Window::onActiveChanged_);

    //setMouseTracking(true);
    widget_->repaintRequested().connect(onRepaintRequested());
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

void Window::onDestroyed() {
    // Destroying the engine will stop it.
    engine_ = nullptr;
}

WindowPtr Window::create(ui::WidgetPtr widget) {
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

void Window::mouseMoveEvent(QMouseEvent* event) {
    ui::MouseEventPtr e = fromQt(event);
    event->setAccepted(widget_->onMouseMove(e.get()));
}

void Window::mousePressEvent(QMouseEvent* event) {
    ui::MouseEventPtr e = fromQt(event);
    event->setAccepted(widget_->onMousePress(e.get()));
}

void Window::mouseReleaseEvent(QMouseEvent* event) {
    ui::MouseEventPtr e = fromQt(event);
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

void Window::focusInEvent(QFocusEvent* event) {
    ui::FocusReason reason = static_cast<ui::FocusReason>(event->reason());
    widget_->setTreeActive(true, reason);
}

void Window::focusOutEvent(QFocusEvent* event) {
    ui::FocusReason reason = static_cast<ui::FocusReason>(event->reason());
    widget_->setTreeActive(false, reason);
}

void Window::resizeEvent(QResizeEvent* event) {
    QSize size = event->size();
    [[maybe_unused]] int w = size.width();
    [[maybe_unused]] int h = size.height();
    if (debugEvents) {
        VGC_DEBUG(LogVgcUi, core::format("resizeEvent({:04d}, {:04d})", w, h));
    }

#if !defined(VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX)
    width_ = w;
    height_ = h;
    deferredResize_ = true;
    requestUpdate();
#endif
}

void Window::keyPressEvent(QKeyEvent* event) {
    bool accepted = widget_->onKeyPress(event);
    event->setAccepted(accepted);
}

void Window::keyReleaseEvent(QKeyEvent* event) {
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

    /*static float triangle[15] = {
         20.f,  20.f, 1.f, 0.f, 0.f,
        160.f,  50.f, 0.f, 1.f, 0.f,
         50.f, 160.f, 0.f, 0.f, 1.f,
    };
    static auto bptr = engine_->createPrimitiveBuffer([]{ return triangle; }, 15*4, false);
    engine_->drawPrimitives(bptr.get(), graphics::PrimitiveType::TriangleList);*/

    /*static int i = 0;
    static graphics::ShapedText shapedText(graphics::fontLibrary()->defaultFont()->getSizedFont(), "text");
    std::string s = core::format("{:d} {:04d} {:04d}x{:04d} {:04d}x{:04d}", swapChain_->numPendingPresents(), ++i, width_, height_, width(), height());
    shapedText.setText(s);
    VGC_DEBUG(LogVgcUi, s);
    core::Array<float> a;
    shapedText.fill(a, geometry::Vec2f(60.f, 60.f), 0.f, 0.f, 0.f, 0.f, 1000.f, 0.f, 1000.f);
    static auto textBuf = engine_->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
    engine_->updateBufferData(textBuf.get(), [a = std::move(a)](){ return a.data(); }, a.length() * 4);
    engine_->drawPrimitives(textBuf.get(), graphics::PrimitiveType::TriangleList);*/

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

bool Window::event(QEvent* e) {
    switch (e->type()) {
    case QEvent::UpdateRequest:
        if (!activeSizemove_) {
            if (debugEvents) {
                VGC_DEBUG(LogVgcUi, "paint from UpdateRequest");
            }
#if !defined(VGC_WINDOWS_WINDOW_ARTIFACTS_ON_RESIZE_FIX)
            if (deferredResize_) {
                deferredResize_ = false;

                geometry::Camera2d c;
                c.setViewportSize(width(), height());
                proj_ = detail::toMat4f(c.projectionMatrix());

                // Set new widget geometry. Note: if w or h is > 16777216 (=2^24), then static_cast
                // silently rounds to the nearest integer representable as a float. See:
                //   https://stackoverflow.com/a/60339495/1951907
                // Should we issue a warning in these cases?
                widget_->updateGeometry(
                    0, 0, static_cast<float>(width_), static_cast<float>(height_));

                if (engine_) {
                    engine_->resizeSwapChain(swapChain_, width_, height_);
                }

                engine_->resizeSwapChain(swapChain_, width_, height_);
            }
#endif
            paint(true);
        }
        return true;
    case QEvent::ShortcutOverride:
        e->accept();
        break;
    default:
        break;
    }
    return QWindow::event(e);
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

    static HBRUSH hbrWhite, hbrGray;
    if (eventType == "windows_generic_MSG") {
        *result = 0;
        MSG* msg = reinterpret_cast<MSG*>(message);
        switch (msg->message) {
        case WM_SIZE: {
            UINT w = LOWORD(msg->lParam);
            UINT h = HIWORD(msg->lParam);
            width_ = w;
            height_ = h;
            if (debugEvents) {
                VGC_DEBUG(
                    LogVgcUi, core::format("WM_SIZE({:04d}, {:04d})", width_, height_));
            }

            geometry::Camera2d c;
            c.setViewportSize(w, h);
            proj_ = detail::toMat4f(c.projectionMatrix());

            // Set new widget geometry. Note: if w or h is > 16777216 (=2^24), then static_cast
            // silently rounds to the nearest integer representable as a float. See:
            //   https://stackoverflow.com/a/60339495/1951907
            // Should we issue a warning in these cases?

            widget_->updateGeometry(0, 0, static_cast<float>(w), static_cast<float>(h));
            if (engine_ && swapChain_) {
                // quite slow with msaa on, will probably be better when we have a compositor.
                engine_->resizeSwapChain(swapChain_, w, h);
                //Sleep(50);
                paint(true);
                //qDebug() << "painted";
            }

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
            // On windows Expose events happen on both WM_PAINT and WM_ERASEBKGND
            // but in the case of a resize we already redraw properly.
            requestUpdate();
        }
        else {
            if (debugEvents) {
                VGC_DEBUG(LogVgcUi, "paint from exposeEvent");
            }
            paint(true);
        }
    }
}

void Window::cleanup() {
    // XXX ?
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

} // namespace vgc::ui
