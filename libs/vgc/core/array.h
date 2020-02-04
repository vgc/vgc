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

#include <iostream>

namespace vgc {
namespace core {

/// \class vgc::core::Array
/// \brief Sequence of elements with fast index-based access (= dynamic array).
///
/// An Array is a container allowing you to store an arbitrary number of
/// elements, efficiently accessible via an index from `0` to `length() - 1`.
///
/// ```cpp
/// vgc::core::Array<double> a = {10, 42, 12};
/// std::cout << a.length();    // Prints "3"
/// std::cout << a[1];          // Prints "42"
/// std::cout << a.first();     // Prints "10"
/// std::cout << a.last();      // Prints "12"
/// std::string sep = "";
/// for (double x : a) {        // Prints "10, 42, 12"
///     std::cout << sep << x;
///     sep = ", ";
/// }
/// std::cout << a;             // Prints "[10, 42, 12]"
/// a.append(13);
/// std::cout << a;             // Prints "[10, 42, 12, 13]"
/// ```
///
/// Its elements are stored contiguously in memory, resulting in better
/// performance than other container types in most situations. It should be
/// your default first choice of container whenever you work with the VGC API.
///
/// ---cpp---
/// Array is very similar to std::vector, but is safer to use due to its
/// bounds-checked operator[]. Another difference is that Array uses vgc::Int
/// rather than size_t for lengths and indices, and provides a richer interface
/// making the C++ and Python API of VGC more consistent. For more info, see
/// "Bounds checking" and "Compatibility with the STL".
/// ---cpp---
///
/// ---python---
/// Array is very similar to built-in Python lists, but provides better
/// performance since all its elements have the same type (e.g., IntArray,
/// Vec2dArray), allowing them to be tightly packed in memory, similarly to
/// numpy arrays.
/// ---python---
///
/// # Bounds checking
///
/// All member functions of Array which access elements via an index are
/// bounds-checked: an IndexError is raised if you pass them out-of-range
/// indices. An IndexError is also raised if you call, say, pop() on an empty
/// array.
///
/// ```cpp
/// vgc::core::Array<double> a = {10, 42, 12};
/// a[-1];            // => vgc::core::IndexError!
/// a[3];             // => vgc::core::IndexError!
/// a.pop(3);         // => vgc::core::IndexError!
/// a.removeAt(3);    // => vgc::core::IndexError!
/// a.insert(4, 15);  // => vgc::core::IndexError!
/// a = {};
/// a.first();        // => vgc::core::IndexError!
/// a.last();         // => vgc::core::IndexError!
/// a.removeFirst();  // => vgc::core::IndexError!
/// a.removeLast();   // => vgc::core::IndexError!
/// a.pop();          // => vgc::core::IndexError!
/// ```
///
/// The valid range for accessing or removing an element is [0, n-1] in C++,
/// but [-n, n-1] in Python (where n is the length of the Array). For example,
/// `a[-1]` raises an IndexError in C++, but returns the last element in
/// Python. This provides the expected behavior for Python users, while keeping
/// the possibility to disable bounds checking in C++ based on compiler flags,
/// for use cases where performance matters more than memory safety.
///
/// Similarly, the valid range for inserting an element is [0, n] in C++, but
/// unrestricted in Python. For example, `a.insert(a.length()+1, x)` raises an
/// IndexError in C++, but is equivalent to `a.append(x)` in Python. This is
/// because in Python, the semantics of `a.insert(i, x)` is to be equivalent to
/// `a[i:i] = x`, and the slicing operator in Python never raises an error and
/// instead clamps the indices to the valid range [-n, n].
///
/// ---cpp---
/// An IndexError is considered "unrecoverable" (like a `panic!` in Rust) and
/// is meant to prevent data corruption or security vulnerabilities in case of
/// bugs, and make it easier to locate and fix such bugs. When an IndexError
/// occurs, the code should be fixed such that all indices are in range. You
/// should not write a try-catch block instead, since bounds checking may be
/// disabled via compiler flags.
///
/// Note that bounds checking is only performed when using an index:
/// dereferencing an iterator is *not* checked. This makes it possible to have
/// C-level performance when using range-based loops or well tested generic
/// algorithms (e.g., those in <algorithm>). Therefore, you have to be extra
/// careful when manipulating iterators directly. As a general guideline, you
/// *should not* increment, decrement, or dereference iterators directly, but
/// instead use generic algorithms.
///
/// If performance is critical, but for some reason you must use indices rather
/// than generic algorithms or range-based loops (e.g., you're traversing two
/// arrays simultaneously), then you can use getUnchecked(). This function is
/// like operator[] but without bounds checking: the behavior is undefined if
/// you pass it an out-of-range index. This may cause data corruption, security
/// vulnerabilities, and/or crash the program without any useful hint of where
/// in the code was the bug. With great power comes great responsibility: do
/// not use this unless you really need to, and have actually measured the
/// performance impact of using operator[] rather than getUnchecked() in your
/// specific use case.
///
/// ```cpp
/// vgc::core::Array<double> a = {10, 42, 12};
/// a.getUnchecked(3); // Maybe random value, or segfault, or stolen password...
/// ```
/// ---cpp---
///
/// # Circular arrays
///
/// If you need to use Array as a circular array, you can use `getWrapped(i)`,
/// which "wraps the index around", ensuring that it is never out-of-range
/// (except when the Array is empty).
///
/// ```cpp
/// vgc::core::Array<double> a = {10, 42, 12};
/// std::cout << a.getWrapped(-1); // Prints "12"
/// std::cout << a.getWrapped(3);  // Prints "10"
///
/// template<typename T>
/// vgc::core::Array<T> smoothedCircular(const vgc::core::Array<T>& a) {
///     vgc::core::Array<T> out;
///     out.reserve(a.length());
///     for (vgc::Int i = 0; i < a.length(); ++i) {
///         out.append(
///             0.5 * a[i] +
///             0.5 * (a.getWrapped(i-1) + a.getWrapped(i+1)));
///     }
///     return out;
/// }
///
/// a = {};
/// a.getWrapped(-1); // => vgc::core::IndexError!
/// ```
///
/// ---cpp---
/// # Compatibility with the STL
///
/// Like std::vector, this class meets the requirements of:
/// - Container
/// - ReversibleContainer
/// - SequenceContainer
/// - ContiguousContainer
///
/// This means that you should be able to use Array with all generic STL
/// algorithms that work on std::vector.
///
/// Below are the main differences between Array and std::vector, and some
/// rationale why we implemented such class:
///
/// - Bounds checking. Array::operator[] performs bounds checks, while
///   vector::operator[] does not (unless using compiler-dependent flags). One
///   could use vector::at(), which is indeed bounds-checked, but the syntax
///   isn't as nice, and then it would be impossible to disable those bounds
///   checks via compiler flags (using Array, you get safety by default, and
///   unsafety as opt-in). Finally, vector::at() throws std::out_of_range,
///   while Array::operator[] throws vgc::core::IndexError.
///
/// - Signed integers. The C++ Standard Library made the choice to use unsigned
///   integers for container sizes and indices, and we believe this decision to
///   be a mistake (see vgc/core/int.h). By using our own class, we can provide
///   an interface based on signed integers, safeguarding against common
///   pitfalls caused by signed-to-unsigned comparisons and casts.
///
/// - Python bindings. Using a separate type than std::vector makes it possible
///   to increase consistency between our C++ API and Python API.
///
/// - Allocation strategies. A separate type makes it possible to have fine
///   control over allocation strategies. With std::vector, we could of course
///   specify our own allocator, but it isn't as flexible as having our own
///   Array class.
///
/// - Higher-level API. We provide convenient functions such as getWrapped(i),
///   or remove(x). In the future, we may complete the API with more convenient
///   functions for tasks which are common in VGC applications.
///
/// Although we recommend using vgc::Int whenever possible, a few functions
/// also accept or return size_t for compatibility with the STL. For example,
/// the following functions accept either size_t, vgc::Int, or any signed
/// integer type:
///
/// - Array(IntType length, T value)
/// - assign(IntType length, T value)
/// - insert(iterator it, IntType n, T value)
///
/// The following functions return a size_t, but have an equivalent function
/// returning a vgc::Int instead:
///
///  | Returns size_t | Returns vgc::Int |
///  | -------------- | -----------------|
///  | size()         | length()         |
///  | max_size()     | maxLength()      |
///
/// Note that we do not provide a function called capacity(), but instead
/// provide the function reservedLength(), which returns a vgc::Int().
///
/// Finally, we provide `empty()` for compatibility with the STL, but the
/// preferred way to check whether an Array is empty is via `isEmpty()`, which
/// is equivalent but follows the naming conventions of the other classes in
/// our API.
/// ---cpp---
///
template<typename T>
class VGC_CORE_API Array
{
public:
    // Define all typedefs necessary to meet the requirements of
    // SequenceContainer. Note that size_type must be unsigned (cf.
    // [tab:container.req] in the C++ standard), therefore we define it to be
    // size_t, despite most our API working with signed integers. Finally, note
    // that for now, we define our iterators to be typedefs of the std::vector
    // iterators. This may change in the future, if/when we don't use
    // std::vector anymore as backend.
    //
    using value_type             = T;
    using reference              = T&;
    using const_reference        = const T&;
    using pointer                = T*;
    using const_pointer          = const T*;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using iterator               = typename std::vector<T>::iterator;
    using const_iterator         = typename std::vector<T>::const_iterator;
    using reverse_iterator       = typename std::vector<T>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;

