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

#include <vgc/vacomplex/detail/operations.h>

namespace vgc::vacomplex::detail {

void Operations::onNodeCreated_(Node* node) {
    complex_->opDiff_.onNodeCreated_(node);
}

void Operations::onNodeDestroyed_(core::Id nodeId) {
    complex_->opDiff_.onNodeDestroyed_(nodeId);
}

void Operations::onNodeInserted_(
    Node* node,
    Node* oldParent,
    NodeInsertionType insertionType) {

    complex_->opDiff_.onNodeInserted_(node, oldParent, insertionType);
}

void Operations::onNodeModified_(Node* node, NodeModificationFlags diffFlags) {
    complex_->opDiff_.onNodeModified_(node, diffFlags);
}

void Operations::onNodePropertyModified_(Node* node, core::StringId name) {
    complex_->opDiff_.onNodePropertyModified_(node, name);
}

void Operations::onBoundaryChanged_(Cell* boundedCell, Cell* boundingCell) {
    onNodeModified_(
        boundedCell,
        {NodeModificationFlag::BoundaryChanged,
         NodeModificationFlag::BoundaryGeometryChanged});
    onNodeModified_(boundingCell, NodeModificationFlag::StarChanged);
}

void Operations::onGeometryChanged_(Cell* cell) {
    onNodeModified_(cell, NodeModificationFlag::GeometryChanged);
    for (Cell* starCell : cell->star()) { // See [1]
        onNodeModified_(starCell, NodeModificationFlag::BoundaryGeometryChanged);
    }
    // Note: no need for recursion in the `cell->star()` loops below, since
    // starCell->star() is a subset of cell->star().
}

} // namespace vgc::vacomplex::detail
