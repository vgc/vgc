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

#ifndef VGC_CANVAS_TOOLMANAGER_H
#define VGC_CANVAS_TOOLMANAGER_H

#include <vgc/canvas/api.h>
#include <vgc/canvas/canvas.h>
#include <vgc/canvas/canvastool.h>
#include <vgc/core/object.h>
#include <vgc/ui/action.h>
#include <vgc/ui/actiongroup.h>
#include <vgc/ui/module.h>
#include <vgc/ui/panel.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(PanelManager);

} // namespace vgc::ui

namespace vgc::canvas {

VGC_DECLARE_OBJECT(ToolManager);

/// \class vgc::canvas::ToolManager
/// \brief Stores a list of registered canvas tools and controls which one is the current tool.
///
class VGC_CANVAS_API ToolManager : public ui::Module {
private:
    VGC_OBJECT(ToolManager, ui::Module)

protected:
    ToolManager(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `ToolManager` module.
    ///
    static ToolManagerPtr create(const ui::ModuleContext& context);

    // XXX This is temporary
    //
    // TODO: allow having several canvases for the same ToolManager. This
    // requires to invert the dependency: it should be each Canvas instance
    // that stores a pointer to a ToolManager, and listens to
    // `currentToolChanged()`.
    //
    void setCanvas(Canvas* canvas);

    /// Adds a tool to this `ToolManager`.
    ///
    void registerTool(core::StringId commandId, canvas::CanvasToolPtr tool);

    /// Returns the current tool.
    ///
    /// \sa `setCurrentTool()`, `currentToolChanged()`.
    ///
    canvas::CanvasTool* currentTool() {
        return currentTool_;
    }

    /// Sets which tool is currently the current tool.
    ///
    /// \sa `currentTool()`, `currentToolChanged()`.
    ///
    void setCurrentTool(canvas::CanvasTool* canvasTool);

    /// This signal is emitted whenever the `currentTool()` changes
    ///
    VGC_SIGNAL(currentToolChanged, (canvas::CanvasTool*, currentTool))

    /// Creates a new `Panel`, as child of the given `panelArea`,
    /// that can be used for switching between tools.
    ///
    ui::Panel* createToolsPanel(ui::PanelManager* panelManager, ui::PanelArea* panelArea);

private:
    Canvas* canvas_ = nullptr;

    ui::ActionGroupPtr toolsActionGroup_;
    std::map<ui::Action*, canvas::CanvasToolPtr> toolMap_;
    std::map<canvas::CanvasTool*, ui::Action*> toolMapInv_;
    canvas::CanvasTool* currentTool_ = nullptr;

    void onToolCheckStateChanged_(ui::Action* toolAction, ui::CheckState checkState);
    VGC_SLOT(onToolCheckStateChanged_);
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_TOOLMANAGER_H
