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

#include <vgc/topology/vac.h>

#include <vgc/topology/detail/operationsimpl.h>

namespace vgc::topology {

void Vac::onDestroyed() {
}

VacPtr Vac::create() {
    return VacPtr(new Vac());
}

void Vac::clear() {
    for (const auto& it : nodes_) {
        onNodeAboutToBeRemoved().emit(it.second.get());
    }
    nodes_.clear();
    root_ = nullptr;
    ++version_;
}

VacGroup* Vac::resetRoot(core::Id id) {
    clear();
    root_ = detail::Operations::createRootGroup(this, id);
    return root_;
}

VacNode* Vac::find(core::Id id) const {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? it->second.get() : nullptr;
}

VacCell* Vac::findCell(core::Id id) const {
    VacNode* n = find(id);
    return (n && n->isCell()) ? static_cast<VacCell*>(n) : nullptr;
}

VacGroup* Vac::findGroup(core::Id id) const {
    VacNode* n = find(id);
    return (n && n->isGroup()) ? static_cast<VacGroup*>(n) : nullptr;
}

bool Vac::containsNode(core::Id id) const {
    return find(id) != nullptr;
}

bool Vac::emitPendingDiff() {
    if (!diff_.isEmpty()) {
        changed().emit(diff_);
        diff_.reset();
        return true;
    }
    return false;
}

} // namespace vgc::topology