    /// Creates an empty Array.
    ///
    /// ```
    /// vgc::core::Array<double> a;
    /// a.length();   // => 0
    /// a.isEmpty();  // => true
    /// ```
    ///
    Array() : data_() {

    }

    /// Creates an Array of given \p length with all values default-inserted.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    ///
    /// ```
    /// vgc::core::Array<double> a(3);
    /// a.length();      // => 3
    /// std::cout << a;  // => [0, 0, 0]
    /// ```
    ///
    explicit Array(Int length) : data_(int_cast<size_type>(length)) {

    }

    /// Creates an Array of given \p length with all values initialized to
    /// the given \p value.
    ///
    /// This is an overload provided for compatibility with the STL.
    ///
    Array(size_type length, const T& value) :
        data_(length, value) {

    }

    /// Creates an Array of given \p length with all values initialized to
    /// the given \p value.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    ///
    /// ```
    /// vgc::core::Array<double> a(3, 42);
    /// a.length();      // => 3
    /// std::cout << a;  // => [42, 42, 42]
    /// ```
    ///
    template<typename IntType, typename = internal::RequireSignedInteger<IntType>>
    Array(IntType length, const T& value) :
        data_(int_cast<size_type>(length), value) {

    }

    /// Creates an Array initialized from the elements in the range given by
    /// the input iterators \p first (inclusive) and \p last (exclusive).
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range.
    ///
    /// ```
    /// std::list<double> l = {1, 2, 3};
    /// vgc::core::Array<double> a(l.begin(), l.end()); // Copy the list as an Array
    /// ```
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    Array(InputIt first, InputIt last) :
        data_(first, last) {

    }

