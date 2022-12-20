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

#include <vgc/core/array.h>
#include <vgc/core/flags.h>
#include <vgc/core/object.h>
#include <vgc/topology/api.h>
#include <vgc/topology/cell.h>
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
    Created             = 0x01,
    Reparented          = 0x02,
    ChildrenChanged     = 0x04,
    AttributeChanged    = 0x08,
    GeometryChanged     = 0x10,
    StarChanged         = 0x20,
    BoundaryChanged     = 0x40
};
VGC_DEFINE_FLAGS(VacNodeDiffFlags, VacNodeDiffFlag)
// clang-format on

class VGC_TOPOLOGY_API VacDiff {
public:
    VacDiff() = default;

    void reset() {
        removedNodes_.clear();
        nodeDiffs_.clear();
    }

    bool isEmpty() const {
        return removedNodes_.empty() && nodeDiffs_.empty();
    }

    const std::unordered_map<VacNode*, VacNodeDiffFlags>& nodeDiffs() const {
        return nodeDiffs_;
    }

    const core::Array<core::Id>& removedNodes() const {
        return removedNodes_;
    }

    // XXX ops helpers

    void onNodeRemoved(VacNode* node) {
        nodeDiffs_.erase(node);
        removedNodes_.append(node->id());
        // XXX can it happen that we re-add a node with a same id ?
    }

    void onNodeDiff(VacNode* node, VacNodeDiffFlags diffFlags) {
        // XXX can it happen that we re-add a node with a same id as a removed node ?
        nodeDiffs_[node].set(diffFlags);
    }

private:
    std::unordered_map<VacNode*, VacNodeDiffFlags> nodeDiffs_;
    core::Array<core::Id> removedNodes_;
};

//class VGC_TOPOLOGY_API Tree {
//public:
//private:
//};

/// \class vgc::dom::Vac
/// \brief Represents an VAC.
///
class VGC_TOPOLOGY_API Vac : public core::Object {
private:
    VGC_OBJECT(Vac, core::Object)

    friend detail::Operations;

protected:
    Vac()
        : root_(this) {
    }

    void onDestroyed() override;

public:
    static VacPtr create();

    void clear();

    VacGroup* rootGroup() const {
        return const_cast<VacGroup*>(&root_);
    }

    VacNode* find(core::Id id) const {
        auto it = nodes_.find(id);
        return it != nodes_.end() ? it->second.get() : nullptr;
    }

    VacCell* findCell(core::Id id) const {
        VacNode* n = find(id);
        return (n && n->isCell()) ? static_cast<VacCell*>(n) : nullptr;
    }

    VacGroup* findGroup(core::Id id) const {
        VacNode* n = find(id);
        return (n && n->isGroup()) ? static_cast<VacGroup*>(n) : nullptr;
    }

    bool containsNode(core::Id id) const {
        return nodes_.find(id) != nodes_.end();
    }

    // An increasing version seems enough, we don't need it to match document version.

    Int64 version() const {
        return version_;
    }

    void incrementVersion() {
        ++version_;
    }

    bool emitPendingDiff();

    VGC_SIGNAL(changed, (const VacDiff&, diff))

protected:
    // insert/rem ops ?

    // graphics...

    // diff

private:
    Int64 version_ = 0;
    std::unordered_map<core::Id, std::unique_ptr<VacNode>> nodes_;
    VacGroup root_;
    VacDiff diff_ = {};
    bool diffEnabled_ = false;
};

} // namespace vgc::topology

#endif // VGC_TOPOLOGY_VAC_H
