// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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

/// \file vgc/core/algorithm.h
/// \brief General-purpose algorithms functions missing in <algorithm>

#ifndef VGC_CORE_ALGORITHM_H
#define VGC_CORE_ALGORITHM_H

#include <algorithm>
#include <cassert>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// Clamps the given value \p v between \p min and \p max.
/// The behavior is undefined if \p min is greater than \p max.
/// This is the same as C++17 std::clamp.
///
template<typename T>
const T& clamp(const T& v, const T& min, const T& max)
{
    assert(!(max < min));
    return (v < min) ? min : (max < v) ? max : v;
}

/// Returns what "zero" means for the given type. When this generic function is
/// not specialized, it uses the default constructor.
///
/// It is important to specialize vgc::core::zero for your own arithmetic types
/// when their default constructors do not perform zero-initialization (See
/// vgc::geometry::Vec2d for an example).
///
/// Below is an example of how to specialize vgc::core::zero for your own types:
///
/// \code
/// namespace MyNamespace {
/// class MyClass {
///     double x_;
/// public:
///     MyClass() {} // default constructor leaves x_ uninitialized
///     MyClass(double x) : x_(x) {} // custom constructor initializes x_
/// };
/// }
///
/// namespace vgc {
/// namespace core {
/// constexpr MyNamespace::MyClass zero<MyNamespace::MyClass>() {
///     return MyNamespace::MyClass(0.0);
/// }
/// }
/// }
///
/// void f() {
///     MyNamespace::MyClass c1;                                           // uninitialized
///     MyNamespace::MyClass c2 = vgc::core::zero<MyNamespace::MyClass>(); // zero-initialized
/// }
/// \endcode
///
/// This function is intended to be used for generic code. If you know the type,
/// prefer to use more readable ways to zero-initialize:
///
/// \code
/// double x = 0.0;
/// Vec2d v(0.0, 0.0);
/// \endcode
///
template<typename T>
constexpr T zero()
{
    return T();
}

/// Returns the sum of all values in the given vector.
/// Returns zero<T>() if the vector is empty.
///
template<typename T>
T sum(const std::vector<T>& v)
{
    return std::accumulate(v.begin(), v.end(), zero<T>());
}

/// Returns the average value of the given vector of values.
/// Returns zero<T>() if the vector is empty.
///
template<typename T>
T average(const std::vector<T>& v)
{
    const size_t n = v.size();

    if (n > 0) {
        return (1.0 / static_cast<double>(n)) * sum(v);
    }
    else {
        return zero<T>();
    }
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_ALGORITHM_H