    /// Copy-constructs the \p other Array.
    ///
    Array(const Array& other) : data_(other.data_) {

    }

    /// Move-constructs the \p other Array.
    ///
    Array(Array&& other) : data_(std::move(other.data_)) {

    }

    /// Creates an Array initialized by the values given in the initializer
    /// list \p init.
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// std::cout << a;  // => [10, 42, 12]
    /// ```
    ///
    Array(std::initializer_list<T> init) :
        data_(init) {

    }

    /// Destructs the given Array.
    ///
    ~Array() {

    }

    /// Copy-assigns the given \p other Array.
    ///
    Array& operator=(const Array& other) {
        data_ = other.data_;
        return *this;
    }

    /// Move-assigns the given \p other Array.
    ///
    Array& operator=(Array&& other) {
        data_ = std::move(other.data_);
        return *this;
    }

    /// Replaces the contents of this Array by the values given in the
    /// initializer list \p ilist.
    ///
    /// ```
    /// vgc::core::Array<double> a;
    /// a = {10, 42, 12};
    /// std::cout << a;  // => [10, 42, 12]
    /// ```
    ///
    Array& operator=(std::initializer_list<T> ilist) {
        data_ = ilist;
        return *this;
    }

    /// Replaces the content of the Array by an array of given \p length with
    /// all values initialized to the given \p value.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// the overload accepting a signed integer as argument instead.
    ///
    void assign(size_type length, T value) {
        data_.assign(length, value);
    }

    /// Replaces the content of the Array by an array of the given \p size
    /// with all values initialized to the given \p value.
    ///
    /// Throws NegativeIntegerError if the given \p size is negative.
    ///
    /// ```
    /// vgc::core::Array<double> a;
    /// a.assign(3, 42);
    /// std::cout << a;  // => [42, 42, 42]
    /// ```
    ///
    template<typename IntType, typename = internal::RequireSignedInteger<IntType>>
    void assign(IntType length, T value) {
        data_.assign(int_cast<size_type>(length), value);
    }

    /// Replaces the content of the Array by the elements in the range given by
    /// the input iterators \p first (inclusive) and \p last (exclusive).
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range,
    /// or is a range into this Array.
    ///
    /// ```
    /// vgc::core::Array<double> a;
    /// std::list<double> l = someList();
    /// a.assign(l); // Make the existing array a copy of the list
    /// ```
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

    /// Returns a mutable reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, length() - 1].
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// std::cout << a;  // => [10, 42, 12]
    /// a[1] = 7;
    /// std::cout << a;  // => [10, 7, 12]
    /// a[-1] = 7;       // => IndexError
    /// a[3] = 7;        // => IndexError
    /// ```
    ///
    T& operator[](Int i) {
        checkInRange_(i);
        return data_[static_cast<size_type>(i)];
        // Note: No need for int_cast (bounds already checked)
    }

    /// Returns a const reference to the element at index \p i.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong
    /// to [0, length() - 1].
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// std::cout << a[1];   // => 42
    /// std::cout << a[-1];  // => IndexError
    /// std::cout << a[3];   // => IndexError
    /// ```
    ///
    const T& operator[](Int i) const {
        checkInRange_(i);
        return data_[static_cast<size_type>(i)];
        // Note: No need for int_cast (bounds already checked)
    }

    /// Returns a mutable reference to the element at index \p i, without
    /// bounds checking.
    ///
    /// The behavior is undefined if this Array is empty or if \p i does not
    /// belong to [0, length() - 1]. In practice, this may cause the
    /// application to crash (segfault), or be a security vulnerability
    /// (leaking a password).
    ///
    /// Do not use this function unless you have measured and documented that
    /// the bounds checking in your particular use case was a significant
    /// performance bottleneck.
    ///
    T& getUnchecked(Int i) {
        return data_[static_cast<size_type>(i)];
    }

    /// Returns a const reference to the element at index \p i, without
    /// bounds checking.
    ///
    /// The behavior is undefined if this Array is empty or if \p i does not
    /// belong to [0, length() - 1]. In practice, this may cause the
    /// application to crash (segfault), or be a security vulnerability
    /// (leaking a password).
    ///
    /// Do not use this function unless you have measured and documented that
    /// the bounds checking in your particular use case was a significant
    /// performance bottleneck.
    ///
    const T& getUnchecked(Int i) const {
        return data_[static_cast<size_type>(i)];
    }

