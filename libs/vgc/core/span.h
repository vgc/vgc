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

#ifndef VGC_CORE_SPAN_H
#define VGC_CORE_SPAN_H

#include <algorithm>
#include <array>
#include <cstddef> // ptrdiff_t
#include <iterator>
#include <memory> // std::addressof
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>
#include <vgc/core/parse.h>
#include <vgc/core/templateutil.h>

#include <vgc/core/detail/containerutil.h>

namespace vgc::core {

inline constexpr Int dynamicExtent = -1;

namespace detail {

template<typename T, Int extent>
struct SpanPair {
    constexpr SpanPair() noexcept = default;
    constexpr SpanPair(T* ptr_, Int) noexcept
        : ptr(ptr_) {
    }

    T* ptr = nullptr;
    static constexpr Int length = extent;
};

template<typename T>
struct SpanPair<T, dynamicExtent> {
    constexpr SpanPair() noexcept = default;
    constexpr SpanPair(T* ptr_, Int length_) noexcept
        : ptr(ptr_)
        , length(length_) {
    }

    T* ptr = nullptr;
    Int length = 0;
};

} // namespace detail

/// \class vgc::core::Span
/// \brief Object referring to a contiguous sequence of elements.
///
/// It is similar to std::span but uses `Int` instead of `size_t`, implements checks
/// in constructors, and has utility methods (search, find, etc...).
///
template<typename T, Int extent_ = dynamicExtent>
class Span {
public:
    // Define all typedefs necessary to meet the requirements of std::span.
    //
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;

    static_assert(extent_ >= -1);

    static constexpr Int extent = extent_;

public:
    constexpr Span(T* ptr, Int length, UncheckedInit) noexcept
        : pair_(ptr, length) {
    }

public:
    /// Creates an empty `Span`.
    /// Only available if `extent == 0 || extent == dynamicExtent`.
    ///
    /// ```
    /// vgc::core::Span<double> s;
    /// s.size(); // => 0
    /// s.data(); // => nullptr
    /// ```
    ///
    template<Int e_ = extent, VGC_REQUIRES(e_ == 0 || e_ == dynamicExtent)>
    constexpr Span() noexcept {
    }

    /// Creates a `Span` that is a view over the range [first, first + length).
    ///
    /// Throws `NegativeIntegerError` if `length` is negative.
    /// Throws `LogicError` if `length != extent` for static extents.
    ///
    /// ```
    /// vgc::core::Array<double> a = {1, 2, 3, 4};
    /// vgc::core::Span<double> s(&a[1], 2);
    /// s.length();      // => 2
    /// std::cout << s;  // => [2, 3]
    /// ```
    ///
    template<
        typename IntType,
        Int e_ = extent,
        VGC_REQUIRES(e_ == dynamicExtent && std::is_arithmetic_v<IntType>)>
    constexpr Span(T* first, IntType length)
        : pair_(first, int_cast<Int>(length)) {

        checkLengthForInit_(length);
    }
    /// \overload
    template<
        typename IntType,
        Int e_ = extent,
        VGC_REQUIRES(e_ != dynamicExtent && std::is_arithmetic_v<IntType>)>
    explicit constexpr Span(T* first, IntType length)
        : pair_(first, int_cast<Int>(length)) {

        checkLengthForInit_(length);
    }

    /// Creates a `Span` that is a view over the range [first, last).
    ///
    /// Throws `LogicError` if `std::distance(first, last) != extent` for static extents.
    ///
    /// ```
    /// vgc::core::Array<double> a = {1, 2, 3, 4};
    /// vgc::core::Span<double> s(&a[1], &a[3]);
    /// s.length();      // => 2
    /// std::cout << s;  // => [2, 3]
    /// ```
    ///
    template<Int e_ = extent, VGC_REQUIRES(e_ == dynamicExtent)>
    constexpr Span(T* first, T* last)
        : pair_(first, int_cast<Int>(std::distance(first, last))) {

        checkLengthForInit_(std::distance(first, last));
    }
    /// \overload
    template<Int e_ = extent, VGC_REQUIRES(e_ != dynamicExtent)>
    explicit constexpr Span(T* first, T* last)
        : pair_(first, int_cast<Int>(std::distance(first, last))) {

        checkLengthForInit_(std::distance(first, last));
    }

