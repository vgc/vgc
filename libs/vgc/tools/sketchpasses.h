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
    Int doUpdateFrom(const SketchPointBuffer& input) override;
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
    Int doUpdateFrom(const SketchPointBuffer& input) override;

private:
    geometry::Mat3d transform_;
};

class VGC_TOOLS_API SmoothingPass : public SketchPass {
protected:
    Int doUpdateFrom(const SketchPointBuffer& input) override;
};

class VGC_TOOLS_API DouglasPeuckerPass : public SketchPass {
protected:
    Int doUpdateFrom(const SketchPointBuffer& input) override;
};

class VGC_TOOLS_API SingleLineSegmentWithFixedEndpointsPass : public SketchPass {
protected:
    Int doUpdateFrom(const SketchPointBuffer& input) override;
};

class VGC_TOOLS_API SingleLineSegmentWithFreeEndpointsPass : public SketchPass {
protected:
    Int doUpdateFrom(const SketchPointBuffer& input) override;
};

} // namespace vgc::tools

#endif // VGC_TOOLS_SKETCHPASSES_H
