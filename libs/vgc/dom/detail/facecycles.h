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

namespace vgc::dom::detail {

/*
virtual bool read(StreamReader& in) = 0;
virtual bool write(StreamWriter& out) const = 0;

virtual FormatterBufferIterator
format(FormatterBufferCtx& ctx, std::string_view fmtString) const = 0;

*/

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

    void write(StreamWriter& out) const;
    void read(StreamReader& in);

    friend void write(StreamWriter& out, const DomCycleComponent& component) {
        component.write(out);
    }

    friend void readTo(DomCycleComponent& component, StreamReader& in) {
        component.read(in);
    }

    void preparePathsForUpdate(const Element* owner) const;
    void updatePaths(const Element* owner, const PathUpdateData& data);

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

private:
    Path path_ = {};
    bool direction_ = true;
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

    void write(StreamWriter& out) const;
    void read(StreamReader& in);

    friend void write(StreamWriter& out, const DomCycle& cycle) {
        cycle.write(out);
    }

    friend void readTo(DomCycle& cycle, StreamReader& in) {
        cycle.read(in);
    }

    void preparePathsForUpdate(const Element* owner) const;
    void updatePaths(const Element* owner, const PathUpdateData& data);

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

private:
    core::Array<DomCycleComponent> components_;
};

// TODO: CustomValueArray<TCustomValue>

class VGC_DOM_API DomFaceCycles final : public CustomValue {
public:
    DomFaceCycles() noexcept
        : CustomValue(true) {
    }

    explicit DomFaceCycles(core::Array<DomCycle>&& cycles)
        : CustomValue(true)
        , cycles_(std::move(cycles)) {
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

protected:
    void preparePathsForUpdate_(const Element* owner) const override;
    void updatePaths_(const Element* owner, const PathUpdateData& data) override;

    std::unique_ptr<CustomValue> clone_(bool move) const override;

    bool compareEqual_(const CustomValue* rhs) const override;
    bool compareLess_(const CustomValue* rhs) const override;

    void read_(StreamReader& in) override;
    void write_(StreamWriter& out) const override;

    FormatterBufferIterator format_(FormatterBufferCtx& ctx) const override;

private:
    core::Array<DomCycle> cycles_;

    // TODO: keep original string from parse
};

} // namespace vgc::dom::detail

template<>
struct fmt::formatter<vgc::dom::detail::DomCycle> : fmt::formatter<double> {
    auto format(const vgc::dom::detail::DomCycle& v, fmt::buffer_context<char>& ctx)
        -> decltype(ctx.out()) {
        std::string s;
        vgc::core::StringWriter sr(s);
        v.write(sr);
        return vgc::core::formatTo(ctx.out(), "{}", s);
    }
};

#endif // VGC_DOM_DETAIL_FACECYCLE_H
