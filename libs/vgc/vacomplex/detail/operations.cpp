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

#include <vgc/core/algorithms.h> // sort

namespace vgc::vacomplex::detail {

Operations::Operations(Complex* complex)
    : complex_(complex) {

    // Ensure complex is non-null
    if (!complex) {
        throw LogicError("Cannot instantiate a VAC `Operations` with a null complex.");
    }

    if (++complex->numOperationsInProgress_ == 1) {
        // Increment version
        complex->version_ += 1;
    }
}

Operations::~Operations() {
    Complex* complex = this->complex();

    // TODO: try/catch?

    if (--complex->numOperationsInProgress_ == 0) {

        // Update geometry from boundary (for example, ensure that edges are
        // snapped to their end vertices). By iterating on cells by increasing
        // dimension, we avoid having to do this recursively.
        //
        core::Array<Cell*> cellsToUpdateGeometryFromBoundary;
        for (const ModifiedNodeInfo& info : complex->opDiff_.modifiedNodes_) {
            if (info.flags().has(NodeModificationFlag::BoundaryGeometryChanged)) {
                Cell* cell = info.node()->toCell();
                if (cell) {
                    cellsToUpdateGeometryFromBoundary.append(cell);
                }
            }
        }
        core::sort(cellsToUpdateGeometryFromBoundary, [](Cell* c1, Cell* c2) {
            return c1->cellType() < c2->cellType();
        });
        for (Cell* cell : cellsToUpdateGeometryFromBoundary) {
            if (cell->updateGeometryFromBoundary()) {
                onNodeModified_(cell, NodeModificationFlag::GeometryChanged);
            }
        }

        // Call finalizeConcat() for all new cells that may have been created
        // via a concatenation operation.
        //
        for (const CreatedNodeInfo& info : complex->opDiff_.createdNodes_) {
            Cell* cell = info.node()->toCell();
            if (cell) {
                switch (cell->cellType()) {
                case CellType::KeyEdge: {
                    KeyEdge* ke = cell->toKeyEdgeUnchecked();
                    ke->data().finalizeConcat();
                    break;
                }
                case CellType::KeyFace: {
                    KeyFace* kf = cell->toKeyFaceUnchecked();
                    kf->data().finalizeConcat();
                    break;
                }
                default:
                    break;
                }
            }
        }

        // Notify the outside world of the change.
        //
        complex->nodesChanged().emit(complex->opDiff_);

        // Clear diff data.
        //
        complex->opDiff_.clear();
        complex->temporaryCellSet_.clear();
    }
}

} // namespace vgc::vacomplex::detail
