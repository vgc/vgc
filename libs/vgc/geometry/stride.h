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

#ifndef VGC_GEOMETRY_STRIDE_H
#define VGC_GEOMETRY_STRIDE_H

#include <vgc/core/arithmetic.h>

namespace vgc::geometry {

/// \class vgc::geometry::StrideIterator
/// \brief Iterates over a memory buffer `T*` with reinterpret_cast of its elements.
///
/// See `vgc::geometry::StrideSpan` for details.
///
template<typename T, typename U>
class StrideIterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = U;
    using pointer = U*;
    using reference = U&;

    StrideIterator(T* p, Int stride)
        : p_(p)
        , stride_(core::int_cast<difference_type>(stride)) {
    }

    reference operator*() const {
        return *reinterpret_cast<U*>(p_);
    }

    pointer operator->() const {
        return reinterpret_cast<U*>(p_);
    }

    reference operator[](difference_type n) const {
        return *reinterpret_cast<U*>(p_ + n * stride_);
    }

    StrideIterator& operator++() {
        p_ += stride_;
        return *this;
    }

    StrideIterator& operator--() {
        p_ -= stride_;
        return *this;
    }

    StrideIterator operator++(int) {
        StrideIterator tmp = *this;
        p_ += stride_;
        return tmp;
    }

    StrideIterator operator--(int) {
        StrideIterator tmp = *this;
        p_ -= stride_;
        return tmp;
    }

    StrideIterator& operator+=(difference_type n) {
        p_ += n * stride_;
        return *this;
    }

    StrideIterator& operator-=(difference_type n) {
        p_ -= n * stride_;
        return *this;
    }

    StrideIterator operator+(difference_type n) const {
        return StrideIterator(p_ + n * stride_, stride_);
    }

    StrideIterator operator-(difference_type n) const {
        return StrideIterator(p_ - n * stride_, stride_);
    }

    friend StrideIterator operator+(difference_type n, const StrideIterator& rhs) {
        return rhs + n;
    }

    difference_type operator-(const StrideIterator& rhs) const {
        return (p_ - rhs.p_) / stride_;
    }

    friend bool operator==(const StrideIterator& a, const StrideIterator& b) {
        return a.p_ == b.p_;
    }

    friend bool operator!=(const StrideIterator& a, const StrideIterator& b) {
        return a.p_ != b.p_;
    }

    friend bool operator<(const StrideIterator& a, const StrideIterator& b) {
        return a.p_ < b.p_;
    }

    friend bool operator<=(const StrideIterator& a, const StrideIterator& b) {
        return a.p_ <= b.p_;
    }

    friend bool operator>(const StrideIterator& a, const StrideIterator& b) {
        return a.p_ > b.p_;
    }

    friend bool operator>=(const StrideIterator& a, const StrideIterator& b) {
        return a.p_ >= b.p_;
    }

private:
    T* p_;
    difference_type stride_;
};

/// \class vgc::geometry::StrideSpan
/// \brief Iterates over a memory buffer `T*` with reinterpret_cast of its elements.
///
/// ```cpp
/// float[] buffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
/// for (const Vec2f& v : StrideSpan<float, Vec2f>(buffer, 2, 5)) {
///     std::cout << v << std::end;
/// }
/// // => prints "(1, 2)(6, 7)"
/// ```
///
/// Note that this practice is in general not allowed by the C++ standard, and
/// should only be used with classes `U` that are trivially destructible and
/// whose memory layout is made of a known number of `T` elements (example:
/// `Vec2f` is trivially destructible and made of 2 floats).
///
template<typename T, typename U>
class StrideSpan {
public:
    /// Creates a `StrideSpan` to iterate over the given `count` of `U`
    /// elements, starting at `begin`, and separated in memory by the given
    /// `stride` number of `T` elements.
    ///
    StrideSpan(T* begin, Int count, Int stride)
        : begin_(begin)
        , count_(count)
        , stride_(stride) {
    }

    /// The begin iterator of the span.
    ///
    StrideIterator<T, U> begin() const {
        return StrideIterator<T, U>(begin_, stride_);
    }

    /// The end iterator of the span.
    ///
    StrideIterator<T, U> end() const {
        return StrideIterator<T, U>(begin_ + (stride_ * count_), stride_);
    }

    /// Accesses the element at index `n`.
    ///
    U& operator[](Int n) const {
        return *reinterpret_cast<U*>(begin_ + n * stride_);
    }

private:
    T* begin_;
    Int count_;
    Int stride_;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_STRIDE_H