    /// Creates a `Span` that is a view over the given C array `arr`.
    ///
    /// Throws `LogicError` if `n != extent` for static extents.
    ///
    /// ```
    /// double a[4] = {1, 2, 3, 4};
    /// vgc::core::Span<double> s(a);
    /// s.length();      // => 4
    /// std::cout << s;  // => [1, 2, 3, 4]
    /// ```
    ///
    template<size_t n, VGC_REQUIRES(extent == dynamicExtent || n == extent)>
    constexpr Span(core::TypeIdentity<T> (&arr)[n]) noexcept(extent == dynamicExtent)
        : pair_(&arr[0], int_cast<Int>(n)) {

        checkLengthMatchesExtent_(n);
    }

    /// Creates a `Span` that is a view over the given `std::array`.
    ///
    /// Throws `LogicError` if `n != extent` for static extents.
    ///
    /// ```
    /// std::array<double, 4> a = {1, 2, 3, 4};
    /// vgc::core::Span<double> s(a);
    /// s.length();      // => 4
    /// std::cout << s;  // => [1, 2, 3, 4]
    /// ```
    ///
    template<
        typename U,
        size_t n,
        VGC_REQUIRES((extent == dynamicExtent || n == extent) && std::is_convertible_v<U (*)[], T (*)[]>)>
    constexpr Span(std::array<U, n>& arr) noexcept(extent == dynamicExtent)
        : pair_(arr.data(), int_cast<Int>(n)) {

        checkLengthMatchesExtent_(n);
    }
    /// \overload
    template<
        typename U,
        size_t n,
        VGC_REQUIRES((extent == dynamicExtent || n == extent) && std::is_convertible_v<const U (*)[], T (*)[]>)>
    constexpr Span(const std::array<U, n>& arr) noexcept(extent == dynamicExtent)
        : pair_(arr.data(), int_cast<Int>(n)) {

        checkLengthMatchesExtent_(n);
    }

    /// Creates a `Span` that is a view over the elements of `Array<U>`.
    /// U* must be convertible to T*.
    ///
    /// Throws `LogicError` if `array.length() != extent` for static extents.
    ///
    /// ```
    /// vgc::core::Array<double> a = {1, 2, 3, 4};
    /// vgc::core::Span<double> b(a);
    /// s.length();      // => 4
    /// std::cout << s;  // => [1, 2, 3, 4]
    /// ```
    ///
    template<typename U, VGC_REQUIRES(std::is_convertible_v<U (*)[], T (*)[]>)>
    constexpr Span(Array<U>& array)
        : pair_(array.data(), array.length()) {

        checkLengthMatchesExtent_(array.length());
    }
    /// \overload
    template<typename U, VGC_REQUIRES(std::is_convertible_v<const U (*)[], T (*)[]>)>
    constexpr Span(const Array<U>& array)
        : pair_(array.data(), array.length()) {

        checkLengthMatchesExtent_(array.length());
    }

    /// Creates a `Span` that is a view over the elements of `Span<U, e>`.
    /// U* must be convertible to T*.
    ///
    /// Throws `LogicError` if `e != extent` for static extents.
    ///
    /// ```
    /// std::array<double, 4> a = {1, 2, 3, 4};
    /// vgc::core::Span<double> b(a);
    /// vgc::core::Span<const double> s(b);
    /// s.length();      // => 4
    /// std::cout << s;  // => [1, 2, 3, 4]
    /// ```
    ///
    template<
        typename U,
        Int e,
        VGC_REQUIRES((extent == dynamicExtent || e == dynamicExtent || e == extent) && std::is_convertible_v<U (*)[], T (*)[]>)>
    constexpr Span(const Span<U, e>& other) noexcept(e == extent)
        : pair_(other.data(), other.length()) {

        checkLengthMatchesExtent_(other.length());
    }

