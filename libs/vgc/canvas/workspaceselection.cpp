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

#include <vgc/canvas/workspaceselection.h>

namespace vgc::canvas {

WorkspaceSelection::WorkspaceSelection(CreateKey key)
    : Object(key) {
}

WorkspaceSelectionPtr WorkspaceSelection::create() {
    return core::createObject<WorkspaceSelection>();
}

void WorkspaceSelection::setItemIds(core::ConstSpan<core::Id> itemIds) {

    if (core::equal(itemIds_, itemIds)) {
        return;
    }

    // Copies the IDs without duplicates
    itemIds_.clear();
    for (core::Id id : itemIds) {
        if (!itemIds_.contains(id)) {
            itemIds_.append(id);
        }
    }

    changed().emit();
}

} // namespace vgc::canvas
