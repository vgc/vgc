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

#ifndef VGC_DOM_DETAIL_FACECYCLE_H
#define VGC_DOM_DETAIL_FACECYCLE_H

#include <vgc/core/format.h>
#include <vgc/dom/document.h>
#include <vgc/dom/path.h>
#include <vgc/dom/value.h>

namespace vgc::dom {

namespace detail {

class VGC_DOM_API DomCycleComponent {
public:
    DomCycleComponent() noexcept = default;

    DomCycleComponent(Path path, bool direction)
        : path_(std::move(path))
        , direction_(direction) {
    }

    const Path& path() const {
        return path_;
    }

    bool direction() const {
        return direction_;
    }

    friend bool
    operator==(const DomCycleComponent& lhs, const DomCycleComponent& rhs) noexcept {
        return lhs.direction_ == rhs.direction_ && lhs.path_ == rhs.path_;
    }

    friend bool
    operator!=(const DomCycleComponent& lhs, const DomCycleComponent& rhs) noexcept {
        return !(lhs == rhs);
    }

    friend bool
    operator<(const DomCycleComponent& lhs, const DomCycleComponent& rhs) noexcept {
        if (lhs.direction_ != rhs.direction_) {
            return lhs.direction_ == 0;
        }
        else {
            return lhs.path_ < rhs.path_;
        }
    }

    template<typename OStream>
    friend void write(OStream& out, const DomCycleComponent& self) {
        using core::write;
        write(out, self.path_);
        if (!self.direction_) {
            write(out, '*');
        }
    }

    template<typename IStream>
    friend void readTo(DomCycleComponent& self, IStream& in) {
        using core::readTo;
        readTo(self.path_, in);
        char c = -1;
        if (in.get(c)) {
            if (c == '*') {
                self.direction_ = false;
            }
            else {
                self.direction_ = true;
                in.unget();
            }
        }
        else {
            self.direction_ = true;
        }
    }

private:
    Path path_ = {};
    bool direction_ = true;

    friend PathVisitor<DomCycleComponent>;
};

class VGC_DOM_API DomCycle {
public:
    DomCycle() noexcept = default;

    explicit DomCycle(core::Array<DomCycleComponent> components)
        : components_(std::move(components)) {
    }

    const core::Array<DomCycleComponent>& components() const {
        return components_;
    }

    core::Array<DomCycleComponent>::const_iterator begin() const {
        return components_.begin();
    }

    core::Array<DomCycleComponent>::const_iterator end() const {
        return components_.end();
    }

    core::Array<DomCycleComponent>::iterator begin() {
        return components_.begin();
    }

    core::Array<DomCycleComponent>::iterator end() {
        return components_.end();
    }

    friend bool operator==(const DomCycle& lhs, const DomCycle& rhs) noexcept {
        return std::equal(
            lhs.components_.begin(),
            lhs.components_.end(),
            rhs.components_.begin(),
            rhs.components_.end());
    }

    friend bool operator!=(const DomCycle& lhs, const DomCycle& rhs) noexcept {
        return !(lhs == rhs);
    }

    friend bool operator<(const DomCycle& lhs, const DomCycle& rhs) noexcept {
        return std::lexicographical_compare(
            lhs.components_.begin(),
            lhs.components_.end(),
            rhs.components_.begin(),
            rhs.components_.end());
    }

    template<typename OStream>
    friend void write(OStream& out, const DomCycle& self) {
        using core::write;
        bool first = true;
        for (const DomCycleComponent& component : self.components_) {
            if (!first) {
                write(out, ' ');
            }
            else {
                first = false;
            }
            write(out, component);
        }
    }

