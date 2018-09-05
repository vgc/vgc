// Copyright 2018 The VGC Developers
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

#ifndef VGC_CORE_DOUBLEARRAY_H
#define VGC_CORE_DOUBLEARRAY_H

#include <cassert>
#include <string>
#include <vector>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

// XXX Currently, in a lot (most?) of places, the doc of DoubleArray (and
// Vec2dArray, etc.) says that the memory is "uninitialized", while it is in
// fact zero-initialized due to std::vector not really allowing us to have an
// uninitialized vector of a given size. Our options are:
//
// 1. Actually ensure initialization and say so in the doc.
//    Pros: closer to std::vector API
//    Cons: we might have to sacrifice performance in some cases, see below.
//
// 2. Change implementation from std::vector to custom allocations allowing
//    to have an uninitialized size-n vector
//    Const: might have to give up on the DoubleArray::stdVector() method
//
// 3. Keep it as is. We can mention (or not) in the doc of DoubleArray that
//    "currently, the memory is zero-initialized, but clients
//    shouldn't rely on it as this may change in the future."
//
// Action item:
//
// Defer decision to when the software is more finished and we have better data
// to decide which option is best. For now, we keep the zero-initialization but
// say "uninitialized" in the doc, and then we'll measure whether it is ever a
// problem. In most performance-critical code, it is possible to use reserve()
// + emplace_back() and have nearly (XXX how nearly?) the same performance as
// using a "double* a = new double[size]". However, a potential issue is that
// some algorithms may be required to fill the vector in "random" order, but
// still are guaranteed to fill all values. In these cases, you can't use the
// reserve() + push_back() idiom. In many other cases, using the idiom is
// possible, but less readable.

/// \class vgc::core::DoubleArray
/// \brief Sequence of double elements stored contiguously in memory.
///
/// DoubleArray is the preferred container type to store a variable number of
/// double elements. The elements are stored contiguously in memory, that is,
/// access to any given double from its index is very fast (constant-time
/// complexity).
///
/// # Note to Python programmers
///
/// This class is akin to a Python "list" of double objects, although it is much
/// more efficient since it doesn't store the double elements as separate Python
/// objects.
///
/// # Note to C++ programmers
///
/// This class is a thin wrapper around std::vector<double> that provides
/// additional functionality, naming conventions consistent with the rest of
/// the VGC codebase, and consistency between C++ and Python code.
///
/// This class meets the requirements of Container, AllocatorAwareContainer,
/// SequenceContainer, ContiguousContainer, and ReversibleContainer, and
/// therefore can be used with all STL algorithms that apply to std::vector.
///
/// For consistency across the codebase, always prefer to use DoubleArray over
/// std::vector<double>. If you ever need to pass it to a function that expects
/// a std::vector, you can use DoubleArray::stdVector() to get a (possibly
/// mutable) reference to the underlying std::vector.
///
class VGC_CORE_API DoubleArray
{

public:
    using StdVectorType = std::vector<double>;

    using value_type             = StdVectorType::value_type;
    using allocator_type         = StdVectorType::allocator_type;
    using size_type              = StdVectorType::size_type;
    using difference_type        = StdVectorType::difference_type;
    using reference              = StdVectorType::reference;
    using const_reference        = StdVectorType::const_reference;
    using pointer                = StdVectorType::pointer;
    using const_pointer          = StdVectorType::const_pointer;
    using iterator               = StdVectorType::iterator;
    using const_iterator         = StdVectorType::const_iterator;
    using reverse_iterator       = StdVectorType::reverse_iterator;
    using const_reverse_iterator = StdVectorType::const_reverse_iterator;

    using Iterator             = iterator;
    using ConstIterator        = const_iterator;
    using ReverseIterator      = reverse_iterator;
    using ConstReverseIterator = const_reverse_iterator;

    /// Creates an empty DoubleArray.
    ///
    DoubleArray() {

    }

    /// Creates an empty doubleArra .using the given \p allocator. This is
    /// provided for compatibility with std::vector. As of now, since
    /// DoubleArray always use std::allocator which is stateless, there is no
    /// reason why you would call this method.
    ///
    explicit DoubleArray(const allocator_type& allocator) : data_(allocator) {

    }

