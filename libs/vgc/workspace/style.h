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

#ifndef VGC_WORKSPACE_STYLE_H
#define VGC_WORKSPACE_STYLE_H

#include <optional>

#include <vgc/core/color.h>
#include <vgc/core/stringid.h>
#include <vgc/vacomplex/cellproperty.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/strings.h>

namespace vgc::workspace {

class VGC_WORKSPACE_API CellStyle : public vacomplex::CellProperty {
public:
    CellStyle() noexcept
        : CellProperty(strings::style) {
    }

    const core::Color& color() const {
        const_cast<CellStyle*>(this)->finalizeConcat_();
        return style_.color;
    }

    void setColor(const core::Color& color) {
        concatArray_.clear();
        style_.color = color;
    }

protected:
    std::unique_ptr<vacomplex::CellProperty> clone_() const override {
        return std::make_unique<CellStyle>(*this);
    }

    OpResult onTranslateGeometry_(const geometry::Vec2d& delta) override;

    OpResult onTransformGeometry_(const geometry::Mat3d& transformation) override;

    OpResult onUpdateGeometry_(const geometry::AbstractStroke2d* newStroke) override;

    std::unique_ptr<vacomplex::CellProperty> fromConcatStep_(
        const vacomplex::KeyHalfedgeData& khd1,
        const vacomplex::KeyHalfedgeData& khd2) const override;

    std::unique_ptr<CellProperty> fromConcatStep_(
        const vacomplex::KeyFaceData& kfd1,
        const vacomplex::KeyFaceData& kfd2) const override;

    OpResult finalizeConcat_() override;

    std::unique_ptr<vacomplex::CellProperty> fromGlue_(
        core::ConstSpan<vacomplex::KeyHalfedgeData> khds,
        const geometry::AbstractStroke2d* gluedStroke) const override;

private:
    struct Style {
        core::Color color;
    };

    struct StyleConcatEntry {
        std::optional<Style> style;
        double sourceWeight;
    };

    core::Array<StyleConcatEntry> concatArray_;
    Style style_;
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_STYLE_H
