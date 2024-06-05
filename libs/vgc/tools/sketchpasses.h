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

#ifndef VGC_TOOLS_SKETCHPASSES_H
#define VGC_TOOLS_SKETCHPASSES_H

#include <vgc/geometry/mat3d.h>
#include <vgc/tools/api.h>
#include <vgc/tools/sketchpass.h>

namespace vgc::tools {

class VGC_TOOLS_API EmptyPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

class VGC_TOOLS_API TransformPass : public SketchPass {
public:
    const geometry::Mat3d& transformMatrix() const {
        return transform_;
    }

    void setTransformMatrix(const geometry::Mat3d& transform) {
        transform_ = transform;
    }

    geometry::Vec2d transform(const geometry::Vec2d& v) {
        return transform_.transform(v);
    }

    geometry::Vec2d transformAffine(const geometry::Vec2d& v) {
        return transform_.transformAffine(v);
    }

protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;

private:
    geometry::Mat3d transform_;
};

class VGC_TOOLS_API SmoothingPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

class VGC_TOOLS_API DouglasPeuckerPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

class VGC_TOOLS_API SingleLineSegmentWithFixedEndpointsPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

class VGC_TOOLS_API SingleLineSegmentWithFreeEndpointsPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

namespace detail {

// Buffer used to minimize dynamic allocations across multiple fits.
//
struct FitBuffer {
    geometry::Vec2dArray positions;
    core::DoubleArray params;
};

// Info about the mapping between input points and output points
// of one of the fit part of a recursive fit.
//
struct FitInfo {
    Int lastInputIndex;
    Int lastOutputIndex;
};

} // namespace detail

class VGC_TOOLS_API SingleQuadraticSegmentWithFixedEndpointsPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;

private:
    detail::FitBuffer buffer_;
};

class VGC_TOOLS_API QuadraticSplinePass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
    void doReset() override;

private:
    core::Array<detail::FitInfo> info_;
    detail::FitBuffer buffer_;
};

} // namespace vgc::tools

template<>
struct fmt::formatter<vgc::tools::detail::FitInfo> : fmt::formatter<vgc::Int> {
    template<typename FormatContext>
    auto format(const vgc::tools::detail::FitInfo& i, FormatContext& ctx) {
        return format_to(ctx.out(), "({}, {})", i.lastInputIndex, i.lastOutputIndex);
    }
};

#endif // VGC_TOOLS_SKETCHPASSES_H
