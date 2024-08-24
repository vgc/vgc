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

// Note: we cannot use make_unique due to the non-public constructor,
// which is why we have to use `new` here.
//
template<class T, typename... Args>
T* Operations::createNode_(Args&&... args) {
    core::Id id = core::genId();
    T* node = new T(id, std::forward<Args>(args)...);
    std::unique_ptr<T> nodePtr(node);
    Complex::NodePtrMap& nodes = complex()->nodes_;
    bool emplaced = nodes.try_emplace(id, std::move(nodePtr)).second;
    if (!emplaced) {
        throw LogicError("Id collision error.");
    }
    onNodeCreated_(node);
    return node;
}

template<class T, typename... Args>
T* Operations::createNodeAt_(Group* parentGroup, Node* nextSibling, Args&&... args) {
    T* node = createNode_<T>(std::forward<Args>(args)...);
    moveToGroup(node, parentGroup, nextSibling);
    return node;
}

Group* Operations::createRootGroup() {
    Group* group = createNode_<Group>(complex());
    return group;
}

Group* Operations::createGroup(Group* parentGroup, Node* nextSibling) {
    Group* group = createNodeAt_<Group>(parentGroup, nextSibling, complex());
    return group;
}

KeyVertex* Operations::createKeyVertex(
    const geometry::Vec2d& position,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    KeyVertex* kv = createNodeAt_<KeyVertex>(parentGroup, nextSibling, t);

    // Topological attributes
    // -> None

    // Geometric attributes
    kv->position_ = position;

    return kv;
}

KeyEdge* Operations::createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    KeyEdgeData&& data,
    Group* parentGroup,
    Node* nextSibling) {

    KeyEdge* ke = createNodeAt_<KeyEdge>(parentGroup, nextSibling, startVertex->time());

    // Topological attributes
    ke->startVertex_ = startVertex;
    ke->endVertex_ = endVertex;
    addToBoundary_(ke, startVertex);
    addToBoundary_(ke, endVertex);

    // Geometric attributes
    ke->data().moveInit_(std::move(data));

    return ke;
}

KeyEdge* Operations::createKeyClosedEdge(
    KeyEdgeData&& data,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    KeyEdge* ke = createNodeAt_<KeyEdge>(parentGroup, nextSibling, t);

    // Topological attributes
    // -> None

    // Geometric attributes
    ke->data().moveInit_(std::move(data));

    return ke;
}

// Assumes `cycles` are valid.
// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
KeyFace* Operations::createKeyFace(
    core::Array<KeyCycle> cycles,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    KeyFace* kf = createNodeAt_<KeyFace>(parentGroup, nextSibling, t);

    // Topological attributes
    kf->cycles_ = std::move(cycles);
    for (const KeyCycle& cycle : kf->cycles()) {
        addToBoundary_(kf, cycle);
    }

    // Geometric attributes
    // -> None

    return kf;
}

// Assumes `kf` is not null.
// Assumes `cycle` is valid and matches kf's complex and time.
//
void Operations::addCycleToFace(KeyFace* kf, KeyCycle cycle) {
    // Topological attributes
    kf->cycles_.append(std::move(cycle));
    addToBoundary_(kf, kf->cycles_.last());
    // Geometric attributes
    // -> None
}

void Operations::setKeyVertexPosition(KeyVertex* kv, const geometry::Vec2d& pos) {
    if (pos == kv->position_) {
        // same data
        return;
    }

    kv->position_ = pos;

    onGeometryChanged_(kv);
}

} // namespace vgc::vacomplex::detail
