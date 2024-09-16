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

// This file was automatically generated, please do not edit directly.
// Instead, edit tools/vec2x.h then run tools/generate.py.

#ifndef VGC_GEOMETRY_VEC_H
#define VGC_GEOMETRY_VEC_H

#include <vgc/core/templateutil.h>

namespace vgc::geometry {

template<typename T>
class Vec2;

template<typename T>
class Vec3;

template<typename T>
class Vec4;

namespace detail {

template<int dimension, typename T>
struct Vec_ {};

template<typename T>
struct Vec_<2, T> {
    using type = Vec2<T>;
};

template<typename T>
struct Vec_<3, T> {
    using type = Vec3<T>;
};

template<typename T>
struct Vec_<4, T> {
    using type = Vec4<T>;
};

} // namespace detail

/// Alias template for `Vec` classes.
///
/// ```
/// Vec<2, float>; // alias for Vec2f
/// ```
///
/// Note that `Vec` is not a class template that `Vec2`, `Vec3`, and `Vec4`
/// specialize. Instead, `Vec2`, `Vec3`, and `Vec4` are implemented as separate
/// class templates, and `Vec<dim, T>` is simply an alias to the appropriate
/// class `Vec2<T>`, `Vec3<T>`, or `Vec4<T>`.
///
template<int dimension, typename T>
using Vec = typename detail::Vec_<dimension, T>::type;

/// Type trait for `isVec<T>`.
///
template<typename T, typename SFINAE = void>
struct IsVec : std::false_type {};

template<typename T>
struct IsVec<
    T,
    core::Requires<std::is_same_v<T, Vec<T::dimension, typename T::ScalarType>>>>
    : std::true_type {};

/// Checks whether `T` is a `vgc::geometry::Vec` type.
///
/// \sa `IsVec<T>`
///
template<typename T>
inline constexpr bool isVec = IsVec<T>::value;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_VEC_H
