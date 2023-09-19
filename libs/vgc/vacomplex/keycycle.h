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

#ifndef VGC_VACOMPLEX_KEYCYCLE_H
#define VGC_VACOMPLEX_KEYCYCLE_H

#include <initializer_list>

#include <vgc/core/span.h>
#include <vgc/geometry/windingrule.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/keyhalfedge.h>

namespace vgc::vacomplex {

class KeyCycle;

class VGC_VACOMPLEX_API KeyPath {
public:
    KeyPath() noexcept = default;

    explicit KeyPath(core::Span<const KeyHalfedge> halfedges) noexcept;

    explicit KeyPath(std::initializer_list<KeyHalfedge> halfedges)
        : halfedges_(halfedges) {
    }

    explicit KeyPath(core::Array<KeyHalfedge>&& halfedges) noexcept
        : halfedges_(std::move(halfedges)) {
    }

    explicit KeyPath(KeyVertex* singleVertex) noexcept
        : singleVertex_(singleVertex) {
    }

    KeyVertex* singleVertex() const {
        return singleVertex_;
    }

    const core::Array<KeyHalfedge>& halfedges() const {
        return halfedges_;
    }

    void reverse();

    void append(const KeyHalfedge& khe) {
        halfedges_.append(khe);
        singleVertex_ = nullptr;
    }

    void extend(const KeyPath& other) {
        halfedges_.extend(other.halfedges_);
        if (halfedges_.length()) {
            singleVertex_ = nullptr;
        }
    }

    void extendReversed(const KeyPath& other) {
        halfedges_.reserve(halfedges_.length() + other.halfedges_.length());
        for (auto it = other.halfedges_.rbegin(); it != other.halfedges_.rend(); ++it) {
            halfedges_.append(it->opposite());
        }
        if (halfedges_.length()) {
            singleVertex_ = nullptr;
        }
    }

private:
    friend detail::Operations;
    friend KeyCycle;

    KeyVertex* singleVertex_ = nullptr;
    core::Array<KeyHalfedge> halfedges_;
};

class VGC_VACOMPLEX_API KeyCycle {
public:
    KeyCycle() noexcept = default;

private:
    explicit KeyCycle(const KeyPath& path);
    explicit KeyCycle(KeyPath&& path);

public:
    explicit KeyCycle(core::Span<const KeyHalfedge> halfedges) noexcept;

    explicit KeyCycle(std::initializer_list<KeyHalfedge> halfedges)
        : halfedges_(halfedges) {
    }

    explicit KeyCycle(core::Array<KeyHalfedge>&& halfedges) noexcept
        : halfedges_(std::move(halfedges)) {
    }

    explicit KeyCycle(KeyVertex* steinerVertex) noexcept
        : steinerVertex_(steinerVertex) {
    }

    KeyVertex* steinerVertex() const {
        return steinerVertex_;
    }

    const core::Array<KeyHalfedge>& halfedges() const {
        return halfedges_;
    }

    void reverse();

    KeyCycle reversed() const;

    core::Array<geometry::Vec2d> sampleUniformly(Int numSamples) const;

    Int computeWindingNumberAt(const geometry::Vec2d& point) const;

    bool interiorContains(
        const geometry::Vec2d& position,
        geometry::WindingRule windingRule) const {
        Int windingNumber = computeWindingNumberAt(position);
        return geometry::isWindingNumberSatisfyingRule(windingNumber, windingRule);
    }

    double interiorContainedRatio(
        const KeyCycle& other,
        geometry::WindingRule windingRule,
        Int numSamples);

    bool isValid() const {
        if (steinerVertex_) {
            return halfedges_.isEmpty();
        }
        else if (!halfedges_.isEmpty()) {
            KeyVertex* kv = halfedges_.last().endVertex();
            for (const KeyHalfedge& khe : halfedges_) {
                if (kv != khe.startVertex()) {
                    return false;
                }
                kv = khe.endVertex();
            }
            return true;
        }
        return false;
    }

    void debugPrint(core::StringWriter& out) const;

private:
    friend detail::Operations;
    friend class KeyFace;

    KeyVertex* steinerVertex_ = nullptr;
    core::Array<KeyHalfedge> halfedges_;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYCYCLE_H
