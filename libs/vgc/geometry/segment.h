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

#ifndef VGC_GEOMETRY_SEGMENT_H
#define VGC_GEOMETRY_SEGMENT_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/core/templateutil.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \enum vgc::geometry::SegmentIntersectionType
/// \brief The nature of an intersection between two segments.
///
enum class SegmentIntersectionType : UInt8 {
    Empty,
    Point,
    Segment
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(SegmentIntersectionType)

template<typename T>
class Segment2;

namespace detail {

template<int dimension, typename T>
struct Segment_ {};

template<typename T>
struct Segment_<2, T> {
    using type = Segment2<T>;
};

// clang-format on

} // namespace detail

/// Alias template for `Segment` classes.
///
/// ```
/// Segment<2, float>; // alias for Segment2f
/// ```
///
/// Note that `Segment` is not a class template. It simply provides an alias to
/// the appropriate class, such as `Segment2<float>`.
///
template<int dimension, typename T>
using Segment = typename detail::Segment_<dimension, T>::type;

/// Type trait for `isSegment<T>`.
///
template<typename T, typename SFINAE = void>
struct IsSegment : std::false_type {};

template<typename T>
struct IsSegment<
    T,
    core::Requires<std::is_same_v<T, Segment<T::dimension, typename T::ScalarType>>>>
    : std::true_type {};

/// Checks whether `T` is a `vgc::geometry::Segment` type.
///
/// \sa `IsSegment<T>`
///
template<typename T>
inline constexpr bool isSegment = IsSegment<T>::value;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_SEGMENT_H
