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

class Vec2f;
class Vec2d;
class Vec3f;
class Vec3d;
class Vec4f;
class Vec4d;

namespace internal {

template<int dimension, typename T> struct Vec_ {};
template<> struct Vec_<2, float>  { using type = Vec2f; };
template<> struct Vec_<2, double> { using type = Vec2d; };
template<> struct Vec_<3, float>  { using type = Vec3f; };
template<> struct Vec_<3, double> { using type = Vec3d; };
template<> struct Vec_<4, float>  { using type = Vec4f; };
template<> struct Vec_<4, double> { using type = Vec4d; };

} // namespace internal

/// Alias template for `Vec` classes.
///
/// ```
/// vgc::geometry::Vec<2, float>; // alias for vgc::geometry::Vec2f
/// ```
///
/// Note that `Vec` is not a class template, and `Vec2f` is not a class
/// template instanciation of `Vec`. It is the other way around: `Vec2f` is a
/// "regular" class (not defined from a template), and `Vec<2, float>` is
/// defined as an alias to `Vec2f`.
///
/// This design has the advantage to be slightly more flexible and also
/// generates shorter symbol names, which is useful for debugging (e.g.,
/// shorter error messages) and may potentially speed up dynamic linking.
///
template<int dimension, typename T>
using Vec = typename internal::Vec_<dimension, T>::type;

/// Type trait for `isVec<T>`.
///
template<typename T, typename SFINAE = void>
struct IsVec : std::false_type {};

template<typename T>
struct IsVec<T, core::Requires<
        std::is_same_v<T, Vec<T::dimension, typename T::ScalarType>>>> :
    std::true_type {};

/// Checks whether `T` is a `vgc::geometry::Vec` type.
///
/// \sa `IsVec<T>`
///
template<typename T>
inline constexpr bool isVec = IsVec<T>::value;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_VEC_H