    /// Returns a mutable reference to the element at index \p i, with
    /// wrapping behavior.
    ///
    /// Throws IndexError if the Array is empty.
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// a.getWrapped(-1) = 7; // => a == [10, 42, 7]
    /// a.getWrapped(3) = 8;  // => a == [8, 42, 7]
    /// a = {};
    /// a.getWrapped(-1) = 7; // => IndexError
    /// a.getWrapped(3) = 8;  // => IndexError
    /// ```
    ///
    T& getWrapped(Int i) {
        if (isEmpty()) {
            throw IndexError(
                "Calling getWrapped(" + toString(i) + ") on an empty Array");
        }
        return data_[static_cast<size_type>(wrap_(i))];
    }

    /// Returns a const reference to the element at index \p i, with
    /// wrapping behavior.
    ///
    /// Throws IndexError if the Array is empty.
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// std::cout << a.getWrapped(-1); // => 12
    /// std::cout << a.getWrapped(3);  // => 10
    /// a = {};
    /// std::cout << a.getWrapped(-1); // => IndexError
    /// std::cout << a.getWrapped(3);  // => IndexError
    /// ```
    ///
    const T& getWrapped(Int i) const {
        if (isEmpty()) {
            throw IndexError(
                "Calling getWrapped(" + toString(i) + ") on an empty Array");
        }
        return data_[static_cast<size_type>(wrap_(i))];
    }

    /// Returns a mutable reference to the first element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// a.first() = 7;
    /// std::cout << a;  // => [7, 42, 12]
    /// a = {};
    /// a.first() = 7;   // => IndexError
    /// ```
    ///
    T& first() {
        if (isEmpty()) {
            throw IndexError(
                "Attempting to access the first element of an empty Array");
        }
        return *begin();
    }

    /// Returns a const reference to the first element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// std::cout << a.first();  // => 10
    /// a = {};
    /// std::cout << a.first();  // => IndexError
    /// ```
    ///
    const T& first() const {
        if (isEmpty()) {
            throw IndexError(
                "Attempting to access the first element of an empty Array");
        }
        return *begin();
    }

    /// Returns a mutable reference to the last element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// a.last() = 7;
    /// std::cout << a;  // => [10, 42, 7]
    /// a = {};
    /// a.last() = 7;    // => IndexError
    /// ```
    ///
    T& last() {
        if (isEmpty()) {
            throw IndexError(
                "Attempting to access the last element of an empty Array");
        }
        return *(end() - 1);
    }

    /// Returns a const reference to the last element in this Array.
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// std::cout << a.last();  // => 12
    /// a = {};
    /// std::cout << a.last();  // => IndexError
    /// ```
    ///
    const T& last() const {
        if (isEmpty()) {
            throw IndexError(
                "Attempting to access the last element of an empty Array");
        }
        return *(end() - 1);
    }

    /// Returns a mutable pointer to the underlying data.
    ///
    /// You can use data() together with length() or size() to pass the content
    /// of this Array to a API expecting a C-style array.
    ///
    T* data() noexcept {
        return data_.data();
    }

    /// Returns a const pointer to the underlying data.
    ///
    /// You can use data() together with length() or size() to pass the content
    /// of this Array to a API expecting a C-style array.
    ///
    const T* data() const noexcept {
        return data_.data();
    }

    /// Returns an iterator to the first element in this Array.
    ///
    iterator begin() noexcept {
        return data_.begin();
    }

    /// Returns a const iterator to the first element in this Array.
    ///
    const_iterator begin() const noexcept {
        return data_.begin();
    }

    /// Returns a const iterator to the first element in this Array.
    ///
    const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }

    /// Returns an iterator to the past-the-last element in this Array.
    ///
    iterator end() noexcept {
        return data_.end();
    }

    /// Returns a const iterator to the past-the-last element in this Array.
    ///
    const_iterator end() const noexcept {
        return data_.end();
    }

    /// Returns a const iterator to the past-the-last element in this Array.
    ///
    const_iterator cend() const noexcept {
        return data_.cend();
    }

    /// Returns a reverse iterator to the first element of the reversed
    /// Array.
    ///
    reverse_iterator rbegin() noexcept {
        return data_.rbegin();
    }

    /// Returns a const reverse iterator to the first element of the reversed
    /// Array.
    ///
    const_reverse_iterator rbegin() const noexcept {
        return data_.rbegin();
    }

    /// Returns a const reverse iterator to the first element of the reversed
    /// Array.
    ///
    const_reverse_iterator crbegin() const noexcept {
        return data_.crbegin();
    }

    /// Returns a reverse iterator to the past-the-last element of the reversed
    /// Array.
    ///
    reverse_iterator rend() noexcept {
        return data_.rend();
    }

    /// Returns a const reverse iterator to the past-the-last element of the
    /// reversed Array.
    ///
    const_reverse_iterator rend() const noexcept {
        return data_.rend();
    }

    /// Returns a const reverse iterator to the past-the-last element of the
    /// reversed Array.
    ///
    const_reverse_iterator crend() const noexcept {
        return data_.crend();
    }

    /// Returns whether this Array is empty.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// isEmpty() instead.
    ///
    bool empty() const noexcept {
        return isEmpty();
    }

    /// Returns whether this Array is empty.
    ///
    bool isEmpty() const noexcept {
        return data_.empty();
    }

    /// Returns, as an unsigned integer, the number of elements in this Array.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// length() instead.
    ///
    size_type size() const noexcept {
        return data_.size();
    }

