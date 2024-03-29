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

#ifndef VGC_CANVAS_DOCUMENTMANAGER_H
#define VGC_CANVAS_DOCUMENTMANAGER_H

#include <vgc/canvas/api.h>
#include <vgc/core/id.h>
#include <vgc/ui/module.h>

VGC_DECLARE_OBJECT(vgc::dom, Document);
VGC_DECLARE_OBJECT(vgc::workspace, Workspace);

namespace vgc::canvas {

VGC_DECLARE_OBJECT(DocumentManager);
VGC_DECLARE_OBJECT(WorkspaceSelection);

/// \class vgc::tools::DocumentManager
/// \brief A module to specify a current document/workspace and selection.
///
class VGC_CANVAS_API DocumentManager : public ui::Module {
private:
    VGC_OBJECT(DocumentManager, ui::Module)

protected:
    DocumentManager(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `DocumentManager` module.
    ///
    static DocumentManagerPtr create(const ui::ModuleContext& context);

    /// Returns the current workspace.
    ///
    workspace::WorkspaceWeakPtr currentWorkspace() const {
        return currentWorkspace_;
    }

    /// Sets the current workspace.
    ///
    /// The `DocumentManager` will take ownership of the workspace, and release
    /// ownership of the previous `currentWorkspace`, if any.
    ///
    void setCurrentWorkspace(workspace::WorkspaceSharedPtr workspace);

    /// This signal is emitted whenever the `currentWorkpace()` object
    /// is replaced to point to a different `Workspace` instance.
    ///
    // TODO: currentWorkspaceChanged(), forwarding currentWorkspace->changed()
    //
    VGC_SIGNAL(currentWorkspaceReplaced, (workspace::WorkspaceWeakPtr, workspace))

    /// Returns the current document.
    ///
    dom::DocumentWeakPtr currentDocument() const;

    /// Returns the current `WorkspaceSelection`.
    ///
    // Note: for now, there is only one `WorkspaceSelection` for the lifetime of
    // the DocumentManager, and the selection is cleared when switching documents/workspaces.
    // In the future, we might want to store Workspace-WorkspaceSelection pairs for the ability
    // to have several tabbed documents opened in parallel, each with its current selection.
    //
    WorkspaceSelectionWeakPtr currentWorkspaceSelection() const {
        return workspaceSelection_;
    }

    /// This signal is emitted whenever the `currentWorkspaceSelection()` object:
    ///
    /// - is replaced to point to a different `WorkspaceSelection` instance, or
    ///
    /// - emits `changed()`, that is when the selection contains different items
    ///
    VGC_SIGNAL(currentWorkspaceSelectionChanged)

private:
    workspace::WorkspaceSharedPtr currentWorkspace_;
    WorkspaceSelectionSharedPtr workspaceSelection_;
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_DOCUMENTMANAGER_H
