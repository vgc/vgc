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

namespace vgc::vacomplex {

namespace {

void addFromOp(
    core::ConstSpan<Node*> input,
    core::Array<Node*>& result,
    CellRangeView (Cell::*op)() const) {

    for (Node* node : input) {
        if (Cell* cell = node->toCell()) {
            for (Cell* otherCell : (cell->*op)()) {
                if (!result.contains(otherCell)) {
                    result.append(otherCell);
                }
            }
        }
    }
}

void addFromOp(
    core::ConstSpan<Cell*> input,
    core::Array<Cell*>& result,
    CellRangeView (Cell::*op)() const) {

    for (Cell* cell : input) {
        for (Cell* otherCell : (cell->*op)()) {
            if (!result.contains(otherCell)) {
                result.append(otherCell);
            }
        }
    }
}

template<typename T>
core::Array<T*> addFromOp(core::ConstSpan<T*> input, CellRangeView (Cell::*op)() const) {
    core::Array<T*> result(input);
    addFromOp(input, result, op);
    return result;
}

template<typename T>
core::Array<T*> setFromOp(core::ConstSpan<T*> input, CellRangeView (Cell::*op)() const) {
    core::Array<T*> result;
    addFromOp(input, result, op);
    return result;
}

} // namespace

core::Array<Node*> boundary(core::ConstSpan<Node*> nodes) {
    return setFromOp(nodes, &Cell::boundary);
};

core::Array<Cell*> boundary(core::ConstSpan<Cell*> cells) {
    return setFromOp(cells, &Cell::boundary);
};

core::Array<Node*> star(core::ConstSpan<Node*> nodes) {
    return setFromOp(nodes, &Cell::star);
};

core::Array<Cell*> star(core::ConstSpan<Cell*> cells) {
    return setFromOp(cells, &Cell::star);
};

core::Array<Node*> closure(core::ConstSpan<Node*> nodes) {
    return addFromOp(nodes, &Cell::boundary);
};

core::Array<Cell*> closure(core::ConstSpan<Cell*> cells) {
    return addFromOp(cells, &Cell::boundary);
};

core::Array<Node*> opening(core::ConstSpan<Node*> nodes) {
    return addFromOp(nodes, &Cell::star);
};

core::Array<Cell*> opening(core::ConstSpan<Cell*> cells) {
    return addFromOp(cells, &Cell::star);
};

} // namespace vgc::vacomplex
