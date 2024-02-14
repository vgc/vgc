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

#include <vgc/vacomplex/algorithms.h>

#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/keyedge.h>
#include <vgc/vacomplex/keyface.h>

namespace vgc::vacomplex {

namespace {

template<typename OutType, typename InType>
core::Array<OutType*> initAddFromOp(core::ConstSpan<InType*> input) {
    return core::Array<OutType*>(input);
}

template<>
core::Array<Cell*> initAddFromOp(core::ConstSpan<Node*> input) {
    core::Array<Cell*> output;
    output.reserve(input.length());
    for (Node* node : input) {
        if (Cell* cell = node->toCell()) {
            output.append(cell);
        }
    }
    return output;
}

template<typename OutType>
void addFromOp(
    core::ConstSpan<Node*> input,
    core::Array<OutType*>& output,
    CellRangeView (Cell::*op)() const) {

    for (Node* node : input) {
        if (Cell* cell = node->toCell()) {
            for (Cell* otherCell : (cell->*op)()) {
                if (!output.contains(otherCell)) {
                    output.append(otherCell);
                }
            }
        }
    }
}

template<typename OutType>
void addFromOp(
    core::ConstSpan<Cell*> input,
    core::Array<OutType*>& output,
    CellRangeView (Cell::*op)() const) {

    for (Cell* cell : input) {
        for (Cell* otherCell : (cell->*op)()) {
            if (!output.contains(otherCell)) {
                output.append(otherCell);
            }
        }
    }
}

template<typename OutType, typename InType>
core::Array<OutType*>
addFromOp(core::ConstSpan<InType*> input, CellRangeView (Cell::*op)() const) {
    core::Array<OutType*> output = initAddFromOp<OutType>(input);
    addFromOp(input, output, op);
    return output;
}

template<typename T>
core::Array<T*> setFromOp(core::ConstSpan<T*> input, CellRangeView (Cell::*op)() const) {
    core::Array<T*> output;
    addFromOp(input, output, op);
    return output;
}

template<typename OutType, typename InType>
core::Array<OutType*> closure_(core::ConstSpan<InType*> input) {
    return addFromOp<OutType>(input, &Cell::boundary);
};

template<typename OutType, typename InType>
core::Array<OutType*> opening_(core::ConstSpan<InType*> input) {
    return addFromOp<OutType>(input, &Cell::star);
};

// Count the number of uses of `edge` by the given `face`.
//
Int countUses_(KeyEdge* edge, KeyFace* face) {
    Int count = 0;
    for (const KeyCycle& cycle : face->cycles()) {
        if (cycle.steinerVertex()) {
            continue;
        }
        for (const KeyHalfedge& halfedge : cycle.halfedges()) {
            if (halfedge.edge() == edge) {
                ++count;
            }
        }
    }
    return count;
}

// Count the number of uses of `vertex` among the given `edges`.
//
Int countUses_(KeyVertex* vertex, KeyEdge* edge) {
    Int count = 0;
    if (edge->startVertex() == vertex) {
        ++count;
    }
    if (edge->endVertex() == vertex) {
        ++count;
    }
    return count;
}

template<typename InType>
void maybeAddVertexToBoundary(
    core::ConstSpan<InType*> input,
    core::Array<Cell*>& output,
    KeyVertex* vertex) {

    Int nUses = 0;
    for (Cell* starCell : vertex->star()) {
        if (input.contains(starCell)) {
            if (KeyFace* face = starCell->toKeyFace()) {
                return;
            }
            else if (KeyEdge* edge = starCell->toKeyEdge()) {
                nUses += countUses_(vertex, edge);
                if (nUses > 1) {
                    return;
                }
            }
        }
    }
    if (nUses == 1) {
        output.append(vertex);
    }
}

template<typename InType>
void maybeAddEdgeToBoundary(
    core::ConstSpan<InType*> input,
    core::Array<Cell*>& output,
    KeyEdge* edge) {

    Int nUses = 0;
    for (Cell* starCell : edge->star()) {
        if (input.contains(starCell)) {
            if (KeyFace* face = starCell->toKeyFace()) {
                nUses += countUses_(edge, face);
                if (nUses > 1) {
                    return;
                }
            }
        }
    }
    if (nUses == 1) {
        output.append(edge);
    }
}

template<typename InType>
void maybeAddCellToBoundary(
    core::ConstSpan<InType*> input,
    core::Array<Cell*>& output,
    Cell* cell) {

    if (KeyVertex* vertex = cell->toKeyVertex()) {
        maybeAddVertexToBoundary(input, output, vertex);
    }
    else if (KeyEdge* edge = cell->toKeyEdge()) {
        maybeAddEdgeToBoundary(input, output, edge);
    }
}

template<typename OutType>
core::Array<OutType*> boundary_(core::ConstSpan<OutType*> input) {

    core::Array<Cell*> output;

    // Iterate over the closure of the input and add to the boundary:
    // - each cell that is not in the input, and
    // - each (n-1)-cell whose star within the input is homeomorphic to Hn = Rn x [0, infinity)
    //
    for (Cell* cell : closure_<Cell>(input)) {
        if (!input.contains(cell)) {
            output.append(cell);
        }
        else {
            maybeAddCellToBoundary(input, output, cell);
        }
    }

    return closure_<OutType, Cell>(output);
}

template<typename OutType>
core::Array<OutType*> outerBoundary_(core::ConstSpan<OutType*> input) {

    core::Array<Cell*> output;
    core::Array<Cell*> input_ = closure_<Cell>(input);

    // Same as boundary_(), except we do not need to check `if
    // (!input_.contains(cell))`, since we know it is always false.
    //
    for (Cell* cell : input_) {
        maybeAddCellToBoundary(input, output, cell);
    }

    return closure_<OutType, Cell>(output);
}

} // namespace

core::Array<Node*> boundary(core::ConstSpan<Node*> nodes) {
    return boundary_<Node>(nodes);
};

core::Array<Cell*> boundary(core::ConstSpan<Cell*> cells) {
    return boundary_<Cell>(cells);
};

core::Array<Node*> outerBoundary(core::ConstSpan<Node*> nodes) {
    return outerBoundary_<Node>(nodes);
};

core::Array<Cell*> outerBoundary(core::ConstSpan<Cell*> cells) {
    return outerBoundary_<Cell>(cells);
};

core::Array<Node*> star(core::ConstSpan<Node*> nodes) {
    return setFromOp(nodes, &Cell::star);
};

core::Array<Cell*> star(core::ConstSpan<Cell*> cells) {
    return setFromOp(cells, &Cell::star);
};

core::Array<Node*> closure(core::ConstSpan<Node*> nodes) {
    return closure_<Node>(nodes);
};

core::Array<Cell*> closure(core::ConstSpan<Cell*> cells) {
    return closure_<Cell>(cells);
};

core::Array<Node*> opening(core::ConstSpan<Node*> nodes) {
    return opening_<Node>(nodes);
};

core::Array<Cell*> opening(core::ConstSpan<Cell*> cells) {
    return opening_<Cell>(cells);
};

} // namespace vgc::vacomplex
