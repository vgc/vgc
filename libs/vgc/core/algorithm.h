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

/// \file vgc/core/algorithm.h
/// \brief General-purpose algorithms functions missing in <algorithm>

#ifndef VGC_CORE_ALGORITHM_H
#define VGC_CORE_ALGORITHM_H

#include <algorithm>
#include <cassert>
#include <numeric>
#include <string>
#include <vector>
#include <vgc/core/api.h>
#include <vgc/core/logging.h>

namespace vgc {
namespace core {

/// Returns the given value clamped to the interval [min, max]. If max < min,
/// then a warning is issued and the value is clamped to [max, min] instead.
///
template<typename T>
const T& clamp(const T& value, const T& min, const T& max)
{
    if (max < min) {
        warning()
            << "Warning: vgc::core::clamp("
            << "value=" << value << ", min=" << min << ", max=" << max << ") "
            << "called with max < min\n";
        return (value < max) ? max : (min < value) ? min : value;
    }
    else {
        return (value < min) ? min : (max < value) ? max : value;
    }
}

/// Returns what "zero" means for the given type. When this generic function is
/// not specialized, it uses the default constructor.
///
/// It is important to specialize vgc::core::zero for your own arithmetic types
/// when their default constructors do not perform zero-initialization (See
/// vgc::core::Vec2d for an example).
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

/// Returns the vector index corresponding to the given vector iterator \p it,
/// or -1 if pos == v.end();
///
template<typename T, typename It>
int toIndex(const std::vector<T>& v, It pos)
{
    return pos == v.end() ? -1 : pos - v.begin();
}

/// Returns the vector iterator corresponding to the given index \p i. Returns
/// v.end() if i = -1.
///
template<typename T>
typename std::vector<T>::const_iterator
toIterator(const std::vector<T>& v, int i)
{
    return i == -1 ? v.end() : v.begin() + i;
}

/// Returns the index to the first element in the given vector \p v which is
/// equal to \p x, or -1 if there is no such element.
///
template<typename T>
int find(const std::vector<T>& v, const T& x)
{
    auto it = std::find(v.begin(), v.end(), x);
    return toIndex(v, it);
}

/// Returns whether the given vector \p v contains the given value \p x. Writes
/// out in \p i the index of the first occurence of \p x, or -1 if \p v does
/// not contain \p x.
///
template<typename T>
bool contains(const std::vector<T>& v, const T& x, int& i)
{
    i = find(v, x);
    return i != -1;
}

/// Returns whether the given vector \p v contains the given value \p x.
///
template<typename T>
bool contains(const std::vector<T>& v, const T& x)
{
    int i;
    return contains(v, x, i);
}

/// Removes from the given vector \p v the first element which is equal to \p
/// x, if any. Returns whether an element was removed.
///
template<typename T>
bool removeOne(std::vector<T>& v, const T& x)
{
    int i;
    if (contains(v, x, i)) {
        v.erase(toIterator(v, i));
        return true;
    }
    else {
        return false;
    }
}

/// Returns the index of the first element in the vector that is (strictly)
/// greater than value, or -1 if no such element is found.
///
/// The vector must be at least partially ordered, that is it must satisfy:
/// v[i] < v[j] => i < j.
///
/// This is a convenient wrapper around std::upper_bound.
///
/// Example:
/// \code
/// std::vector<int> data = {2, 4, 6, 6, 8};
/// int i1 = vgc::core::upper_bound(data, 1); // = 0
/// int i2 = vgc::core::upper_bound(data, 2); // = 1
/// int i3 = vgc::core::upper_bound(data, 3); // = 1
/// int i4 = vgc::core::upper_bound(data, 4); // = 2
/// int i5 = vgc::core::upper_bound(data, 5); // = 2
/// int i6 = vgc::core::upper_bound(data, 6); // = 4
/// int i7 = vgc::core::upper_bound(data, 7); // = 4
/// int i8 = vgc::core::upper_bound(data, 8); // = 5
/// int i9 = vgc::core::upper_bound(data, 9); // = 5
/// \endcode
///
template<typename T>
int upper_bound(const std::vector<T>& v, const T& x)
{
    auto it = std::upper_bound(v.begin(), v.end(), x);
    return toIndex(v, it);
}

/// Returns a copy of the string \p s where all occurences of
/// \p from are replaced by \p to.
///
VGC_CORE_API
std::string replace(const std::string& s,
                    const std::string& from,
                    const std::string& to);

} // namespace core
} // namespace vgc

#endif // VGC_CORE_ALGORITHM_H
