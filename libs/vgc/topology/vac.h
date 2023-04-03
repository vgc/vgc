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

#ifndef VGC_TOPOLOGY_VAC_H
#define VGC_TOPOLOGY_VAC_H

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <vgc/core/array.h>
#include <vgc/core/flags.h>
#include <vgc/core/object.h>
#include <vgc/topology/api.h>
#include <vgc/topology/cell.h>
#include <vgc/topology/exceptions.h>
#include <vgc/topology/inbetweenedge.h>
#include <vgc/topology/inbetweenface.h>
#include <vgc/topology/inbetweenvertex.h>
#include <vgc/topology/keyedge.h>
#include <vgc/topology/keyface.h>
#include <vgc/topology/keyvertex.h>

namespace vgc::topology {

VGC_DECLARE_OBJECT(Vac);

// clang-format off
enum class VacNodeDiffFlag {
    Created                 = 0x01,
    Removed                 = 0x02,
    Reparented              = 0x04,
    ChildrenChanged         = 0x08,
    AttributeChanged        = 0x10,
    GeometryChanged         = 0x20,
    StarChanged             = 0x40,
};
VGC_DEFINE_FLAGS(VacNodeDiffFlags, VacNodeDiffFlag)
// clang-format on

class VGC_TOPOLOGY_API VacNodeDiff {
public:
    VacNode* node() const {
        return node_;
    }

    VacNodeDiffFlags flags() const {
        return flags_;
    }

    void setNode(VacNode* node) {
        node_ = node;
    }

    void setFlags(VacNodeDiffFlags flags) {
        flags_ = flags;
    }

private:
    VacNode* node_ = nullptr;
    VacNodeDiffFlags flags_ = {};
};

class VGC_TOPOLOGY_API VacDiff {
public:
    VacDiff() = default;

    void clear() {
        nodeDiffs_.clear();
    }

    bool isEmpty() const {
        return nodeDiffs_.empty();
    }

    const std::unordered_map<core::Id, VacNodeDiff>& nodeDiffs() const {
        return nodeDiffs_;
    }

    void merge(const VacDiff& other);

private:
    friend class vgc::topology::detail::Operations;
    friend class vgc::topology::Vac;

    std::unordered_map<core::Id, VacNodeDiff> nodeDiffs_;

    // ops helpers

    void onNodeRemoved(VacNode* node) {
        auto& it = nodeDiffs_[node->id()];
        it.setNode(node);
    }

    void onNodeDiff(VacNode* node, VacNodeDiffFlags diffFlags) {
        VacNodeDiff& nodeDiff = nodeDiffs_[node->id()];
        nodeDiff.setFlags(nodeDiff.flags() | diffFlags);
    }
};

/// \class vgc::dom::Vac
/// \brief Represents a VAC.
///
class VGC_TOPOLOGY_API Vac : public core::Object {
private:
    VGC_OBJECT(Vac, core::Object)

    friend detail::Operations;

protected:
    Vac() {
        resetRoot();
    }

    void onDestroyed() override;

public:
    static VacPtr create();

    void clear();

    VacGroup* resetRoot();

    VacGroup* rootGroup() const {
        return root_;
    }

    VacNode* find(core::Id id) const;

    VacCell* findCell(core::Id id) const;

    VacGroup* findGroup(core::Id id) const;

    bool containsNode(core::Id id) const;

    // An increasing version seems enough, we don't need it to match document version.

    Int64 version() const {
        return version_;
    }

    //const VacDiff& pendingDiff() {
    //    return diff_;
    //}

    //bool emitPendingDiff();

    VGC_SIGNAL(nodeAboutToBeRemoved, (VacNode*, node))
    VGC_SIGNAL(
        nodeCreated,
        (VacNode*, node),
        (core::Span<VacNode*>, operationSourceNodes))
    VGC_SIGNAL(nodeMoved, (VacNode*, node))
    VGC_SIGNAL(cellModified, (VacCell*, cell))

    //VGC_SIGNAL(changed, (const VacDiff&, diff))

protected:
    void incrementVersion() {
        ++version_;
    }

    bool insertNode(std::unique_ptr<VacNode>&& node);

private:
    Int64 version_ = 0;
    std::unordered_map<core::Id, std::unique_ptr<VacNode>> nodes_;
    // TODO: maybe store the root in the map.
    VacGroup* root_;
    VacDiff diff_ = {};
    bool isDiffEnabled_ = false;
    bool isBeingCleared_ = false;
};

} // namespace vgc::topology

namespace vgc::vacomplex {

using Complex = topology::Vac;
using ComplexPtr = topology::VacPtr;

using Node = topology::VacNode;

using Cell = topology::VacCell;
using Group = topology::VacGroup;

using CellType = topology::VacCellType;
using topology::CellSpatialType;
using topology::CellTemporalType;

using NodeDiffFlag = topology::VacNodeDiffFlag;
using Diff = topology::VacDiff;

using topology::EdgeCell;
using topology::FaceCell;
using topology::VertexCell;

using topology::KeyCell;
using topology::KeyEdge;
using topology::KeyFace;
using topology::KeyVertex;

using topology::InbetweenCell;
using topology::InbetweenEdge;
using topology::InbetweenFace;
using topology::InbetweenVertex;

using topology::EdgeGeometry;
using topology::FaceGeometry;

} // namespace vgc::vacomplex

#endif // VGC_TOPOLOGY_VAC_H