    /// Creates a DoubleArray of the given \p size with all values initialized
    /// to the given \p value.
    ///
    DoubleArray(int size, double value) {
        assert(size >= 0);
        data_.resize(cast_(size), value);
    }

    /// Creates a DoubleArray of the given \p size with all values initialized
    /// to the given \p value.
    ///
    DoubleArray(size_type size, double value) : data_(size, value) {

    }

    /// Creates a DoubleArray of the given \p size with all values initialized
    /// to the given \p value, and using the given \p allocator. This is
    /// provided for compatibility with std::vector. As of now, since
    /// DoubleArray always use std::allocator which is stateless, there is no
    /// reason why you would call this method.
    ///
    DoubleArray(size_type size, double value,
               const allocator_type& allocator) :
        data_(size, value, allocator) {

    }

    /// Creates an uninitialized DoubleArray of the given \p size.
    ///
    DoubleArray(int size) {
        assert(size >= 0);
        data_.resize(cast_(size));
    }

    /// Creates an uninitialized DoubleArray of the given \p size.
    ///
    DoubleArray(size_type size) : data_(size) {

    }

    /// Creates a DoubleArray initialized with the double elements in the range
    /// given by \p first and \p last, and using the given \p allocator.
    ///
    template <typename InputIt>
    DoubleArray(InputIt first, InputIt last,
               const allocator_type& allocator = allocator_type()) :
        data_(first, last, allocator) {

    }

    /// Creates a DoubleArray initialized by the values given in the initializer
    /// list \p init, and using the given \p allocator.
    ///
    DoubleArray(std::initializer_list<double> init,
               const allocator_type& allocator = allocator_type()) :
        data_(init, allocator) {

    }

    /// Replaces the content of the DoubleArray by an array of the given \p size
    /// with all values initialized to the given \p value.
    ///
    void assign(int size, double value) {
        assert(size >= 0);
        assign(cast_(size), value);
    }

    /// Replaces the content of the DoubleArray by an array of the given \p size
    /// with all values initialized to the given \p value.
    ///
    void assign(size_type size, double value) {
        data_.assign(size, value);
    }

    /// Returns the allocator associated with the underlying std::vector.
    ///
    allocator_type get_allocator() const {
        return data_.get_allocator();
    }

    /// Returns a mutable reference to the double at index \p i, with bound
    /// checking. If \p i does not belong to the range of the array, an
    /// std::out_of_range exception is thrown.
    ///
    double& at(int i) {
        return data_.at(cast_(i));
    }

    /// Returns the double at index \p i, with bound checking. If \p i does not
    /// belong to the range of the array, an std::out_of_range exception is
    /// thrown.
    ///
    double at(int i) const {
        return data_.at(cast_(i));
    }

    /// Returns a mutable reference to the double at index \p i, without bound
    /// checking. If \p i does not belong to the range of the array, the
    /// behavior is undefined.
    ///
    double& operator[](int i) {
        return data_[cast_(i)];
    }

    /// Returns the double at index \p i, without bound checking. If \p i does
    /// not belong to the range of the array, the behavior is undefined.
    ///
    double operator[](int i) const {
        return data_[cast_(i)];
    }

    /// Returns a mutable reference to the first double in this DoubleArray.
    /// If this DoubleArray is empty, the behavior is undefined.
    ///
    double& first() {
        return data_.front();
    }

    /// Returns the first double in this DoubleArray. If this DoubleArray is
    /// empty, the behavior is undefined.
    ///
    double first() const {
        return data_.front();
    }

    /// Same as first(). This is provided for compatibility with std::vector.
    ///
    double& front() {
        return first();
    }

    /// Same as first(). This is provided for compatibility with std::vector.
    ///
    double front() const {
        return first();
    }

    /// Returns a mutable reference to the last double in this DoubleArray.
    /// If this DoubleArray is empty, the behavior is undefined.
    ///
    double& last() {
        return data_.back();
    }

    /// Returns the last double in this DoubleArray. If this DoubleArray is
    /// empty, the behavior is undefined.
    ///
    double last() const {
        return data_.back();
    }

    /// Same as last(). This is provided for compatibility with std::vector.
    ///
    double& back() {
        return last();
    }