    /// Returns the number of elements in this Array.
    ///
    Int length() const {
        // Note: this function isn't noexcept due to our use of int_cast, which
        // may throw. It'd be nice to just use static_cast (for performance and
        // noexcept guarantees), but unfortunately this won't be safe as we
        // currently don't prevent users from adding more than IntMax elements
        // to the vector.
        // TODO short term: update implementation of Array to make it
        // impossible to create more than IntMax elements.
        // TODO long term: implement array from scratch, storing the current
        // length as an Int, and making it impossible to create more
        // than IntMax elements.
        return int_cast<Int>(data_.size());
    }

    /// Returns, as an unsigned integer, the maximum number of elements this
    /// Array is able to hold due to system or library implementation
    /// limitations.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// maxLength() instead.
    ///
    size_type max_size() const noexcept {
        return data_.max_size();
    }

    /// Returns the maximum number of elements this Array is able to hold due
    /// to system or library implementation limitations.
    ///
    Int maxLength() const noexcept {
        // TODO: enforce length() <= maxLength.
        // See comment in length().
        return vgc::internal::type_max<Int>::value;
    }

    /// Increases the resevedLength() of this Array, that is, the maximum
    /// number of elements this Array can contain without having to perform a
    /// reallocation. It may improve performance to call this function before
    /// performing multiple append(), when you know an upper bound or an
    /// estimate of the number of elements to append.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    ///
    void reserve(Int length) {
        data_.reserve(int_cast<size_type>(length));
    }

    /// Returns the maximum number of elements this Array can contain without
    /// having to perform a reallocation.
    ///
    Int reservedLength() const {
        // TODO: make noexcept.
        // See comment in length().
        return int_cast<Int>(data_.capacity());
    }

    /// Reclaims unused memory. Use this if the current length() of this Array
    /// is much smaller than its current reservedLength(), and you don't expect
    /// the number of elements to grow anytime soon. Indeed, by default,
    /// removing elements to an Array keeps the memory allocated in order to
    /// make adding them back efficient.
    ///
    void shrinkToFit()  {
        data_.shrink_to_fit();
    }

    /// Removes all the elements in this Array, making it empty.
    ///
    void clear() noexcept {
        data_.clear();
    }

    /// Inserts the given \p value just before the element referred to by the
    /// iterator \p it, or after the last element if `it == end()`. Returns an
    /// iterator pointing to the inserted element.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    iterator insert(const_iterator it, const T& value) {
        return data_.insert(it, value);
    }

