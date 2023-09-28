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

#include <vgc/vacomplex/keycycle.h>
#include <vgc/vacomplex/keyvertex.h>

namespace vgc::vacomplex {

void KeyPath::reverse() {
    std::reverse(halfedges_.begin(), halfedges_.end());
    for (KeyHalfedge& kh : halfedges_) {
        kh.setDirection(!kh.direction());
    }
}

namespace {

geometry::Vec2dArray sampleCenterline_(const core::Array<KeyHalfedge>& halfedges) {
    if (halfedges.isEmpty()) {
        return {};
    }
    geometry::Vec2dArray result;
    Int sampleCount = 0;
    for (const KeyHalfedge& he : halfedges) {
        const geometry::StrokeSample2dArray& samples =
            he.edge()->strokeSampling().samples();
        sampleCount += samples.length();
    }
    result.reserve(sampleCount);
    KeyHalfedge he0 = halfedges.first();
    const geometry::StrokeSample2dArray& samples0 =
        he0.edge()->strokeSampling().samples();
    if (he0.direction()) {
        result.append(samples0.first().position());
    }
    else {
        result.append(samples0.last().position());
    }
    for (const KeyHalfedge& he : halfedges) {
        const geometry::StrokeSample2dArray& samples =
            he.edge()->strokeSampling().samples();
        if (he.direction()) {
            for (auto it = samples.begin(); it != samples.end(); ++it) {
                geometry::Vec2d p = it->position();
                if (result.last() != p) {
                    result.append(p);
                }
            }
        }
        else {
            for (auto it = samples.rbegin(); it != samples.rend(); ++it) {
                geometry::Vec2d p = it->position();
                if (result.last() != p) {
                    result.append(p);
                }
            }
        }
    }
    return result;
}

} // namespace

geometry::Vec2dArray KeyPath::sampleCenterline() const {
    return sampleCenterline_(halfedges_);
}

KeyCycle::KeyCycle(const KeyPath& path)
    : steinerVertex_(path.singleVertex_)
    , halfedges_(path.halfedges_) {
}

KeyCycle::KeyCycle(KeyPath&& path)
    : steinerVertex_(path.singleVertex_)
    , halfedges_(std::move(path.halfedges_)) {
}

KeyCycle::KeyCycle(core::Span<const KeyHalfedge> halfedges)
    : halfedges_(halfedges) {

    if (halfedges.isEmpty()) {
        // invalid cycle
    }
    else if (halfedges.first().isClosed()) {
        KeyHalfedge h0 = halfedges.first();
        for (const KeyHalfedge& h : halfedges) {
            if (h0 != h) {
                // invalid cycle
                halfedges_.clear();
                break;
            }
        }
    }
    else {
        // Note: there is no need to check that all halfedges
        // have the same key time since each consecutive pair
        // of halfedges share a vertex, and its time.
        auto it = halfedges.begin();
        KeyVertex* firstStartVertex = it->startVertex();
        KeyVertex* previousEndVertex = it->endVertex();
        for (++it; it != halfedges.end(); ++it) {
            if (previousEndVertex != it->startVertex()) {
                // invalid cycle
                halfedges_.clear();
                break;
            }
            previousEndVertex = it->endVertex();
        }
        if (previousEndVertex != firstStartVertex) {
            // invalid cycle
            halfedges_.clear();
        }
    }
}

void KeyCycle::reverse() {
    for (KeyHalfedge& khe : halfedges_) {
        khe.setOppositeDirection();
    }
}

KeyCycle KeyCycle::reversed() const {
    KeyCycle result = *this;
    result.reverse();
    return result;
}

geometry::Vec2dArray KeyCycle::sampleCenterline() const {
    geometry::Vec2dArray result = sampleCenterline_(halfedges_);
    if (result.length() > 1) {
        result.pop();
    }
    return result;
}

core::Array<geometry::Vec2d> KeyCycle::sampleUniformly(Int numSamples) const {

    core::Array<geometry::Vec2d> result;
    result.reserve(numSamples);

    // TODO: handle transforms.

    double totalS = 0;
    for (const KeyHalfedge& khe : halfedges()) {
        totalS += khe.edge()->strokeSampling().samples().last().s();
    }
    double stepS = totalS / numSamples;
    double nextStepS = stepS / 2;
    for (const KeyHalfedge& khe : halfedges()) {
        const geometry::StrokeSample2dArray& samples =
            khe.edge()->strokeSampling().samples();
        double heS = samples.last().s();
        if (heS == 0) {
            // XXX TODO: handle cases where heS == 0 more appropriately.
            // For example, ensure that this function still returns at least one sample.
            continue;
        }
        if (khe.direction()) {
            auto it = samples.begin();
            geometry::Vec2d previousP = it->position();
            double previousS = 0;
            ++it;
            for (auto end = samples.end(); it != end; ++it) {
                geometry::Vec2d p = it->position();
                double nextS = it->s();
                while (nextS >= nextStepS) {
                    double t = (nextStepS - previousS) / (nextS - previousS);
                    previousP = core::fastLerp(previousP, p, t);
                    previousS = nextStepS;
                    nextStepS += stepS;
                    result.emplaceLast(previousP);
                }
                previousP = p;
                previousS = nextS;
            }
        }
        else {
            auto rit = samples.rbegin();
            geometry::Vec2d previousP = rit->position();
            double previousS = heS - rit->s();
            ++rit;
            for (auto rend = samples.rend(); rit != rend; ++rit) {
                geometry::Vec2d p = rit->position();
                double nextS = heS - rit->s();
                while (nextS >= nextStepS) {
                    double t = (nextStepS - previousS) / (nextS - previousS);
                    previousP = core::fastLerp(previousP, p, t);
                    previousS = nextStepS;
                    nextStepS += stepS;
                    result.emplaceLast(previousP);
                }
                previousP = p;
                previousS = nextS;
            }
        }
        nextStepS -= heS;
    }

    return result;
}

Int KeyCycle::computeWindingNumberAt(const geometry::Vec2d& position) const {
    Int result = 0;
    for (const KeyHalfedge& keyHalfedge : halfedges()) {
        // TODO: handle transforms.
        Int contribution = keyHalfedge.computeWindingContributionAt(position);
        result += contribution;
    }
    return result;
}

double KeyCycle::interiorContainedRatio(
    core::ConstSpan<geometry::Vec2d> positions,
    geometry::WindingRule windingRule) const {

    Int count = 0;
    for (const auto& pos : positions) {
        if (interiorContains(pos, windingRule)) {
            ++count;
        }
    }
    return core::narrow_cast<double>(count)
           / core::narrow_cast<double>(positions.length());
}

double KeyCycle::interiorContainedRatio(
    const KeyCycle& other,
    geometry::WindingRule windingRule,
    Int numSamples) const {

    core::Array<geometry::Vec2d> samples = other.sampleUniformly(numSamples);
    return interiorContainedRatio(samples, windingRule);
}

// TODO: implement KeyCycleType to make the switch below more readable/convenient

void KeyCycle::debugPrint(core::StringWriter& out) const {
    if (steinerVertex()) {
        // Steiner cycle
        out << steinerVertex()->id();
    }
    else if (!halfedges().isEmpty()) {
        // Simple or Non-simple cycle
        bool isFirst = true;
        for (const KeyHalfedge& h : halfedges()) {
            if (isFirst) {
                isFirst = false;
            }
            else {
                out << ' ';
            }
            out << h.edge()->id();
            if (!h.direction()) {
                out << '*';
            }
        }
    }
    else {
        // invalid cycle
        out << "Invalid";
    }
}

} // namespace vgc::vacomplex