    /// Copy-constructs from `other`.
    ///
    constexpr Span(const Span& other) noexcept = default;

    /// Copy-assigns from `other`.
    ///
    constexpr Span& operator=(const Span& other) noexcept = default;

    /// Returns an iterator to the first element in this `Span`.
    ///
    constexpr iterator begin() const noexcept {
        return data();
    }

    /// Returns an iterator to the past-the-last element in this `Span`.
    ///
    constexpr iterator end() const noexcept {
        return data() + length();
    }

    /// Returns a reverse iterator to the first element of the reversed
    /// `Span`.
    ///
    constexpr reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
    }

    /// Returns a reverse iterator to the past-the-last element of the reversed
    /// `Span`.
    ///
    constexpr reverse_iterator rend() const noexcept {
        return reverse_iterator(begin());
    }

    /// Returns a reference to the first element in this `Span`.
    ///
    /// Throws `IndexError` if this `Span` is empty and has dynamic extent.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// s.first() = 11;
    /// std::cout << a.first(); // => [11, 42, 12];
    /// ```
    ///
    constexpr T& first() const noexcept(extent != dynamicExtent) {
        if constexpr (extent != dynamicExtent) {
            static_assert(extent > 0, "Span of extent 0 has no first element.");
        }
        else {
            if (isEmpty()) {
                throw IndexError(
                    "Attempting to access the first element of an empty Span.");
            }
        }
        return *begin();
    }

    /// Returns a reference to the last element in this `Span`.
    ///
    /// Throws `IndexError` if this `Span` is empty and has dynamic extent.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// s.last() = 11;
    /// std::cout << a.first(); // => [10, 42, 11];
    /// ```
    ///
    constexpr T& last() const noexcept(extent != dynamicExtent) {
        if constexpr (extent != dynamicExtent) {
            static_assert(extent > 0, "Calling Span::last() on a Span of extent 0.");
        }
        else {
            if (isEmpty()) {
                throw IndexError(
                    "Attempting to access the last element of an empty Span.");
            }
        }
        return *(end() - 1);
    }

    /// Returns a reference to the element at index `i`.
    ///
    /// Throws `IndexError` if this `Span` is empty or if `i` does not belong
    /// to the range [`0`, `length() - 1`].
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// std::cout << s;  // => [10, 42, 12]
    /// s[1] = 7;
    /// std::cout << a;  // => [10, 7, 12]
    /// std::cout << s;  // => [10, 7, 12]
    /// s[-1] = 7;       // => IndexError
    /// s[3] = 7;        // => IndexError
    /// ```
    ///
    constexpr T& operator[](Int i) const {
        checkIndexInRange_(i);
        return data()[static_cast<size_t>(i)];
        // Note: No need for int_cast (bounds already checked)
    }

    /// Returns a reference to the element at index `i`, without
    /// bounds checking.
    ///
    /// The behavior is undefined if this `Span` is empty or if `i` does not
    /// belong to [`0`, `length() - 1`]. In practice, this may cause the
    /// application to crash (segfault), or be a security vulnerability
    /// (leaking a password).
    ///
    /// Do not use this function unless you have measured and documented that
    /// the bounds checking in your particular use case was a significant
    /// performance bottleneck.
    ///
    constexpr T& getUnchecked(Int i) const {
        return data()[static_cast<size_t>(i)];
    }

    /// Returns a const reference to the element at index `i`, with wrapping
    /// behavior.
    ///
    /// Throws `IndexError` if this `Span` is empty.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// std::cout << s.getWrapped(-1); // => 12
    /// std::cout << s.getWrapped(3);  // => 10
    /// ```
    ///
    constexpr T& getWrapped(Int i) const {
        if (isEmpty()) {
            throw IndexError("Calling getWrapped(" + toString(i) + ") on an empty Span.");
        }
        return data()[static_cast<size_type>(wrap_(i))];
    }

