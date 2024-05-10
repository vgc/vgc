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

VGC_DECLARE_OBJECT(ExperimentalModule);

/// \class vgc::tools::ExperimentalModule
/// \brief A module for registering experimental settings.
///
class VGC_CANVAS_API ExperimentalModule : public ui::Module {
private:
    VGC_OBJECT(ExperimentalModule, ui::Module)

protected:
    ExperimentalModule(CreateKey, const ui::ModuleContext& context);

public:
    static ExperimentalModulePtr create(const ui::ModuleContext& context);

    void addWidget(ui::Widget& widget);

    const core::Array<ui::WidgetSharedPtr>& widgets() const {
        return widgets_;
    }

    VGC_SIGNAL(widgetAdded, (ui::Widget&, widget))

private:
    // We need to keep ownership since the widget may otherwise
    // have no other owner until the panel is opened.
    core::Array<ui::WidgetSharedPtr> widgets_;

    void onDestroyed() override;
};

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

private:
    ui::FlexWeakPtr layout_;

    void onModuleWidgetAdded_(ui::Widget& widget);
    VGC_SLOT(onModuleWidgetAdded_)
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_EXPERIMENTAL_H
