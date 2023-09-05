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

#include <vgc/vacomplex/keyedgedata.h>
#include <vgc/workspace/style.h>

namespace vgc::workspace {

CellStyle::OpResult CellStyle::onTranslateGeometry_(const geometry::Vec2d& /*delta*/) {
    // XXX: gradient ?
    return OpResult::Unchanged;
}

CellStyle::OpResult
CellStyle::onTransformGeometry_(const geometry::Mat3d& /*transformation*/) {
    // XXX: gradient ?
    return OpResult::Unchanged;
}

CellStyle::OpResult
CellStyle::onUpdateGeometry_(const geometry::AbstractStroke2d* /*newStroke*/) {
    // XXX: something to do ?
    return OpResult::Unchanged;
}

std::unique_ptr<vacomplex::CellProperty> CellStyle::fromConcatStep_(
    const vacomplex::KeyHalfedgeData& khd1,
    const vacomplex::KeyHalfedgeData& khd2) const {

    const CellStyle* s1 = nullptr;
    double l1 = 0;
    Int n1 = 0;
    if (khd1.edgeData()) {
        vacomplex::KeyEdgeData* data = khd1.edgeData();
        s1 = static_cast<const CellStyle*>(data->findProperty(strings::style));
        if (s1) {
            n1 = std::max<Int>(1, s1->concatArray_.length());
        }
        if (data->stroke()) {
            l1 = data->stroke()->approximateLength();
        }
    }
    const CellStyle* s2 = nullptr;
    double l2 = 0;
    Int n2 = 0;
    if (khd2.edgeData()) {
        vacomplex::KeyEdgeData* data = khd2.edgeData();
        s2 = static_cast<const CellStyle*>(data->findProperty(strings::style));
        if (s2) {
            n2 = std::max<Int>(1, s2->concatArray_.length());
        }
        if (data->stroke()) {
            l2 = data->stroke()->approximateLength();
        }
    }

    auto result = std::make_unique<CellStyle>();

    result->concatArray_.reserve(n1 + n2);
    if (s1) {
        if (!s1->concatArray_.isEmpty()) {
            result->concatArray_.extend(s1->concatArray_);
        }
        else {
            result->concatArray_.append(StyleConcatEntry{s1->style_, l1});
        }
    }
    else {
        result->concatArray_.append(StyleConcatEntry{std::nullopt, 0});
    }
    if (s2) {
        if (!s2->concatArray_.isEmpty()) {
            result->concatArray_.extend(s2->concatArray_);
        }
        else {
            result->concatArray_.append(StyleConcatEntry{s2->style_, l2});
        }
    }
    else {
        result->concatArray_.append(StyleConcatEntry{std::nullopt, 0});
    }

    return result;
}

CellStyle::OpResult CellStyle::finalizeConcat_() {
    if (!concatArray_.isEmpty()) {
        // for now, we take the longest edge style
        double maxWeight = 0;
        Int maxWeightIndex = -1;
        Int i = 0;
        for (const StyleConcatEntry& entry : concatArray_) {
            if (entry.sourceWeight > maxWeight) {
                maxWeight = entry.sourceWeight;
                maxWeightIndex = i;
            }
            ++i;
        }
        if (maxWeightIndex < 0 || !concatArray_[maxWeightIndex].style.has_value()) {
            style_ = Style(); // XXX: default style
        }
        else {
            style_ = concatArray_[maxWeightIndex].style.value();
        }
        concatArray_.clear();
        return OpResult::Success;
    }
    return OpResult::Unchanged;
}

std::unique_ptr<vacomplex::CellProperty> CellStyle::fromGlue_(
    core::ConstSpan<vacomplex::KeyHalfedgeData> khds,
    const geometry::AbstractStroke2d* /*gluedStroke*/) const {

    // use color used the most arclength-wise

    std::map<core::Color, double> arclengthByColor;

    Style defaultStyle = {}; // XXX: default style

    for (const vacomplex::KeyHalfedgeData& khd : khds) {
        vacomplex::KeyEdgeData* data = khd.edgeData();
        if (data && data->stroke()) {
            double arclength = data->stroke()->approximateLength();
            const CellStyle* cellStyle =
                static_cast<const CellStyle*>(data->findProperty(strings::style));
            core::Color c = (cellStyle ? cellStyle->style_ : defaultStyle).color;
            arclengthByColor.try_emplace(c, 0).first->second += arclength;
        }
    }

    auto result = std::make_unique<CellStyle>();
    if (!arclengthByColor.empty()) {
        auto it = std::max_element(
            arclengthByColor.begin(),
            arclengthByColor.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        result->style_.color = it->first;
    }
    else {
        result->style_ = defaultStyle;
    }

    return result;
}

} // namespace vgc::workspace