    /// Move-inserts the given \p value just before the element referred to by
    /// the iterator \p it, or after the last element if `it == end()`. Returns
    /// an iterator pointing to the inserted element.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    iterator insert(const_iterator it, T&& value) {
        return data_.insert(it, std::move(value));
    }

    /// Inserts \p n copies of the given \p value just before the element
    /// referred to by the iterator \p it, or after the last element if `it ==
    /// end()`. Returns an iterator pointing to the first inserted element, or
    /// \p it if `n == 0`.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// the overload passing n as a signed integer instead.
    ///
    iterator insert(const_iterator it, size_type n, const T& value) {
        return data_.insert(it, n, value);
    }

    /// Inserts \p n copies of the given \p value just before the element
    /// referred to by the iterator \p it, or after the last element if `it ==
    /// end()`. Returns an iterator pointing to the first inserted element, or
    /// \p it if `n == 0`.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    /// Throws NegativeIntegerError if \p n is negative.
    ///
    template<typename IntType, typename = internal::RequireSignedInteger<IntType>>
    iterator insert(const_iterator it, IntType n, const T& value) {
        return data_.insert(it, int_cast<size_type>(n), value);
    }

    /// Inserts the range given by the input iterators \p first (inclusive) and
    /// \p last (exclusive) just before the element referred to by the iterator
    /// \p it, or after the last element if `it == end()`. Returns an iterator
    /// pointing to the first inserted element, or \p it if `first == last`.
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range,
    /// or is a range into this Array.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    iterator insert(const_iterator it, InputIt first, InputIt last) {
        return data_.insert(it, first, last);
    }

    /// Inserts the values given in the initializer list \p ilist just before
    /// the element referred to by the iterator \p it, or after the last
    /// element if `it == end()`. Returns an iterator pointing to the first
    /// inserted element, or \p it if \p ilist is empty.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    iterator insert(const_iterator it, std::initializer_list<T> ilist) {
        return data_.insert(it, ilist);
    }

    /// Inserts the given \p value just before the element at index \p i, or
    /// after the last element if `i == length()`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// a.insert(2, 15);       // => [10, 42, 15, 12]
    /// a.insert(0, 4);        // => [4, 10, 42, 15, 12]
    /// a.insert(5, 13);       // => [4, 10, 42, 15, 12, 13]
    /// a.insert(-1, 10);      // => vgc::core::IndexError!
    /// a.insert(7, 10);       // => vgc::core::IndexError!
    /// ```
    ///
    void insert(Int i, const T& value) {
        checkInRangeForInsert_(i);
        insert(toConstIterator_(i), value);
    }

    /// Move-inserts the given \p value just before the element at index \p i,
    /// or after the last element if `i == length()`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    void insert(Int i, T&& value) {
        checkInRangeForInsert_(i);
        insert(toConstIterator_(i), std::move(value));
    }


    /// Inserts \p n copies of the given \p value just before the element
    /// at index \p i, or after the last element if `i == length()`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    /// Throws NegativeIntegerError if \p n is negative.
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// a.insert(2, 3, 15);    // => [10, 42, 15, 15, 15, 12]
    /// ```
    ///
    void insert(Int i, Int n, const T& value) {
        checkInRangeForInsert_(i);
        insert(toConstIterator_(i), n, value);
    }


    /// Inserts the range given by the input iterators \p first (inclusive) and
    /// \p last (exclusive) just before the element at index \p i, or after the
    /// last element if `i == length()`.
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range,
    /// or is a range into this Array.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    void insert(Int i, InputIt first, InputIt last) {
        checkInRangeForInsert_(i);
        insert(toConstIterator_(i), first, last);
    }

    /// Inserts the values given in the initializer list \p ilist just before
    /// the element at index \p i, or after the last element if `i ==
    /// length()`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    void insert(Int i, std::initializer_list<T> ilist) {
        checkInRangeForInsert_(i);
        data_.insert(toConstIterator_(i), ilist);
    }

    /// Inserts a new element, constructed from the arguments \p args, just
    /// before the element referred to by the iterator \p it, or after the last
    /// element if `it == end()`. Returns an iterator pointing to the inserted
    /// element.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    template <typename... Args>
    iterator emplace(const_iterator it, Args&&... args) {
        return data_.emplace(it, args...);
    }

    /// Inserts a new element, constructed from the arguments \p args, just
    /// before the element at index \p i, or after the last element if `i ==
    /// length()`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    template <typename... Args>
    void emplace(Int i, Args&&... args) {
        checkInRangeForInsert_(i);
        emplace(toConstIterator_(i), args...);
    }

    /// Removes the element referred to by the iterator \p it. Returns an
    /// iterator to the element following the removed element, or end() if the
    /// removed element was the last element of this Array.
    ///
    /// The behavior is undefined if \p it is not a valid and derefereancable
    /// iterator into this Array. Note: end() is valid, but not
    /// dereferenceable, and thus passing end() to this function is undefined
    /// behavior.
    //
    iterator erase(const_iterator it) {
        return data_.erase(it);
    }

    /// Removes all elements in the range given by the iterators \p it1
    /// (inclusive) and \p it2 (exclusive). Returns an iterator pointing to the
    /// element pointed to by \p it2 prior to any elements being removed. If
    /// it2 == end() prior to removal, then the updated end() is returned.
    ///
    /// The behavior is undefined if [it1, it2) isn't a valid range in this
    /// Array.
    //
    iterator erase(const_iterator it1, const_iterator it2) {
        return data_.erase(it1, it2);
    }

    /// Removes the element at index \p i, shifting all subsequent elements one
    /// index to the left.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong to
    /// [0, length() - 1].
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {8, 10, 42, 12, 15};
    /// a.removeAt(1);             // => [8, 42, 12, 15]
    /// a.removeAt(0);             // => [42, 12, 15]
    /// a.removeAt(a.length()-1);  // => [42, 12]
    /// a.removeAt(-1);            // => vgc::core::IndexError!
    /// a.removeAt(a.length());    // => vgc::core::IndexError!
    /// ```
    ///
    void removeAt(Int i) {
        checkInRange_(i);
        erase(toConstIterator_(i));
    }

    /// Removes all elements from index \p i1 (inclusive) to index \p i2
    /// (exclusive), shifting all subsequent elements to the left.
    ///
    /// Throws IndexError if [i1, i2) isn't a valid range in this Array, that
    /// is, if it doesn't satisfy:
    ///
    ///     0 <= i1 <= i2 <= length()
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {8, 10, 42, 12, 15};
    /// a.removeRange(1, 3);   // => [8, 12, 15]
    /// a.removeRange(2, 3);   // => [42, 12]
    /// a.removeRange(1, 0);   // => vgc::core::IndexError!
    /// a.removeRange(-1, 0);  // => vgc::core::IndexError!
    /// a.removeRange(2, 3);   // => vgc::core::IndexError!
    /// ```
    ///
    void removeRange(Int i1, Int i2) {
        checkInRange_(i1, i2);
        erase(toConstIterator_(i1), toConstIterator_(i2));
    }

    /// Appends the given \p value to the end of this Array. This is fast:
    /// amortized O(1). This is equivalent to insert(length(), value).
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// a.append(15);          // => [10, 42, 12, 15]
    /// ```
    ///
    void append(const T& value) {
        insert(end(), value);
    }

    /// Move-appends the given \p value to the end of this Array.
    ///
    void append(T&& value) {
        insert(end(), std::move(value));
    }

    /// Prepends the given \p value to the beginning of this Array, shifting
    /// all existing elements one index to the right. This is slow: O(n). This
    /// is equivalent to insert(0, value).
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// a.prepend(15);         // => [15, 10, 42, 12]
    /// ```
    ///
    void prepend(const T& value) {
        insert(begin(), value);
    }

    /// Move-prepends the given \p value to the beginning of this Array.
    ///
    void prepend(T&& value) {
        insert(begin(), std::move(value));
    }

    /// Removes the first element of this Array, shifting all existing elements
    /// one index to the left. This is slow: O(n). This is equivalent to
    /// removeAt(0).
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {15, 10, 42, 12};
    /// a.removeFirst();       // => [10, 42, 12]
    /// ```
    ///
    void removeFirst() {
        if (isEmpty()) {
            throw IndexError(
                "Attempting to remove the first element of an empty Array");
        }
        erase(begin());
    }

    /// Removes the last element of this Array. This is fast: O(1). This is
    /// equivalent to removeAt(length() - 1).
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {10, 42, 12, 15};
    /// a.removeLast();        // => [10, 42, 12]
    /// ```
    ///
    void removeLast() {
        if (isEmpty()) {
            throw IndexError(
                "Attempting to remove the last element of an empty Array");
        }
        erase(end()-1);
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
    /// current length(). If \p count is smaller than the current length(), the
    /// last (length() - \p count) elements are discarded. If \p count is
    /// greater than the current length(), (\p count - length()) copies of the
    /// given \p value are appended.
    ///
    /// Throws NegativeIntegerError if the given \p count is negative.
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
    std::vector<T> data_;

    // Casts from integer to const_iterator.
    //
    // Note: the range of difference_type typically includes the range of Int,
    // in which case the int_cast resolves to a no-overhead static_cast.
    //
    const_iterator toConstIterator_(Int i) const {
        return cbegin() + int_cast<difference_type>(i);
    }

    // Checks whether index/iterator i is valid and dereferenceable:
    //     0 <= i < size()
    //
    template<typename IntType>
    void throwNotInRange_(IntType i) const {
        throw IndexError(
            "Array index " + toString(i) + " out of range " +
            (isEmpty() ? "(the array is empty)" :
                         ("[0, " + toString(size() - 1) + "] " +
                         "(array length is " + toString(size()) + ")")));
    }
    void checkInRange_(size_type i) const {
        // Note: we compare as an unsigned int with size(), rather than as a
        // signed int with length(), because we are currently using std::vector
        // as a backend, thus size() is cached while length() isn't, thus
        // comparing with size() should be faster (one less comparison).
        if (i >= size()) {
            throwNotInRange_(i);
        }
    }
    void checkInRange_(Int i) const {
        try {
            checkInRange_(int_cast<size_type>(i));
        }
        catch (NegativeIntegerError&) {
            throwNotInRange_(i);
        }
    }

    // Checks whether range [i1, i2) is valid:
    //     0 <= i1 <= i2 <= size()
    //
    // Note that i2 (or even i1 when i1 == i2) doesn't have to be
    // dereferenceable. In particular, [end(), end()) is a valid empty range.
    //
    template<typename IntType>
    void throwNotInRange_(IntType i1, IntType i2) const {
        throw IndexError(
            "Array index range [" + toString(i1) + ", " + toString(i2) + ") " +
            (i1 > i2 ? "invalid (second index must be greater or equal than first index)" :
                       "out of range [0, " + toString(size()) + ")"));
    }
    void checkInRange_(size_type i1, size_type i2) const {
        // Note: we compare as an unsigned int with size(), rather than as a
        // signed int with length(), because we are currently using std::vector
        // as a backend, thus size() is cached while length() isn't, thus
        // comparing with size() should be faster (one less comparison).
        if (i1 > i2 || i2 > size()) {
            throwNotInRange_(i1, i2);
        }
    }
    void checkInRange_(Int i1, Int i2) const {
        try {
            checkInRange_(int_cast<size_type>(i1), int_cast<size_type>(i2));
        }
        catch (NegativeIntegerError&) {
            throwNotInRange_(i1, i2);
        }
    }

    // Checks whether index/iterator i is valid for insertion:
    //     0 <= i <= size()
    //
    // Note that i doesn't have to be dereferencable.
    // In particular, end() is a valid iterator for insertion.
    //
    void checkInRangeForInsert_(const_iterator it) const {
        difference_type i = std::distance(begin(), it);
        if (i < 0 || static_cast<size_type>(i) > size()) {
            throw IndexError("Array index " + toString(i) + " out of range for insertion (array length is " +
                              toString(size()) + ")");
        }
    }
    void checkInRangeForInsert_(size_type i) const {
        // Note: we compare as an unsigned int with size(), rather than as a
        // signed int with length(), because we are currently using std::vector
        // as a backend, thus size() is cached while length() isn't, thus
        // comparing with size() should be faster (one less comparison).
        if (i > size()) {
            throw IndexError("Array index " + toString(i) + " out of range for insertion (array length is " +
                              toString(size()) + ")");
        }
    }
    void checkInRangeForInsert_(Int i) const {
        if (i < 0 || i > length()) {
            throw IndexError("Array index " + toString(i) + " out of range for insertion (array length is " +
                              toString(length()) + ")");
        }
    }

    // Wraps the given integer to the [0, length()-1] range.
    // Precondition: length() > 0.
    // In a nutshell, this is "i % n", but with a correction
    // that handles negative input without introducing an "if".
    Int wrap_(Int i) const {
        Int n = length();
        return (n + (i%n)) % n;
        // Examples:
        // n = 10, i = 11:   (10 + (11%10))  % 10 = (10 + 1)  % 10 = 1
        // n = 10, i = -11:  (10 + (-11%10)) % 10 = (10 + -1) % 10 = 9
        // We could also do (i % n) + (i < 0 ? n : 0), but the above
        // avoids the conditional and is more robust to pre-C++11
        // implementations (the behavior for negative inputs wasn't
        // clearly specified, and varied across implementations).
    }

};

