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

#ifndef VGC_UI_TABBAR_H
#define VGC_UI_TABBAR_H

#include <vgc/ui/label.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(TabBar);
VGC_DECLARE_OBJECT(TabBody);

/// \class vgc::ui::TabBar
/// \brief A bar showing different tabs.
///
class VGC_UI_API TabBar : public Label { // Inheriting from Label temporarily
private:
    VGC_OBJECT(TabBar, Label)

protected:
    //TabBar(TabBody* panels);
    TabBar();

public:
    /// Creates a `PanelTabs`.
    ///
    static TabBarPtr create();
    //static TabBarPtr create(TabBody* panels);

    /*
    /// Returns the `PanelStack` this `PanelTabs` refers to.
    ///
    /// Returns `nullptr` if this `PanelTabs` does not have any associated panels.
    ///
    // XXX Should this directly return panels->children() instead?
    //
    TabBody* panels() {
        return panels_;
    }
*/

private:
    //TabBody* panels_ = nullptr;

    //void onPanelsDestroyed_();
    //VGC_SLOT(onPanelsDestroyedSlot_, onPanelsDestroyed_)
};

} // namespace vgc::ui

#endif // VGC_UI_TABBAR_H