    /// Returns a pointer to the underlying data.
    ///
    /// You can use `data()` together with `length()` or `size()` to pass the
    /// content of this `Span` to an API expecting a C-style array and size.
    ///
    constexpr T* data() const noexcept {
        return pair_.ptr;
    }

    /// Returns, as an unsigned integer, the number of elements in this `Span`.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// `length()` instead.
    ///
    constexpr size_type size() const noexcept {
        return static_cast<size_type>(length());
    }

    /// Returns the number of elements in this `Span`.
    ///
    constexpr Int length() const noexcept {
        return pair_.length;
    }

    /// Returns the size in bytes of the contiguous sequence of elements in memory.
    ///
    constexpr Int sizeInBytes() const noexcept {
        return length() * sizeof(element_type);
    }

    /// Returns whether this `Span` is empty.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// `isEmpty()` instead.
    ///
    constexpr bool empty() const noexcept {
        return isEmpty();
    }

    /// Returns whether this `Span` is empty.
    ///
    constexpr bool isEmpty() const noexcept {
        return length() == 0;
    }

    /// Returns a new `Span` that is a view over the first `count` elements of this `Span`.
    ///
    /// Throws `IndexError` if this `Span` length is smaller than `count`.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// std::cout << s.first<2>(); // => [10, 42];
    /// ```
    ///
    template<Int count, VGC_REQUIRES(count >= 0 || count == dynamicExtent)>
    constexpr Span<T, count> first() const noexcept(extent != dynamicExtent) {
        if constexpr (extent != dynamicExtent) {
            static_assert(
                count <= extent, "Count exceeds extent in Span::first<count>()");
        }
        else {
            if (count > length()) {
                throwRangeNotInRange_(Int(0), count);
            }
        }
        return Span<T, count>(data(), count, uncheckedInit);
    }

    /// Returns a new `Span` that is a view over the last `count` elements of this `Span`.
    ///
    /// Throws `IndexError` if this `Span` length is smaller than `count`.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// std::cout << s.last<2>(); // => [42, 12];
    /// ```
    ///
    template<Int count, VGC_REQUIRES(count >= 0 || count == dynamicExtent)>
    constexpr Span<T, count> last() const noexcept(extent != dynamicExtent) {
        const Int len = length();
        if constexpr (extent != dynamicExtent) {
            static_assert(
                count <= extent, "'count' exceeds 'extent' in Span::last<count>()");
        }
        else {
            if (count > len) {
                throwRangeNotInRange_(len - count, len);
            }
        }
        return Span<T, count>(data() + len - count, count, uncheckedInit);
    }

    /// Returns a new `Span` that is a view over the `count` elements of this `Span`
    /// starting at `offset`. If `count` is dynamicExtent the returned `Span` ends where
    /// this `Span` ends.
    ///
    /// Throws `IndexError` if the given range is out of this `Span` range.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// std::cout << s.subspan<1, 1>(); // => [42];
    /// ```
    ///
    template<
        Int offset,
        Int count = dynamicExtent,
        VGC_REQUIRES(offset >= 0 && (count >= 0 || count == dynamicExtent))>
    constexpr auto subspan() const noexcept(extent != dynamicExtent) {

        const Int len = length();
        if constexpr (extent != dynamicExtent) {
            if constexpr (count != dynamicExtent) {
                // span<T, n>::subspan<T, n>
                static_assert(
                    offset + count <= extent,
                    "'offset + count' exceeds 'extent' in Span::subspan<offset, "
                    "count>()");
            }
            else {
                // span<T, n>::subspan<T, dynamicExtent>
                static_assert(
                    offset <= extent,
                    "'offset' exceeds 'extent' in Span::subspan<offset, "
                    "dynamicExtent>()");
            }
        }
        else if constexpr (count != dynamicExtent) {
            // span<T, dynamicExtent>::subspan<T, n>
            if (count > len - offset) {
                throwRangeNotInRange_(offset, offset + count);
            }
        }
        else if (offset > len) {
            // span<T, dynamicExtent>::subspan<T, dynamicExtent>
            throwRangeNotInRange_(offset, offset + count);
        }

        constexpr Int subspanExtent =
            (count != dynamicExtent
                 ? count
                 : (extent == dynamicExtent ? dynamicExtent : extent - offset));

        return Span<T, subspanExtent>(
            data() + offset,
            count == dynamicExtent ? len - offset : count,
            uncheckedInit);
    }

