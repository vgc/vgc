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

#ifndef VGC_UI_PANELSTACK_H
#define VGC_UI_PANELSTACK_H

#include <vgc/ui/panel.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(PanelStack);

/// \class vgc::ui::PanelList
/// \brief A widget holding the stack of `Panel` of a `PanelArea` of type `Tabs`.
///
class VGC_UI_API PanelStack : public Widget {
private:
    VGC_OBJECT(PanelStack, Widget)

protected:
    PanelStack();

public:
    /// Creates a `PanelList`.
    ///
    // XXX Improve API
    //
    static PanelStackPtr create();

    /// Returns the active panel, that is, the panel which is currently visible.
    ///
    /// Returns `nullptr` if there is no active panel.
    ///
    Panel* activePanel() const {
        return static_cast<Panel*>(firstChild());
        // TODO: support multiple panels
    }

protected:
    // Reimplementation of Widget protected virtual methods
    void updateChildrenGeometry() override;
    void onWidgetAdded(Widget* child, bool wasOnlyReordered) override;
};

} // namespace vgc::ui

#endif // VGC_UI_PANELSTACK_H
