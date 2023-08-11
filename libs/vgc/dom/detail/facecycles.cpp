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

namespace vgc::dom::detail {

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

void DomCycleComponent::preparePathsForUpdate(const Element* owner) const {
    detail::preparePathForUpdate(path_, owner);
}

void DomCycleComponent::updatePaths(const Element* owner, const PathUpdateData& data) {
    detail::updatePath(path_, owner, data);
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

void DomCycle::preparePathsForUpdate(const Element* owner) const {
    for (const DomCycleComponent& component : components_) {
        component.preparePathsForUpdate(owner);
    }
}

void DomCycle::updatePaths(const Element* owner, const PathUpdateData& data) {
    for (DomCycleComponent& component : components_) {
        component.updatePaths(owner, data);
    }
}

void DomFaceCycles::preparePathsForUpdate_(const Element* owner) const {
    for (const DomCycle& cycle : cycles_) {
        cycle.preparePathsForUpdate(owner);
    }
}

void DomFaceCycles::updatePaths_(const Element* owner, const PathUpdateData& data) {
    for (DomCycle& cycle : cycles_) {
        cycle.updatePaths(owner, data);
    }
}

std::unique_ptr<CustomValue> DomFaceCycles::clone_(bool move) const {
    auto result = std::make_unique<DomFaceCycles>();
    if (move) {
        result->cycles_ = std::move(cycles_);
    }
    else {
        result->cycles_ = cycles_;
    }
    return result;
}

bool DomFaceCycles::compareEqual_(const CustomValue* rhs) const {
    auto other = dynamic_cast<const DomFaceCycles*>(rhs);
    if (other) {
        return std::equal(
            cycles_.begin(), cycles_.end(), other->cycles_.begin(), other->cycles_.end());
    }
    return false;
}

bool DomFaceCycles::compareLess_(const CustomValue* rhs) const {
    auto other = dynamic_cast<const DomFaceCycles*>(rhs);
    if (other) {
        return std::lexicographical_compare(
            cycles_.begin(), cycles_.end(), other->cycles_.begin(), other->cycles_.end());
    }
    return false;
}

void DomFaceCycles::read_(StreamReader& in) {
    readTo(cycles_, in);
}

void DomFaceCycles::write_(StreamWriter& out) const {
    using core::write;
    write(out, cycles_);
}

FormatterBufferIterator DomFaceCycles::format_(FormatterBufferCtx& ctx) const {
    return core::formatTo(ctx.out(), "{}", cycles_);
}

} // namespace vgc::dom::detail
