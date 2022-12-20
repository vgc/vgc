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

#include <vgc/topology/operations.h>

#include <vgc/topology/detail/operationsimpl.h>

namespace vgc::topology::ops {

KeyEdge* createKeyEdge(
    core::Id id,
    VacGroup* parentGroup,
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    VacNode* nextSibling,
    core::AnimTime t) {

    Vac* vac = parentGroup->vac();

    if (vac != startVertex->vac()) {
        throw LogicError("createKeyEdge: given `parentGroup` and `startVertex` are not "
                         "in the same `Vac`.");
    }
    if (vac != endVertex->vac()) {
        throw LogicError("createKeyEdge: given `parentGroup` and `endVertex` are not "
                         "in the same `Vac`.");
    }
    if (t != startVertex->time()) {
        throw LogicError(
            "createKeyEdge: given `startVertex` is not at the given time `t`.");
    }
    if (t != endVertex->time()) {
        throw LogicError(
            "createKeyEdge: given `endVertex` is not at the given time `t`.");
    }

    return detail::Operations::createKeyEdge(
        id, parentGroup, startVertex, endVertex, nextSibling, t);
}

} // namespace vgc::topology::ops
