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

#ifndef VGC_CORE_ARRAY_H
#define VGC_CORE_ARRAY_H

#include <algorithm>
#include <cstddef> // for ptrdiff_t
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>
#include <vgc/core/parse.h>

#include <vgc/core/internal/containerutil.h>

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
class Array
{
private:
    // Used internally to select value initialization behavior.
    //
    struct ValueInitTag {};

public:
    /// Used to select default initialization behavior.
    /// Public use is limited to the fill constructor.
    ///
    struct DefaultInitTag {};

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
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /// Creates an empty Array.
    ///
    /// ```
    /// vgc::core::Array<double> a;
    /// a.length();   // => 0
    /// a.isEmpty();  // => true
    /// ```
    ///
    Array() noexcept {

    }

    /// Creates an Array of given \p length.
    /// Elements are value initialized.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    /// Throws LengthError if the given \p length is greater than maxLength().
    ///
    /// ```
    /// vgc::core::Array<double> a(3);
    /// a.length();      // => 3
    /// std::cout << a;  // => [0, 0, 0]
    /// ```
    ///
    explicit Array(Int length) {
        checkLength_(length);
        init_(length, ValueInitTag{});
    }

    /// Creates an Array of given \p length.
    /// Elements are default initialized.
    /// 
    /// Throws NegativeIntegerError if the given \p length is negative.
    /// Throws LengthError if the given \p length is greater than maxLength().
    /// 
    /// ```
    /// vgc::core::Array<double> a(2);
    /// a.length();      // => 2
    /// std::cout << a;  // => [undefined, undefined,undefined]
    /// ```
    ///
    explicit Array(Int length, DefaultInitTag) {
        checkLength_(length);
        init_(length, DefaultInitTag{});
    }

    /// Creates an Array of given \p length with all values initialized to
    /// the given \p value.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    /// Throws LengthError if the given \p length is greater than maxLength().
    /// 
    /// ```
    /// vgc::core::Array<double> a(3, 42);
    /// a.length();      // => 3
    /// std::cout << a;  // => [42, 42, 42]
    /// ```
    ///
    template<typename IntType, typename = internal::RequireSignedInteger<IntType>>
    Array(IntType length, const T& value) {
        checkLength_(length);
        init_(length, value);
    }

    /// Creates an Array of given \p length with all values initialized to
    /// the given \p value.
    ///
    /// This is an overload provided for compatibility with the STL.
    ///
    /// Throws LengthError if the given \p length is greater than maxLength().
    /// 
    Array(size_type length, const T& value) {
        checkLength_(length);
        init_(static_cast<Int>(length), value);
    }

    /// Creates an Array initialized from the elements in the range given by
    /// the input iterators \p first (inclusive) and \p last (exclusive).
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range.
    ///
    /// Throws LengthError if the length of the given range is greater than maxLength().
    /// 
    /// ```
    /// std::list<double> l = {1, 2, 3};
    /// vgc::core::Array<double> a(l.begin(), l.end()); // Copy the list as an Array
    /// ```
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    Array(InputIt first, InputIt last) {
        rangeConstruct_(first, last);
    }

    /// Copy-constructs the \p other Array.
    ///
    Array(const Array& other) :
        Array(other.begin(), other.end()) {

    }

    /// Move-constructs the \p other Array.
    ///
    Array(Array&& other) :
        data_(other.data_),
        length_(other.length_),
        reservedLength_(other.reservedLength_) {

        other.data_ = nullptr;
        other.length_ = 0;
        other.reservedLength_ = 0;
    }

    /// Creates an Array initialized by the values given in the initializer
    /// list \p init.
    ///
    /// Throws LengthError if the length of the given \p ilist is greater than maxLength().
    ///
    /// ```
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// std::cout << a;  // => [10, 42, 12]
    /// ```
    ///
    Array(std::initializer_list<T> ilist) {
        rangeConstruct_(ilist.begin(), ilist.end());
    }

    /// Destructs the given Array.
    ///
    ~Array() {
        destroyStorage_();
        // todo: Remove when unit-tests are implemented.
        // A destroyed object shouldn't be reused
        data_ = (T*)0x2143658778563412;
        length_ = -1;
        reservedLength_ = -1;
    }

    /// Copy-assigns the given \p other Array.
    ///
    Array& operator=(const Array& other) {
        if (this != &other) {
            assign(other.begin(), other.end());
        }
        return *this;
    }

    /// Move-assigns the given \p other Array.
    ///
    Array& operator=(Array&& other) {
        if (this != &other) {
            data_ = other.data_;
            length_ = other.length_;
            reservedLength_ = other.reservedLength_;
            other.data_ = nullptr;
            other.length_ = 0;
            other.reservedLength_ = 0;
        }
        return *this;
    }

