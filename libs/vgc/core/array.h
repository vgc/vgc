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

#ifndef VGC_CORE_ARRAY_H
#define VGC_CORE_ARRAY_H

#include <vector>

#include <vgc/core/api.h>
#include <vgc/core/exceptions.h>
#include <vgc/core/int.h>
#include <vgc/core/stringutil.h>

#include <vgc/core/internal/containerutil.h>

namespace vgc {
namespace core {

/// \class vgc::core::Array
/// \brief Sequence of elements stored contiguously in memory.
///
/// The class template Array represents a "dynamic array", that is, a container
/// type where all its elements are stored contiguously in the heap, and are
/// accessible via an index from 0 to length() - 1. This makes it the most
/// cache-friendly of all containers, and for this reason is the preferred
/// container to use in most cases.
///
/// This class template meets the requirements of Container,
/// AllocatorAwareContainer, SequenceContainer, ContiguousContainer, and
/// ReversibleContainer, and therefore can be used with all generic STL
/// algorithms that work on std::vector.
///
/// In fact, this class template is nothing else but a thin wrapper around
/// std::vector. If necessary, you can get a (possibly mutable) reference to
/// the underlying std::vector by calling Array::stdVector(). The difference
/// with std::vector (and the reason why we defined such a wrapper) is
/// three-fold:
///
/// 1. Unlike std::vector, accessing elements via operator[], front(), and
///    back() is bounds-checked. Note that some methods using iterators are still
///    not bounds-checked, so be extra careful when using those.
///
/// 2. It implements additional function overloads taking signed integers
///    (vgc::core::Int) as arguments, performing safe conversions to the
///    unsigned Array::size_type. Similarly, it also adds new functions such as
///    length() to return a vgc::core::Int, while still implementing size() to
///    return a size_type (as per requirements of the Container concept).
///
/// 3. It adds redundant functions such as isEmpty() (= empty()) and
///    shrinkToFit() (= shrink_to_fit()), and typedefs such as Iterator (=
///    iterator) to follow VGC naming conventions.
///
/// For these reasons (most importantly for bounds-checking and safe signed to
/// unsigned type casting), you should always use vgc::core::Array rather than
/// std::vector in all the VGC codebase. If performance is an issue, you can
/// use the atWithoutBoundsChecking() method, which as its intentionally long
/// name suggests, allows you to access elements without bounds-checking.
///
template<typename T, typename Allocator = std::allocator<T>>
class VGC_CORE_API Array
{

public:
    // The underlying std::vector type.
    using StdVectorType = std::vector<T, Allocator>;

    // Define all typedefs necessary to meet the requirements of Container,
    // AllocatorAwareContainer, SequenceContainer, ContiguousContainer, and
    // ReversibleContainer.
    //
    // Note: It is required that size_type be an unsigned integer, as per
    // [container.requirements.general] of the C++11 standard (and most likely
    // any future revisions). Therefore, we define it to be
    // std::vector<T>:size_type rather than vgc::core::Int, despite providing
    // additional function overloads to work with vgc::core::Int.
    //
    // Note: for now, we also use std::vector<T>:iterator (and variants) as our
    // iterator types. In the future, we may want to consider using custom
    // bounds-checked iterators, to ensure that methods taking iterators as
    // arguments are also bounds-checked (when these iterators are from
    // vgc::core containers). However, this might impact performance of
    // range-for loops, which are typically safe to use even if not
    // bounds-checked. So maybe it's a good idea to keep unsafe iterators, and
    // just discourage their explicit use.
    //
    // Note: [const_]reference and [const_]pointer are historical artifacts
    // which can now be assumed to simply be [const] T& and [const] T*.
    //
    using value_type             = typename StdVectorType::value_type;
    using allocator_type         = typename StdVectorType::allocator_type;
    using size_type              = typename StdVectorType::size_type;
    using difference_type        = typename StdVectorType::difference_type;
    using reference              = typename StdVectorType::reference;
    using const_reference        = typename StdVectorType::const_reference;
    using pointer                = typename StdVectorType::pointer;
    using const_pointer          = typename StdVectorType::const_pointer;
    using iterator               = typename StdVectorType::iterator;
    using const_iterator         = typename StdVectorType::const_iterator;
    using reverse_iterator       = typename StdVectorType::reverse_iterator;
    using const_reverse_iterator = typename StdVectorType::const_reverse_iterator;

    // Additional typedefs following VGC naming conventions.
    using Iterator             = iterator;
    using ConstIterator        = const_iterator;
    using ReverseIterator      = reverse_iterator;
    using ConstReverseIterator = const_reverse_iterator;

    /// Creates an empty Array (with a default-constructed allocator).
    ///
    Array() : data_() {

    }

