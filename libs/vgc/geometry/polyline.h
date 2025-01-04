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

#ifndef VGC_GEOMETRY_POLYLINE_H
#define VGC_GEOMETRY_POLYLINE_H

#include <vgc/core/array.h>
#include <vgc/core/ranges.h>
#include <vgc/core/templateutil.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/traits.h>

namespace vgc::geometry {

/// \class vgc::geometry::Polyline
/// \brief Stores a sequence of points representing a polyline.
///
/// The `Polyline<Point>` class extends `core::Array<Point>` with helper
/// methods that are useful when the list of points represent a polyline.
///
/// Note that since its base class does not have a virtual destructor, it is
/// undefined behavior to destroy this class via a pointer to its base class.
///
template<typename Point>
class Polyline : public core::Array<Point> {
public:
    using ScalarType = scalarType<Point>;
    static constexpr Int dimension = geometry::dimension<Point>;
    using BaseType = core::Array<Point>;

    // Forward all constructors
    using BaseType::BaseType;
};

namespace polyline {

template<typename R, typename Proj = c20::identity>
using pointType =
    c20::remove_cvref_t<decltype(std::declval<Proj>()(*std::declval<R>().begin()))>;

template<typename R, typename Proj = c20::identity>
using scalarType = typename geometry::scalarType<pointType<R, Proj>>;

// Keep in mind when implementing function with generic ranges that:
// - It's UB to call begin() more than once on input ranges
// - Input iterators may be move-only

// TODO: struct PolylineType {bool isClosed = false, bool hasDuplicateEndpoints}?

// Returns the length of the polyline, that is, the sum of distances between consecutive samples.
//
template<
    typename R,
    typename Proj = c20::identity,
    VGC_REQUIRES(c20::ranges::input_range<R>)>
scalarType<R, Proj> length(
    R&& range,
    Proj proj = {},
    bool isClosed = false,
    bool hasDuplicateEndpoints = false) {

    auto it = range.begin();
    auto end = range.end();
    if (it == end) {
        return 0;
    }
    auto firstPosition = proj(*it);
    auto p = firstPosition;
    ++it;
    scalarType<R, Proj> res = 0;
    for (; it != end; ++it) {
        auto q = proj(*it);
        res += (q - p).length();
        p = q;
    }
    if (isClosed && !hasDuplicateEndpoints) {
        auto q = proj(*it);
        res += (firstPosition - p).length();
    }
    return res;
}

} // namespace polyline

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_POLYLINE_H
