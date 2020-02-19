// Copyright 2020 The VGC Developers
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

/// \file vgc/core/zero.h
/// \brief Defines "zero" for built-in types, specializable for custom types.

#ifndef VGC_CORE_ZERO_H
#define VGC_CORE_ZERO_H

#include <type_traits>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// Sets the given character, integer or floating-point number to zero.
///
/// This function is called by vgc::core::zero<T>(), and can be overloaded for
/// custom types.
///
/// \sa zero<T>()
///
template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value>::type
setZero(T& x)
{
    x = T();
}

/// Returns a zero-initialized value for the given type.
///
/// ```cpp
/// int x = vgc::core::zero();            // 0
/// double y = vgc::core::zero<double>(); // 0.0
/// Vec2d v = vgc::core::zero<Vec2d>();   // (0.0, 0.0)
/// ```
///
/// This function relies on ADL (= argument-dependent lookup), calling the
/// unqualified setZero(x) function under the hood. Therefore, you can
/// specialize zero() by overloading the setZero(x) function in the same
/// namespace of your custom type:
///
/// ```cpp
/// namespace foo {
///
/// struct A {
///     int m;
///     A() {}
/// };
///
/// void setZero(A& a)
/// {
///     a.m = 0;
/// }
///
/// } // namespace foo
///
/// A a1;                        // a1.m == uninitialized
/// A a2 = A();                  // a2.m == uninitialized
/// A a3 = vgc::core::zero<A>(); // a3.m == 0
/// ```
///
/// This function is primarily intended to be used in template functions. If
/// you know the type, prefer to use more readable ways to zero-initialize:
///
/// ```cpp
/// double x = 0.0;
/// Vec2d v(0.0, 0.0);
/// ```
///
/// \sa setZero(T& x)
///
/// More info on default/value/zero-initialization:
/// - https://en.cppreference.com/w/cpp/language/default_initialization
/// - https://en.cppreference.com/w/cpp/language/value_initialization
/// - https://stackoverflow.com/questions/29765961/default-value-and-zero-initialization-mess
/// - vgc/core/tests/test_zero.cpp
///
template<typename T>
T zero()
{
    T res;
    setZero(res);
    return res;
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_ZERO_H
