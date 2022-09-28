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

#ifndef VGC_UI_PANEL_H
#define VGC_UI_PANEL_H

#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Panel);
VGC_DECLARE_OBJECT(PanelArea);

/// \class vgc::ui::Panel
/// \brief A widget that can be placed in a PanelArea
///
// TODO: inherit from Scrollable instead, once implemented
//
class VGC_UI_API Panel : public Widget {
private:
    VGC_OBJECT(Panel, Widget)

protected:
    Panel();

public:
    /// Creates a PanelArea.
    ///
    static PanelPtr create();

    // reimpl
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;

protected:
    void onWidgetAdded(Widget* child, bool wasOnlyReordered) override;
    void onWidgetRemoved(Widget* child) override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;
};

} // namespace vgc::ui

#endif // VGC_UI_PANEL_H