/// Returns whether the two given Arrays \p a1 and \p a2 are equal, that is,
/// whether they have the same number of elements and `a1[i] == a2[i]` for all
/// elements.
///
template<typename T>
bool operator==(const Array<T>& a1, const Array<T>& a2) {
    return a1.size() == a2.size() &&
           std::equal(a1.begin(), a1.end(), a2.begin());
}

/// Returns whether the two given Arrays \p a1 and \p a2 are different, that
/// is, whether they have a different number of elements or `a1[i] != a2[i]`
/// for some elements.
///
template<typename T>
bool operator!=(const Array<T>& a1, const Array<T>& a2) {
    return !(a1 == a2);
}

/// Compares the two Arrays \p a1 and \p a2 in lexicographic order.
///
template<typename T>
bool operator<(const Array<T>& a1, const Array<T>& a2) {
    return std::lexicographical_compare(
                a1.begin(), a1.end(),
                a2.begin(), a2.end());
}

/// Compares the two Arrays \p a1 and \p a2 in lexicographic order.
///
template<typename T>
bool operator<=(const Array<T>& a1, const Array<T>& a2) {
    return !(a2 < a1);
}

/// Compares the two Arrays \p a1 and \p a2 in inverse lexicographic order.
///
template<typename T>
bool operator>(const Array<T>& a1, const Array<T>& a2) {
    return a2 < a1;
}

