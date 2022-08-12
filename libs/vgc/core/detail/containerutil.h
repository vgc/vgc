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

namespace vgc::core::detail {

// clang-format off

// Checks whether the given template argument is a forward iterator
// whose value_type is assignable to a T

template<typename It, typename T, typename SFINAE = void>
struct IsCompatibleForwardIterator : std::false_type {};

template<typename It, typename T>
struct IsCompatibleForwardIterator<It, T, Requires<
        std::is_convertible_v<
            typename std::iterator_traits<It>::iterator_category,
            std::forward_iterator_tag>
        && std::is_assignable_v<
            T&,
            typename std::iterator_traits<It>::value_type>>>
    : std::true_type {
};

template<typename It, typename T>
inline constexpr bool isCompatibleForwardIterator =
    IsCompatibleForwardIterator<It, T>::value;

// Checks whether the given template argument is a range whose value_type of its
// iterators is assignable to a T

template<typename Range, typename T, typename SFINAE = void>
struct IsCompatibleRange : std::false_type {};

template<typename Range, typename T>
struct IsCompatibleRange<Range, T, Requires<
        isCompatibleForwardIterator<decltype(std::declval<Range>().begin()), T>
        && isCompatibleForwardIterator<decltype(std::declval<Range>().end()), T>>>
    : std::true_type {};

template<typename Range, typename T>
inline constexpr bool isCompatibleRange = IsCompatibleRange<Range, T>::value;

// Checks whether the given template argument has a NoInit constructor

template<typename T>
inline constexpr bool isNoInitConstructible = std::is_constructible_v<T, NoInit>;

// clang-format on

} // namespace vgc::core::detail

#endif // VGC_CORE_INTERNAL_CONTAINERUTIL_H
