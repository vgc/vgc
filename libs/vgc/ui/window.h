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

#ifndef VGC_UI_WINDOW_H
#define VGC_UI_WINDOW_H

#include <QWindow>

#include <vgc/core/array.h>
#include <vgc/core/stopwatch.h>
#include <vgc/graphics/engine.h>
#include <vgc/ui/widget.h>

class QInputMethodEvent;
class QInputMethodQueryEvent;

namespace vgc::ui {

VGC_DECLARE_OBJECT(Window);

/// \class vgc::ui::Window
/// \brief A window able to contain a vgc::ui::widget.
///
class VGC_UI_API Window : public core::Object, public QWindow {
    VGC_OBJECT(Window, core::Object)

protected:
    /// Constructs a Window containing the given vgc::ui::Widget.
    ///
    Window(WidgetPtr widget);

    /// Destructs the Window.
    ///
    void onDestroyed() override;

public:
    static WindowPtr create(WidgetPtr widget);

    /// Returns the contained vgc::ui::Widget
    ///
    Widget* widget() {
        return widget_.get();
    }

protected:
#if defined(VGC_CORE_COMPILER_MSVC)
    HWND hwnd_ = {};
    static LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
#endif
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void tabletEvent(QTabletEvent* event) override;

    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void exposeEvent(QExposeEvent* event) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

    /// Handles mouse enter events.
    ///
    virtual void enterEvent(QEvent* event);

    /// Handles mouse leave events.
    ///
    virtual void leaveEvent(QEvent* event);

    // Handles input methods (dead keys, Asian characters, virtual keyboards, etc.)
    //
    virtual void inputMethodQueryEvent(QInputMethodQueryEvent* event);
    virtual void inputMethodEvent(QInputMethodEvent* event);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using NativeEventResult = long;
#else
    using NativeEventResult = qintptr;
#endif

    bool nativeEvent(
        const QByteArray& eventType,
        void* message,
        NativeEventResult* result) override;

private:
    WidgetPtr widget_;
    graphics::EnginePtr engine_;
    graphics::SwapChainPtr swapChain_;
    graphics::RasterizerStatePtr rasterizerState_;
    graphics::BlendStatePtr blendState_;
    int width_ = 0;
    int height_ = 0;
    bool activeSizemove_ = false;
    bool deferredResize_ = false;
    bool entered_ = false;

    float screenScaleRatio_ = 1.0;
    void updateScreenScaleRatio_();

    geometry::Mat4f proj_;
    core::Color clearColor_;

    bool updateDeferred_ = false;

    MouseButtons pressedMouseButtons_;
    MouseButtons pressedTabletButtons_;
    bool tabletInProximity_ = false;
    core::Stopwatch timeSinceLastTableEvent_ = {};

    bool isTabletInUse_() const;

    bool mouseMoveEvent_(Widget* receiver, MouseEvent* event);
    bool mousePressEvent_(Widget* receiver, MouseEvent* event);
    bool mouseReleaseEvent_(Widget* receiver, MouseEvent* event);

    void onActiveChanged_();
    void onRepaintRequested_();
    void onMouseCaptureStarted_();
    void onMouseCaptureStopped_();
    void onKeyboardCaptureStarted_();
    void onKeyboardCaptureStopped_();

    VGC_SLOT(onRepaintRequestedSlot_, onRepaintRequested_);
    VGC_SLOT(onMouseCaptureStartedSlot_, onMouseCaptureStarted_);
    VGC_SLOT(onMouseCaptureStoppedSlot_, onMouseCaptureStopped_);
    VGC_SLOT(onKeyboardCaptureStartedSlot_, onKeyboardCaptureStarted_);
    VGC_SLOT(onKeyboardCaptureStoppedSlot_, onKeyboardCaptureStopped_);

    /////////////////////////////
    // TO FACTOR OUT
    ////////////////////////////

    virtual void paint(bool sync = false);
    virtual void cleanup();
};

} // namespace vgc::ui

#endif // VGC_UI_WINDOW_H
