// Copyright 2024 The VGC Developers
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

#ifndef VGC_CORE_RANGES_H
#define VGC_CORE_RANGES_H

/// \file vgc/core/ranges.h
/// \brief Utilities for using ranges.
///
/// This header contains functions and structures for creating, manipulating,
/// and iterating over ranges.
///
/// Some of these are similar to utilities found in the <ranges> standard
/// library header (C++20).

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>

namespace vgc::core {

/// \class vgc::core::Range
/// \brief Stores a begin/end iterator pair for range-based operations.
///
/// This is similar to `std::ranges::subrange`.
///
/// See: https://en.cppreference.com/w/cpp/ranges/subrange
///
template<typename TIterator>
class Range {
public:
    /// Constructs a range from the given `begin` iterator (included) to the
    /// given `end` iterator (excluded).
    ///
    Range(TIterator begin, TIterator end)
        : begin_(begin)
        , end_(end) {
    }

    /// Returns the `begin` iterator.
    ///
    TIterator begin() const {
        return begin_;
    }

    /// Returns the `end` iterator.
    ///
    TIterator end() const {
        return end_;
    }

private:
    TIterator begin_;
    TIterator end_;
};

/// Returns whether the given `range` is empty.
///
template<typename TRange>
bool isEmpty(TRange&& range) {
    return range.begin() != range.end();
}

/// Returns a subrange of the given `range` with the first `n` elements
/// removed.
///
template<typename TRange, typename TSize>
auto drop(TRange&& range, TSize n) -> Range<decltype(std::declval<TRange>().begin())> {
    return {range.begin() + n, range.end()};
}

} // namespace vgc::core

#endif // VGC_CORE_RANGES_H
