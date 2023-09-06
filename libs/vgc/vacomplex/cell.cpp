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

#include <vgc/vacomplex/cell.h>

namespace vgc::vacomplex {

Node::Node(core::Id id) noexcept
    : id_(id)
    , cellType_(notACell) {
}

Node::Node(core::Id id, CellType cellType) noexcept
    : id_(id)
    , cellType_(static_cast<UInt8>(cellType)) {
}

Node::~Node() {
}

void Node::debugPrint(core::StringWriter& out) {
    std::string idString = core::format("[{}]", id());
    out << core::format("{:<6}", idString);
    debugPrint_(out);
}

Cell::Cell()
    : Node(-1, CellType::KeyVertex) {

    throw core::LogicError(
        "Calling vgc::vacomplex::Cell default constructor. "
        "This constructor is reserved as unused default constructor in the context "
        "of virtual inheritance.");
}

Cell::Cell(
    core::Id id,
    CellSpatialType spatialType,
    CellTemporalType temporalType) noexcept
    : Node(id, detail::vacCellTypeCombine(spatialType, temporalType)) {
}

Complex* Cell::complex() const {
    Group* p = parentGroup();
    return p ? p->complex() : nullptr;
}

void Cell::bindCellProperties(CellProperties* properties) {
    properties->cell_ = this;
}

void Cell::unbindCellProperties(CellProperties* properties) {
    properties->cell_ = nullptr;
}

void Cell::dirtyMesh_() {
}

bool Cell::updateGeometryFromBoundary_() {
    return false;
}

Transform Group::computeInverseTransformTo(Group* ancestor) const {
    Transform t = inverseTransform_;
    Group* g = parentGroup();
    while (g && g != ancestor) {
        t *= g->inverseTransform();
        g = parentGroup();
    }
    return t;
}

void Group::setTransform_(const Transform& transform) {
    transform_ = transform;
    // todo: handle non-invertible case.
    inverseTransform_ = transform_.inverted();
    updateTransformFromRoot_();
}

void Group::updateTransformFromRoot_() {
    const Group* p = parentGroup();
    if (p) {
        transformFromRoot_ = p->transformFromRoot() * transform_;
    }
    else {
        transformFromRoot_ = transform_;
    }
}

void Group::debugPrint_(core::StringWriter& out) {
    out << core::format( //
        "{:<12} numChildren={} ",
        "Group",
        numChildren());
}

} // namespace vgc::vacomplex
