// Copyright 2023 The VGC Developers
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

#include <vgc/dom/detail/facecycles.h>

#include <vgc/dom/value.h>

namespace vgc::dom {

namespace detail {

void DomCycleComponent::write(StreamWriter& out) const {
    using core::write;
    write(out, path_);
    if (!direction_) {
        write(out, '*');
    }
}

void DomCycleComponent::read(StreamReader& in) {
    using core::readTo;
    readTo(path_, in);
    char c = -1;
    if (in.get(c)) {
        if (c == '*') {
            direction_ = false;
        }
        else {
            direction_ = true;
            in.unget();
        }
    }
    else {
        direction_ = true;
    }
}

void DomCycle::write(StreamWriter& out) const {
    using core::write;
    bool first = true;
    for (const DomCycleComponent& component : components_) {
        if (!first) {
            write(out, ' ');
        }
        else {
            first = false;
        }
        write(out, component);
    }
}

void DomCycle::read(StreamReader& in) {
    using core::readTo;
    components_.clear();
    char c = -1;
    bool got = false;
    readTo(components_.emplaceLast(), in);
    core::skipWhitespaceCharacters(in);
    got = bool(in.get(c));
    while (got && dom::isValidPathFirstChar(c)) {
        in.unget();
        readTo(components_.emplaceLast(), in);
        core::skipWhitespaceCharacters(in);
        got = bool(in.get(c));
    }
    if (got) {
        in.unget();
    }
}

bool DomFaceCycles::operator==(const DomFaceCycles& other) const {
    return std::equal(
        cycles_.begin(), cycles_.end(), other.cycles_.begin(), other.cycles_.end());
}

bool DomFaceCycles::operator<(const DomFaceCycles& other) const {
    return std::lexicographical_compare(
        cycles_.begin(), cycles_.end(), other.cycles_.begin(), other.cycles_.end());
}

} // namespace detail

using detail::DomCycle;
using detail::DomCycleComponent;
using detail::DomFaceCycles;

void PathTraits<DomCycleComponent>::preparePathsForUpdate(
    const DomCycleComponent& self,
    const Node* owner) {

    PathTraits<Path>::preparePathsForUpdate(self.path_, owner);
}

void PathTraits<DomCycleComponent>::updatePaths(
    DomCycleComponent& self,
    const Node* owner,
    const PathUpdateData& data) {

    PathTraits<Path>::updatePaths(self.path_, owner, data);
}

void PathTraits<DomCycle>::preparePathsForUpdate(
    const DomCycle& self,
    const Node* owner) {

    for (const DomCycleComponent& component : self.components_) {
        PathTraits<DomCycleComponent>::preparePathsForUpdate(component, owner);
    }
}

void PathTraits<DomCycle>::updatePaths(
    DomCycle& self,
    const Node* owner,
    const PathUpdateData& data) {

    for (DomCycleComponent& component : self.components_) {
        PathTraits<DomCycleComponent>::updatePaths(component, owner, data);
    }
}

void PathTraits<DomFaceCycles>::preparePathsForUpdate(
    const DomFaceCycles& self,
    const Node* owner) {

    for (const DomCycle& cycle : self.cycles_) {
        PathTraits<DomCycle>::preparePathsForUpdate(cycle, owner);
    }
}

void PathTraits<DomFaceCycles>::updatePaths(
    DomFaceCycles& self,
    const Node* owner,
    const PathUpdateData& data) {

    for (DomCycle& cycle : self.cycles_) {
        PathTraits<DomCycle>::updatePaths(cycle, owner, data);
    }
}

} // namespace vgc::dom
