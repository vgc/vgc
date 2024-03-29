// Copyright 2024 The VGC Developers
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

#ifndef VGC_CANVAS_EXPERIMENTAL_H
#define VGC_CANVAS_EXPERIMENTAL_H

#include <vgc/canvas/api.h>
#include <vgc/canvas/canvastool.h>
#include <vgc/canvas/toolmanager.h>
#include <vgc/ui/boolsetting.h>
#include <vgc/ui/panel.h>

namespace vgc::canvas {

namespace experimental {

VGC_CANVAS_API
ui::BoolSetting& saveInputSketchPoints();

VGC_CANVAS_API
ui::BoolSetting& showInputSketchPoints();

} // namespace experimental

VGC_DECLARE_OBJECT(ExperimentalPanel);

/// \class vgc::canvas::ExperimentalPanel
/// \brief A `Panel` with experimental settings and options.
///
class VGC_CANVAS_API ExperimentalPanel : public ui::Panel {
private:
    VGC_OBJECT(ExperimentalPanel, ui::Panel)

protected:
    ExperimentalPanel(CreateKey key, const ui::PanelContext& context);

public:
    // TODO: A cleaner way to do this, also supporting translations.
    static constexpr std::string_view label = "Experimental";
    static constexpr std::string_view id = "vgc.experimental";
    static constexpr ui::PanelDefaultArea defaultArea = ui::PanelDefaultArea::Right;

    /// Creates a `ToolsPanel`.
    ///
    static ExperimentalPanelPtr create(const ui::PanelContext& context);
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_EXPERIMENTAL_H