    /// Returns a new `Span` that is a view over the first `count` elements of this `Span`.
    ///
    /// Throws `IndexError` if this `Span` length is smaller than `count`.
    /// Throws `NegativeIntegerError` if `count` is negative.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// std::cout << s.first<2>(); // => [10, 42];
    /// ```
    ///
    constexpr Span<T, dynamicExtent> first(Int count) const {
        if (count < 0) {
            throw NegativeIntegerError(
                format("Subspan::first({}): 'count' cannot be negative.", count));
        }
        if (count > length()) {
            throwRangeNotInRange_(Int(0), count);
        }
        return Span<T, dynamicExtent>(data(), count, uncheckedInit);
    }

    /// Returns a new `Span` that is a view over the last `count` elements of this `Span`.
    ///
    /// Throws `IndexError` if this `Span` length is smaller than `count`.
    /// Throws `NegativeIntegerError` if `count` is negative.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// std::cout << s.last<2>(); // => [42, 12];
    /// ```
    ///
    constexpr Span<T, dynamicExtent> last(Int count) const {
        const Int len = length();
        if (count < 0) {
            throw NegativeIntegerError(
                format("Subspan::last({}): 'count' cannot be negative.", count));
        }
        if (count > len) {
            throwRangeNotInRange_(len - count, len);
        }
        return Span<T, dynamicExtent>(data() + len - count, count, uncheckedInit);
    }

    /// Returns a new `Span` that is a view over the `count` elements of this `Span`
    /// starting at `offset`. If `count` is dynamicExtent the returned `Span` ends where
    /// this `Span` ends.
    ///
    /// Throws `IndexError` if the given range is out of this `Span` range.
    /// Throws `NegativeIntegerError` if `count` or `offset` is negative.
    ///
    /// ```
    /// std::array<double, 4> a = {10, 42, 12};
    /// vgc::core::Span<double> s(a);
    /// std::cout << s.subspan<1, 1>(); // => [42];
    /// ```
    ///
    constexpr Span<T, dynamicExtent>
    subspan(Int offset, Int count = dynamicExtent) const {
        const Int len = length();
        if (offset < 0) {
            throw NegativeIntegerError(format(
                "Subspan::subspan({}, {}): 'offset' cannot be negative.", offset, count));
        }
        if (offset > len) {
            throw IndexError(format(
                "Subspan::subspan({}, {}): 'offset' cannot exceed the length of the "
                "span.",
                offset,
                count));
        }
        if (count != dynamicExtent) {
            if (count < 0) {
                throw NegativeIntegerError(format(
                    "Subspan::subspan({}, {}): 'count' cannot be negative if not "
                    "equal "
                    "to 'dynamicExtent'.",
                    offset,
                    count));
            }
            if (count > len - offset) {
                throwRangeNotInRange_(offset, offset + count);
            }
        }
        return Span<T, dynamicExtent>(
            data() + offset,
            count == dynamicExtent ? len - offset : count,
            uncheckedInit);
    }

    /// Returns whether this `Span` contains `value`.
    ///
    bool contains(const T& value) const {
        return search(value) != nullptr;
    }

    /// Returns an iterator to the first element that compares equal to `value`,
    /// or the end iterator if there is no such element.
    ///
    iterator find(const T& value) const {
        T* p = data();
        T* end = p + length();
        for (; p != end; ++p) {
            if (*p == value) {
                break;
            }
        }
        return makeIterator(p);
    }

