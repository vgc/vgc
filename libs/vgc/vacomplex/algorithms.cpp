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

#include <unordered_map>
#include <unordered_set>

#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/keyedge.h>
#include <vgc/vacomplex/keyface.h>

namespace vgc::vacomplex {

namespace {

// Copies the ConstSpan into an Array, safe-casting Node* to Cell* if necessary.
//
template<typename OutType, typename InType>
core::Array<OutType*> copy(core::ConstSpan<InType*> input) {
    return core::Array<OutType*>(input);
}

template<>
core::Array<Cell*> copy(core::ConstSpan<Node*> input) {
    core::Array<Cell*> output;
    output.reserve(input.length());
    for (Node* node : input) {
        if (Cell* cell = node->toCell()) {
            output.append(cell);
        }
    }
    return output;
}

// Executes the given `fn` for all `cells`.
//
template<typename Fn>
void forEachCell(core::ConstSpan<Cell*> cells, Fn fn) {
    for (Cell* cell : cells) {
        fn(cell);
    }
}

// Executes the given `fn` for all cells among the given `nodes`.
//
template<typename Fn>
void forEachCell(core::ConstSpan<Node*> nodes, Fn fn) {
    for (Node* node : nodes) {
        if (Cell* cell = node->toCell()) {
            fn(cell);
        }
    }
}

// Appends all the given `cells` to `output`, except if already in the `output`.
//
template<typename OutType>
void extendUnique(core::Array<OutType*>& output, CellRangeView cells) {
    for (Cell* cell : cells) {
        if (!output.contains(cell)) {
            output.append(cell);
        }
    }
}

// Appends all the given `cells` to `output`, except if already in the `output`
// or in the `exclusion` list.
//
template<typename OutType, typename ExType>
void extendUniqueExcluded(
    core::Array<OutType*>& output,
    CellRangeView cells,
    core::ConstSpan<ExType*> exclusion) {

    for (Cell* cell : cells) {
        if (!output.contains(cell) && !exclusion.contains(cell)) {
            output.append(cell);
        }
    }
}

template<typename OutType, typename InType>
core::Array<OutType*> closure_(core::ConstSpan<InType*> input) {
    core::Array<OutType*> output = copy<OutType>(input);
    forEachCell(input, [&](Cell* cell) { //
        extendUnique(output, cell->boundary());
    });
    return output;
}

template<typename OutType, typename InType>
core::Array<OutType*> opening_(core::ConstSpan<InType*> input) {
    core::Array<OutType*> output = copy<OutType>(input);
    forEachCell(input, [&](Cell* cell) { //
        extendUnique(output, cell->star());
    });
    return output;
}

template<typename OutType>
core::Array<OutType*> star_(core::ConstSpan<OutType*> input) {
    core::Array<OutType*> output;
    forEachCell(input, [&](Cell* cell) { //
        extendUniqueExcluded(output, cell->star(), input);
    });
    return output;
}

// Returns the number of uses of `edge` by the given `face`.
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

// Returns the number of uses of `vertex` by the given `edge`.
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

// Appends the given `vertex` to `output` if its opening in `closure(input)` is
// locally homeomorphic to the boundary of an open curve.
//
// Note:
//
// [1] `input.contains(starCell)` is equivalent to
//     `closure(input).contains(starCell)` in this context.
//
template<typename InType>
void appendVertexIfBoundaryLike(
    core::ConstSpan<InType*> input,
    core::Array<Cell*>& output,
    KeyVertex* vertex) {

    Int nUses = 0;
    for (Cell* starCell : vertex->star()) {
        if (input.contains(starCell)) { // [1]
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

// Appends the given `edge` to `output` if its opening in `closure(input)` is
// locally homeomorphic to the boundary of a surface.
//
template<typename InType>
void appendEdgeIfBoundaryLike(
    core::ConstSpan<InType*> input,
    core::Array<Cell*>& output,
    KeyEdge* edge) {

    Int nUses = 0;
    for (Cell* starCell : edge->star()) {
        if (input.contains(starCell)) { // [1]
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

// Appends the given `cell` to `output` if its opening in `closure(input)` is
// locally homeomorphic to Hn = Rn x [0, infinity), where n = dim(cell) + 1.
//
template<typename InType>
void appendCellIfBoundaryLike(
    core::ConstSpan<InType*> input,
    core::Array<Cell*>& output,
    Cell* cell) {

    if (KeyVertex* vertex = cell->toKeyVertex()) {
        appendVertexIfBoundaryLike(input, output, vertex);
    }
    else if (KeyEdge* edge = cell->toKeyEdge()) {
        appendEdgeIfBoundaryLike(input, output, edge);
    }
}

template<typename OutType>
core::Array<OutType*> boundary_(core::ConstSpan<OutType*> input) {

    core::Array<Cell*> output;

    // Iterate over the closure of the input and add to the boundary:
    // - each cell that is not in the input, and
    // - each (n-1)-cell whose opening in closure(input) is homeomorphic to Hn = Rn x [0, infinity)
    //
    for (Cell* cell : closure_<Cell>(input)) {
        if (!input.contains(cell)) {
            output.append(cell);
        }
        else {
            appendCellIfBoundaryLike(input, output, cell);
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
        appendCellIfBoundaryLike(input, output, cell);
    }

    return closure_<OutType, Cell>(output);
}

template<typename T>
using Set = std::unordered_set<T>;

template<typename OutType>
Set<OutType*> toSet(core::ConstSpan<OutType*> input) {
    return Set<OutType*>(input.begin(), input.end());
}

template<typename InType>
Set<Cell*> toCellSet(core::ConstSpan<InType*> nodes) {
    Set<Cell*> output;
    output.reserve(nodes.length());
    forEachCell(nodes, [&](Cell* cell) { //
        output.insert(cell);
    });
    return output;
}

template<typename OutType>
bool contains(const Set<OutType*>& nodes, Cell* cell) {
    return nodes.find(cell) != nodes.end();
}

template<typename OutType>
core::Array<OutType*> connected_(core::ConstSpan<OutType*> input) {

    Set<OutType*> output = toSet(input);
    Set<Cell*> cellsAddedInPreviousIteration = toCellSet(input);
    Set<Cell*> cellsAddedInThisIteration;

    while (cellsAddedInPreviousIteration.size() != 0) {
        cellsAddedInThisIteration.clear();
        for (Cell* c : cellsAddedInPreviousIteration) {
            for (Cell* d : c->boundary()) {
                if (!contains(output, d)) {
                    output.insert(d);
                    cellsAddedInThisIteration.insert(d);
                }
            }
            for (Cell* d : c->star()) {
                if (!contains(output, d)) {
                    output.insert(d);
                    cellsAddedInThisIteration.insert(d);
                }
            }
        }
        cellsAddedInPreviousIteration.swap(cellsAddedInThisIteration);
    }

    return core::Array<OutType*>(output);
}

template<typename T, typename U>
using Map = std::unordered_map<T, U>;

template<typename OutType>
core::Array<core::Array<OutType*>> connectedComponents_(core::ConstSpan<OutType*> input) {

    // Initialization

    Map<OutType*, int> component;
    for (OutType* node : input) {
        component[node] = -1;
    }
    int componentIndex = -1;

    // Main loop

    for (OutType* node : input) {

        // If already assigned a connected component, do nothing
        if (component[node] != -1) {
            continue;
        }

        // Otherwise, create a new connected component and assign it to this node
        ++componentIndex;
        component[node] = componentIndex;

        // If the node is not a cell, then it's alone in its connected component
        Cell* cell = node->toCell();
        if (!cell) {
            continue;
        }

        // Otherwise, let's find all other cells in the input that are
        // connected to this cell. We use a stack for yet unvisited cells
        // belonging to the same component.
        //
        core::Array<Cell*> stack;
        stack.append(cell);

        // While there are cells to visit
        while (!stack.isEmpty()) {

            Cell* cellToVisit = stack.pop();
            CellRangeView boundary = cellToVisit->boundary();
            CellRangeView star = cellToVisit->star();

            //  Find other cells to visit
            for (OutType* otherNode : input) {
                if (component[otherNode] != -1) {
                    continue;
                }
                if (Cell* otherCell = otherNode->toCell()) {
                    if (boundary.contains(otherCell) || star.contains(otherCell)) {
                        component[otherNode] = componentIndex;
                        stack.append(otherCell);
                    }
                }
            }
        }
    }

    // Convert to output

    core::Array<core::Array<OutType*>> output;
    for (int i = 0; i <= componentIndex; ++i) {
        output.append({});
    }
    for (auto& [cell, i] : component) {
        output[i].append(cell);
    }
    return output;
}

} // namespace

core::Array<Node*> boundary(core::ConstSpan<Node*> nodes) {
    return boundary_<Node>(nodes);
}

core::Array<Cell*> boundary(core::ConstSpan<Cell*> cells) {
    return boundary_<Cell>(cells);
}

core::Array<Node*> outerBoundary(core::ConstSpan<Node*> nodes) {
    return outerBoundary_<Node>(nodes);
}

core::Array<Cell*> outerBoundary(core::ConstSpan<Cell*> cells) {
    return outerBoundary_<Cell>(cells);
}

core::Array<Node*> star(core::ConstSpan<Node*> nodes) {
    return star_<Node>(nodes);
}

core::Array<Cell*> star(core::ConstSpan<Cell*> cells) {
    return star_<Cell>(cells);
}

core::Array<Node*> closure(core::ConstSpan<Node*> nodes) {
    return closure_<Node>(nodes);
}

core::Array<Cell*> closure(core::ConstSpan<Cell*> cells) {
    return closure_<Cell>(cells);
}

core::Array<Node*> opening(core::ConstSpan<Node*> nodes) {
    return opening_<Node>(nodes);
}

core::Array<Cell*> opening(core::ConstSpan<Cell*> cells) {
    return opening_<Cell>(cells);
}

core::Array<Node*> connected(core::ConstSpan<Node*> nodes) {
    return connected_<Node>(nodes);
}

core::Array<Cell*> connected(core::ConstSpan<Cell*> cells) {
    return connected_<Cell>(cells);
}

core::Array<core::Array<Node*>> connectedComponents(core::ConstSpan<Node*> nodes) {
    return connectedComponents_<Node>(nodes);
}

core::Array<core::Array<Cell*>> connectedComponents(core::ConstSpan<Cell*> cells) {
    return connectedComponents_<Cell>(cells);
}

} // namespace vgc::vacomplex