    /// Creates an empty Array with the given \p allocator.
    ///
    explicit Array(const Allocator& allocator) : data_(allocator) {

    }

    /// Creates an Array of the given \p size with all values initialized to
    /// the given \p value.
    ///
    Array(size_type size, const T& value, const Allocator& allocator = Allocator()) :
        data_(size, value, allocator) {

    }

    /// Creates an Array of the given \p size with all values initialized to
    /// the given \p value.
    ///
    /// Throws NegativeIntegerError if the given \p size is negative.
    ///
    Array(Int size, const T& value, const Allocator& allocator = Allocator()) :
        data_(int_cast<size_type>(size), value, allocator) {

    }

    /// Creates an Array of the given \p size with all values default-inserted.
    ///
    // Note: C++14 adds the optional arg: const Allocator& allocator = Allocator()
    //       to this function.
    //
    explicit Array(size_type size) : data_(size) {

    }

    /// Creates an Array of the given \p size with all values default-inserted.
    ///
    /// Throws NegativeIntegerError if the given \p size is negative.
    ///
    // Note: C++14 adds the optional arg: const Allocator& allocator = Allocator()
    //       to this function.
    //
    explicit Array(Int size) : data_(int_cast<size_type>(size)) {

    }

    /// Creates an Array initialized from the elements in the range given by
    /// the input iterators \p first (inclusive) and \p last (exclusive).
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range.
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    Array(InputIt first, InputIt last, const Allocator& allocator = Allocator()) :
        data_(first, last, allocator) {

    }

    /// Copy-constructs the \p other Array.
    ///
    Array(const Array& other) : data_(other) {

    }

    /// Copy-constructs the \p other Array with the given \p allocator.
    ///
    Array(const Array& other, const Allocator& allocator) :
        data_(other, allocator) {

    }

    /// Move-constructs the \p other Array.
    ///
    Array(Array&& other) : data_(std::move(other.data_)) {

    }

    /// Move-constructs the \p other Array with the given \p allocator.
    ///
    Array(Array&& other, const Allocator& allocator) :
        data_(std::move(other.data_), allocator) {

    }

    /// Creates an Array initialized by the values given in the initializer
    /// list \p init.
    ///
    Array(std::initializer_list<T> init, const Allocator& allocator = Allocator()) :
        data_(init, allocator) {

    }

    /// Destructs the given Array.
    ///
    ~Array() {

    }

    /// Copy-assigns the given \p other Array.
    ///
    Array& operator=(const Array& other) {
        data_ = other.data_;
    }

    /// Move-assigns the given \p other Array.
    ///
    Array& operator=(Array&& other) {
        data_ = std::move(other.data_);
    }

    /// Replaces the contents of this Array by the values given in the
    /// initializer list \p ilist.
    ///
    Array& operator=(std::initializer_list<T> ilist) {
        data_ = ilist;
    }

    /// Replaces the content of the Array by an array of the given \p size with
    /// all values initialized to the given \p value.
    ///
    void assign(size_type size, T value) {
        data_.assign(size, value);
    }

    /// Replaces the content of the Array by an array of the given \p size
    /// with all values initialized to the given \p value.
    ///
    /// Throws NegativeIntegerError if the given \p size is negative.
    ///
    void assign(Int size, T value) {
        data_.assign(int_cast<size_type>(size), value);
    }

    /// Replaces the content of the Array by the elements in the range given by
    /// the input iterators \p first (inclusive) and \p last (exclusive).
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range,
    /// or is a range into this Array.
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    void assign(InputIt first, InputIt last) {
        data_.assign(first, last);
    }

    /// Replaces the contents of this Array by the values given in the
    /// initializer list \p ilist.
    ///
    void assign(std::initializer_list<T> ilist) {
        data_.assign(ilist);
    }

    /// Returns the allocator associated with this Array.
    ///
    /// This function is equivalent to getAllocator(), and is only provided for
    /// compatibility with std::vector.
    ///
    Allocator get_allocator() const {
        return data_.get_allocator();
    }

    /// Returns the allocator associated with this Array.
    ///
    Allocator getAllocator() const {
        return get_allocator();
    }

    /// Returns a mutable reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, size() - 1].
    ///
    /// This function is equivalent to operator[], and is only provided for
    /// compatibility with std::vector.
    ///
    T& at(size_type i) {
        checkInRange_(i);
        return data_[i];
    }

    /// Returns a mutable reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, length() - 1].
    ///
    /// This function is equivalent to operator[], and is only provided for
    /// compatibility with std::vector.
    ///
    T& at(Int i) {
        checkInRange_(i);
        return data_[static_cast<size_type>(i)];
        // Note: No need for int_cast (bounds already checked)
    }

