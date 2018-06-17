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

#ifndef VGC_CORE_VEC2DARRAY_H
#define VGC_CORE_VEC2DARRAY_H

#include <cassert>
#include <string>
#include <vector>
#include <vgc/core/api.h>
#include <vgc/core/vec2d.h>

namespace vgc {
namespace core {

/// \class vgc::core::Vec2dArray
/// \brief Sequence of Vec2d elements stored contiguously in memory.
///
/// Vec2dArray is the preferred container type to store a variable number of
/// Vec2d elements. The elements are stored contiguously in memory, that is,
/// access to any given Vec2d from its index is very fast (constant-time
/// complexity).
///
/// # Note to Python programmers
///
/// This class is akin to a Python "list" of Vec2d objects, although it is much
/// more efficient since it doesn't store the Vec2d elements as separate Python
/// objects.
///
/// # Note to C++ programmers
///
/// This class is a thin wrapper around std::vector<Vec2d> that provides
/// additional functionality, naming conventions consistent with the rest of
/// the VGC codebase, and consistency between C++ and Python code.
///
/// This class meets the requirements of Container, AllocatorAwareContainer,
/// SequenceContainer, ContiguousContainer, and ReversibleContainer, and
/// therefore can be used with all STL algorithms that apply to std::vector.
///
/// For consistency across the codebase, always prefer to use Vec2dArray over
/// std::vector<Vec2d>. If you ever need to pass it to a function that expects
/// a std::vector, you can use Vec2dArray::stdVector() to get a (possibly
/// mutable) reference to the underlying std::vector.
///
class VGC_CORE_API Vec2dArray
{

public:
    using StdVectorType = std::vector<Vec2d>;

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

    /// Creates an empty Vec2dArray.
    ///
    Vec2dArray() {

    }

    /// Creates an empty Vec2dArra .using the given \p allocator. This is
    /// provided for compatibility with std::vector. As of now, since
    /// Vec2dArray always use std::allocator which is stateless, there is no
    /// reason why you would call this method.
    ///
    explicit Vec2dArray(const allocator_type& allocator) : data_(allocator) {

    }

    /// Creates a Vec2dArray of the given \p size with all values initialized
    /// to the given \p value.
    ///
    Vec2dArray(int size, const Vec2d& value) {
        assert(size >= 0);
        data_.resize(cast_(size), value);
    }

    /// Creates a Vec2dArray of the given \p size with all values initialized
    /// to the given \p value.
    ///
    Vec2dArray(size_type size, const Vec2d& value) : data_(size, value) {

    }

    /// Creates a Vec2dArray of the given \p size with all values initialized
    /// to the given \p value, and using the given \p allocator. This is
    /// provided for compatibility with std::vector. As of now, since
    /// Vec2dArray always use std::allocator which is stateless, there is no
    /// reason why you would call this method.
    ///
    Vec2dArray(size_type size, const Vec2d& value,
               const allocator_type& allocator) :
        data_(size, value, allocator) {

    }

    /// Creates an uninitialized Vec2dArray of the given \p size.
    ///
    Vec2dArray(int size) {
        assert(size >= 0);
        data_.resize(cast_(size));
    }

    /// Creates an uninitialized Vec2dArray of the given \p size.
    ///
    Vec2dArray(size_type size) : data_(size) {

    }

    /// Creates a Vec2dArray initialized with the Vec2d elements in the range
    /// given by \p first and \p last, and using the given \p allocator.
    ///
    template <typename InputIt>
    Vec2dArray(InputIt first, InputIt last,
               const allocator_type& allocator = allocator_type()) :
        data_(first, last, allocator) {

    }

    /// Creates a Vec2dArray initialized by the values given in the initializer
    /// list \p init, and using the given \p allocator.
    ///
    Vec2dArray(std::initializer_list<Vec2d> init,
               const allocator_type& allocator = allocator_type()) :
        data_(init, allocator) {

    }

    /// Replaces the content of the Vec2dArray by an array of the given \p size
    /// with all values initialized to the given \p value.
    ///
    void assign(int size, const Vec2d& value) {
        assert(size >= 0);
        assign(cast_(size), value);
    }

