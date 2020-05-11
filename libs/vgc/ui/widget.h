// Copyright 2020 The VGC Developers
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

#ifndef VGC_UI_WIDGET_H
#define VGC_UI_WIDGET_H

#include <vgc/core/innercore.h>
#include <vgc/graphics/engine.h>
#include <vgc/ui/api.h>
#include <vgc/ui/mouseevent.h>

namespace vgc {
namespace ui {

VGC_DECLARE_OBJECT(Widget);

/// \class vgc::ui::Widget
/// \brief Base class of all elements in the user interface.
///
class VGC_UI_API Widget : public core::Object {
private:
    VGC_OBJECT(Widget)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected :
    /// Constructs a Widget. This constructor is an implementation detail only
    /// available to derived classes. In order to create a Widget, please use
    /// the following:
    ///
    /// ```cpp
    /// WidgetPtr widget = Widget::create();
    /// ```
    ///
    Widget();

public:
    /// Creates a widget.
    ///
    static WidgetPtr create();

    /// Returns the width of the widget.
    ///
    float width() const
    {
        return width_;
    }

    /// Returns the height of the widget.
    ///
    float height() const
    {
        return height_;
    }

    /// Resizes the widget to the given width and height.
    ///
    void resize(float width, float height);

    /// Requests this widget to be repainted, for example because the data
    /// displayed by this widget has changed. The widget is not immediately
    /// repainted: it is only scheduled for repaint. The actual moment when the
    /// widget is repainted depends on the graphics::Engine and other
    /// platform-dependent factors.
    ///
    /// You should typically call this function in your event handlers (e.g.,
    /// mousePressEvent()), to notify that the appearance of this widget has
    /// changed as a result of the event. Such call can be indirect, below is a
    /// example scenario:
    ///
    /// 1. The user clicks on a "Add Circle" button.
    /// 2. The event handler of the button emits the "clicked" signal.
    /// 3. A listener of this signal calls scene->addCircle().
    /// 4. This modifies the scene, which emits a "changed" signal.
    /// 5. A view of the scene detects the change, and calls this->repaint().
    ///
    /// Note how in this scenario, the repainted view is unrelated to the
    /// button which initially handled the event.
    ///
    void repaint();

    /// This signal is emitted when someone requested this widget, or one of
    /// its descendent widgets, to be repainted.
    ///
    const core::Signal<> repaintRequested;

    /// This virtual function is called once before the first call to
    /// onPaintDraw(), and should be reimplemented to create required GPU
    /// resources.
    ///
    virtual void onPaintCreate(graphics::Engine* engine);

    /// This virtual function is called whenever the widget needs to be
    /// repainted. Subclasses should reimplement this, typically by issuing
    /// draw calls.
    ///
    virtual void onPaintDraw(graphics::Engine* engine);

    /// This virtual function is called once after the last call to
    /// onPaintDraw(), for example before the widget is destructed, or if
    /// switching graphics engine. It should be reimplemented to destroy the
    /// created GPU resources.
    ///
    virtual void onPaintDestroy(graphics::Engine* engine);

    /// Override this function if you wish to handle MouseMove events. You must
    /// return true if the event was handled, false otherwise.
    ///
    virtual bool onMouseMove(MouseEvent* event);

    /// Override this function if you wish to handle MousePress events. You
    /// must return true if the event was handled, false otherwise.
    ///
    virtual bool onMousePress(MouseEvent* event);

    /// Override this function if you wish to handle MouseRelease events. You
    /// must return true if the event was handled, false otherwise.
    ///
    virtual bool onMouseRelease(MouseEvent* event);

    /// Override this function if you wish to handle MouseEnter events. You
    /// must return true if the event was handled, false otherwise.
    ///
    virtual bool onMouseEnter();

    /// Override this function if you wish to handle MouseLeave events. You
    /// must return true if the event was handled, false otherwise.
    ///
    virtual bool onMouseLeave();

private:
    float width_;
    float height_;
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_WIDGET_H