    /// Returns a const reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, size() - 1].
    ///
    /// This function is equivalent to operator[], and is only provided for
    /// compatibility with std::vector.
    ///
    const T& at(size_type i) const {
        checkInRange_(i);
        return data_[i];
    }

    /// Returns a mutable reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, length() - 1].
    ///
    /// This function is equivalent to operator[], and is only provided for
    /// compatibility with std::vector.
    ///
    const T& at(Int i) const {
        checkInRange_(i);
        return data_[static_cast<size_type>(i)];
        // Note: No need for int_cast (bounds already checked)
    }

    /// Returns a mutable reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, size() - 1].
    ///
    T& operator[](size_type i) {
        return at(i);
    }

    /// Returns a mutable reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, length() - 1].
    ///
    T& operator[](Int i) {
        return at(i);
    }

    /// Returns a const reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, size() - 1].
    ///
    const T& operator[](size_type i) const {
        return at(i);
    }

    /// Returns a mutable reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, length() - 1].
    ///
    const T& operator[](Int i) const {
        return at(i);
    }

    /// Returns a mutable reference to the element at index \p i, without
    /// bounds checking.
    ///
    /// The behavior is undefined if this Array is empty or if \p i does not
    /// belong to [0, size() - 1].
    ///
    /// Do not use this function unless you have measured and documented that
    /// the bounds checking in your particular use case was a significant
    /// performance bottleneck.
    ///
    T& atWithoutBoundsChecking(size_t i) {
        return data_[i];
    }

    /// Returns a mutable reference to the element at index \p i, without
    /// bounds checking.
    ///
    /// The behavior is undefined if this Array is empty or if \p i does not
    /// belong to [0, length() - 1].
    ///
    /// Do not use this function unless you have measured and documented that
    /// the bounds checking in your particular use case was a significant
    /// performance bottleneck.
    ///
    T& atWithoutBoundsChecking(Int i) {
        return data_[static_cast<size_type>(i)];
    }

    /// Returns a const reference to the element at index \p i, without bounds
    /// checking.
    ///
    /// The behavior is undefined if this Array is empty or if \p i does not
    /// belong to [0, size() - 1].
    ///
    /// Do not use this function unless you have measured and documented that
    /// the bounds checking in your particular use case was a significant
    /// performance bottleneck.
    ///
    const T& atWithoutBoundsChecking(size_t i) const {
        return data_[i];
    }

    /// Returns a const reference to the element at index \p i, without bounds
    /// checking.
    ///
    /// The behavior is undefined if this Array is empty or if \p i does not
    /// belong to [0, length() - 1].
    ///
    /// Do not use this function unless you have measured and documented that
    /// the bounds checking in your particular use case was a significant
    /// performance bottleneck.
    ///
    const T& atWithoutBoundsChecking(Int i) const {
        return data_[static_cast<size_type>(i)];
    }

    /// Returns a mutable reference to the first element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// This function is equivalent to first(), and is only provided for
    /// compatibility with std::vector.
    ///
    T& front() {
        if (empty()) { throw IndexError("Attempting to access first element of an empty Array"); }
        return *begin();
    }

    /// Returns a const reference to the first element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// This function is equivalent to first(), and is only provided for
    /// compatibility with std::vector.
    ///
    const T& front() const {
        if (empty()) { throw IndexError("Attempting to access first element of an empty Array"); }
        return *begin();
    }

    /// Returns a mutable reference to the first element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    T& first() {
        return front();
    }

    /// Returns a const reference to the first element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    const T& first() const {
        return front();
    }

    /// Returns a mutable reference to the last element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// This function is equivalent to last(), and is only provided for
    /// compatibility with std::vector.
    ///
    T& back() {
        if (empty()) { throw IndexError("Attempting to access last element of an empty Array"); }
        return *(end() - 1);
    }

    /// Returns a const reference to the last element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// This function is equivalent to last(), and is only provided for
    /// compatibility with std::vector.
    ///
    const T& back() const {
        if (empty()) { throw IndexError("Attempting to access last element of an empty Array"); }
        return *(end() - 1);
    }