    /// Replaces the content of the Vec2dArray by an array of the given \p size
    /// with all values initialized to the given \p value.
    ///
    void assign(size_type size, const Vec2d& value) {
        data_.assign(size, value);
    }

    /// Returns the allocator associated with the underlying std::vector.
    ///
    allocator_type get_allocator() const {
        return data_.get_allocator();
    }

    /// Returns a mutable reference to the Vec2d at index \p i, with bound
    /// checking. If \p i does not belong to the range of the array, an
    /// std::out_of_range exception is thrown.
    ///
    Vec2d& at(int i) {
        return data_.at(cast_(i));
    }

    /// Returns a const reference to the Vec2d at index \p i, with bound
    /// checking. If \p i does not belong to the range of the array, an
    /// std::out_of_range exception is thrown.
    ///
    const Vec2d& at(int i) const {
        return data_.at(cast_(i));
    }

    /// Returns a mutable reference to the Vec2d at index \p i, without bound
    /// checking. If \p i does not belong to the range of the array, the
    /// behavior is undefined.
    ///
    Vec2d& operator[](int i) {
        return data_[cast_(i)];
    }

    /// Returns a const reference to the Vec2d at index \p i, without bound
    /// checking. If \p i does not belong to the range of the array, the
    /// behavior is undefined.
    ///
    const Vec2d& operator[](int i) const {
        return data_[cast_(i)];
    }

    /// Returns a mutable reference to the first Vec2d in this Vec2dArray.
    /// If this Vec2dArray is empty, the behavior is undefined.
    ///
    Vec2d& first() {
        return data_.front();
    }

    /// Returns a const reference to the first Vec2d in this Vec2dArray. If
    /// this Vec2dArray is empty, the behavior is undefined.
    ///
    const Vec2d& first() const {
        return data_.front();
    }

    /// Same as first(). This is provided for compatibility with std::vector.
    ///
    Vec2d& front() {
        return first();
    }

    /// Same as first(). This is provided for compatibility with std::vector.
    ///
    const Vec2d& front() const {
        return first();
    }

    /// Returns a mutable reference to the last Vec2d in this Vec2dArray.
    /// If this Vec2dArray is empty, the behavior is undefined.
    ///
    Vec2d& last() {
        return data_.back();
    }

    /// Returns a const reference to the last Vec2d in this Vec2dArray. If
    /// this Vec2dArray is empty, the behavior is undefined.
    ///
    const Vec2d& last() const {
        return data_.back();
    }

    /// Same as last(). This is provided for compatibility with std::vector.
    ///
    Vec2d& back() {
        return last();
    }

