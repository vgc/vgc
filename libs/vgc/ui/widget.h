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

#include <vgc/core/object.h>
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
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_WIDGET_H
