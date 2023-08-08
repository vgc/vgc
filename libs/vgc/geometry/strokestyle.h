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

#ifndef VGC_GEOMETRY_STROKESTYLE_H
#define VGC_GEOMETRY_STROKESTYLE_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \enum vgc::geometry::StrokeCap
/// \brief Specifies the style of stroke caps
///
enum class StrokeCap : UInt8 {

    /// The stroke is terminated by a straight line passing through the curve
    /// endpoint.
    ///
    Butt,

    /// The stroke is terminated by a smooth "round" shape. Typically, the
    /// shape is a circle, but it can be a more general shape (such as a cubic
    /// Bézier) for curves with variable width, otherwise the cap wouldn't be
    /// smooth.
    ///
    Round,

    /// The stroke is terminated by straight line, similar to `Butt` but
    /// extending the length of the curve by half its width.
    ///
    Square
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(StrokeCap)

/// \class vgc::geometry::StrokeStyle
/// \brief Specifies style parameters to use when stroking a curve
///
class VGC_GEOMETRY_API StrokeStyle {
public:
    StrokeStyle() {
    }

    StrokeStyle(StrokeCap cap)
        : cap_(cap) {
    }

    StrokeCap cap() const {
        return cap_;
    }

    void setCap(StrokeCap cap) {
        cap_ = cap;
    }

private:
    StrokeCap cap_ = StrokeCap::Butt;
};

} // namespace vgc::geometry

#endif
