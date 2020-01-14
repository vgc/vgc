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
#include <vgc/ui/api.h>

namespace vgc {
namespace ui {

VGC_CORE_DECLARE_PTRS(Widget);

/// \class vgc::ui::Widget
/// \brief Base class of all elements in the user interface.
///
class VGC_UI_API Widget : public core::Object
{  
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

    /// This virtual function is called once before the first call to paint()
    /// or resize(), and should be reimplemented to allocate required GPU
    /// resources.
    ///
    virtual void initialize();

    /// This virtual function is called whenever the widget has been resized
    /// (the new size is given as w and h). Subclasses should reimplement this,
    /// typically by adapting GPU resources to the new size.
    ///
    virtual void resize(Int w, Int h);

    /// This virtual function is called whenever the widget needs to be
    /// repainted. Subclasses should reimplement this, typically by issuing
    /// draw calls.
    ///
    virtual void paint();

    /// This virtual function is called once after the last call to paint(),
    /// for example before the widget is destructed, and should be reimplemented
    /// to release the allocated GPU resources.
    ///
    virtual void cleanup();
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_WIDGET_H