    /// Returns a mutable reference to the last element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    T& last() {
        return back();
    }

    /// Returns a const reference to the last element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    const T& last() const {
        return back();
    }

    /// Returns a mutable reference to the underlying std::vector.
    ///
    StdVectorType& stdVector() noexcept {
        return data_;
    }

    /// Returns a const reference to the underlying std::vector.
    ///
    const StdVectorType& stdVector() const noexcept {
        return data_;
    }

    /// Returns a mutable pointer to the underlying data.
    ///
    T* data() noexcept {
        return data_.data();
    }

    /// Returns a const pointer to the underlying data.
    ///
    const T* data() const noexcept {
        return data_.data();
    }

    /// Returns an iterator to the first element in this Array.
    ///
    Iterator begin() noexcept {
        return data_.begin();
    }

    /// Returns a const iterator to the first element in this Array.
    ///
    ConstIterator begin() const noexcept {
        return data_.begin();
    }

    /// Returns a const iterator to the first element in this Array.
    ///
    ConstIterator cbegin() const noexcept {
        return data_.cbegin();
    }

    /// Returns an iterator to the past-the-last element in this Array.
    ///
    Iterator end() noexcept {
        return data_.end();
    }

    /// Returns a const iterator to the past-the-last element in this Array.
    ///
    ConstIterator end() const noexcept {
        return data_.end();
    }

    /// Returns a const iterator to the past-the-last element in this Array.
    ///
    ConstIterator cend() const noexcept {
        return data_.cend();
    }

    /// Returns a reverse iterator to the first element of the reversed
    /// Array.
    ///
    ReverseIterator rbegin() noexcept {
        return data_.rbegin();
    }

    /// Returns a const reverse iterator to the first element of the reversed
    /// Array.
    ///
    ConstReverseIterator rbegin() const noexcept {
        return data_.rbegin();
    }

    /// Returns a const reverse iterator to the first element of the reversed
    /// Array.
    ///
    ConstReverseIterator crbegin() const noexcept {
        return data_.crbegin();
    }

    /// Returns a reverse iterator to the past-the-last element of the reversed
    /// Array.
    ///
    ReverseIterator rend() noexcept {
        return data_.rend();
    }

    /// Returns a const reverse iterator to the past-the-last element of the
    /// reversed Array.
    ///
    ConstReverseIterator rend() const noexcept {
        return data_.rend();
    }

    /// Returns a const reverse iterator to the past-the-last element of the
    /// reversed Array.
    ///
    ConstReverseIterator crend() const noexcept {
        return data_.crend();
    }

    /// Returns whether this Array is empty.
    ///
    /// This function is equivalent to isEmpty(), and is only provided for
    /// compatibility with std::vector.
    ///
    bool empty() const noexcept {
        return data_.empty();
    }

    /// Returns whether this Array is empty.
    ///
    bool isEmpty() const noexcept {
        return empty();
    }

    /// Returns the number of elements in this Array as an unsigned integer.
    ///
    size_type size() const noexcept {
        return data_.size();
    }

    /// Returns the number of elements in this Array as a signed integer.
    ///
    Int length() const noexcept {
        return int_cast<Int>(data_.size());
    }

    /// Returns the maximum number of elements this Array is able to hold due
    /// to system or library implementation limitations, as an unsigned
    /// integer.
    ///
    size_type max_size() const noexcept {
        return data_.max_size();
    }

    /// Returns the maximum number of elements this Array is able to hold due
    /// to system or library implementation limitations, as a signed integer.
    ///
    Int maxLength() const noexcept {
        return int_cast<Int>(data_.max_size());
    }

    /// Increases the reservedLength() of this Array. Use this function before
    /// performing multiple append() if you know an upper bound or an estimate
    /// of the number of elements to append, in order to prevent multiple
    /// memory re-allocations.
    ///
    void reserve(size_type length) {
        data_.reserve(length);
    }

    /// Increases the reservedLength() of this Array. Use this function before
    /// performing multiple append() if you know an upper bound or an estimate
    /// of the number of elements to append, in order to prevent multiple
    /// memory re-allocations.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    ///
    void reserve(Int length) {
        data_.reserve(int_cast<size_type>(length));
    }

    /// Returns how many elements this Array can currently contain without
    /// performing any memory re-allocations, as an unsigned integer.
    ///
    size_type capacity() const noexcept {
        return data_.capacity();
    }

    /// Returns how many elements this Array can currently contain without
    /// performing any memory re-allocations, as a signed integer.
    ///
    Int reservedLength() const noexcept {
        return int_cast<Int>(data_.capacity());
    }

    /// Reclaims unused memory. Use this if the current length() of this Array
    /// is much smaller than its current reservedLength(), and you don't expect
    /// the number of elements to grow anytime soon. Indeed, by default,
    /// removing elements to an Array keeps the memory allocated in order to
    /// make adding them back efficient.
    ///
    /// This function is equivalent to shringToFit(), and is only provided for
    /// compatibility with std::vector.
    ///
    void shrink_to_fit()  {
        data_.shrink_to_fit();
    }

    /// Reclaims unused memory. Use this if the current length() of this Array
    /// is much smaller than its current reservedLength(), and you don't expect
    /// the number of elements to grow anytime soon. Indeed, by default,
    /// removing elements to an Array keeps the memory allocated in order to
    /// make adding them back efficient.
    ///
    void shrinkToFit()  {
        shrink_to_fit();
    }

    /// Removes all the elements in this Array.
    ///
    void clear() noexcept {
        data_.clear();
    }

    /// Inserts the given \p value just before the element referred to by the
    /// iterator \p it, or after the last element if `it == end()`. Returns an
    /// iterator pointing to the inserted element.
    ///
    /// Throws IndexError if \p it is not a valid iterator into this Array.
    ///
    Iterator insert(ConstIterator it, const T& value) {
        checkInRangeForInsert_(it);
        return data_.insert(it, value);
    }

    /// Inserts the given \p value just before the element at index \p i, or
    /// after the last element if `i == length()`. Returns an iterator pointing
    /// to the inserted element.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    Iterator insert(Int i, const T& value) {
        checkInRangeForInsert_(i);
        return data_.insert(toConstIterator_(i), value);
    }

    /// Move-inserts the given \p value just before the element referred to by
    /// the iterator \p it, or after the last element if `it == end()`. Returns
    /// an iterator pointing to the inserted element.
    ///
    /// Throws IndexError if \p it is not a valid iterator into this Array.
    ///
    Iterator insert(ConstIterator it, T&& value) {
        checkInRangeForInsert_(it);
        return data_.insert(it, value);
    }

    /// Move-inserts the given \p value just before the element at index \p i,
    /// or after the last element if `i == length()`. Returns an iterator
    /// pointing to the inserted element.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    Iterator insert(Int i, T&& value) {
        checkInRangeForInsert_(i);
        return data_.insert(toConstIterator_(i), value);
    }

    /// Inserts \p count copies of the given \p value just before the element
    /// referred to by the iterator \p it, or after the last element if `it ==
    /// end()`. Returns an iterator pointing to the first inserted element, or
    /// \p it if `count == 0`.
    ///
    /// Throws IndexError if \p it is not a valid iterator into this Array.
    ///
    Iterator insert(ConstIterator it, size_type count, const T& value) {
        checkInRangeForInsert_(it);
        return data_.insert(it, count, value);
    }

    /// Inserts \p count copies of the given \p value just before the element
    /// referred to by the iterator \p it, or after the last element if `it ==
    /// end()`. Returns an iterator pointing to the first inserted element, or
    /// \p it if `count == 0`.
    ///
    /// Throws IndexError if \p it is not a valid iterator into this Array.
    ///
    /// Throws NegativeIntegerError if the given \p count is negative.
    ///
    Iterator insert(ConstIterator it, Int count, const T& value) {
        checkInRangeForInsert_(it);
        return data_.insert(it, int_cast<size_type>(count), value);
    }

    /// Inserts \p count copies of the given \p value just before the element
    /// at index \p i, or after the last element if `i == length()`. Returns an
    /// iterator pointing to the first inserted element, or the element at
    /// index \p i if `count == 0`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    /// Throws NegativeIntegerError if the given \p count is negative.
    ///
    Iterator insert(Int i, Int count, const T& value) {
        checkInRangeForInsert_(i);
        return data_.insert(toConstIterator_(i), int_cast<size_type>(count), value);
    }

    /// Inserts the range given by the input iterators \p first (inclusive) and
    /// \p last (exclusive) just before the element referred to by the iterator
    /// \p it, or after the last element if `it == end()`. Returns an iterator
    /// pointing to the first inserted element, or \p it if `first == last`.
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range,
    /// or is a range into this Array.
    ///
    /// Throws IndexError if \p it is not a valid iterator into this Array.
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    Iterator insert(ConstIterator it, InputIt first, InputIt last) {
        checkInRangeForInsert_(it);
        return data_.insert(it, first, last);
    }

    /// Inserts the range given by the input iterators \p first (inclusive) and
    /// \p last (exclusive) just before the element at index \p i, or after the
    /// last element if `i == length()`. Returns an iterator pointing to the
    /// first inserted element, or the element at index \p i if `first ==
    /// last`.
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range,
    /// or is a range into this Array.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    Iterator insert(Int i, InputIt first, InputIt last) {
        checkInRangeForInsert_(i);
        return data_.insert(toConstIterator_(i), first, last);
    }

    /// Inserts the values given in the initializer list \p ilist just before
    /// the element referred to by the iterator \p it, or after the last
    /// element if `it == end()`. Returns an iterator pointing to the first
    /// inserted element, or \p it if \p ilist is empty.
    ///
    /// Throws IndexError if \p it is not a valid iterator into this Array.
    ///
    Iterator insert(ConstIterator it, std::initializer_list<T> ilist) {
        checkInRangeForInsert_(it);
        return data_.insert(it, ilist);
    }

    /// Inserts the values given in the initializer list \p ilist just before
    /// the element at index \p i, or after the last element if `i ==
    /// length()`. Returns an iterator pointing to the first inserted element,
    /// or the element at index \p i if \p ilist is empty.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    Iterator insert(Int i, std::initializer_list<T> ilist) {
        checkInRangeForInsert_(i);
        return data_.insert(toConstIterator_(i), ilist);
    }

    /// Inserts a new element, constructed from the arguments \p args, just
    /// before the element referred to by the iterator \p it, or after the last
    /// element if `it == end()`. Returns an iterator pointing to the inserted
    /// element.
    ///
    /// Throws IndexError if \p it is not a valid iterator into this Array.
    ///
    template <typename... Args>
    Iterator emplace(ConstIterator it, Args&&... args) {
        checkInRangeForInsert_(it);
        return data_.emplace(it, args...);
    }

    /// Inserts a new element, constructed from the arguments \p args, just
    /// before the element at index \p i, or after the last element if `i ==
    /// length()`. Returns an iterator pointing to the inserted element.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    template <typename... Args>
    Iterator emplace(Int i, Args&&... args) {
        checkInRangeForInsert_(i);
        return data_.emplace(toConstIterator_(i), args...);
    }

    /// Removes the element referred to by the iterator \p it. Returns an
    /// iterator to the element following the removed element, or end() if the
    /// removed element was the last element of this Array.
    ///
    /// Throws IndexError if \p it is not a valid and derefereancable iterator
    /// into this Array. Note: end() is valid, but not dereferenceable, and
    /// thus passing end() to this function would throw IndexError.
    ///
    /// This function is equivalent to remove(), and is only provided for
    /// compatibility with std::vector.
    ///
    Iterator erase(ConstIterator it) {
        checkInRange_(it);
        return data_.erase(it);
    }

    /// Removes the element at index \p i. Returns an iterator to the element
    /// following the removed element, or end() if the removed element was the
    /// last element of this Array.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, length() - 1].
    ///
    /// This function is equivalent to remove(), and is only provided for
    /// compatibility with std::vector.
    ///
    Iterator erase(Int i) {
        checkInRange_(i);
        return data_.erase(toConstIterator_(i));
    }

    /// Removes all elements in the range given by the iterators \p it1
    /// (inclusive) and \p it2 (exclusive). Returns an iterator pointing to the
    /// element pointed to by \p it2 prior to any elements being removed. If no
    /// such element exists, end() is returned.
    ///
    /// Throws IndexError if [it1, it2) isn't a valid range in this Array.
    ///
    /// This function is equivalent to remove(), and is only provided for
    /// compatibility with std::vector.
    ///
    Iterator erase(ConstIterator it1, ConstIterator it2) {
        checkInRange_(it1, it2);
        return data_.erase(it1, it2);

    }

    /// Removes all elements from index \p i1 (inclusive) to index \p i2
    /// (exclusive). Returns an iterator pointing to the element at index \p i2
    /// prior to any elements being removed. If no such element exists, end()
    /// is returned.
    ///
    /// Throws IndexError if [i1, i2) isn't a valid range in this Array, that
    /// is, if it doesn't satisfy:
    ///
    ///     0 <= i1 <= i2 <= length()
    ///
    /// This function is equivalent to remove(), and is only provided for
    /// compatibility with std::vector.
    ///
    Iterator erase(Int i1, Int i2) {
        checkInRange_(i1, i2);
        return data_.erase(toConstIterator_(i1), toConstIterator_(i2));
    }

    /// Removes the element referred to by the iterator \p it. Returns an
    /// iterator to the element following the removed element, or end() if the
    /// removed element was the last element of this Array.
    ///
    /// Throws IndexError if \p it is not a valid and derefereancable iterator
    /// into this Array. Note: end() is valid, but not dereferenceable, and
    /// thus passing end() to this function would throw IndexError.
    ///
    Iterator remove(ConstIterator it) {
        return erase(it);
    }

    /// Removes the element at index \p i. Returns an iterator to the element
    /// following the removed element, or end() if the removed element was the
    /// last element of this Array.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, length() - 1].
    ///
    Iterator remove(Int i) {
        return erase(i);
    }

    /// Removes all elements in the range given by the iterators \p it1
    /// (inclusive) and \p it2 (exclusive). Returns an iterator pointing to the
    /// element pointed to by \p it2 prior to any elements being removed. If no
    /// such element exists, end() is returned.
    ///
    /// Throws IndexError if [it1, it2) isn't a valid range in this Array.
    ///
    Iterator remove(ConstIterator it1, ConstIterator it2) {
        return erase(it1, it2);

    }

    /// Removes all elements from index \p i1 (inclusive) to index \p i2
    /// (exclusive). Returns an iterator pointing to the element at index \p i2
    /// prior to any elements being removed. If no such element exists, end()
    /// is returned.
    ///
    /// Throws IndexError if [i1, i2) isn't a valid range in this Array, that
    /// is, if it doesn't satisfy:
    ///
    ///     0 <= i1 <= i2 <= length()
    ///
    Iterator remove(Int i1, Int i2) {
        return erase(i1, i2);
    }

    /// Appends the given \p value at the end of this Array.
    ///
    /// This function is equivalent to append(), and is only provided for
    /// compatibility with std::vector.
    ///
    void push_back(const T& value) {
        data_.push_back(value);
    }

    /// Move-appends the given \p value at the end of this Array.
    ///
    /// This function is equivalent to append(), and is only provided for
    /// compatibility with std::vector.
    ///
    void push_back(T&& value) {
        data_.push_back(value);
    }

    /// Appends the given \p value at the end of this Array.
    ///
    /// This function is equivalent to append(), and is only provided for
    /// compatibility with std::vector.
    ///
    void append(const T& value) {
        push_back(value);
    }

    /// Move-appends the given \p value at the end of this Array.
    ///
    /// This function is equivalent to append(), and is only provided for
    /// compatibility with std::vector.
    ///
    void append(T&& value) {
        push_back(value);
    }

    /// Appends a new element at the end of this Array, constructed in-place
    /// from the given arguments \p args.
    ///
    template<typename... Args>
    void emplace_back(Args&&... args) {
        data_.emplace_back(args...);
    }

    /// Removes the last element of this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// This function is equivalent to removeLast(), and is only provided for
    /// compatibility with std::vector.
    ///
    void pop_back() {
        if (empty()) { throw IndexError("Attempting to remove the last element of an empty Array"); }
        data_.pop_back();
    }

    /// Removes the last element of this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    void removeLast() {
        pop_back();
    }

    /// Resizes this Array so that it contains \p count elements instead of its
    /// current size(). If \p count is smaller than the current length(), the
    /// last (size() - \p count) elements are discarded. If \p count is greater
    /// than the current size(), (\p count - size()) default-inserted elements
    /// are appended.
    ///
    void resize(size_type count) {
        data_.resize(count);
    }

    /// Resizes this Array so that it contains \p count elements instead of its
    /// current length(). If \p count is smaller than the current length(), the
    /// last (length() - \p count) elements are discarded. If \p count is
    /// greater than the current length(), (\p count - length())
    /// default-inserted elements are appended.
    ///
    /// Throws NegativeIntegerError if the given \p count is negative.
    ///
    void resize(Int count) {
        data_.resize(int_cast<size_type>(count));
    }

    /// Resizes this Array so that it contains \p count elements instead of its
    /// current size(). If \p count is smaller than the current size(), the
    /// last (size() - \p count) elements are discarded. If \p count is greater
    /// than the current size(), (\p count - size()) copies of the given \p
    /// value are appended.
    ///
    void resize(size_type count, const T& value) {
        data_.resize(count, value);
    }

    /// Resizes this Array so that it contains \p count elements instead of its
    /// current length(). If \p count is smaller than the current length(), the
    /// last (length() - \p count) elements are discarded. If \p count is
    /// greater than the current length(), (\p count - length()) copies of the
    /// given \p value are appended.
    ///
    void resize(Int count, const T& value) {
        data_.resize(int_cast<size_type>(count), value);
    }

    /// Exchanges the content of this Array with the content of the \p other
    /// Array.
    ///
    void swap(Array& other) {
        data_.swap(other.data_);
    }

