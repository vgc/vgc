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

#ifndef VGC_CANVAS_WORKSPACESELECTION_H
#define VGC_CANVAS_WORKSPACESELECTION_H

#include <vgc/canvas/api.h>
#include <vgc/core/array.h>
#include <vgc/core/id.h>
#include <vgc/core/object.h>
#include <vgc/core/span.h>

namespace vgc::canvas {

VGC_DECLARE_OBJECT(WorkspaceSelection);

/// \class WorkspaceSelection
/// \brief Stores a list of selected items in a Workspace.
///
/// A `WorkspaceSelection` is an object that allows you to manipulate a list of
/// selected items in a `Workspace`, and allows listeners to be notified of any
/// changes in this selection.
///
/// Note that selection is a UI concept, which is why this class is not defined
/// in the `workspace` library (which is a back-end library), but is instead
/// defined in the `canvas` library (which acts as a bridge between the
/// back-end and the UI). For example, the `Canvas` class is given both a
/// `Workspace` object and a `WorkspaceSelection` object that it operates on.
///
class VGC_CANVAS_API WorkspaceSelection : public core::Object {
private:
    VGC_OBJECT(WorkspaceSelection, core::Object)

protected:
    WorkspaceSelection(CreateKey);

public:
    /// Creates a `WorkspaceSelection`.
    ///
    static WorkspaceSelectionPtr create();

    /// Returns the list of selected item IDs.
    ///
    const core::Array<core::Id>& itemIds() const {
        return itemIds_;
    }

    /// Sets the list of selected item IDs.
    ///
    void setItemIds(core::ConstSpan<core::Id> itemIds);

    /// Clears the selection, that is, unselects all.
    ///
    void clear() {
        setItemIds({});
    }

    /// This signal is emitted whenever the list of selected item IDs changes.
    ///
    VGC_SIGNAL(changed)

private:
    core::Array<core::Id> itemIds_;

    // TODO: store an history of item IDs, to allow undoing selection actions?
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_WORKSPACESELECTION_H
