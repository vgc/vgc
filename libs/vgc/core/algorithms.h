// Copyright 2021 The VGC Developers
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

/// \file vgc/core/algorithms.h
/// \brief General-purpose algorithms functions missing in <algorithm>

#ifndef VGC_CORE_ALGORITHMS_H
#define VGC_CORE_ALGORITHMS_H

#include <algorithm> // find, sort, set_difference, upper_bound, ...
#include <cassert>
#include <iterator> // output_iterator_tag
#include <memory>   // addressof
#include <numeric>
#include <string>
#include <vector>

#include <vgc/core/api.h>
#include <vgc/core/logging.h>

// The following is not a dependency, but included for convenience: it defines
// vgc::core::clamp, which some people may expect in <vgc/core/algorithms.h>,
// since std::clamp is in <algorithm>
//
#include <vgc/core/arithmetic.h>

#include <vgc/core/detail/containerutil.h> // IsCompatibleRange

namespace vgc::core {

/// Returns the sum of all values in the given vector.
/// Returns zero<T>() if the vector is empty.
///
template<typename ContainerType>
typename ContainerType::value_type sum(const ContainerType& v) {
    return std::accumulate(
        v.begin(), v.end(), zero<typename ContainerType::value_type>());
}

/// Returns the average value of the given vector of values.
/// Returns zero<T>() if the vector is empty.
///
template<typename ContainerType>
typename ContainerType::value_type average(const ContainerType& v) {
    const size_t n = v.size();

    if (n > 0) {
        return (1.0 / static_cast<double>(n)) * sum(v);
    }
    else {
        return zero<typename ContainerType::value_type>();
    }
}

/// Sorts the given container `c`.
///
/// This is equivalent to `std::sort(c.begin(), c.end())`.
///
template<typename Container>
void sort(Container& c) {
    std::sort(c.begin(), c.end());
}

/// Sorts the given container `c`.
///
/// This is equivalent to `std::sort(c.begin(), c.end(), comp)`.
///
template<typename Container, typename Compare>
void sort(Container& c, Compare comp) {
    std::sort(c.begin(), c.end(), comp);
}

/// Pseudo-removes consecutive duplicates in the container `c`.
///
/// This is done by moving elements such that after calling this function, the
/// range `[c.begin(), res)` does not contain consecutive duplicates, and the
/// range `[res, c.end())` is undefined, where `res` is the iterator returned
/// by this function.
///
/// This is equivalent to `std::unique(c.begin(), c.end())`.
///
/// A call to this function should typically be followed by a call to
/// `c.erase(res, c.end())` method to actually remove the elements.
///
/// \sa `removeConsecutiveDuplicates()`.
///
template<typename Container>
[[nodiscard]] auto unique(Container& c) {
    return std::unique(c.begin(), c.end());
}

/// Same as `unique(c)` but using the given binary predicate instead of
/// `operator==`.
///
template<typename Container, typename BinaryPred>
[[nodiscard]] auto unique(Container& c, BinaryPred p) {
    return std::unique(c.begin(), c.end(), p);
}

/// Removes consecutive duplicates in the container `c`.
///
/// This is equivalent to:
///
/// ```
/// auto last = unique(c);
/// c.erase(last, c.cend());
/// ```
///
/// \sa `unique()`.
///
template<typename Container>
void removeConsecutiveDuplicates(Container& c) {
    auto last = unique(c);
    c.erase(last, c.cend());
}

/// Same as `removeConsecutiveDuplicates(c)` but using the given binary
/// predicate instead of `operator==`.
///
template<typename Container, typename BinaryPred>
void removeConsecutiveDuplicates(Container& c, BinaryPred p) {
    auto last = unique(c, p);
    c.erase(last, c.cend());
}

/// Returns the vector index corresponding to the given vector iterator `it`,
/// or `-1` if `pos == v.end()`.
///
template<typename T, typename It>
int toIndex(const std::vector<T>& v, It pos) {
    return pos == v.end() ? -1 : pos - v.begin();
}

/// Returns the vector iterator corresponding to the given index `i`. Returns
/// `v.end()` if `i == -1`.
///
template<typename T>
typename std::vector<T>::const_iterator toIterator(const std::vector<T>& v, int i) {
    return i == -1 ? v.end() : v.begin() + i;
}

/// Returns the index to the first element in the given vector `v` which is
/// equal to `x`, or `-1` if there is no such element.
///
template<typename T>
int find(const std::vector<T>& v, const T& x) {
    auto it = std::find(v.begin(), v.end(), x);
    return toIndex(v, it);
}

/// Returns whether the given vector `v` contains the given value `x`. Writes
/// out in `i` the index of the first occurence of `x`, or `-1` if `v` does
/// not contain `x`.
///
template<typename T>
bool contains(const std::vector<T>& v, const T& x, int& i) {
    i = find(v, x);
    return i != -1;
}

/// Returns whether the given vector `v` contains the given value `x`.
///
template<typename T>
bool contains(const std::vector<T>& v, const T& x) {
    int i;
    return contains(v, x, i);
}

/// Returns whether the given container `c` contains the given value `x`.
///
template<
    typename Container,
    typename T,
    VGC_REQUIRES(detail::isCompatibleRange<Container, T>)>
bool contains(const Container& c, const T& x) {
    return std::find(c.begin(), c.end(), x) != c.end();
}

/// Removes from the given vector `v` the first element which is equal to
/// `x`, if any. Returns whether an element was removed.
///
template<typename T>
bool removeOne(std::vector<T>& v, const T& x) {
    int i;
    if (contains(v, x, i)) {
        v.erase(toIterator(v, i));
        return true;
    }
    else {
        return false;
    }
}

/// Copies elements from the sorted container `c1` which are not found in the
/// sorted container `c2` to the given output iterator `out`. The output range
/// is also sorted.
///
/// This is equivalent to
/// `std::set_difference(c1.begin(), c1.end(), c2.begin(), c2.end(), out)`.
///
template<class Container1, class Container2, class OutputIt>
OutputIt set_difference(const Container1& c1, const Container2& c2, OutputIt out) {
    return std::set_difference(c1.begin(), c1.end(), c2.begin(), c2.end(), out);
}

/// Returns the index of the first element in the vector that is (strictly)
/// greater than value, or -1 if no such element is found.
///
/// The vector must be at least partially ordered, that is it must satisfy:
/// `v[i] < v[j]` => `i < j`.
///
/// This is a convenient wrapper around std::upper_bound.
///
/// Example:
/// ```cpp
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
/// ```
///
template<typename T>
int upper_bound(const std::vector<T>& v, const T& x) {
    auto it = std::upper_bound(v.begin(), v.end(), x);
    return toIndex(v, it);
}

/// Modifies in-place the given hash value `res` by combining it with
/// `std::hash(v)`.
///
template<typename T>
void hashCombine(std::size_t& res, const T& v) {
    res ^= std::hash<T>()(v) + 0x9E3779B1 // closest prime to commonly used 0x9E3779B9
           + (res << 6) + (res >> 2);
}

/// Modifies in-place the given hash value `res` by combining it with the
/// `std::hash()` of all the given `values`.
///
template<typename... Ts>
std::enable_if_t<(sizeof...(Ts) > 1), void>
hashCombine(std::size_t& res, const Ts&... values) {
    (hashCombine(res, values), ...);
}

/// \class vgc::core::AppendIterator
/// \brief An output iterator for adding to containers via append() method.
///
/// This is similar to `std::back_insert_iterator`, but for containers
/// providing an `append()` method instead.
///
/// \sa `vgc::core::appender()`.
///
template<class Container>
class AppendIterator {
protected:
    Container* container_;

public:
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = ptrdiff_t;
    using pointer = void;
    using reference = void;
    using container_type = Container;

    constexpr explicit AppendIterator(Container& c)
        : container_(std::addressof(c)) {
    }

    constexpr AppendIterator& operator=(const typename Container::value_type& v) {
        container_->append(v);
        return *this;
    }

    constexpr AppendIterator& operator=(typename Container::value_type&& v) {
        container_->append(std::move(v));
        return *this;
    }

    constexpr AppendIterator& operator*() {
        return *this;
    }

    constexpr AppendIterator& operator++() {
        return *this;
    }

    constexpr AppendIterator operator++(int) {
        return *this;
    }
};

/// Constructs an output `AppendIterator` for the given `container`.
///
/// ```cpp
/// core::Array<int> v = {1, 2, 3};
/// std::fill_n(core::appender(v), 2, 0); // v = {1, 2, 3, 0, 0}
/// ```
///
template<class Container>
constexpr AppendIterator<Container> appender(Container& container) {
    return AppendIterator<Container>(container);
}

} // namespace vgc::core

#endif // VGC_CORE_ALGORITHMS_H
