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

KeyEdge* createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyOpenEdge: parentGroup is nullptr.");
    }
    if (!startVertex) {
        throw LogicError("createKeyOpenEdge: startVertex is nullptr.");
    }
    if (!endVertex) {
        throw LogicError("createKeyOpenEdge: endVertex is nullptr.");
    }

    Vac* vac = parentGroup->vac();

    if (vac != startVertex->vac()) {
        throw LogicError(
            "createKeyOpenEdge: given `parentGroup` and `startVertex` are not "
            "in the same `Vac`.");
    }
    if (vac != endVertex->vac()) {
        throw LogicError("createKeyOpenEdge: given `parentGroup` and `endVertex` are not "
                         "in the same `Vac`.");
    }
    if (t != startVertex->time()) {
        throw LogicError(
            "createKeyOpenEdge: given `startVertex` is not at the given time `t`.");
    }
    if (t != endVertex->time()) {
        throw LogicError(
            "createKeyOpenEdge: given `endVertex` is not at the given time `t`.");
    }

    return detail::Operations::createKeyOpenEdge(
        startVertex, endVertex, points, widths, parentGroup, nextSibling, {}, t);
}

KeyFace* createKeyFace(
    core::Array<KeyCycle> cycles,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyFace: parentGroup is nullptr.");
    }
    for (const KeyCycle& cycle : cycles) {
        if (!cycle.isValid()) {
            throw LogicError(
                "createKeyFace: at least one of the input cycles is not valid.");
        }
    }

    return detail::Operations::createKeyFace(
        std::move(cycles), parentGroup, nextSibling, {}, t);
}

KeyFace* createKeyFace(
    KeyCycle cycle,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyFace: parentGroup is nullptr.");
    }
    if (!cycle.isValid()) {
        throw LogicError("createKeyFace: the input cycle is not valid.");
    }

    return detail::Operations::createKeyFace(
        core::Array<KeyCycle>(1, std::move(cycle)), parentGroup, nextSibling, {}, t);
}

} // namespace vgc::topology::ops
