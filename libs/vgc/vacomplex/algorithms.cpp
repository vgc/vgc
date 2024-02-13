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

core::Array<Cell*> boundary(core::ConstSpan<Cell*> cells) {
    core::Array<Cell*> result;
    for (Cell* c : cells) {
        for (Cell* sc : c->boundary()) {
            if (!result.contains(sc)) {
                result.append(sc);
            }
        }
    }
    return result;
};

core::Array<Cell*> star(core::ConstSpan<Cell*> cells) {
    core::Array<Cell*> result;
    for (Cell* c : cells) {
        for (Cell* sc : c->star()) {
            if (!result.contains(sc)) {
                result.append(sc);
            }
        }
    }
    return result;
};

core::Array<Cell*> closure(core::ConstSpan<Cell*> cells) {
    core::Array<Cell*> result(cells);
    for (Cell* c : cells) {
        for (Cell* bc : c->boundary()) {
            if (!result.contains(bc)) {
                result.append(bc);
            }
        }
    }
    return result;
};

core::Array<Cell*> opening(core::ConstSpan<Cell*> cells) {
    core::Array<Cell*> result(cells);
    for (Cell* c : cells) {
        for (Cell* sc : c->star()) {
            if (!result.contains(sc)) {
                result.append(sc);
            }
        }
    }
    return result;
};

} // namespace vgc::vacomplex