/// Compares the two Arrays \p a1 and \p a2 in inverse lexicographic order.
///
template<typename T>
bool operator>=(const Array<T>& a1, const Array<T>& a2) {
    return !(a1 < a2);
}

/// Exchanges the content of \p a1 with the content of \p a2.
///
template<typename T>
void swap(Array<T>& a1, Array<T>& a2) {
    a1.swap(a2);
};

/// Returns a string representation of the given Array.
///
template <typename T>
std::string toString(const Array<T>& v) {
    std::string res = "[";
    std::string sep = "";
    for (const T& x : v) {
        res += sep;
        res += toString(x);
        sep = ", ";
    }
    res += "]";
    return res;
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_ARRAY_H

// # C++ concept requirements
//
// For reference, below are some C++ concept requirements (or rather, "named
// requirements", as they don't use the `concept` syntax introduced in C++20),
// as per the C++20 draft generated on 2020-01-15. These requirements are
// important to know what in std::vector must be kept in order for functions in
// <algorithm> to work with vgc::core::Array, and which can be skipped. For
// more details and up-to-date info:
//
// https://eel.is/c++draft/containers
//
// ## Notations
//
// X: container class containing objects of type T
// u: identifier
// a, b: values of type X
// r: non-const value of type X
// rv: non-const rvalue of type X
// n: value of size_type
// t: lvalue or const rvalue of type T
// trv: non-const rvalue of type T
// [i, j): valid range of input iterators convertible to T
// il: initializer list
// args: parameter pack
// p: valid const_iterator in a
// q: valid dereferenceable const_iterator in a
// [q1, q2): valid range of const_iterators in a
//
// ## Container requirements
//
// https://eel.is/c++draft/containers#tab:container.req
//
// X::value_type       T
// X::reference        T&
// X::const_reference  const T&
// X::iterator         satisfy ForwardIterator, convertible to const_iterator
// X::const_iterator   satisfy ConstForwardIterator
// X::difference_type  signed integer (= X::iterator::difference_type)
// X::size_type        unsigned integer that can represent any non-negative value of difference_type
// X u, X(), X(a), X u(a), X u = a, X u(rv), X u = rv, a = rv, a.~X(), r = a
// a.begin(), a.end(), a.cbegin(), a.cend()
// a == b, a != b
// a.swap(b), swap(a, b)
// a.size(), a.max_size() -> size_type
// a.empty()
//
// Optional:
//   a <=> b (if implemented, must be lexicographical)
//
// ## ReversibleContainer requirements
//
// https://eel.is/c++draft/containers#tab:container.rev.req
//
// X::reverse_iterator        reverse_iterator<iterator>
// X::const_reverse_iterator  reverse_iterator<const_iterator>
// a.rbegin(), a.rend(), a.rcbegin(), a.rcend()
//
// ## SequenceContainer requirements
//
// https://eel.is/c++draft/containers#tab:container.seq.req
//
// X(n, t), X u(n, t), X(i, j), X u(i, j), X(il), a = il
// a.emplace(p, args)
// a.insert(p, t)/(p, trv)/(p, n, t)/(p, i, j)/(p, il)
// a.erase(q)/(q1, q2)
// a.clear()
// a.assign(i, j)/(il)/(n, t)
//
// Optional:
//   a.front(), a.back(),
//   a.emplace_front(args), a.emplace_back(args),
//   a.push_front(t)/(trv), a.push_back(t)/(trv),
//   a.pop_front(), a.pop_back(),
//   a[n], a.at(n)
//
// ## std::vector
//
// std::vector implements all of the optional SequenceContainer
// requirements (except push/emplace_front), also satisfies
// AllocatorAwareContainer, and defines the following functions which
// aren't part of any concept requirements:
//
//   X(n), X u(n) + allocator variants
//   capacity(), resize(n), reserve(n), shrink_to_fit()
//   data()
//
