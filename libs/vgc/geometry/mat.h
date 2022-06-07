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

namespace internal {

template<int dimension, typename T>
struct Mat_ {};

} // namespace internal

/// Alias template for `Mat` classes.
///
/// ```
/// vgc::geometry::Mat<3, float>; // alias for vgc::geometry::Mat3f
/// ```
///
/// Note that `Mat` is not a class template, and `Mat3f` is not a class
/// template instanciation of `Mat`. It is the other way around: `Mat3f` is a
/// "regular" class (not defined from a template), and `Mat<3, float>` is
/// defined as an alias to `Mat3f`.
///
/// This design has the advantage to be slightly more flexible and also
/// generates shorter symbol names, which is useful for debugging (e.g.,
/// shorter error messages) and may potentially speed up dynamic linking.
///
template<int dimension, typename T>
using Mat = typename internal::Mat_<dimension, T>::type;

/// Type trait for `isMat<T>`.
///
template<typename T, typename SFINAE = void>
struct IsMat : std::false_type {};

template<typename T>
struct IsMat<T, core::Requires<
        std::is_same_v<T, Mat<T::dimension, typename T::ScalarType>>>> :
    std::true_type {};

/// Checks whether `T` is a `vgc::geometry::Mat` type.
///
/// \sa `IsMat<T>`
///
template<typename T>
inline constexpr bool isMat = IsMat<T>::value;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_MAT_H
