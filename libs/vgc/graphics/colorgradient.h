// Copyright 2022 The VGC Developers
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

#ifndef VGC_GRAPHICS_COLORGRADIENT_H
#define VGC_GRAPHICS_COLORGRADIENT_H

#include <vgc/core/arithmetic.h>
#include <vgc/geometry/vec4f.h>
#include <vgc/graphics/api.h>

namespace vgc::graphics {

struct Color4f {
    float r, g, b, a;
};

/// \class vgc::graphics::ColorGradientPoint
/// \brief Describes a color gradient point.
///
class VGC_GRAPHICS_API ColorGradientPoint {
public:
    Color4f color() const {
        return color_;
    }

    float position() const {
        return position_;
    }

private:
    Color4f color_;
    float position_;
};

/// \class vgc::graphics::ColorGradient
/// \brief Describes a color gradient.
///
class VGC_GRAPHICS_API ColorGradient {
public:
    size_t hash() const noexcept {
        return 0;
    }
};

} // namespace vgc::graphics

template<>
struct std::hash<vgc::graphics::ColorGradient>
{
    size_t operator()(const vgc::graphics::ColorGradient& x) const noexcept {
        return x.hash();
    }
};

namespace vgc::graphics {

class VGC_GRAPHICS_API ColorPaletteBatch {
    // XXX todo
};

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_COLORGRADIENT_H