private:
    std::vector<double> data_;

    // Helper cast
    ConstIterator toConstIterator_(Int i) {
        return cbegin() + int_cast<difference_type>(i);
        // Note: most likely, either Int and difference_type are the same type,
        // or the range of Int is included in the range of difference_type. So
        // the int_cast above is in fact a no-overhead, no-throw static_cast.
    }

    // Checks whether index/iterator i is valid and dereferenceable:
    //     0 <= i < size()
    //
    void checkInRange_(ConstIterator it) {
        difference_type i = std::distance(begin(), it);
        if (i < 0 || static_cast<size_type>(i) >= size()) {
            throw IndexError("Array index " + toString(i) + " out of range (array length is " +
                              toString(size()) + ")");
        }
    }
    void checkInRange_(size_type i) {
        if (i >= size()) {
            throw IndexError("Array index " + toString(i) + " out of range (array length is " +
                              toString(size()) + ")");
        }
    }
    void checkInRange_(Int i) {
        if (i < 0 || i >= length()) {
            throw IndexError("Array index " + toString(i) + " out of range (array length is " +
                              toString(length()) + ")");
        }
    }

    // Checks whether range [i1, i2) is valid:
    //     0 <= i1 <= i2 <= size()
    //
    // Note that i2 (or even i1 when i1 == i2) doesn't have to be dereferenceable.
    // In particular, [end(), end()) is a valid empty range.
    //
    void checkInRange_(ConstIterator it1, ConstIterator it2) {
        difference_type i1 = std::distance(begin(), it1);
        difference_type i2 = std::distance(begin(), it2);
        if (0 > i1 || i1 > i2 || static_cast<size_type>(i2) > size()) { // <=> not (0 <=i1 && i1 <= i2 && i2 <= size())
            throw IndexError("Array index range [" + toString(i1) + ", " + toString(i2) +
                             ") is invalid (array length is " + toString(size()) + ")");
        }
    }
    void checkInRange_(size_type i1, size_type i2) {
        if (i1 > i2 || i2 > size()) { // <=> not (i1 <= i2 && i2 <= size())
            throw IndexError("Array index range [" + toString(i1) + ", " + toString(i2) +
                             ") is invalid (array length is " + toString(size()) + ")");
        }
    }
    void checkInRange_(Int i1, Int i2) {
        if (0 > i1 || i1 > i2 || i2 > length()) { // <=> not (0 <=i1 && i1 <= i2 && i2 <= length())
            throw IndexError("Array index range [" + toString(i1) + ", " + toString(i2) +
                             ") is invalid (array length is " + toString(length()) + ")");
        }
    }

    // Checks whether index/iterator i is valid for insertion:
    //     0 <= i <= size()
    //
    // Note that i doesn't have to be dereferencable.
    // In particular, end() is a valid iterator for insertion.
    //
    void checkInRangeForInsert_(ConstIterator it) {
        difference_type i = std::distance(begin(), it);
        if (i < 0 || static_cast<size_type>(i) > size()) {
            throw IndexError("Array index " + toString(i) + " out of range for insertion (array length is " +
                              toString(size()) + ")");
        }
    }
    void checkInRangeForInsert_(size_type i) {
        if (i > size()) {
            throw IndexError("Array index " + toString(i) + " out of range for insertion (array length is " +
                              toString(size()) + ")");
        }
    }
    void checkInRangeForInsert_(Int i) {
        if (i < 0 || i > length()) {
            throw IndexError("Array index " + toString(i) + " out of range for insertion (array length is " +
                              toString(length()) + ")");
        }
    }
};