    /// Returns an iterator to the first element for which `predicate(element)`
    /// returns `true`, or the end iterator if there is no such element.
    ///
    template<typename UnaryPredicate>
    iterator find(UnaryPredicate predicate) const {
        T* p = data();
        T* end = p + length();
        for (; p != end; ++p) {
            if (predicate(*p)) {
                break;
            }
        }
        return makeIterator(p);
    }

    /// Returns a pointer to the first element that compares equal to `value`,
    /// or `nullptr` if there is no such element.
    ///
    T* search(const T& value) const {
        T* p = data();
        T* end = p + length();
        for (; p != end; ++p) {
            if (*p == value) {
                return p;
            }
        }
        return nullptr;
    }

    /// Returns a pointer to the first element for which `predicate(element)`
    /// returns `true`, or `nullptr` if there is no such element.
    ///
    template<typename UnaryPredicate>
    T* search(UnaryPredicate predicate) const {
        T* p = data();
        T* end = p + length();
        for (; p != end; ++p) {
            if (predicate(*p)) {
                return p;
            }
        }
        return nullptr;
    }

    /// Returns the index of the first element that compares equal to
    /// `value`, or `-1` if there is no such element.
    ///
    Int index(const T& value) const {
        const T* p = data();
        const T* end = p + length();
        for (; p != end; ++p) {
            if (*p == value) {
                // invariant: smaller than length()
                return static_cast<Int>(p - data());
            }
        }
        return -1;
    }

    /// Returns the index of the first element for which `predicate(element)`
    /// returns `true`, or `-1` if there is no such element.
    ///
    template<typename UnaryPredicate>
    Int index(UnaryPredicate predicate) const {
        const T* p = data();
        const T* end = p + length();
        for (; p != end; ++p) {
            if (predicate(*p)) {
                // invariant: smaller than length()
                return static_cast<Int>(p - data());
            }
        }
        return -1;
    }

private:
    detail::SpanPair<T, extent> pair_ = {};

    iterator makeIterator(pointer p) const {
        return p;
    }

    // Wraps the given integer to the [0, length()-1] range.
    // See Array::wrap_().
    Int wrap_(Int i) const {
        Int n = length();
        return (n + (i % n)) % n;
    }

    // Throws LogicError if (extent != dynamicExtent) and (len != extent) with an error message
    // appropriate for constructors.
    //
    template<typename IntType>
    constexpr void checkLengthMatchesExtent_([[maybe_unused]] IntType len) const {
        if constexpr (extent != dynamicExtent) {
            if (len != extent) {
                throw LogicError(format(
                    "Cannot create a Span<T, {}> over {} elements: the count must "
                    "match the static extent.",
                    extent,
                    len));
            }
        }
    }

    // Throws NegativeIntegerError if (len < 0), with an error message
    // appropriate for constructors.
    // Throws LogicError if (extent != dynamicExtent) and (len != extent) with an error message
    // appropriate for constructors.
    //
    template<typename IntType>
    constexpr void checkLengthForInit_([[maybe_unused]] IntType len) const {
        if constexpr (std::is_signed_v<IntType>) {
            if (len < 0) {
                throw NegativeIntegerError(format(
                    "Cannot create a Span over {} elements: the extent cannot be "
                    "negative.",
                    len));
            }
        }
        checkLengthMatchesExtent_(len);
    }

    // Checks whether index i is in range [0, size):
    //
    template<typename IntType>
    void throwIndexNotInRange_(IntType i) const {
        std::string err = format("Index {} out of Span range", i);
        err += isEmpty() ? "(empty)" : format("[0, {}).", length());
        throw IndexError(err);
    }
    void checkIndexInRange_(Int i) const {
        if (i < 0 || i >= length()) {
            throwIndexNotInRange_(i);
        }
    }
    void checkIndexInRange_(size_t i) const {
        if (i > tmax<Int>) {
            throwIndexNotInRange_(i);
        }
        checkIndexInRange_(static_cast<Int>(i));
    }

