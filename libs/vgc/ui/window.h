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
#include <vgc/core/os.h>
#include <vgc/core/stopwatch.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/engine.h>
#include <vgc/ui/widget.h>

class QInputMethodEvent;
class QInputMethodQueryEvent;

namespace vgc::ui {

namespace detail {

#ifdef VGC_CORE_OS_MACOS
constexpr float baseLogicalDpi = 72.0f;
#else
constexpr float baseLogicalDpi = 96.0f;
#endif

} // namespace detail

VGC_DECLARE_OBJECT(Window);

/// \class vgc::ui::Window
/// \brief A window able to contain a vgc::ui::widget.
///
/// Note: for now, `vgc::ui::Window` inherits from `QWindow`, but we may change
/// this in the future, so clients of `vgc::ui::Window` should avoid calling
/// methods inherited from `QWindow`.
///
class VGC_UI_API Window : public core::Object, public QWindow {
    VGC_OBJECT(Window, core::Object)

protected:
    /// Constructs a `Window` containing the given `Widget`.
    ///
    Window(const WidgetPtr& widget);

    /// Destructs the `Window`.
    ///
    void onDestroyed() override;

public:
    /// Creates a `Window` containing the given `Widget`.
    ///
    static WindowPtr create(const WidgetPtr& widget);

    /// Returns the contained `Widget`
    ///
    Widget* widget() {
        return widget_.get();
    }

    /// Returns the geometry of this `Window` as a rectangle.
    ///
    // TODO: we want this to be equivalent to `Rect2f::fromPositionSize(0, 0,
    // size())` (see comment in Widget::rect()), but currently `Window::size()`
    // is a QSize instead of a Vec2f like in Widget.
    //
    geometry::Rect2f rect() const {
        return geometry::Rect2f::fromPositionSize(
            0, 0, static_cast<float>(width_), static_cast<float>(height_));
    }

    /// The scale factor to apply when converting from global coordinates
    /// (virtual space including all monitors) to window coordinates.
    ///
    /// ```cpp
    /// sizeInWindowCoords = globalToWindowScale() * sizeInGlobalCoords;
    /// ```
    ///
    /// Indeed, global coordinates and window coordinates may not always have
    /// the same scale in order to support multi-monitor configurations with
    /// varying DPI.
    ///
    float globalToWindowScale() const;

    /// Converts a position from global coordinates (virtual space including
    /// all monitors) to window coordinates.
    ///
    /// \sa `mapToGlobal()`, `globalToWindowScale()`.
    ///
    geometry::Vec2f mapFromGlobal(const geometry::Vec2f& globalPosition) const;

    /// Converts a position from window coordinates to global coordinates
    /// (virtual space including all monitors).
    ///
    /// \sa `mapFromGlobal()`, `globalToWindowScale()`.
    ///
    geometry::Vec2f mapToGlobal(const geometry::Vec2f& position) const;

    /// Returns whether the background color should be painted.
    ///
    /// The default is true.
    ///
    /// \sa `setBackgroundPainted()`, `backgroundColor()`.
    ///
    bool isBackgroundPainted() const {
        return isBackgroundPainted_;
    }

    /// Sets whether the background color should be painted.
    ///
    /// If `true`, this means that a call to `Engine::clear(backgroundColor())`
    /// is performed.
    ///
    /// Using `false` makes it possible to improve performance by avoiding
    /// overdraw when you know that the whole window will be covered anyway by
    /// further painting operations. If some areas are not painted, then the
    /// color of non-painted regions is undefined (on some platforms, it might
    /// be the previous render buffer value, or might be some default color
    /// such as black/white, or be some random color).
    ///
    /// \sa `isBackgroundPainted()`, `backgroundcolor()`.
    ///
    void setBackgroundPainted(bool isPainted);

    /// Returns the background color of the window.
    ///
    /// The default is black (`Color(0, 0, 0, 1)`).
    ///
    /// \sa `setBackgroundColor()`, `isBackgroundPainted()`.
    ///
    core::Color backgroundColor() const {
        return backgroundColor_;
    }

    /// Sets the background color of the window.
    ///
    /// \sa `backgroundColor()`, `isBackgroundPainted()`.
    ///
    void setBackgroundColor(const core::Color& color);

    // ===================== Handle mouse/tablet input ========================

protected:
    /// Handles mouse enter events.
    ///
    virtual void enterEvent(QEvent* event);

    /// Handles mouse leave events.
    ///
    virtual void leaveEvent(QEvent* event);

