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

#ifndef VGC_CANVAS_TOOLOPTIONSPANEL_H
#define VGC_CANVAS_TOOLOPTIONSPANEL_H

#include <vgc/canvas/api.h>
#include <vgc/canvas/canvastool.h>
#include <vgc/canvas/toolmanager.h>
#include <vgc/ui/panel.h>

namespace vgc::canvas {

VGC_DECLARE_OBJECT(ToolOptionsPanel);

/// \class vgc::canvas::ToolOptionsPanel
/// \brief A `Panel` that shows the options of the current canvas tool.
///
class VGC_CANVAS_API ToolOptionsPanel : public ui::Panel {
private:
    VGC_OBJECT(ToolOptionsPanel, ui::Panel)

protected:
    ToolOptionsPanel(CreateKey key, const ui::PanelContext& context);

public:
    // TODO: A cleaner way to do this, also supporting translations.
    static constexpr std::string_view label = "Tool Options";
    static constexpr std::string_view id = "vgc.common.toolOptions";
    static constexpr ui::PanelDefaultArea defaultArea = ui::PanelDefaultArea::Left;

    /// Creates a `ToolOptionsPanel`.
    ///
    static ToolOptionsPanelPtr create(const ui::PanelContext& context);

private:
    void onCurrentToolChanged_(CanvasTool* tool);
    VGC_SLOT(onCurrentToolChanged_)
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_TOOLOPTIONSPANEL_H
