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

#ifndef VGC_GEOMETRY_FILLSTYLE_H
#define VGC_GEOMETRY_FILLSTYLE_H

#include <vgc/geometry/api.h>
#include <vgc/geometry/windingrule.h>

namespace vgc::geometry {

/// \class vgc::geometry::FillStyle
/// \brief Specifies style parameters to use when filling a curve
///
// For now, the only parameter is winding rule, but in the future, we can
// imagine having additional parameters, for example 'allowOverlapping' which
// would tell the tesselator to create overlapping triangles when the winding
// number of a subarea is more than 1.
//
//          ---
//         | 1 |
//    -------------------------
//   |  1  | 2 | 1             |   <- Have overlapping triangles in the
//    ---------------------    |      subarea whose winding number is 2
//         | 1 |           |   |
//         |   |           |   |
//         |   |     0     |   |
//         |   |           |   |
//         |   |           |   |
//         |    -----------    |
//         |                   |
//          -------------------
//
class VGC_GEOMETRY_API FillStyle {
public:
    /// Creates a default `FillStyle`.
    ///
    constexpr FillStyle() noexcept {
    }

    /// Creates a `FillStyle` with the given winding rule.
    ///
    constexpr explicit FillStyle(WindingRule windingRule) noexcept
        : windingRule_(windingRule) {
    }

    /// Returns the winding rule of the fill.
    ///
    /// The default is `WindingRule::NonZero`.
    ///
    /// \sa `setWindingRule()`.
    ///
    constexpr WindingRule windingRule() const {
        return windingRule_;
    }

    /// Sets the winding rule of the fill.
    ///
    /// \sa `windingRule()`.
    ///
    constexpr void setWindingRule(WindingRule windingRule) {
        windingRule_ = windingRule;
    }

private:
    WindingRule windingRule_ = WindingRule::NonZero;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_FILLSTYLE_H