    template<typename IntType>
    void throwRangeNotInRange_(IntType i, IntType j) const {
        std::string err = format("Range [{}, {}) out of Span range", i, j);
        err += isEmpty() ? "(empty)" : format("[0, {}).", length());
        throw IndexError(err);
    }

    template<typename IntType>
    void throwNegativeSubspanLengthError_(IntType length) const {
        throw NegativeIntegerError(format(
            "Cannot create a Subspan of length {}: length cannot be negative.", length));
    }
};

// deduction guides
template<typename T, size_t n>
Span(std::array<T, n>&) -> Span<T, n>;
template<typename T, size_t n>
Span(const std::array<T, n>&) -> Span<const T, n>;
template<typename T>
Span(core::Array<T>&) -> Span<T, dynamicExtent>;
template<typename T, size_t n>
Span(const core::Array<T>&) -> Span<const T, dynamicExtent>;
template<typename T, typename IntType, VGC_REQUIRES(std::is_arithmetic_v<IntType>)>
Span(T*, IntType) -> Span<T, dynamicExtent>;
template<typename T>
Span(T*, T*) -> Span<T, dynamicExtent>;

template<typename T, Int extent_ = dynamicExtent>
using ConstSpan = Span<const T, extent_>;

/// Writes the given `Span<T>` to the output stream.
///
template<typename OStream, typename T, Int extent>
void write(OStream& out, const Span<T, extent>& a) {
    if (a.isEmpty()) {
        write(out, "[]");
    }
    else {
        write(out, '[');
        auto it = a.begin();
        auto last = a.end() - 1;
        write(out, *it);
        while (it != last) {
            write(out, ", ", *++it);
        }
        write(out, ']');
    }
}

/// Reads the given `Span<T>` from the input stream, and stores it in the given
/// output parameter `a`. Leading whitespaces are allowed.
///
/// Throws `ParseError` if the stream does not start with an `Span<T>`.
/// Throws `RangeError` if one of the values in the array is outside of the
/// representable range of its type.
///
template<typename IStream, typename T, Int extent>
void readTo(Span<T, extent>& a, IStream& in) {
    a.clear();
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, '[');
    skipWhitespaceCharacters(in);
    char c = readCharacter(in);
    if (c != ']') {
        in.unget();
        c = ',';
        while (c == ',') {
            skipWhitespaceCharacters(in);
            a.append(read<T>(in));
            skipWhitespaceCharacters(in);
            c = readExpectedCharacter(in, {',', ']'});
        }
    }
}

namespace detail {

template<typename T>
struct IsSpan : std::false_type {};

template<typename T, Int extent>
struct IsSpan<Span<T, extent>> : std::true_type {};

} // namespace detail

/// Checks whether the type `T` is a specialization of core::Span.
///
template<typename T>
inline constexpr bool isSpan = detail::IsSpan<T>::value;

} // namespace vgc::core

template<typename T, vgc::Int extent>
struct fmt::formatter<vgc::core::Span<T, extent>>
    : fmt::formatter<vgc::core::RemoveCVRef<T>> {

    template<typename FormatContext>
    auto format(const vgc::core::Span<T, extent>& x, FormatContext& ctx)
        -> decltype(ctx.out()) {

        auto out = ctx.out();
        if (x.isEmpty()) {
            out = vgc::core::copyStringTo(out, "[]");
        }
        else {
            *out++ = '[';
            auto it = x.begin();
            auto last = x.end() - 1;
            out = vgc::core::formatTo(out, "{}", *it);
            while (it != last) {
                out = vgc::core::copyStringTo(out, ", ");
                ctx.advance_to(out);
                out = fmt::formatter<vgc::core::RemoveCVRef<T>>::format(*++it, ctx);
            }
            *out++ = ']';
        }

        ctx.advance_to(out);
        return out;
    }
};

#endif // VGC_CORE_SPAN_H
