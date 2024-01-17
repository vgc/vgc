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
#include <vgc/core/array.h>
#include <vgc/ui/action.h>
#include <vgc/ui/actiongroup.h>
#include <vgc/ui/module.h>
#include <vgc/ui/panel.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(PanelManager);

} // namespace vgc::ui

namespace vgc::canvas {

VGC_DECLARE_OBJECT(ToolManager);

class ToolsPanel;

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
    void setCanvas(CanvasWeakPtr canvas);

    /// Adds a tool to this `ToolManager`.
    ///
    void registerTool(core::StringId commandId, canvas::CanvasToolSharedPtr tool);

    /// Returns the current tool.
    ///
    /// \sa `setCurrentTool()`, `currentToolChanged()`.
    ///
    canvas::CanvasToolWeakPtr currentTool() {
        return currentTool_;
    }

    /// Sets which tool is currently the current tool.
    ///
    /// \sa `currentTool()`, `currentToolChanged()`.
    ///
    void setCurrentTool(canvas::CanvasToolWeakPtr canvasTool);

    /// This signal is emitted whenever the `currentTool()` changes
    ///
    VGC_SIGNAL(currentToolChanged, (canvas::CanvasToolWeakPtr, currentTool))

private:
    // Friendship is used so that the Tools panel can iterate on registered tools.
    // TODO: publicize what's needed to avoid friendship.
    friend ToolsPanel;

    CanvasWeakPtr canvas_;

    ui::ActionGroupPtr toolsActionGroup_;

    // We use a flat birectional map to store relation between tool and action.
    // The array order is currently used as order tool order in the Tools panel.
    //
    struct RegisteredTool_ {
        ui::Action* action;
        canvas::CanvasToolSharedPtr tool;
    };
    core::Array<RegisteredTool_> tools_; // flat bidirectional map

    canvas::CanvasToolWeakPtr currentTool_;

    ui::Action* getActionFromTool_(canvas::CanvasTool* tool) const;
    canvas::CanvasToolWeakPtr getToolFromAction_(ui::Action* action) const;

    bool hasTool_(canvas::CanvasTool* tool);
    bool hasAction_(ui::Action* action);

    void onToolCheckStateChanged_(ui::Action* toolAction, ui::CheckState checkState);
    VGC_SLOT(onToolCheckStateChanged_);
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_TOOLMANAGER_H
