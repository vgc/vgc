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

#ifndef VGC_GEOMETRY_TRAITS_H
#define VGC_GEOMETRY_TRAITS_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/templateutil.h>

namespace vgc::geometry {

/// Type trait for `scalarType<T>`.
///
template<typename T, typename SFINAE = void>
struct ScalarType {};

template<typename T>
struct ScalarType<T, core::RequiresValid<typename T::ScalarType>> {
    using type = typename T::ScalarType;
};

template<typename T>
struct ScalarType<T, core::Requires<std::is_floating_point_v<T>>> {
    using type = T;
};

template<typename T>
struct ScalarType<T, core::Requires<std::is_integral_v<T>>> {
    using type = T;
};

/// Returns the underlying scalar type of the given geometric type.
///
/// ```cpp
/// using T1 = scalarType<int>;       // `int`
///
/// using T1 = scalarType<float>;     // `float`
/// using T2 = scalarType<Vec3f>;     // `float`
/// using T3 = scalarType<Mat4f>;     // `float`
/// using T4 = scalarType<Segment2f>; // `float`
///
/// using T5 = scalarType<double>;    // `double`
/// using T6 = scalarType<Vec3d>;     // `double`
/// using T7 = scalarType<Mat4d>;     // `double`
/// using T8 = scalarType<Segment2d>; // `double`
/// ```
///
template<typename T>
using scalarType = typename ScalarType<T>::type;

/// Type trait for `dimension<T>`.
///
template<typename T, typename SFINAE = void>
struct Dimension {};

template<typename T>
struct Dimension<T, core::Requires<std::is_integral_v<decltype(T::dimension)>>> {
    static constexpr Int value = T::dimension;
};

template<typename T>
struct Dimension<T, core::Requires<std::is_floating_point_v<T>>> {
    static constexpr Int value = 1;
};

template<typename T>
struct Dimension<T, core::Requires<std::is_integral_v<T>>> {
    static constexpr Int value = 1;
};

/// Returns the underlying dimension of the given geometric type.
///
/// ```cpp
/// constexpr Int dim1 = dimension<int>;       // 1
/// constexpr Int dim2 = dimension<float>;     // 1
/// constexpr Int dim3 = dimension<double>;    // 1
///
/// constexpr Int dim4 = dimension<Vec2f>;     // 2
/// constexpr Int dim5 = dimension<Mat2d>;     // 2
/// constexpr Int dim6 = dimension<Segment2d>; // 2
///
/// constexpr Int dim4 = dimension<Vec3f>;     // 3
/// constexpr Int dim5 = dimension<Mat3d>;     // 3
/// ```
///
template<typename T>
inline constexpr Int dimension = Dimension<T>::value;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_TRAITS_H
