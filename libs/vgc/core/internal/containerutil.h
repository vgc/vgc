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

#ifndef VGC_CORE_INTERNAL_CONTAINERUTIL_H
#define VGC_CORE_INTERNAL_CONTAINERUTIL_H

#include <iterator>
#include <type_traits>

#include <vgc/core/arithmetic.h>
#include <vgc/core/templateutil.h>

namespace vgc::core::internal {

// Checks whether the given template argument is a signed integer.

template<typename T, typename Enable = void>
struct IsSignedInteger : std::false_type {};

template<typename T>
struct IsSignedInteger<T, std::enable_if_t<
        std::is_integral_v<T> &&
        std::is_signed_v<T>>> :
    std::true_type {};

template<typename T>
constexpr bool isSignedInteger = IsSignedInteger<T>::value;

template<typename T>
using RequiresSignedInteger = Requires<isSignedInteger<T>>;

// Checks whether the given template argument is an input iterator.

template<typename T, typename Enable = void>
struct IsInputIterator : std::false_type {};

template<typename T>
struct IsInputIterator<T, std::enable_if_t<
    std::is_convertible_v<
        typename std::iterator_traits<T>::iterator_category,
        std::input_iterator_tag>>> :
    std::true_type {};

template<typename T>
constexpr bool isInputIterator = IsInputIterator<T>::value;

template<typename T>
using RequiresInputIterator = Requires<isInputIterator<T>>;

// Checks whether the given template argument is a forward iterator.

template<typename T, typename Enable = void>
struct IsForwardIterator : std::false_type {};

template<typename T>
struct IsForwardIterator<T, std::enable_if_t<
    std::is_convertible_v<
        typename std::iterator_traits<T>::iterator_category,
        std::forward_iterator_tag>>> :
    std::true_type {};

template<typename T>
constexpr bool isForwardIterator = IsForwardIterator<T>::value;

template<typename T>
using RequiresForwardIterator = Requires<isForwardIterator<T>>;

// Checks whether the given template argument is a range, that is, whether it
// has Range::begin() and Range::end() and they return forward iterators.

template<typename T, typename Enable = void>
struct IsRange : std::false_type {};

template<typename T>
struct IsRange<T, std::enable_if_t<
        isForwardIterator<decltype(std::declval<T>().begin())> &&
        isForwardIterator<decltype(std::declval<T>().end())>>> :
    std::true_type {};

template<typename T>
constexpr bool isRange = IsRange<T>::value;

template<typename T>
using RequiresRange = Requires<isRange<T>>;

// Checks whether the given template argument is a forward iterator
// whose value_type is assignable to a T

template<typename It, typename T, typename Enable = void>
struct IsCompatibleForwardIterator : std::false_type {};

template<typename It, typename T>
struct IsCompatibleForwardIterator<It, T, std::enable_if_t<
    std::is_convertible_v<
        typename std::iterator_traits<It>::iterator_category,
        std::forward_iterator_tag> &&
    std::is_assignable_v<
        T&,
        typename std::iterator_traits<It>::value_type>>> :
    std::true_type {};

template<typename It, typename T>
constexpr bool isCompatibleForwardIterator = IsCompatibleForwardIterator<It, T>::value;

template<typename It, typename T>
using RequiresCompatibleForwardIterator = Requires<isCompatibleForwardIterator<It, T>>;

// Checks whether the given template argument is a range whose value_type of its
// iterators is assignable to a T

template<typename Range, typename T, typename Enable = void>
struct IsCompatibleRange : std::false_type {};

template<typename Range, typename T>
struct IsCompatibleRange<Range, T, std::enable_if_t<
        isCompatibleForwardIterator<decltype(std::declval<Range>().begin()), T> &&
        isCompatibleForwardIterator<decltype(std::declval<Range>().end()), T>>> :
    std::true_type {};

template<typename Range, typename T>
constexpr bool isCompatibleRange = IsCompatibleRange<Range, T>::value;

template<typename Range, typename T>
using RequiresCompatibleRange = Requires<isCompatibleRange<Range, T>>;

// Checks whether the given template argument has a NoInit constructor

template<typename T>
constexpr bool isNoInitConstructible = std::is_constructible_v<T, NoInit>;

template<typename T>
using RequiresNoInitConstructible = Requires<isNoInitConstructible<T>>;

} // namespace vgc::core::internal

#endif // VGC_CORE_INTERNAL_CONTAINERUTIL_H