/// Returns whether the two given Arrays \p a1 and \p a2 are equal, that is,
/// whether they have the same number of elements and `a1[i] == a2[i]` for all
/// elements.
///
template<typename T, typename Allocator>
bool operator==(const Array<T, Allocator>& a1, const Array<T, Allocator>& a2) {
    return a1.stdVector() == a2.stdVector();
}

/// Returns whether the two given Arrays \p a1 and \p a2 are different, that
/// is, whether they have a different number of elements or `a1[i] != a2[i]`
/// for some elements.
///
template<typename T, typename Allocator>
bool operator!=(const Array<T, Allocator>& a1, const Array<T, Allocator>& a2) {
    return a1.stdVector() != a2.stdVector();
}

/// Compares the two Arrays \p a1 and \p a2 in lexicographic order.
///
template<typename T, typename Allocator>
bool operator<(const Array<T, Allocator>& a1, const Array<T, Allocator>& a2) {
    return a1.stdVector() < a2.stdVector();
}

/// Compares the two Arrays \p a1 and \p a2 in lexicographic order.
///
template<typename T, typename Allocator>
bool operator<=(const Array<T, Allocator>& a1, const Array<T, Allocator>& a2) {
    return a1.stdVector() <= a2.stdVector();
}

/// Compares the two Arrays \p a1 and \p a2 in inverse lexicographic order.
///
template<typename T, typename Allocator>
bool operator>(const Array<T, Allocator>& a1, const Array<T, Allocator>& a2) {
    return a1.stdVector() > a2.stdVector();
}

/// Compares the two Arrays \p a1 and \p a2 in inverse lexicographic order.
///
template<typename T, typename Allocator>
bool operator>=(const Array<T, Allocator>& a1, const Array<T, Allocator>& a2) {
    return a1.stdVector() >= a2.stdVector();
}

/// Exchanges the content of \p a1 with the content of \p a2.
///
template<typename T, typename Allocator>
void swap(const Array<T, Allocator>& a1, const Array<T, Allocator>& a2) {
    a1.swap(a2);
};

/// Returns a string representation of the given Array.
///
template<typename T, typename Allocator>
std::string toString(const Array<T, Allocator>& a) {
    return toString(a.stdVector());
}

// TODO: utility to convert from a string to an Array. Example:
//       template<typename T> parseString(const std::string& s)

} // namespace core
} // namespace vgc

#endif // VGC_CORE_ARRAY_H