    template<typename IStream>
    friend void readTo(DomCycle& self, IStream& in) {
        using core::readTo;
        self.components_.clear();
        char c = -1;
        bool got = false;
        readTo(self.components_.emplaceLast(), in);
        core::skipWhitespaceCharacters(in);
        got = bool(in.get(c));
        while (got && dom::isValidPathFirstChar(c)) {
            in.unget();
            readTo(self.components_.emplaceLast(), in);
            core::skipWhitespaceCharacters(in);
            got = bool(in.get(c));
        }
        if (got) {
            in.unget();
        }
    }

private:
    core::Array<DomCycleComponent> components_;

    friend PathVisitor<DomCycle>;
};

class VGC_DOM_API DomFaceCycles {
public:
    DomFaceCycles() noexcept {
    }

    explicit DomFaceCycles(core::Array<DomCycle>&& cycles)
        : cycles_(std::move(cycles)) {
    }

    const core::Array<DomCycle>& cycles() const {
        return cycles_;
    }

    core::Array<DomCycle>::const_iterator begin() const {
        return cycles_.begin();
    }

    core::Array<DomCycle>::const_iterator end() const {
        return cycles_.end();
    }

    core::Array<DomCycle>::iterator begin() {
        return cycles_.begin();
    }

    core::Array<DomCycle>::iterator end() {
        return cycles_.end();
    }

    bool operator==(const DomFaceCycles& other) const;

    bool operator<(const DomFaceCycles& other) const;

    template<typename OStream>
    friend void write(OStream& out, const DomFaceCycles& self) {
        using core::write;
        write(out, self.cycles_);
    }

    template<typename IStream>
    friend void readTo(DomFaceCycles& self, IStream& in) {
        readTo(self.cycles_, in);
    }

private:
    core::Array<DomCycle> cycles_;

    // TODO: keep original string from parse

    friend PathVisitor<DomFaceCycles>;
};

} // namespace detail

template<>
struct PathVisitor<detail::DomCycleComponent> {
    template<typename Component, typename Fn>
    static void visit(Component&& component, Fn&& fn) {
        fn(component.path_);
    }
};

template<>
struct PathVisitor<detail::DomCycle> {
    template<typename Cycle, typename Fn>
    static void visit(Cycle&& cycle, Fn&& fn) {
        for (auto& component : cycle.components_) { // auto needed to deduce constness
            PathVisitor<detail::DomCycleComponent>::visit(component, fn);
        }
    }
};

template<>
struct PathVisitor<detail::DomFaceCycles> {
    template<typename Cycles, typename Fn>
    static void visit(Cycles&& cycles, Fn&& fn) {
        for (auto& cycle : cycles.cycles_) { // auto needed to deduce constness
            PathVisitor<detail::DomCycle>::visit(cycle, fn);
        }
    }
};

} // namespace vgc::dom

template<>
struct fmt::formatter<vgc::dom::detail::DomCycleComponent> : fmt::formatter<double> {
    template<typename FormatContext>
    auto format(const vgc::dom::detail::DomCycleComponent& v, FormatContext& ctx) {
        std::string s;
        vgc::core::StringWriter sr(s);
        write(sr, v);
        return vgc::core::formatTo(ctx.out(), "{}", s);
    }
};

template<>
struct fmt::formatter<vgc::dom::detail::DomCycle> : fmt::formatter<double> {
    template<typename FormatContext>
    auto format(const vgc::dom::detail::DomCycle& v, FormatContext& ctx) {
        std::string s;
        vgc::core::StringWriter sr(s);
        write(sr, v);
        return vgc::core::formatTo(ctx.out(), "{}", s);
    }
};

template<>
struct fmt::formatter<vgc::dom::detail::DomFaceCycles> : fmt::formatter<double> {
    template<typename FormatContext>
    auto format(const vgc::dom::detail::DomFaceCycles& v, FormatContext& ctx) {
        std::string s;
        vgc::core::StringWriter sr(s);
        write(sr, v);
        return vgc::core::formatTo(ctx.out(), "{}", s);
    }
};

#endif // VGC_DOM_DETAIL_FACECYCLE_H