    /// Same as last(). This is provided for compatibility with std::vector.
    ///
    const Vec2d& back() const {
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
    Vec2d* data() noexcept {
        return data_.data();
    }

    /// Returns a const pointer to the underlying data.
    ///
    const Vec2d* data() const noexcept {
        return data_.data();
    }

    /// Returns an iterator to the first Vec2d in this Vec2dArray.
    ///
    Iterator begin() noexcept {
        return data_.begin();
    }

    /// Returns a const iterator to the first Vec2d in this Vec2dArray.
    ///
    ConstIterator begin() const noexcept {
        return data_.begin();
    }

    /// Returns a const iterator to the first Vec2d in this Vec2dArray.
    ///
    ConstIterator cbegin() const noexcept {
        return data_.cbegin();
    }

    /// Returns an iterator to the past-the-last Vec2d in this Vec2dArray.
    ///
    Iterator end() noexcept {
        return data_.end();
    }

    /// Returns a const iterator to the past-the-last Vec2d in this Vec2dArray.
    ///
    ConstIterator end() const noexcept {
        return data_.end();
    }

    /// Returns a const iterator to the past-the-last Vec2d in this Vec2dArray.
    ///
    ConstIterator cend() const noexcept {
        return data_.cend();
    }

    /// Returns a reverse iterator to the first Vec2d of the reversed
    /// Vec2dArray.
    ///
    ReverseIterator rbegin() noexcept {
        return data_.rbegin();
    }

    /// Returns a const reverse iterator to the first Vec2d of the reversed
    /// Vec2dArray.
    ///
    ConstReverseIterator rbegin() const noexcept {
        return data_.rbegin();
    }

    /// Returns a const reverse iterator to the first Vec2d of the reversed
    /// Vec2dArray.
    ///
    ConstReverseIterator crbegin() const noexcept {
        return data_.crbegin();
    }

    /// Returns a reverse iterator to the past-the-last Vec2d of the reversed
    /// Vec2dArray.
    ///
    ReverseIterator rend() noexcept {
        return data_.rend();
    }

    /// Returns a const reverse iterator to the past-the-last Vec2d of the
    /// reversed Vec2dArray.
    ///
    ConstReverseIterator rend() const noexcept {
        return data_.rend();
    }

    /// Returns a const reverse iterator to the past-the-last Vec2d of the
    /// reversed Vec2dArray.
    ///
    ConstReverseIterator crend() const noexcept {
        return data_.crend();
    }

    /// Returns whether this Vec2dArray is empty, that is, whether it contains
    /// no Vec2d at all.
    ///
    bool isEmpty() const noexcept {
        return data_.empty();
    }

    /// Same as isEmpty(). This is provided for compatibility with std::vector.
    ///
    bool empty() const noexcept {
        return isEmpty();
    }

    /// Returns the number of Vec2d in this Vec2dArray.
    ///
    size_type size() const noexcept {
        return data_.size();
    }

    /// Returns the maximum number of Vec2d this Vec2dArray is able to hold due
    /// to system or library implementation limitations.
    ///
    size_type max_size() const noexcept {
        return data_.max_size();
    }

    /// Increases the capacity() of this Vec2dArray. Use this function before
    /// performing multiple append() if you know an upper bound or an estimate
    /// of the number of elements to append, in order to prevent multiple
    /// memory re-allocations.
    ///
    void reserve(int capacity) {
        assert(capacity >= 0);
        data_.reserve(cast_(capacity));
    }

    /// Increases the capacity() of this Vec2dArray. Use this function before
    /// performing multiple append() if you know an upper bound or an estimate
    /// of the number of elements to append, in order to prevent multiple
    /// memory re-allocations.
    ///
    void reserve(size_type capacity) {
        data_.reserve(capacity);
    }

    /// Returns how many Vec2d this Vec2dArray can currently contain without
    /// performing any memory re-allocations.
    ///
    size_type capacity() const noexcept {
        return data_.capacity();
    }

    /// Reclaims unused memory. Use this if the current size() of this
    /// Vec2dArray is much smaller than its current capacity(), and you don't
    /// expect the number of elements to grow anytime soon. Indeed, by default,
    /// removing elements to a Vec2dArray keeps the memory allocated in order
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

    /// Removes all the Vec2d elements in this Vec2dArray.
    ///
    void clear() noexcept {
        data_.clear();
    }

    /// Insert the given Vec2d \p value at index \p i, copying all subsequent
    /// elements up one index. Returns an iterator pointing to the inserted
    /// Vec2d. The behavior is undefined if i is out of bounds.
    ///
    Iterator insert(int i, const Vec2d& value) {
        Iterator pos = begin() + i;
        return data_.insert(pos, value);
    }

    /// Insert the given Vec2d \p value at index \p i, copying all subsequent
    /// elements up one index. Returns an iterator pointing to the inserted
    /// Vec2d. The behavior is undefined if i is out of bounds.
    ///
    Iterator insert(size_type i, const Vec2d& value) {
        Iterator pos = begin() + i;
        return data_.insert(pos, value);
    }

    /// Insert the given Vec2d \p value before the Vec2d referred to by the
    /// iterator \p pos. Returns an iterator pointing to the inserted Vec2d.
    ///
    /// Note: we do not provide the equivalent std::vector function taking \p
    /// value by rvalue reference since moving a Vec2d is not faster than
    /// copying it.
    ///
    Iterator insert(ConstIterator pos, const Vec2d& value) {
        return data_.insert(pos, value);
    }

    /// Insert \p n copies of the given Vec2d \p value at index \p i, copying
    /// all subsequent elements up \p n index. Returns an iterator pointing to
    /// the first Vec2d inserted, or to the existing Vec2d at index \p i if \p
    /// n == 0. The behavior is undefined if i is out of bounds.
    ///
    Iterator insert(int i, int n, const Vec2d& value) {
        assert(n >= 0);
        Iterator pos = begin() + i;
        return data_.insert(pos, cast_(n), value);
    }

    /// Insert \p n copies of the given Vec2d \p value at index \p i, copying
    /// all subsequent elements up \p n index. Returns an iterator pointing to
    /// the first Vec2d inserted, or to the existing Vec2d at index \p i if \p
    /// n == 0. The behavior is undefined if i is out of bounds.
    ///
    Iterator insert(size_type i, size_type n, const Vec2d& value) {
        Iterator pos = begin() + i;
        return data_.insert(pos, n, value);
    }

    /// Insert \p n copies of the given Vec2d \p value before the Vec2d
    /// referred to by the iterator \p pos. Returns an iterator pointing to the
    /// first Vec2d inserted, or \p pos if \p n == 0.
    ///
    Iterator insert(ConstIterator pos, int n, const Vec2d& value) {
        assert(n >= 0);
        return data_.insert(pos, cast_(n), value);
    }

    /// Insert \p n copies of the given Vec2d \p value before the Vec2d
    /// referred to by the iterator \p pos. Returns an iterator pointing to the
    /// first Vec2d inserted, or \p pos if \p n == 0.
    ///
    Iterator insert(ConstIterator pos, size_type n, const Vec2d& value) {
        return data_.insert(pos, n, value);
    }

    /// Insert the Vec2d elements in the range given by \p first and \p last at
    /// index \p i. Returns an iterator pointing to the first Vec2d inserted,
    /// or to the existing Vec2d at index \p i if the range is empty. The
    /// behavior is undefined if i is out of bounds.
    ///
    template <typename InputIt>
    Iterator insert(int i, InputIt first, InputIt last) {
        Iterator pos = begin() + i;
        return data_.insert(pos, first, last);
    }

    /// Insert the Vec2d elements in the range given by \p first and \p last at
    /// index \p i. Returns an iterator pointing to the first Vec2d inserted,
    /// or to the existing Vec2d at index \p i if the range is empty. The
    /// behavior is undefined if i is out of bounds.
    ///
    template <typename InputIt>
    Iterator insert(size_type i, InputIt first, InputIt last) {
        Iterator pos = begin() + i;
        return data_.insert(pos, first, last);
    }

    /// Insert the Vec2d elements in the range given by \p first and \p last
    /// before the Vec2d referred to by the iterator \p pos. Returns an
    /// iterator pointing to the first Vec2d inserted, or \p pos if the range
    /// is empty.
    ///
    template <typename InputIt>
    Iterator insert(ConstIterator pos, InputIt first, InputIt last) {
        return data_.insert(pos, first, last);
    }

    /// Insert all the Vec2d elements in the initializer list \p ilist at index
    /// \p i. Returns an iterator pointing to the first Vec2d inserted, or to
    /// the existing Vec2d at index \p i if \p ilist is empty. The behavior is
    /// undefined if i is out of bounds.
    ///
    Iterator insert(int i, std::initializer_list<Vec2d> ilist) {
        Iterator pos = begin() + i;
        return data_.insert(pos, ilist);
    }

    /// Insert all the Vec2d elements in the initializer list \p ilist at index
    /// \p i. Returns an iterator pointing to the first Vec2d inserted, or to
    /// the existing Vec2d at index \p i if \p ilist is empty. The behavior is
    /// undefined if i is out of bounds.
    ///
    Iterator insert(size_type i, std::initializer_list<Vec2d> ilist) {
        Iterator pos = begin() + i;
        return data_.insert(pos, ilist);
    }

    /// Insert all the Vec2d elements in the initializer list \p ilist before
    /// the Vec2d referred to by the iterator \p pos. Returns an iterator
    /// pointing to the first Vec2d inserted, or \p pos if \p ilist is empty.
    ///
    Iterator insert(ConstIterator pos, std::initializer_list<Vec2d> ilist) {
        return data_.insert(pos, ilist);
    }

    /// Insert a new Vec2d constructed from the arguments \p args before the
    /// Vec2d referred to by the iterator \p pos. Returns an iterator pointing
    /// to the inserted Vec2d. This is provided for compatibility with
    /// std::vector. Prefer using insert().
    ///
    template <typename... Args>
    Iterator emplace(ConstIterator pos, Args&&... args) {
        return data_.emplace(pos, args...);
    }

    /// Removes the Vec2d at index \p i. Returns an iterator to the Vec2d
    /// following the removed Vec2d, or end() if the removed Vec2d was the last
    /// element of the Vec2dArray. The behavior is undefined if i is out of bounds.
    ///
    Iterator remove(int i) {
        Iterator pos = begin() + i;
        return data_.erase(pos);
    }

    /// Removes the Vec2d at index \p i. Returns an iterator to the Vec2d
    /// following the removed Vec2d, or end() if the removed Vec2d was the last
    /// element of the Vec2dArray. The behavior is undefined if i is out of
    /// bounds.
    ///
    Iterator remove(size_type i) {
        Iterator pos = begin() + i;
        return data_.erase(pos);
    }

    /// Removes the Vec2d referred to by the iterator \p pos. Returns an
    /// iterator to the Vec2d following the removed Vec2d, or end() if the
    /// removed Vec2d was the last element of the Vec2dArray.
    ///
    Iterator remove(ConstIterator pos) {
        return data_.erase(pos);
    }

    /// Same as remove(). This is provided for compatibility with std::vector.
    ///
    Iterator erase(ConstIterator pos) {
        return remove(pos);
    }

    /// Removes all Vec2d elements from index \p i (inclusive) to index \p j
    /// (exclusive). Returns an iterator to the last Vec2d removed, or end() if
    /// the last Vec2d removed was the last element of the Vec2dArray. The
    /// behavior is undefined if i or j are out of bounds, or if i > j.
    ///
    Iterator remove(int i, int j) {
        Iterator first = begin() + i;
        Iterator last = begin() + j;
        return data_.erase(first, last);
    }

    /// Removes all Vec2d elements from index \p i (inclusive) to index \p j
    /// (exclusive). Returns an iterator to the last Vec2d removed, or end() if
    /// the last Vec2d removed was the last element of the Vec2dArray. The
    /// behavior is undefined if i or j are out of bounds, or if i > j.
    ///
    Iterator remove(size_type i, size_type j) {
        Iterator first = begin() + i;
        Iterator last = begin() + j;
        return data_.erase(first, last);
    }

    /// Removes all Vec2d elements in the range given by \p first (inclusive)
    /// and \p last (exclusive). Returns an iterator to the last Vec2d removed,
    /// or end() if the last Vec2d removed was the last element of the
    /// Vec2dArray.
    ///
    Iterator remove(ConstIterator first, ConstIterator last) {
        return data_.erase(first, last);
    }

    /// Same as remove(). This is provided for compatibility with std::vector.
    ///
    Iterator erase(ConstIterator first, ConstIterator last) {
        return remove(first, last);
    }

    /// Appends the given Vec2d \p value to the end of the Vec2dArray.
    ///
    void append(const Vec2d& value) {
        data_.push_back(value);
    }

    /// Same as append(). This is provided for compatibility with std::vector.
    ///
    /// Note: we do not provide the equivalent std::vector function taking \p
    /// value by rvalue reference since moving a Vec2d is not faster than
    /// copying it.
    ///
    void push_back(const Vec2d& value ) {
        append(value);
    }

    /// Appends a new Vec2d constructed from the arguments \p args to the end
    /// of the Vec2dArray. This is provided for compatibility with std::vector.
    /// Prefer using append().
    ///
    template <typename... Args>
    void emplace_back(Args&&... args) {
        data_.emplace_back(args...);
    }

    /// Removes the last Vec2d of this Vec2dArray. If this Vec2dArray is empty,
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

    /// Resizes to Vec2dArray so that it contains \p count Vec2d elements
    /// instead of its current size(). If \p count is smaller than the current
    /// size(), the last (size() - \p count) Vec2d elements are discarded. If
    /// \p count is greater than the current size(), (\p count - size())
    /// uninitialized Vec2d elements are appended.
    ///
    void resize(int count) {
        assert(count >= 0);
        data_.resize(cast_(count));
    }

    /// Resizes to Vec2dArray so that it contains \p count Vec2d elements
    /// instead of its current size(). If \p count is smaller than the current
    /// size(), the last (size() - \p count) Vec2d elements are discarded. If
    /// \p count is greater than the current size(), (\p count - size())
    /// uninitialized Vec2d elements are appended.
    ///
    void resize(size_type count) {
        data_.resize(count);
    }

    /// Resizes to Vec2dArray so that it contains \p count Vec2d elements
    /// instead of its current size(). If \p count is smaller than the current
    /// size(), the last (size() - \p count) Vec2d elements are discarded. If
    /// \p count is greater than the current size(), (\p count - size())
    /// copies of the given Vec2d \p value are appended.
    ///
    void resize(int count, const value_type& value) {
        assert(count >= 0);
        data_.resize(cast_(count), value);
    }

    /// Resizes to Vec2dArray so that it contains \p count Vec2d elements
    /// instead of its current size(). If \p count is smaller than the current
    /// size(), the last (size() - \p count) Vec2d elements are discarded. If
    /// \p count is greater than the current size(), (\p count - size())
    /// copies of the given Vec2d \p value are appended.
    ///
    void resize(size_type count, const value_type& value) {
        data_.resize(count, value);
    }

    /// Exchanges the content of this Vec2dArray with the content of the \p other
    /// Vec2dArray.
    ///
    void swap(Vec2dArray& other) {
        data_.swap(other.data_);
    }

    /// Returns whether the two given Vec2dArray \p a1 and \p a2 are equal,
    /// that is, whether they have the same size and `a1[i] == a2[i]` for all
    /// valid i indices.
    ///
    friend bool operator==(const Vec2dArray& a1, const Vec2dArray& a2) {
        return a1.data_ == a2.data_;
    }

    /// Returns whether the two given Vec2dArray \p a1 and \p a2 are different,
    /// that is, whether they have different size or `a1[i] == a2[i]` for some
    /// valid i index.
    ///
    friend bool operator!=(const Vec2dArray& a1, const Vec2dArray& a2) {
        return a1.data_ != a2.data_;
    }

    /// Compares the two Vec2dArray \p a1 and \p a2 using the lexicographic
    /// order.
    ///
    friend bool operator<(const Vec2dArray& a1, const Vec2dArray& a2) {
        return a1.data_ < a2.data_;
    }

    /// Compares the two Vec2dArray \p a1 and \p a2 using the lexicographic
    /// order.
    ///
    friend bool operator<=(const Vec2dArray& a1, const Vec2dArray& a2) {
        return a1.data_ <= a2.data_;
    }

    /// Compares the two Vec2dArray \p a1 and \p a2 using the lexicographic
    /// order.
    ///
    friend bool operator>(const Vec2dArray& a1, const Vec2dArray& a2) {
        return a1.data_ > a2.data_;
    }

    /// Compares the two Vec2dArray \p a1 and \p a2 using the lexicographic
    /// order.
    ///
    friend bool operator>=(const Vec2dArray& a1, const Vec2dArray& a2) {
        return a1.data_ >= a2.data_;
    }

private:
    std::vector<Vec2d> data_;

    static size_type cast_(int i) {
        return static_cast<size_type>(i);
    }
};

/// Exchanges the content of \p lhs with the content of \p rhs.
///
inline void swap(Vec2dArray& a1, Vec2dArray& a2) {
    a1.swap(a2);
};

/// Returns a string representation of the given Vec2dArray.
///
VGC_CORE_API
std::string toString(const Vec2dArray& a);

} // namespace core
} // namespace vgc

#endif // VGC_CORE_VEC2DARRAY_H
