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

namespace {

void checkAddToBoundaryArgs_(Cell* boundedCell, Cell* boundingCell) {
    if (!boundingCell) {
        throw core::LogicError("Cannot add or remove null cell to boundary.");
    }
    if (!boundedCell) {
        throw core::LogicError("Cannot modify the boundary of a null cell.");
    }
}

} // namespace

void Operations::addToBoundary_(Cell* boundedCell, Cell* boundingCell) {
    checkAddToBoundaryArgs_(boundedCell, boundingCell);
    if (!boundedCell->boundary_.contains(boundingCell)) {
        boundedCell->boundary_.append(boundingCell);
        boundingCell->star_.append(boundedCell);
        onBoundaryChanged_(boundedCell, boundingCell);
    }
}

void Operations::removeFromBoundary_(Cell* boundedCell, Cell* boundingCell) {
    checkAddToBoundaryArgs_(boundedCell, boundingCell);
    if (boundedCell->boundary_.contains(boundingCell)) {
        boundedCell->boundary_.removeOne(boundingCell);
        boundingCell->star_.removeOne(boundedCell);
        onBoundaryChanged_(boundedCell, boundingCell);
    }
}

void Operations::addToBoundary_(FaceCell* face, const KeyCycle& cycle) {
    if (cycle.steinerVertex()) {
        // Steiner cycle
        addToBoundary_(face, cycle.steinerVertex());
    }
    else if (cycle.halfedges().first().isClosed()) {
        // Simple cycle
        addToBoundary_(face, cycle.halfedges().first().edge());
    }
    else {
        // Non-simple cycle
        for (const KeyHalfedge& halfedge : cycle.halfedges()) {
            addToBoundary_(face, halfedge.edge());
            addToBoundary_(face, halfedge.endVertex());
        }
    }
}

void Operations::addToBoundary_(FaceCell* face, const KeyPath& path) {
    if (path.singleVertex()) {
        addToBoundary_(face, path.singleVertex());
    }
    else {
        addToBoundary_(face, path.halfedges().first().startVertex());
        for (const KeyHalfedge& halfedge : path.halfedges()) {
            addToBoundary_(face, halfedge.edge());
            addToBoundary_(face, halfedge.endVertex());
        }
    }
}

} // namespace vgc::vacomplex::detail