    /// Replaces the contents of this Array by the values given in the
    /// initializer list \p ilist.
    ///
    /// Throws LengthError if the length of the given \p ilist is greater than maxLength().
    ///
    /// ```
    /// vgc::core::Array<double> a;
    /// a = {10, 42, 12};
    /// std::cout << a;  // => [10, 42, 12]
    /// ```
    ///
    Array& operator=(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    /// Replaces the content of the Array by an array of given \p length with
    /// all values initialized to the given \p value.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// the overload accepting a signed integer as argument instead.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    /// Throws LengthError if the given \p length is greater than maxLength().
    /// 
    void assign(size_type length, T value) {
        checkLength_(length);
        assignFill_(static_cast<Int>(length), value);
    }

    /// Replaces the content of the Array by an array of the given \p length
    /// with all values initialized to the given \p value.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    /// Throws LengthError if the given \p length is greater than maxLength().
    /// 
    /// ```
    /// vgc::core::Array<double> a;
    /// a.assign(3, 42);
    /// std::cout << a;  // => [42, 42, 42]
    /// ```
    ///
    template<typename IntType, typename = internal::RequireSignedInteger<IntType>>
    void assign(IntType length, T value) {
        checkLength_(length);
        assignFill_(static_cast<Int>(length), value);
    }

    /// Replaces the content of the Array by the elements in the range given by
    /// the input iterators \p first (inclusive) and \p last (exclusive).
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range,
    /// or is a range into this Array.
    ///
    /// Throws LengthError if the length of the given range is greater than maxLength().
    /// 
    /// ```
    /// vgc::core::Array<double> a;
    /// std::list<double> l = someList();
    /// a.assign(l); // Make the existing array a copy of the list
    /// ```
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    void assign(InputIt first, InputIt last) {
        assignRange_(first, last);
    }

    /// Replaces the contents of this Array by the values given in the
    /// initializer list \p ilist.
    ///
    /// Throws LengthError if the length of the given \p ilist is greater than maxLength().
    /// 
    void assign(std::initializer_list<T> ilist) {
        assignRange_(ilist.begin(), ilist.end());
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
                "Calling getWrapped(" + toString(i) + ") on an empty Array.");
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
                "Calling getWrapped(" + toString(i) + ") on an empty Array.");
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
                "Attempting to access the first element of an empty Array.");
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
                "Attempting to access the first element of an empty Array.");
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
                "Attempting to access the last element of an empty Array.");
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
                "Attempting to access the last element of an empty Array.");
        }
        return *(end() - 1);
    }

    /// Returns a mutable pointer to the underlying data.
    ///
    /// You can use data() together with length() or size() to pass the content
    /// of this Array to a API expecting a C-style array.
    ///
    T* data() noexcept {
        return data_;
    }

    /// Returns a const pointer to the underlying data.
    ///
    /// You can use data() together with length() or size() to pass the content
    /// of this Array to a API expecting a C-style array.
    ///
    const T* data() const noexcept {
        return data_;
    }

    /// Returns an iterator to the first element in this Array.
    ///
    iterator begin() noexcept {
        return data_;
    }

    /// Returns a const iterator to the first element in this Array.
    ///
    const_iterator begin() const noexcept {
        return cbegin();
    }

    /// Returns a const iterator to the first element in this Array.
    ///
    const_iterator cbegin() const noexcept {
        return data_;
    }

    /// Returns an iterator to the past-the-last element in this Array.
    ///
    iterator end() noexcept {
        return data_ + length_;
    }

    /// Returns a const iterator to the past-the-last element in this Array.
    ///
    const_iterator end() const noexcept {
        return cend();
    }

    /// Returns a const iterator to the past-the-last element in this Array.
    ///
    const_iterator cend() const noexcept {
        return data_ + length_;
    }

    /// Returns a reverse iterator to the first element of the reversed
    /// Array.
    ///
    reverse_iterator rbegin() noexcept {
        return reverse_iterator(data_ + length_);
    }

    /// Returns a const reverse iterator to the first element of the reversed
    /// Array.
    ///
    const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    /// Returns a const reverse iterator to the first element of the reversed
    /// Array.
    ///
    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(data_ + length_);
    }

    /// Returns a reverse iterator to the past-the-last element of the reversed
    /// Array.
    ///
    reverse_iterator rend() noexcept {
        return reverse_iterator(data_);
    }

    /// Returns a const reverse iterator to the past-the-last element of the
    /// reversed Array.
    ///
    const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /// Returns a const reverse iterator to the past-the-last element of the
    /// reversed Array.
    ///
    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(data_);
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
        return length_ == 0;
    }

    /// Returns whether this Array contains value.
    ///
    bool contains(const T& value) const noexcept {
        return std::find(begin(), end(), value) != end();
    }

    /// Returns, as an unsigned integer, the number of elements in this Array.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// length() instead.
    ///
    size_type size() const noexcept {
        return static_cast<size_type>(length_);
    }

    /// Returns the number of elements in this Array.
    ///
    Int length() const {
        return length_;
    }

    /// Returns, as an unsigned integer, the maximum number of elements this
    /// Array is able to hold due to system or library implementation
    /// limitations.
    ///
    /// This function is provided for compatibility with the STL: prefer using
    /// maxLength() instead.
    ///
    size_type max_size() const noexcept {
        return static_cast<size_type>(maxLength());
    }

    /// Returns the maximum number of elements this Array is able to hold due
    /// to system or library implementation limitations.
    ///
    constexpr Int maxLength() const noexcept {
        return vgc::core::tmax_<Int>::value;
    }

    /// Increases the reservedLength() of this Array, that is, the maximum
    /// number of elements this Array can contain without having to perform a
    /// reallocation. It may improve performance to call this function before
    /// performing multiple append(), when you know an upper bound or an
    /// estimate of the number of elements to append.
    ///
    /// Throws NegativeIntegerError if the given \p length is negative.
    /// Throws LengthError if \p the given length is greater maxLength().
    ///
    void reserve(Int length) {
        checkLength_(length);
        if (length > reservedLength_) {
            reallocateExactly_(length); 
        }
    }

    /// Returns the maximum number of elements this Array can contain without
    /// having to perform a reallocation.
    ///
    Int reservedLength() const noexcept {
        return reservedLength_;
    }

    /// Reclaims unused memory. Use this if the current length() of this Array
    /// is much smaller than its current reservedLength(), and you don't expect
    /// the number of elements to grow anytime soon. Indeed, by default,
    /// removing elements to an Array keeps the memory allocated in order to
    /// make adding them back efficient.
    ///
    void shrinkToFit()  {
        shrinkToFit_();
    }

    /// Removes all the elements in this Array, making it empty.
    ///
    void clear() noexcept {
        if (length_ != 0) {
            std::destroy_n(data_, length_);
            length_ = 0;
        }
    }

    /// Inserts the given \p value just before the element referred to by the
    /// iterator \p it, or after the last element if `it == end()`. Returns an
    /// iterator pointing to the inserted element.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    iterator insert(const_iterator it, const T& value) {
        return emplace(it, value);
    }

    /// Move-inserts the given \p value just before the element referred to by
    /// the iterator \p it, or after the last element if `it == end()`. Returns
    /// an iterator pointing to the inserted element.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    iterator insert(const_iterator it, T&& value) {
        return emplace(it, std::move(value));
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
    /// Throws LengthError if the resulting number of elements would exceed maxLength().
    /// 
    iterator insert(const_iterator it, size_type n, const T& value) {
        pointer pos = unwrapIterator(it);
        const Int i = static_cast<Int>(std::distance(data_, pos));
        return makeIterator(insertFill_(i, static_cast<Int>(n), value));
    }

    /// Inserts \p n copies of the given \p value just before the element
    /// referred to by the iterator \p it, or after the last element if `it ==
    /// end()`. Returns an iterator pointing to the first inserted element, or
    /// \p it if `n == 0`.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    /// Throws NegativeIntegerError if the given \p n is negative.
    /// Throws LengthError if the resulting number of elements would exceed maxLength().
    ///
    template<typename IntType, typename = internal::RequireSignedInteger<IntType>>
    iterator insert(const_iterator it, IntType n, const T& value) {
        checkPositive_(n);
        pointer pos = unwrapIterator(it);
        const Int i = static_cast<Int>(std::distance(data_, pos));
        return makeIterator(insertFill_(i, static_cast<Int>(n), value));
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
    /// Throws LengthError if the resulting number of elements would exceed maxLength().
    /// 
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    iterator insert(const_iterator it, InputIt first, InputIt last) {
        pointer pos = unwrapIterator(it);
        const Int i = static_cast<Int>(std::distance(data_, pos));
        return makeIterator(insertRange_(i, first, last));
    }

    /// Inserts the values given in the initializer list \p ilist just before
    /// the element referred to by the iterator \p it, or after the last
    /// element if `it == end()`. Returns an iterator pointing to the first
    /// inserted element, or \p it if \p ilist is empty.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    /// Throws LengthError if the resulting number of elements would exceed maxLength().
    /// 
    iterator insert(const_iterator it, std::initializer_list<T> ilist) {
        return insert(it, ilist.begin(), ilist.end());
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
        emplaceAt_(i, value);
    }

    /// Move-inserts the given \p value just before the element at index \p i,
    /// or after the last element if `i == length()`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    ///
    void insert(Int i, T&& value) {
        checkInRangeForInsert_(i);
        emplaceAt_(i, std::move(value));
    }

    /// Inserts \p n copies of the given \p value just before the element
    /// at index \p i, or after the last element if `i == length()`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    /// Throws NegativeIntegerError if \p n is negative.
    /// Throws LengthError if the resulting number of elements would exceed maxLength().
    /// 
    /// ```cpp
    /// vgc::core::Array<double> a = {10, 42, 12};
    /// a.insert(2, 3, 15);    // => [10, 42, 15, 15, 15, 12]
    /// ```
    ///
    void insert(Int i, Int n, const T& value) {
        checkInRangeForInsert_(i);
        checkPositive_(n);
        insertFill_(i, n, value);
    }

    /// Inserts the range given by the input iterators \p first (inclusive) and
    /// \p last (exclusive) just before the element at index \p i, or after the
    /// last element if `i == length()`.
    ///
    /// The behavior is undefined if [\p first, \p last) isn't a valid range,
    /// or is a range into this Array.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    /// Throws LengthError if the resulting number of elements would exceed maxLength().
    ///
    template<typename InputIt, typename = internal::RequireInputIterator<InputIt>>
    void insert(Int i, InputIt first, InputIt last) {
        checkInRangeForInsert_(i);
        insertRange_(i, first, last);
    }

    /// Inserts the values given in the initializer list \p ilist just before
    /// the element at index \p i, or after the last element if `i ==
    /// length()`.
    ///
    /// Throws IndexError if \p i does not belong to [0, length()].
    /// Throws LengthError if the resulting number of elements would exceed maxLength().
    ///
    void insert(Int i, std::initializer_list<T> ilist) {
        checkInRangeForInsert_(i);
        insertRange_(i, ilist.begin(), ilist.end());
    }

    /// Inserts a new element, constructed from the arguments \p args, just
    /// before the element referred to by the iterator \p it, or after the last
    /// element if `it == end()`. Returns an iterator pointing to the inserted
    /// element.
    ///
    /// The behavior is undefined if \p it is not a valid iterator into this Array.
    ///
    /// Throws LengthError if the resulting number of elements would exceed maxLength().
    /// 
    template <typename... Args>
    iterator emplace(const_iterator it, Args&&... args) {
        pointer pos = unwrapIterator(it);
        const Int i = static_cast<Int>(std::distance(data_, pos));
        return makeIterator(emplaceAt_(i, std::forward<Args>(args)...));
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
        emplaceAt_(i, std::forward<Args>(args)...);
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
        pointer pos = unwrapIterator(it);
        const Int i = static_cast<Int>(std::distance(data_, pos));
        return makeIterator(erase_(i));
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
        const pointer p1 = unwrapIterator(it1);
        const pointer p2 = unwrapIterator(it2);
        return makeIterator(erase_(p1, p2));
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
        erase_(i);
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
        const pointer data = data_;
        erase_(data + i1, data + i2);
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
        if (length_ == maxLength()) {
            throwLengthError(length_, 1);
        }
        emplaceAt_(length_, value);
    }

    /// Move-appends the given \p value to the end of this Array.
    ///
    void append(T&& value) {
        if (length_ == maxLength()) {
            throwLengthError(length_, 1);
        }
        emplaceAt_(length_, std::move(value));
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
        if (length_ == maxLength()) {
            throwLengthError(length_, 1);
        }
        emplaceAt_(0, value);
    }

    /// Move-prepends the given \p value to the beginning of this Array.
    ///
    void prepend(T&& value) {
        if (length_ == maxLength()) {
            throwLengthError(length_, 1);
        }
        emplaceAt_(0, std::move(value));
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
    /// Throws IndexError if this Array is empty.
    ///
    /// \sa pop(0)
    ///
    void removeFirst() {
        if (isEmpty()) {
            throw IndexError(
                "Attempting to remove the first element of an empty Array.");
        }
        erase_(0);
    }

    /// Removes the last element of this Array. This is fast: O(1). This is
    /// equivalent to removeAt(length() - 1).
    ///
    /// ```cpp
    /// vgc::core::Array<double> a = {10, 42, 12, 15};
    /// a.removeLast();        // => [10, 42, 12]
    /// ```
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// \sa pop()
    ///
    void removeLast() {
        if (isEmpty()) {
            throw IndexError(
                "Attempting to remove the last element of an empty Array.");
        }
        erase_(length_ - 1);
    }

    /// Removes and returns the last element of this Array. This is fast: O(1).
    ///
    /// Throws IndexError if this Array is empty.
    ///
    /// \sa removeLast(), pop(i)
    ///
    T pop() {
        T res = last();
        removeLast();
        return res;
    }

    /// Removes and returns the element at index \p i, shifting all subsequent
    /// elements one index to the left.
    ///
    /// Throws IndexError if this Array is empty or if \p i does not belong to
    /// [0, length() - 1].
    ///
    /// \sa removeAt(i), pop()
    ///
    T pop(Int i) {
        checkInRange_(i);
        T res = (*this)[i];
        erase_(i);
        return res;
    }

    /// Resizes this Array so that it contains \p count elements instead of its
    /// current length(). If \p count is smaller than the current length(), the
    /// last (length() - \p count) elements are discarded. If \p count is
    /// greater than the current length(), (\p count - length())
    /// value-initialized elements are appended.
    ///
    /// Throws NegativeIntegerError if the given \p count is negative.
    ///
    void resize(Int count) {
        checkPositive_(count);
        resize_(count, ValueInitTag{});
    }

    /// Resizes this Array so that it contains \p count elements instead of its
    /// current length(). If \p count is smaller than the current length(), the
    /// last (length() - \p count) elements are discarded. If \p count is
    /// greater than the current length(), (\p count - length())
    /// default-initialized elements are appended.
    ///
    /// Throws NegativeIntegerError if the given \p count is negative.
    ///
    void resizeNoInit(Int count) {
        checkPositive_(count);
        resize_(count, DefaultInitTag{});
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
        checkPositive_(count);
        resize_(count, value);
    }

    /// Exchanges the content of this Array with the content of the \p other
    /// Array.
    ///
    void swap(Array& other) {
        if (this != &other) {
            std::swap(data_, other.data_);
            std::swap(length_, other.length_);
            std::swap(reservedLength_, other.reservedLength_);
        }
    }

private:
    T* data_ = nullptr;
    Int length_ = 0;
    Int reservedLength_ = 0;

    T* unwrapIterator(iterator it) {
        return it;
    }
    T* unwrapIterator(const_iterator it) {
        return const_cast<pointer>(it);
    }
    iterator makeIterator(pointer p) {
        return p;
    }

    // Throws NegativeIntegerError if n is negative.
    //
    [[nodiscard]] T* allocate_(Int n) {
        using Allocator = std::allocator<T>;
        return Allocator().allocate(n);
    }

    // Throws NegativeIntegerError if n is negative.
    //
    void deallocate_(T* p, Int n) {
        using Allocator = std::allocator<T>;
        Allocator().deallocate(p, n);
    }

    // Standard construction
    //
    template<typename... Args>
    void constructElement_(T* p, Args&&... args) {
        using Allocator = std::allocator<T>;
        using AllocatorTraits = std::allocator_traits<Allocator>;
        Allocator al = {};
        AllocatorTraits::construct(al, p, std::forward<Args>(args)...);
    }

    // Standard destruction
    //
    void destroyElement_(T* p) {
        using Allocator = std::allocator<T>;
        using AllocatorTraits = std::allocator_traits<Allocator>;
        Allocator al = {};
        AllocatorTraits::destroy(al, p);
    }

    // Helper for fill construction.
    // New elements are initialized depending on InitValueType:
    // - T, const T&: copy
    // - ValueInitTag: value-initialization
    // - DefaultInitTag: default-initialization
    // Expects: length <= maxLength()
    // 
    // Throws NegativeIntegerError if the given length is negative.
    //
    template<typename InitValueType>
    void init_(Int length, InitValueType v) {
        if (length != 0) {
            allocateStorage_(length);
            uninitializedFillN_(data_, length, v);
            length_ = length;
        }
    }

    // Only for range constructors.
    //
    // Throws LengthError if range size exceeds maxLength().
    //
    template<typename InputIt>
    void rangeConstruct_(InputIt first, InputIt last) {
        using iterator_category = typename std::iterator_traits<InputIt>::iterator_category;

        if constexpr (std::is_base_of_v<std::forward_iterator_tag, iterator_category>) {
            const auto dist = std::distance(first, last);
            if (dist != 0) {
                checkLength_(dist);
                const Int newLen = static_cast<Int>(dist);
                allocateStorage_(newLen);
                std::uninitialized_copy(first, last, data_);
                length_ = newLen;
            }
        }
        else {
            while (first != last) {
                append(*first++);
            }
        }
    }

    // Calculates a new storage size to hold newLength objects.
    // The result both depends on reservedLength() and newLength.
    // Expects: newLength < maxLength()
    //
    Int calculateGrowth_(const Int newLength) const {
        const Int oldReservedLen = reservedLength_;
        const Int maxLen = maxLength();
 
        if (oldReservedLen > maxLen - oldReservedLen) {
            return maxLen;
        }
 
        const Int geometric = oldReservedLen + oldReservedLen;

        if (geometric < newLength) {
            return newLength;
        }
 
        return geometric;
    }

    // Destroys all elements and deallocates current storage.
    // Leaves data_ as a dangling pointer!
    //
    void destroyStorage_() {
        if (data_) {
            std::destroy_n(data_, static_cast<size_t>(length_));
            deallocate_(data_, reservedLength_);
        }
    }

    // Resets the container to its default constructed state (empty, no storage).
    //
    void reset_() noexcept {
        destroyStorage_();
    }

    // Leaves length_ unchanged!
    // Expects: (data_ == nullptr) and (n <= maxLength()).
    // 
    // Throws NegativeIntegerError if n is negative.
    //
    void allocateStorage_(const Int n) {
      data_ = allocate_(n);
      reservedLength_ = n;
    }

    // Reallocates storage to hold exactly n elements.
    // Content is moved from old to new storage.
    // Expects: n <= maxLength()
    //
    // Throws NegativeIntegerError if n is negative.
    //
    void reallocateExactly_(const Int n) {
        if (n < length_) {
            throw LogicError("reallocateExactly_ is being called with n < length()");
        }
        const pointer newData = allocate_(n);
        std::uninitialized_move_n(data_, length_, newData);
        destroyStorage_();
        data_ = newData;
        reservedLength_ = n;
    }

    // Reallocates storage if necessary to fit the elements range.
    // If the container is empty, storage is deallocated.
    //
    void shrinkToFit_() {
        const Int len = length_;
        if (len != reservedLength_) {
          if (len == 0) {
            reset_();
          }
          else {
            reallocateExactly_(len);
          }
        }
    } 

    // Clears content and allocates new storage.
    //
    // Throws NegativeIntegerError if n is negative.
    //
    void clearAllocate_(const Int n) {
        const pointer newData = allocate_(n);
        destroyStorage_();
        data_ = newData;
        length_ = 0;
        reservedLength_ = n;
    }

    // Resizes the container.
    // Eventual new elements are initialized depending on InitValueType:
    // - T, const T&: copy
    // - ValueInitTag: value-initialization
    // - DefaultInitTag: default-initialization
    //
    template<typename InitValueType>
    void resize_(const Int newLen, InitValueType value) {
        const Int oldLen = length_;
        if (newLen == oldLen) {
            return;
        }

        if (newLen > reservedLength_) {
            // Increasing length beyond reserved length, reallocation required
            const Int newReservedLen = calculateGrowth_(newLen);
            const pointer newData = allocate_(newReservedLen);

            std::uninitialized_move_n(data_, oldLen, newData);
            uninitializedFillN_(newData + oldLen, newLen - oldLen, value);

            destroyStorage_();
            data_ = newData;
            length_ = newLen;
            reservedLength_ = newReservedLen;
        }
        else if (newLen < oldLen) {
            // Decreasing length
            std::destroy_n(data_ + newLen, oldLen - newLen);
            length_ = newLen;
        }
        else {
            // Increasing length within reserved range
            uninitializedFillN_(data_ + oldLen, newLen - oldLen, value);
            length_ = newLen;
        }
    }

    // Assigns content to be newLen copies of value.
    // Expects: newLen <= maxLength()
    // 
    void assignFill_(const Int newLen, const T& value) {
        if (newLen <= reservedLength_) {
            // Reuse storage
            const Int oldLen = length_;
            const pointer p = data_;

            if (newLen <= oldLen) {
              // Less or as many elements
              std::fill_n(p, newLen, value);
              std::destroy(p + newLen, p + oldLen);
            }
            else {
              // More elements
              std::fill_n(p, oldLen, value);
              std::uninitialized_fill(p + oldLen, p + newLen, value);
            }

            length_ = newLen;
        }
        else {
            // Allocate bigger storage
            clearAllocate_(newLen);
            uninitializedFillN_(data_, newLen, value);
            length_ = newLen;
        }
    }

    template<typename InputIt>
    void assignRange_(InputIt first, InputIt last) {
        using iterator_category = typename std::iterator_traits<InputIt>::iterator_category;
        const Int oldLen = length_;

        if constexpr (std::is_base_of_v<std::forward_iterator_tag, iterator_category>) {
            // Forward iterator case
            const auto dist = std::distance(first, last);
            checkLength_(dist);
            const Int newLen = static_cast<Int>(dist);

            if (newLen <= reservedLength_) {
                // Reuse storage
                const Int oldLen = length_;
                const pointer p = data_;

                if (newLen <= oldLen) {
                    // Less or as many elements in input range than this container
                    std::copy(first, last, p);
                    std::destroy(p + newLen, p + oldLen);
                }
                else {
                    // More elements in input range than this container
                    const InputIt uBeg = std::next(first, oldLen); // not perfect
                    std::copy(first, uBeg, data_);
                    std::uninitialized_copy(uBeg, last, data_ + oldLen);
                }
            }
            if (newLen > reservedLength_) {
                // Allocate bigger storage
                clearAllocate_(newLen);
                std::uninitialized_copy(first, last, data_);
            }

            length_ = newLen;
        }
        else {
            // Input iterator case (fallback)
            const pointer data = data_;
            const pointer e = data + oldLen;
            pointer p = data;

            // Exhaust either input range or output range
            for (; first != last && p != e; ++first) {
                *p++ = *first;
            }

            length_ = static_cast<Int>(p - data);
            if (length_ < oldLen) {
                // Less elements in input range than this container
                std::destroy(p, e);
            }
            else {
                // More or as many elements in input range than this container
                for (; first != last; ++first) {
                    append(*first);
                }
            }
        }
    }

    // Expects: (length_ < maxLength()) and (0 <= i <= length_)
    //
    template<typename... Args>
    pointer emplaceReallocate_(const Int i, Args&&... args) {
        const Int oldLen = length_;
        const Int newLen = oldLen + 1;
        const Int newReservedLen = calculateGrowth_(newLen);
        const pointer newData = allocate_(newReservedLen);
        const pointer oldData = data_;
        const pointer newElementPtr = newData + i;

        // Move range [0, i) of oldData to newData
        std::uninitialized_move_n(oldData, i, newData);
        // Emplace new element at position i in newData
        constructElement_(newElementPtr, std::forward<Args>(args)...);
        // Move range [i, oldLen) of oldData to range [i + 1, oldLen + 1) of newData
        if (i != oldLen) {
            std::uninitialized_move_n(oldData + i, oldLen - i, newElementPtr + 1);
        }

        destroyStorage_();
        data_ = newData;
        length_ = newLen;
        reservedLength_ = newReservedLen;

        return newElementPtr;
    }

    // Expects: (length_ < reservedLength()) and (0 <= i <= length_)
    //
    template<typename... Args>
    pointer emplaceReserved_(const Int i, Args&&... args) {
        const Int oldLen = length_;
        const pointer data = data_;

        const pointer newElementPtr = data + i;

        if (i == oldLen) {
            constructElement_(newElementPtr, std::forward<Args>(args)...);
        }
        else {
            // (0 <= i <= length()) and (i != oldLen) => (oldLen > 0)
            const pointer last = data + oldLen;
            const pointer back = last - 1;
            // *last is uninitialized!
            constructElement_(last, std::move(*back));
            // Ranges overlap -> Move backard
            std::move_backward(newElementPtr, back, last);
            // Emplace new element
            *newElementPtr = T(std::forward<Args>(args)...);
        }

        ++length_;
        return newElementPtr;
    }

    template<typename... Args>
    pointer emplaceAt_(const Int i, Args&&... args) {
        if (length_ < reservedLength_) {
            return emplaceReserved_(i, std::forward<Args>(args)...);
        }
        else {
            return emplaceReallocate_(i, std::forward<Args>(args)...);
        }
    }

    // Prepares for insertion of a range elements.
    // Returns index of first uninitialized element contained in insertion range, or end of insertion range.
    // Expects: (0 <= i <= length_) and (n > 0)
    //
    // Throws LengthError if the resulting number of elements would exceed maxLength().
    //
    Int prepareInsertRange_(const Int i, const Int n) {

        const Int oldLen = length_;
        const Int newLen = oldLen + n;

        if (oldLen > maxLength() - n) {
            throwLengthError(length_, n);
        }

        if (newLen < reservedLength_) {
            // Reuse storage
            const pointer data = data_;
            const pointer oldEnd = data + oldLen;
            const pointer insertPtr = data + i;
            const Int displacedCount = oldEnd - insertPtr;

            if (displacedCount <= n) {
                // Shift of elements does not overlap.
                std::uninitialized_move(insertPtr, oldEnd, insertPtr + n);
                length_ = newLen;
                return oldLen;
            }
            else {
                // Shift of elements does overlap.
                std::uninitialized_move(oldEnd - n, oldEnd, oldEnd);
                std::move_backward(insertPtr, oldEnd - n, oldEnd);
                length_ = newLen;
                return i + n;
            }
        }
        else {
            // Allocate bigger storage
            const Int newReservedLen = calculateGrowth_(newLen);
            const pointer oldData = data_;
            const pointer newData = allocate_(newReservedLen);
            const pointer insertPtr = newData + i;

            std::uninitialized_move(oldData, oldData + i, newData);
            std::uninitialized_move(oldData + i, oldData + oldLen, insertPtr + n);

            data_ = newData;
            length_ = newLen;
            reservedLength_ = newReservedLen;

            return i;
        }
    }

    // Expects: (0 <= i <= length_) and (n > 0)
    //
    // Throws LengthError if the resulting number of elements would exceed maxLength().
    //
    pointer insertFill_(const Int i, const Int n, const T& value) {
        if (n == 0) {
            return data_ + i;
        }

        const Int j = prepareInsertRange_(i, n);
        const pointer uBeg = data_ + j;
        const pointer insertBeg = data_ + i;
        const pointer insertEnd = insertBeg + n;
        if (insertBeg < uBeg) {
            std::fill(insertBeg, uBeg, value);
        }
        if (uBeg < insertEnd) {
            std::uninitialized_fill(uBeg, insertEnd, value);
        }

        return insertBeg;
    }

    // Expects: (0 <= i <= length_) and (n > 0)
    //
    // Throws LengthError if the resulting number of elements would exceed maxLength().
    //
    template<typename Iter>
    pointer insertRange_(const Int i, Iter first, Iter last) {
        using iterator_category = typename std::iterator_traits<Iter>::iterator_category;

        if (first == last) {
            return data_ + i;
        }

        if constexpr (std::is_base_of_v<std::forward_iterator_tag, iterator_category>) {
            // Forward iterator implementation
            const auto dist = std::distance(first, last);

            checkLength_(dist);
            const Int n = static_cast<Int>(dist);

            const Int j = prepareInsertRange_(i, n);
            const pointer insertBeg = data_ + i;
            const Int initCount = j - i;

            if (initCount > 0) {
                std::copy_n(first, initCount, insertBeg);
                if (initCount < n) {
                    const Iter m = std::next(first, initCount);
                    std::uninitialized_copy(m, last, insertBeg + initCount);
                }
            }
            else {
                std::uninitialized_copy(first, last, insertBeg);
            }

            return insertBeg;
        }
        else {
            // Input iterator implementation (fallback, slow!)
            const Int oldLen = length_;
            for (; first != last; ++first) {
                append(*first);
            }
            const pointer data = data_;
            const pointer insertBeg = data + i;
            std::rotate(insertBeg, data + oldLen, data + length_);
            return insertBeg;
        }
    }

    // Expects: (0 <= i < length_)
    //
    pointer erase_(const Int i) noexcept {
        const pointer data = data_;
        const pointer oldEnd = data + length_;
        const pointer erasePtr = data + i;
        std::move(erasePtr + 1, oldEnd, erasePtr);
        destroyElement_(oldEnd - 1);
        --length_;
        return erasePtr;
    }

    pointer erase_(const pointer first, const pointer last) noexcept {
        if (first != last) {
            const pointer oldEnd = data_ + length_;
            const Int n = static_cast<Int>(std::distance(first, last));
            std::move(last, oldEnd, first);
            std::destroy_n(oldEnd - n, n);
            length_ -= n;
        }
        return first;
    }

    iterator uninitializedFillN_(iterator first, const Int n, const T& value) {
        return std::uninitialized_fill_n(unwrapIterator(first), n, value);
    }

    iterator uninitializedFillN_(iterator first, const Int n, ValueInitTag) {
        return std::uninitialized_value_construct_n(unwrapIterator(first), n);
    }

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
                         "(array length is " + toString(size()) + ").")));
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

    // Available in C++20
    // Used to establish non-deduced contexts in template argument deduction.
    // e.g. throwLengthError's IntType is deduced from first argument only,
    // then second argument must be convertible to it.
    // todo: move it to some common header or adopt c++20.
    template<typename U>
    struct type_identity {
        using type = U;
    };

    // Throws NegativeIntegerError if length is negative..
    //
    template<typename IntType>
    void checkPositive_(IntType length) const {
        if constexpr (std::is_signed_v<IntType>) {
            if (length < 0) {
                throw NegativeIntegerError("Array length cannot be nagative.");
            }
        }
    }

    // Throws NegativeIntegerError if length is negative.
    // Throws LengthError if length > maxLength().
    //
    template<typename IntType>
    void checkLength_(IntType length) const {
        checkPositive_(length);
        // both are positive, thus safe to compare
        if (static_cast<size_t>(length) > static_cast<size_t>(maxLength())) {
            throw LengthError("Exceeding maximum Array length.");
        }
    }
    template<typename IntType>
    void throwLengthError(IntType current, typename type_identity<IntType>::type addend) const {
        throw LengthError("Exceeding maximum Array length.");
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
                       "out of range [0, " + toString(size()) + ")."));
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
                              toString(size()) + ").");
        }
    }
    void checkInRangeForInsert_(size_type i) const {
        // Note: we compare as an unsigned int with size(), rather than as a
        // signed int with length(), because we are currently using std::vector
        // as a backend, thus size() is cached while length() isn't, thus
        // comparing with size() should be faster (one less comparison).
        if (i > size()) {
            throw IndexError("Array index " + toString(i) + " out of range for insertion (array length is " +
                              toString(size()) + ").");
        }
    }
    void checkInRangeForInsert_(Int i) const {
        if (i < 0 || i > length()) {
            throw IndexError("Array index " + toString(i) + " out of range for insertion (array length is " +
                              toString(length()) + ").");
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

/// Writes the given Array<T> to the output stream.
///
template<typename OStream, typename T>
void write(OStream& out, const Array<T>& a)
{
    if (a.isEmpty()) {
        write(out, "[]");
    }
    else {
        write(out, '[');
        auto it = a.cbegin();
        auto last = a.cend()-1;
        write(out, *it);
        while (it != last) {
            write(out, ", ", *++it);
        }
        write(out, ']');
    }
}

/// Reads the given Array<T> from the input stream, and stores it in the given
/// output parameter. Leading whitespaces are allowed. Raises ParseError if the
/// stream does not start with an Array<T>. Raises RangeError if one of the
/// numbers in the array is outside of the representable range of its type.
///
template <typename IStream, typename T>
void readTo(Array<T>& a, IStream& in)
{
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