    /// Same as last(). This is provided for compatibility with std::vector.
    ///
    double back() const {
        return last();
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
    double* data() noexcept {
        return data_.data();
    }

    /// Returns a const pointer to the underlying data.
    ///
    const double* data() const noexcept {
        return data_.data();
    }

    /// Returns an iterator to the first double in this DoubleArray.
    ///
    Iterator begin() noexcept {
        return data_.begin();
    }

    /// Returns a const iterator to the first double in this DoubleArray.
    ///
    ConstIterator begin() const noexcept {
        return data_.begin();
    }

    /// Returns a const iterator to the first double in this DoubleArray.
    ///
    ConstIterator cbegin() const noexcept {
        return data_.cbegin();
    }

    /// Returns an iterator to the past-the-last double in this DoubleArray.
    ///
    Iterator end() noexcept {
        return data_.end();
    }

    /// Returns a const iterator to the past-the-last double in this DoubleArray.
    ///
    ConstIterator end() const noexcept {
        return data_.end();
    }

    /// Returns a const iterator to the past-the-last double in this DoubleArray.
    ///
    ConstIterator cend() const noexcept {
        return data_.cend();
    }

    /// Returns a reverse iterator to the first double of the reversed
    /// DoubleArray.
    ///
    ReverseIterator rbegin() noexcept {
        return data_.rbegin();
    }

    /// Returns a const reverse iterator to the first double of the reversed
    /// DoubleArray.
    ///
    ConstReverseIterator rbegin() const noexcept {
        return data_.rbegin();
    }

    /// Returns a const reverse iterator to the first double of the reversed
    /// DoubleArray.
    ///
    ConstReverseIterator crbegin() const noexcept {
        return data_.crbegin();
    }

    /// Returns a reverse iterator to the past-the-last double of the reversed
    /// DoubleArray.
    ///
    ReverseIterator rend() noexcept {
        return data_.rend();
    }

    /// Returns a const reverse iterator to the past-the-last double of the
    /// reversed DoubleArray.
    ///
    ConstReverseIterator rend() const noexcept {
        return data_.rend();
    }

    /// Returns a const reverse iterator to the past-the-last double of the
    /// reversed DoubleArray.
    ///
    ConstReverseIterator crend() const noexcept {
        return data_.crend();
    }

    /// Returns whether this DoubleArray is empty, that is, whether it contains
    /// no double at all.
    ///
    bool isEmpty() const noexcept {
        return data_.empty();
    }

    /// Same as isEmpty(). This is provided for compatibility with std::vector.
    ///
    bool empty() const noexcept {
        return isEmpty();
    }

    /// Returns the number of double in this DoubleArray.
    ///
    size_type size() const noexcept {
        return data_.size();
    }

    /// Returns the maximum number of double this DoubleArray is able to hold due
    /// to system or library implementation limitations.
    ///
    size_type max_size() const noexcept {
        return data_.max_size();
    }

    /// Increases the capacity() of this DoubleArray. Use this function before
    /// performing multiple append() if you know an upper bound or an estimate
    /// of the number of elements to append, in order to prevent multiple
    /// memory re-allocations.
    ///
    void reserve(int capacity) {
        assert(capacity >= 0);
        data_.reserve(cast_(capacity));
    }

    /// Increases the capacity() of this DoubleArray. Use this function before
    /// performing multiple append() if you know an upper bound or an estimate
    /// of the number of elements to append, in order to prevent multiple
    /// memory re-allocations.
    ///
    void reserve(size_type capacity) {
        data_.reserve(capacity);
    }

    /// Returns how many double this DoubleArray can currently contain without
    /// performing any memory re-allocations.
    ///
    size_type capacity() const noexcept {
        return data_.capacity();
    }

    /// Reclaims unused memory. Use this if the current size() of this
    /// DoubleArray is much smaller than its current capacity(), and you don't
    /// expect the number of elements to grow anytime soon. Indeed, by default,
    /// removing elements to a DoubleArray keeps the memory allocated in order
    /// to make adding them back efficient.
    ///
    void shrinkToFit()  {
        data_.shrink_to_fit();
    }

    /// Same as shrinkToFit(). This is provided for compatibility with
    /// std::vector.
    ///
    void shrink_to_fit()  {
        shrinkToFit();
    }

    /// Removes all the double elements in this DoubleArray.
    ///
    void clear() noexcept {
        data_.clear();
    }

    /// Insert the given double \p value at index \p i, copying all subsequent
    /// elements up one index. Returns an iterator pointing to the inserted
    /// double. The behavior is undefined if i is out of bounds.
    ///
    Iterator insert(int i, double value) {
        Iterator pos = begin() + i;
        return data_.insert(pos, value);
    }

    /// Insert the given double \p value at index \p i, copying all subsequent
    /// elements up one index. Returns an iterator pointing to the inserted
    /// double. The behavior is undefined if i is out of bounds.
    ///
    Iterator insert(size_type i, double value) {
        Iterator pos = begin() + i;
        return data_.insert(pos, value);
    }

    /// Insert the given double \p value before the double referred to by the
    /// iterator \p pos. Returns an iterator pointing to the inserted double.
    ///
    /// Note: we do not provide the equivalent std::vector function taking \p
    /// value by rvalue reference since moving a double is not faster than
    /// copying it.
    ///
    Iterator insert(ConstIterator pos, double value) {
        return data_.insert(pos, value);
    }

    /// Insert \p n copies of the given double \p value at index \p i, copying
    /// all subsequent elements up \p n index. Returns an iterator pointing to
    /// the first double inserted, or to the existing double at index \p i if \p
    /// n == 0. The behavior is undefined if i is out of bounds.
    ///
    Iterator insert(int i, int n, double value) {
        assert(n >= 0);
        Iterator pos = begin() + i;
        return data_.insert(pos, cast_(n), value);
    }

    /// Insert \p n copies of the given double \p value at index \p i, copying
    /// all subsequent elements up \p n index. Returns an iterator pointing to
    /// the first double inserted, or to the existing double at index \p i if \p
    /// n == 0. The behavior is undefined if i is out of bounds.
    ///
    Iterator insert(size_type i, size_type n, double value) {
        Iterator pos = begin() + i;
        return data_.insert(pos, n, value);
    }

    /// Insert \p n copies of the given double \p value before the double
    /// referred to by the iterator \p pos. Returns an iterator pointing to the
    /// first double inserted, or \p pos if \p n == 0.
    ///
    Iterator insert(ConstIterator pos, int n, double value) {
        assert(n >= 0);
        return data_.insert(pos, cast_(n), value);
    }

    /// Insert \p n copies of the given double \p value before the double
    /// referred to by the iterator \p pos. Returns an iterator pointing to the
    /// first double inserted, or \p pos if \p n == 0.
    ///
    Iterator insert(ConstIterator pos, size_type n, double value) {
        return data_.insert(pos, n, value);
    }

    /// Insert the double elements in the range given by \p first and \p last at
    /// index \p i. Returns an iterator pointing to the first double inserted,
    /// or to the existing double at index \p i if the range is empty. The
    /// behavior is undefined if i is out of bounds.
    ///
    template <typename InputIt>
    Iterator insert(int i, InputIt first, InputIt last) {
        Iterator pos = begin() + i;
        return data_.insert(pos, first, last);
    }

    /// Insert the double elements in the range given by \p first and \p last at
    /// index \p i. Returns an iterator pointing to the first double inserted,
    /// or to the existing double at index \p i if the range is empty. The
    /// behavior is undefined if i is out of bounds.
    ///
    template <typename InputIt>
    Iterator insert(size_type i, InputIt first, InputIt last) {
        Iterator pos = begin() + i;
        return data_.insert(pos, first, last);
    }

    /// Insert the double elements in the range given by \p first and \p last
    /// before the double referred to by the iterator \p pos. Returns an
    /// iterator pointing to the first double inserted, or \p pos if the range
    /// is empty.
    ///
    template <typename InputIt>
    Iterator insert(ConstIterator pos, InputIt first, InputIt last) {
        return data_.insert(pos, first, last);
    }

    /// Insert all the double elements in the initializer list \p ilist at index
    /// \p i. Returns an iterator pointing to the first double inserted, or to
    /// the existing double at index \p i if \p ilist is empty. The behavior is
    /// undefined if i is out of bounds.
    ///
    Iterator insert(int i, std::initializer_list<double> ilist) {
        Iterator pos = begin() + i;
        return data_.insert(pos, ilist);
    }

    /// Insert all the double elements in the initializer list \p ilist at index
    /// \p i. Returns an iterator pointing to the first double inserted, or to
    /// the existing double at index \p i if \p ilist is empty. The behavior is
    /// undefined if i is out of bounds.
    ///
    Iterator insert(size_type i, std::initializer_list<double> ilist) {
        Iterator pos = begin() + i;
        return data_.insert(pos, ilist);
    }

    /// Insert all the double elements in the initializer list \p ilist before
    /// the double referred to by the iterator \p pos. Returns an iterator
    /// pointing to the first double inserted, or \p pos if \p ilist is empty.
    ///
    Iterator insert(ConstIterator pos, std::initializer_list<double> ilist) {
        return data_.insert(pos, ilist);
    }

    /// Insert a new double constructed from the arguments \p args before the
    /// double referred to by the iterator \p pos. Returns an iterator pointing
    /// to the inserted double. This is provided for compatibility with
    /// std::vector. Prefer using insert().
    ///
    template <typename... Args>
    Iterator emplace(ConstIterator pos, Args&&... args) {
        return data_.emplace(pos, args...);
    }

    /// Removes the double at index \p i. Returns an iterator to the double
    /// following the removed double, or end() if the removed double was the last
    /// element of the DoubleArray. The behavior is undefined if i is out of bounds.
    ///
    Iterator remove(int i) {
        Iterator pos = begin() + i;
        return data_.erase(pos);
    }

    /// Removes the double at index \p i. Returns an iterator to the double
    /// following the removed double, or end() if the removed double was the last
    /// element of the DoubleArray. The behavior is undefined if i is out of
    /// bounds.
    ///
    Iterator remove(size_type i) {
        Iterator pos = begin() + i;
        return data_.erase(pos);
    }

    /// Removes the double referred to by the iterator \p pos. Returns an
    /// iterator to the double following the removed double, or end() if the
    /// removed double was the last element of the DoubleArray.
    ///
    Iterator remove(ConstIterator pos) {
        return data_.erase(pos);
    }

    /// Same as remove(). This is provided for compatibility with std::vector.
    ///
    Iterator erase(ConstIterator pos) {
        return remove(pos);
    }

    /// Removes all double elements from index \p i (inclusive) to index \p j
    /// (exclusive). Returns an iterator to the last double removed, or end() if
    /// the last double removed was the last element of the DoubleArray. The
    /// behavior is undefined if i or j are out of bounds, or if i > j.
    ///
    Iterator remove(int i, int j) {
        Iterator first = begin() + i;
        Iterator last = begin() + j;
        return data_.erase(first, last);
    }

    /// Removes all double elements from index \p i (inclusive) to index \p j
    /// (exclusive). Returns an iterator to the last double removed, or end() if
    /// the last double removed was the last element of the DoubleArray. The
    /// behavior is undefined if i or j are out of bounds, or if i > j.
    ///
    Iterator remove(size_type i, size_type j) {
        Iterator first = begin() + i;
        Iterator last = begin() + j;
        return data_.erase(first, last);
    }

    /// Removes all double elements in the range given by \p first (inclusive)
    /// and \p last (exclusive). Returns an iterator to the last double removed,
    /// or end() if the last double removed was the last element of the
    /// DoubleArray.
    ///
    Iterator remove(ConstIterator first, ConstIterator last) {
        return data_.erase(first, last);
    }

    /// Same as remove(). This is provided for compatibility with std::vector.
    ///
    Iterator erase(ConstIterator first, ConstIterator last) {
        return remove(first, last);
    }

    /// Appends the given double \p value to the end of the DoubleArray.
    ///
    void append(double value) {
        data_.push_back(value);
    }

    /// Same as append(). This is provided for compatibility with std::vector.
    ///
    /// Note: we do not provide the equivalent std::vector function taking \p
    /// value by rvalue reference since moving a double is not faster than
    /// copying it.
    ///
    void push_back(double value ) {
        append(value);
    }

    /// Appends a new double constructed from the arguments \p args to the end
    /// of the DoubleArray. This is provided for compatibility with std::vector.
    /// Prefer using append().
    ///
    template <typename... Args>
    void emplace_back(Args&&... args) {
        data_.emplace_back(args...);
    }

    /// Removes the last double of this DoubleArray. If this DoubleArray is empty,
    /// the behavior is undefined.
    ///
    void removeLast() {
        data_.pop_back();
    }

    /// Same as removeLast(). This is provided for compatibility with std::vector.
    ///
    void pop_back() {
        removeLast();
    }

    /// Resizes to DoubleArray so that it contains \p count double elements
    /// instead of its current size(). If \p count is smaller than the current
    /// size(), the last (size() - \p count) double elements are discarded. If
    /// \p count is greater than the current size(), (\p count - size())
    /// uninitialized double elements are appended.
    ///
    void resize(int count) {
        assert(count >= 0);
        data_.resize(cast_(count));
    }

    /// Resizes to DoubleArray so that it contains \p count double elements
    /// instead of its current size(). If \p count is smaller than the current
    /// size(), the last (size() - \p count) double elements are discarded. If
    /// \p count is greater than the current size(), (\p count - size())
    /// uninitialized double elements are appended.
    ///
    void resize(size_type count) {
        data_.resize(count);
    }

    /// Resizes to DoubleArray so that it contains \p count double elements
    /// instead of its current size(). If \p count is smaller than the current
    /// size(), the last (size() - \p count) double elements are discarded. If
    /// \p count is greater than the current size(), (\p count - size())
    /// copies of the given double \p value are appended.
    ///
    void resize(int count, double value) {
        assert(count >= 0);
        data_.resize(cast_(count), value);
    }

    /// Resizes to DoubleArray so that it contains \p count double elements
    /// instead of its current size(). If \p count is smaller than the current
    /// size(), the last (size() - \p count) double elements are discarded. If
    /// \p count is greater than the current size(), (\p count - size())
    /// copies of the given double \p value are appended.
    ///
    void resize(size_type count, double value) {
        data_.resize(count, value);
    }

    /// Exchanges the content of this DoubleArray with the content of the \p other
    /// DoubleArray.
    ///
    void swap(DoubleArray& other) {
        data_.swap(other.data_);
    }

    /// Returns whether the two given DoubleArray \p a1 and \p a2 are equal,
    /// that is, whether they have the same size and `a1[i] == a2[i]` for all
    /// valid i indices.
    ///
    friend bool operator==(const DoubleArray& a1, const DoubleArray& a2) {
        return a1.data_ == a2.data_;
    }

    /// Returns whether the two given DoubleArray \p a1 and \p a2 are different,
    /// that is, whether they have different size or `a1[i] == a2[i]` for some
    /// valid i index.
    ///
    friend bool operator!=(const DoubleArray& a1, const DoubleArray& a2) {
        return a1.data_ != a2.data_;
    }

    /// Compares the two DoubleArray \p a1 and \p a2 using the lexicographic
    /// order.
    ///
    friend bool operator<(const DoubleArray& a1, const DoubleArray& a2) {
        return a1.data_ < a2.data_;
    }

    /// Compares the two DoubleArray \p a1 and \p a2 using the lexicographic
    /// order.
    ///
    friend bool operator<=(const DoubleArray& a1, const DoubleArray& a2) {
        return a1.data_ <= a2.data_;
    }

    /// Compares the two DoubleArray \p a1 and \p a2 using the lexicographic
    /// order.
    ///
    friend bool operator>(const DoubleArray& a1, const DoubleArray& a2) {
        return a1.data_ > a2.data_;
    }

    /// Compares the two DoubleArray \p a1 and \p a2 using the lexicographic
    /// order.
    ///
    friend bool operator>=(const DoubleArray& a1, const DoubleArray& a2) {
        return a1.data_ >= a2.data_;
    }

private:
    std::vector<double> data_;

    static size_type cast_(int i) {
        return static_cast<size_type>(i);
    }
};

/// Exchanges the content of \p lhs with the content of \p rhs.
///
inline void swap(DoubleArray& a1, DoubleArray& a2) {
    a1.swap(a2);
};

/// Returns a string representation of the given DoubleArray.
///
VGC_CORE_API
std::string toString(const DoubleArray& a);

/// Converts the given string into a DoubleArray. Raises ParseError if the given
/// string does not represent a DoubleArray.
///
VGC_CORE_API
DoubleArray toDoubleArray(const std::string& s);

} // namespace core
} // namespace vgc

#endif // VGC_CORE_DOUBLEARRAY_H
