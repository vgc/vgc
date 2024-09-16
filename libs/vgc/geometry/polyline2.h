// Copyright 2024 The VGC Developers
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

#ifndef VGC_GEOMETRY_POLYLINE2_H
#define VGC_GEOMETRY_POLYLINE2_H

#include <vgc/geometry/polyline.h>
#include <vgc/geometry/vec2.h>

namespace vgc::geometry {

/// \class vgc::geometry::Polyline2
/// \brief Stores a sequence of 2D points representing a polyline.
///
/// The `Polyline2<Point>` class extends `Polyline<Point>` with methods
/// specific to 2D polylines.
///
/// Note that since its base class does not have a virtual destructor, it is
/// undefined behavior to destroy this class via a pointer to its base class.
///
template<typename Point>
class Polyline2 : public Polyline<Point> {
public:
    // Forward all constructors
    using Polyline<Point>::Polyline;
};

using Polyline2f = Polyline2<Vec2f>;
using Polyline2d = Polyline2<Vec2d>;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_POLYLINE2_H
