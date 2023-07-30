// Copyright 2023 The VGC Developers
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

#ifndef VGC_UI_TABBODY_H
#define VGC_UI_TABBODY_H

#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(TabBody);

/// \class vgc::ui::TabBody
/// \brief Manages the content of tabs, displaying the active tab.
///
class VGC_UI_API TabBody : public Widget {
private:
    VGC_OBJECT(TabBody, Widget)

protected:
    TabBody(CreateKey);

public:
    /// Creates a `TabBody`.
    ///
    static TabBodyPtr create();

    /// Returns the active widget, that is, the widget which is currently visible.
    ///
    /// Returns `nullptr` if there is no active widget.
    ///
    Widget* activeWidget() const {
        return static_cast<Widget*>(firstChild());
        // TODO: support multiple widget
    }

protected:
    // Reimplementation of Widget protected virtual methods
    void updateChildrenGeometry() override;
    void onWidgetAdded(Widget* child, bool wasOnlyReordered) override;
};

} // namespace vgc::ui

#endif // VGC_UI_TABBODY_H
