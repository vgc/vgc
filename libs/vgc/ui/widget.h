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

VGC_CORE_DECLARE_PTRS(Widget);

/// \class vgc::ui::Widget
/// \brief Base class of all elements in the user interface.
///
class VGC_UI_API Widget : public core::Object
{  
    VGC_CORE_OBJECT(Widget)

public:
    /// Constructs a Widget. This constructor is an implementation detail only
    /// available to derived classes. In order to create a Widget, please use
    /// the following:
    ///
    /// \code
    /// WidgetSharedPtr widget = Widget::create();
    /// \endcode
    ///
    Widget(const ConstructorKey&);

public:
    /// Creates a widget.
    ///
    static WidgetSharedPtr create();

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

    /// This virtual function is called once before the first call to paint()
    /// or resize(), and should be reimplemented to allocate required GPU
    /// resources.
    ///
    virtual void initialize(graphics::Engine* engine);

    /// This virtual function is called whenever the widget has been resized
    /// (the new size is given as w and h). Subclasses should reimplement this,
    /// typically by adapting GPU resources to the new size.
    ///
    virtual void resize(graphics::Engine* engine, Int w, Int h);

    /// This virtual function is called whenever the widget needs to be
    /// repainted. Subclasses should reimplement this, typically by issuing
    /// draw calls.
    ///
    virtual void paint(graphics::Engine* engine);

    /// This virtual function is called once after the last call to paint(),
    /// for example before the widget is destructed, and should be reimplemented
    /// to release the allocated GPU resources.
    ///
    virtual void cleanup(graphics::Engine* engine);

    /// This function is called whenever a MouseMove event occurs. Reimplement
    /// it in subclasses if you wish to handle the event.
    ///
    /// You must return true if you accept the event, that is, if you do not
    /// want it propagated to the parent widget. The default implementation
    /// returns false, which means that if you do not reimplement this
    /// function, the event will automatically be propagated to the parent
    /// widget.
    ///
    virtual bool mouseMoveEvent(MouseEvent* event);

    /// This function is called whenever a MousePress event occurs. Reimplement
    /// it in subclasses if you wish to handle the event.
    ///
    /// You must return true if you accept the event, that is, if you do not
    /// want it propagated to the parent widget. The default implementation
    /// returns false, which means that if you do not reimplement this
    /// function, the event will automatically be propagated to the parent
    /// widget.
    ///
    virtual bool mousePressEvent(MouseEvent* event);

    /// This function is called whenever a MouseRelease event occurs.
    /// Reimplement it in subclasses if you wish to handle the event.
    ///
    /// You must return true if you accept the event, that is, if you do not
    /// want it propagated to the parent widget. The default implementation
    /// returns false, which means that if you do not reimplement this
    /// function, the event will automatically be propagated to the parent
    /// widget.
    ///
    virtual bool mouseReleaseEvent(MouseEvent* event);
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_WIDGET_H
