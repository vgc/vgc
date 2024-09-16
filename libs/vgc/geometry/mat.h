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

#ifndef VGC_GEOMETRY_MAT_H
#define VGC_GEOMETRY_MAT_H

#include <vgc/core/templateutil.h>

namespace vgc::geometry {

template<typename T>
class Mat2;

template<typename T>
class Mat3;

template<typename T>
class Mat4;

namespace detail {

template<int dimension, typename T>
struct Mat_ {};

template<typename T>
struct Mat_<2, T> {
    using type = Mat2<T>;
};

template<typename T>
struct Mat_<3, T> {
    using type = Mat3<T>;
};

template<typename T>
struct Mat_<4, T> {
    using type = Mat4<T>;
};

} // namespace detail

/// Alias template for `Mat` classes.
///
/// ```
/// Mat<2, float>; // alias for Mat2f
/// ```
///
/// Note that `Mat` is not a class template that `Mat2`, `Mat3`, and `Mat4`
/// specialize. Instead, `Mat2`, `Mat3`, and `Mat4` are implemented as separate
/// class templates, and `Mat<dim, T>` is simply an alias to the appropriate
/// class `Mat2<T>`, `Mat3<T>`, or `Mat4<T>`.
///
template<int dimension, typename T>
using Mat = typename detail::Mat_<dimension, T>::type;

/// Type trait for `isMat<T>`.
///
template<typename T, typename SFINAE = void>
struct IsMat : std::false_type {};

template<typename T>
struct IsMat<
    T,
    core::Requires<std::is_same_v<T, Mat<T::dimension, typename T::ScalarType>>>>
    : std::true_type {};

/// Checks whether `T` is a `vgc::geometry::Mat` type.
///
/// \sa `IsMat<T>`
///
template<typename T>
inline constexpr bool isMat = IsMat<T>::value;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_MAT_H