    // QWindow overrides
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void tabletEvent(QTabletEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    bool isTabletInUse_() const;
    bool mouseMoveEvent_(MouseMoveEvent* event);
    bool mousePressEvent_(MousePressEvent* event);
    bool mouseReleaseEvent_(MouseReleaseEvent* event);
    bool mouseScrollEvent_(ScrollEvent* event);

    // ==================== Handle keyboard input =============================

protected:
    // QWindow overrides
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    /// Handles complex keyboard input events, such as:
    /// - composing accentuated characters via dead keys
    /// - composing Asian characters via Input Method Editors (Pinyin, etc.)
    /// - composing text via virtual keyboards
    ///
    virtual void inputMethodEvent(QInputMethodEvent* event);

    /// Handles input method query events.
    ///
    /// This corresponds to a request from the operating system or IME for the
    /// information it needs in order to perform its task. For example, an IME
    /// needs to know when should a virtual keyboard appear, at what location,
    /// what are the characters surrounding the in-app cursor, etc.
    ///
    /// The default implementation executes the following code for each
    /// `Qt::InputMethodQuery` in `event->queries()`:
    ///
    /// ```cpp
    /// QVariant value = inputMethodQuery(query);
    /// event->setValue(query, value);
    /// ```
    ///
    /// \sa `inputMethodQuery()`
    ///
    virtual void inputMethodQueryEvent(QInputMethodQueryEvent* event);

    /// Answers to a given input method query.
    ///
    /// \sa `inputMethodQueryEvent()`.
    ///
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query);

    // ===================== Handle redraw and resize =========================

private:
    bool updateScreenScaleRatio_();

    // This function is split in two parts for easier debugging:
    // - The first updates the data members, which can then be used for printing info
    // - The second repaints the widget based on the return value of the first
    [[nodiscard]] bool
    updateScreenScaleRatioAndWindowSize1_(Int unscaledWidth, Int unscaledHeight);
    void updateScreenScaleRatioAndWindowSize2_(bool shouldRepaint);

    void updateViewportSize_();

protected:
    // QWindow overrides
    void exposeEvent(QExposeEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    /// Handles `QEvent::UpdateRequest` events.
    ///
    virtual void updateRequestEvent(QEvent* event);

    /// Repaints the window.
    ///
    virtual void paint(bool sync = false);

    // ============== Dispatching and platform-specific events ================

protected:
    // Defines which type to use for `result` in `nativeEvent()` (depends on Qt version).
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using NativeEventResult = long;
#else
    using NativeEventResult = qintptr;
#endif

    // QWindow overrides
    bool event(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    bool nativeEvent(
        const QByteArray& eventType,
        void* message,
        NativeEventResult* result) override;

private:
#if defined(VGC_CORE_OS_WINDOWS)
    HWND hwnd_ = {};
    static LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
#endif

    // ============== Data members ================

private:
    WidgetPtr widget_;
    std::unordered_map<Shortcut, Action*> shortcutMap_;

    Int width_ = 0;
    Int height_ = 0;
    float logicalDotsPerInch_ = detail::baseLogicalDpi;
    float devicePixelRatio_ = 1.0;
    float screenScaleRatio_ = 1.0;

    graphics::EnginePtr engine_;
    graphics::SwapChainPtr swapChain_;
    graphics::RasterizerStatePtr rasterizerState_;
    graphics::BlendStatePtr blendState_;
    geometry::Mat4f proj_;

    bool isBackgroundPainted_ = true;
    core::Color backgroundColor_ = {};

    MouseButtons pressedMouseButtons_;
    MouseButtons pressedTabletButtons_;
    geometry::Vec2f accumulatedScrollDelta_ = {};

    // The tablet is considered "in use" if a tablet button is pressed, or the
    // pen tablet is in proximity of the tablet, or the time since the last
    // tablet event is less than tabletIdleDuration.
    //
    static constexpr double tabletIdleDuration_ = 1; // in seconds
    core::Stopwatch timeSinceLastTableEvent_ = {-tabletIdleDuration_};
    bool tabletInProximity_ = false;

    bool activeSizemove_ = false;
    bool deferredResize_ = false;
    bool updateDeferred_ = false;
    bool entered_ = false;
    bool isLeaveDeferred_ = false;

    // ============== Other helper methods ================

private:
    void initEngine_();

    void addShortcuts_(Widget* widget);
    void removeShortcuts_(Widget* widget);
    void addShorctut_(Action* action);
    void removeShortcut_(Action* action);

    void onActiveChanged_();
    void onRepaintRequested_();
    void onMouseCaptureStarted_();
    void onMouseCaptureStopped_();
    void onKeyboardCaptureStarted_();
    void onKeyboardCaptureStopped_();
    void onWidgetAddedToTree_(Widget* widget);
    void onWidgetRemovedFromTree_(Widget* widget);
    void onActionAboutToBeDestroyed_(Object* object);

    VGC_SLOT(onRepaintRequestedSlot_, onRepaintRequested_);
    VGC_SLOT(onMouseCaptureStartedSlot_, onMouseCaptureStarted_);
    VGC_SLOT(onMouseCaptureStoppedSlot_, onMouseCaptureStopped_);
    VGC_SLOT(onKeyboardCaptureStartedSlot_, onKeyboardCaptureStarted_);
    VGC_SLOT(onKeyboardCaptureStoppedSlot_, onKeyboardCaptureStopped_);
    VGC_SLOT(onWidgetAddedToTreeSlot_, onWidgetAddedToTree_);
    VGC_SLOT(onWidgetRemovedFromTreeSlot_, onWidgetRemovedFromTree_);
    VGC_SLOT(onActionAboutToBeDestroyedSlot_, onActionAboutToBeDestroyed_);

    Int debugIndent_ = 0;
    core::Stopwatch debugStopwatch_;
};

} // namespace vgc::ui

#endif // VGC_UI_WINDOW_H
