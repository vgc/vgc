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

#ifndef VGC_CORE_ARITHMETIC_H
#define VGC_CORE_ARITHMETIC_H

/// \file vgc/core/arithmetic.h
/// \brief Utilities for arithmetic types (bool, char, int, float)
///
/// # Integer types
///
/// For convenience, VGC defines the following fixed width integer types:
/// - `vgc::Int8` (typedef for `std::int8_t`)
/// - `vgc::Int16` (typedef for `std::int16_t`)
/// - `vgc::Int32` (typedef for `std::int32_t`)
/// - `vgc::Int64` (typedef for `std::int64_t`)
/// - `vgc::Int8` (typedef for `std::int8_t`)
/// - `vgc::UInt8` (typedef for `std::uint8_t`)
/// - `vgc::UInt16` (typedef for `std::uint16_t`)
/// - `vgc::UInt32` (typedef for `std::uint32_t`)
/// - `vgc::UInt64` (typedef for `std::uint64_t`)
///
/// In addition, VGC defines the following compiler-flag dependent integer types:
/// - `vgc::Int` (typedef for either `vgc::Int32` or `vgc::Int64`)
/// - `vgc::UInt` (typedef for either `vgc::UInt32` or `vgc::UInt64`)
///
/// By default, the `vgc::Int` and `vgc::UInt` types are 64-bit, unless
/// `VGC_CORE_USE_32BIT_INT` is defined, in which case they are 32-bit.
///
/// Note that these integer types are defined directly in the `vgc` namespace,
/// not in `vgc::core`. This is a rare exception to our namespace conventions,
/// justified by the widespread use of integer types, and the fact that they
/// are not wrapped in Python anyway (i.e., there is no need to be consistent
/// with our Python module conventions).
///
/// ## Rationale
///
/// In C++, the default integer types (int, unsigned int, long, unsigned long,
/// etc.) have an unspecified width (32-bit, 64-bit, etc.), which is chosen by
/// the compiler depending on the platform. This chosen width may or may not be
/// the most appropriate width for VGC purposes.
///
/// For this reason, we specify our own "default" integer types (vgc::Int,
/// vgc::UInt), whose width can be chosen at compile time independently from
/// the width the compiler would choose for "int". This makes it possible to
/// fine-tune the performance of VGC on each platform, and in particular use a
/// wider 64-bit default integer when performance is not impacted, reducing the
/// risk of integer overflow.
///
/// ## Signed integers vs. unsigned integers
///
/// VGC uses and recommends using the signed integer vgc::Int for most
/// purposes, including array sizes and indices. We recognize that this is
/// somewhat non-idiomatic C++, given that the STL uses unsigned integers for
/// sizes and indices. However, many authors of the STL themselves acknowledge
/// that using unsigned integers was a mistake, see:
///
/// http://channel9.msdn.com/Events/GoingNative/2013/Interactive-Panel-Ask-Us-Anything
/// (at 12min, 42min, and 63min)
///
/// A common problem with unsigned integers in C++ is that they always use
/// modulo arithmetic (unlike in Rust for example, where an overflow causes a
/// "panic" in debug mode), which may lead to subtle and hard to detect bugs,
/// for example in loops with decreasing indices or in expressions involving
/// substractions.
///
/// Another issue in C++ is that the rules for implicit promotions and
/// conversions between integer types are very error-prone. Therefore, it is
/// safer not mixing the two, and since we can't avoid signed integers (index
/// offsets or pointer/iterator difference), using signed integers everywhere
/// makes the most sense.
///
/// Finally, using signed integers makes the C++ API more consistent with the
/// Python API, since Python does not have unsigned integers at all.
///
/// ## Integer type casting
///
/// We define a function template vgc::int_cast<T>(U value) which performs safe
/// type casting from an integer type U to another integer type T. This
/// function raises a vgc::core::RangeError exception if the output type cannot
/// hold the runtime value of the input type. It is the preferred way to cast
/// between integers types in the VGC codebase:
///
/// \code
/// Int a = 42;
/// UInt b = int_cast<UInt>(a); // OK
/// int c = int_cast<int>(a); // OK
/// a = -1;
/// b = int_cast<UInt>(a); // => RangeError exception
/// Int16 d = 128;
/// Int8 e = int_cast<Int8>(d); // => RangeError exception
/// \endcode
///
/// Note that int_cast has zero overhead when the range of T includes the range
/// of U (e.g., from Int8 to Int16). In these cases, the function template
/// resolves to be nothing else but a direct static_cast.
///
/// Finally, be aware that int_cast is only for converting between two integer
/// types (= std::is_integral_v<T> is true). If you want to convert a float to
/// an integer, or vice-versa, you must use another method, such as a
/// static_cast. We may implement our own cast for this in the future, but
/// haven't done it yet.
///

#include <cmath> // fabs, nextafter
#include <cstdint> // int32_t, etc.
#include <limits> // min, max, infinity
#include <type_traits>

#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>
#include <vgc/core/logging.h>

namespace vgc {

/// The 8-bit signed integer type.
///
using Int8 = std::int8_t;

/// The 16-bit signed integer type.
///
using Int16 = std::int16_t;

/// The 32-bit signed integer type.
///
using Int32 = std::int32_t;

/// The 64-bit signed integer type.
///
using Int64 = std::int64_t;

/// The 8-bit unsigned integer type.
///
using UInt8 = std::uint8_t;

/// The 16-bit unsigned integer type.
///
using UInt16 = std::uint16_t;

/// The 32-bit unsigned integer type.
///
using UInt32 = std::uint32_t;

/// The 64-bit unsigned integer type.
///
using UInt64 = std::uint64_t;

/// A signed integer type of unspecified width (at least 32bit).
///
/// This is the preferred integer type to use in the VGC C++ API and
/// implementation, including for values which are not supposed to be negative
/// such as array sizes and indices.
///
#ifdef VGC_CORE_USE_32BIT_INT
    using Int = Int32;
#else
    using Int = Int64;
#endif

/// An unsigned integer type of same width as vgc::Int (at least 32bit).
///
/// Use this type with moderation: we recommend using vgc::Int in most cases,
/// even for values which are not supposed to be negative, such as array sizes
/// and indices.
///
#ifdef VGC_CORE_USE_32BIT_INT
    using UInt = UInt32;
#else
    using UInt = UInt64;
#endif

namespace core {

/// Returns the maximum value representable by the given type.
///
/// ```cpp
/// int m = vgc::core::tmax_<Int8>::value; // 127
/// ```
///
/// This is equivalent to `(std::numeric_limits<T>::max)()`, but avoids the
/// naming conflicts caused by the min/max macros in `<Windows.h>`.
///
/// Once we require C++14, we will define the variable template
/// `vgc::core::tmax<T>`, and deprecate the use of
/// `vgc::core::tmax_<T>::value`.
///
template<typename T>
struct tmax_ {
    static constexpr T value = (std::numeric_limits<T>::max)();
};

/// Returns the minimum value representable by the given type.
///
/// ```cpp
/// int m = vgc::core::tmin_<Int8>::value; // -128
/// ```
///
/// This is equivalent to `(std::numeric_limits<T>::min)()`, but avoids the
/// naming conflicts caused by the min/max macros in `<Windows.h>`.
///
/// Once we require C++14, we will define the variable template
/// `vgc::core::tmin<T>`, and deprecate the use of
/// `vgc::core::tmin_<T>::value`.
///
template<typename T>
struct tmin_ {
    static constexpr T value = (std::numeric_limits<T>::min)();
};

/// Maximum value of an Int.
///
constexpr Int IntMax = tmax_<Int>::value;

/// Maximum value of an Int8.
///
constexpr Int8 Int8Max = tmax_<Int8>::value;

/// Maximum value of an Int16.
///
constexpr Int16 Int16Max = tmax_<Int16>::value;

/// Maximum value of an Int32.
///
constexpr Int32 Int32Max = tmax_<Int32>::value;

/// Maximum value of an Int64.
///
constexpr Int64 Int64Max = tmax_<Int64>::value;

/// Maximum value of a UInt.
///
constexpr UInt UIntMax = tmax_<UInt>::value;

/// Maximum value of a UInt8.
///
constexpr UInt8 UInt8Max = tmax_<UInt8>::value;

/// Maximum value of a UInt16.
///
constexpr UInt16 UInt16Max = tmax_<UInt16>::value;

/// Maximum value of a UInt32.
///
constexpr UInt32 UInt32Max = tmax_<UInt32>::value;

/// Maximum value of a UInt64.
///
constexpr UInt64 UInt64Max = tmax_<UInt64>::value;

/// Maximum finite value of a float.
///
constexpr float FloatMax = tmax_<float>::value;

/// Maximum finite value of a double.
///
constexpr double DoubleMax = tmax_<double>::value;

/// Minimum value of an Int.
///
constexpr Int IntMin = tmin_<Int>::value;

/// Minimum value of an Int8.
///
constexpr Int8 Int8Min = tmin_<Int8>::value;

/// Minimum value of an Int16.
///
constexpr Int16 Int16Min = tmin_<Int16>::value;

/// Minimum value of an Int32.
///
constexpr Int32 Int32Min = tmin_<Int32>::value;

/// Minimum value of an Int64.
///
constexpr Int64 Int64Min = tmin_<Int64>::value;

/// Minimum value of a UInt.
///
constexpr UInt UIntMin = tmin_<UInt>::value;

/// Minimum value of a UInt8.
///
constexpr UInt8 UInt8Min = tmin_<UInt8>::value;

/// Minimum value of a UInt16.
///
constexpr UInt16 UInt16Min = tmin_<UInt16>::value;

/// Minimum value of a UInt32.
///
constexpr UInt32 UInt32Min = tmin_<UInt32>::value;

/// Minimum value of a UInt64.
///
constexpr UInt64 UInt64Min = tmin_<UInt64>::value;

/// Minimum finite value of a float. This is a very large negative number. Not
/// to be confused with FLT_MIN or std::numeric_limits<float>::min(), which is
/// the minimum normalized positive value.
///
constexpr float FloatMin = - tmax_<float>::value;

/// Minimum finite value of a double. This is a very large negative number. Not
/// to be confused with DBL_MIN or std::numeric_limits<double>::min(), which is
/// the minimum normalized positive value.
///
constexpr double DoubleMin = - tmax_<double>::value;

/// Positive infinity value of a float.
///
constexpr float FloatInfinity = std::numeric_limits<float>::infinity();

/// Positive infinity value of a double.
///
constexpr double DoubleInfinity = std::numeric_limits<double>::infinity();

/// Returns a human-readable name for integer types.
///
/// Examples:
///
/// \code
/// std::cout << vgc::core::int_typename<vgc::Int8>()     << std::end; // => "Int8"
/// std::cout << vgc::core::int_typename<vgc::UInt16>()   << std::end; // => "UInt16"
/// std::cout << vgc::core::int_typename<vgc::Int>()      << std::end; // => "Int64" (or "Int32" if VGC_CORE_USE_32BIT_INT is defined)
/// std::cout << vgc::core::int_typename<bool>()          << std::end; // => "Bool"
/// std::cout << vgc::core::int_typename<char>()          << std::end; // => "Char"
/// std::cout << vgc::core::int_typename<signed char>()   << std::end; // => "Int8"
/// std::cout << vgc::core::int_typename<unsigned char>() << std::end; // => "UInt8"
/// std::cout << vgc::core::int_typename<short>()         << std::end; // => "Int16" (on most platforms)
/// std::cout << vgc::core::int_typename<int>()           << std::end; // => "Int32" (on most platforms)
/// std::cout << vgc::core::int_typename<long>()          << std::end; // => "Int32" or "Int64" (on most platforms)
/// std::cout << vgc::core::int_typename<long long>()     << std::end; // => "Int64" (on most platforms)
/// \endcode
///
/// Note how "char" is a separate type from both "signed char" (= Int8) and
/// "unsigned char" (= UInt8).
///
template <typename T>
typename std::enable_if<
    std::is_integral<T>::value,
    std::string>::type int_typename() { return "UnknownInt"; }
template<> inline std::string int_typename<bool>() { return "Bool"; }
template<> inline std::string int_typename<char>() { return "Char"; }
template<> inline std::string int_typename<Int8>() { return "Int8"; }
template<> inline std::string int_typename<Int16>() { return "Int16"; }
template<> inline std::string int_typename<Int32>() { return "Int32"; }
template<> inline std::string int_typename<Int64>() { return "Int64"; }
template<> inline std::string int_typename<UInt8>() { return "UInt8"; }
template<> inline std::string int_typename<UInt16>() { return "UInt16"; }
template<> inline std::string int_typename<UInt32>() { return "UInt32"; }
template<> inline std::string int_typename<UInt64>() { return "UInt64"; }

namespace internal {

template <typename T, typename U>
std::string intErrorReason(U value) {
    return core::format("Cannot convert {}({}) to type {}", int_typename<U>(), value, int_typename<T>());
}

template <typename T, typename U>
void throwIntegerOverflowError(U value) {
    throw core::IntegerOverflowError(intErrorReason<T>(value));
}

template <typename T, typename U>
void throwNegativeIntegerError(U value) {
    throw core::NegativeIntegerError(intErrorReason<T>(value));
}

} // namespace internal

// Implicit promotion/conversion rules between integer types are tricky:
//
// https://stackoverflow.com/q/46073295
//
// This makes comparing two integers of different types in general unreliable
// (= give surprising or unexpected results). In order to guarantee correctness
// of our int_cast implementation, we only perform one of the following types
// of comparisons, which all provide the expected result:
//
// (1) Comparison between types of same signedness: the type of smaller rank
//     if implicitly converted to the type or greater rank.
//
// (2) Comparison between a non-negative signed integer T and an unsigned
//     integer U:
//
//     (a) If rank(U) >= rank(T): T -> U
//
//         => OK because t > 0 and range(U) includes positive_range(T)
//
//     (b) otherwise, if range(T) includes range(U): U -> T
//
//         => OK because range(T) includes range(U)
//
//     (c) otherwise: U -> V and T -> V, with V = make_unsigned<T>
//
//         => OK for the following reasons:
//
//            (i)  We have rank(T) > rank(U)                (otherwise case (a))
//            (ii) We have range(T) not including range(U)  (otherwise case (b))
//
//            Note: (i) + (ii) can occur for example if:
//              LLP64:   T = long        U = unsigned int       (both are 32bit)
//              LP64:    T = long long   U = unsigned long      (both are 64bit)
//
//            (iii) We have rank(T) = rank(V)              (cf. definition of V)
//
//            With (i) + (iii) we can conclude that rank(V) > rank(U), which
//            means that range(V) includes range(U) since U and V are both
//            unsigned. So the first conversion U -> V is OK.
//
//            Finally, since V is the unsigned version of T, and T is non-negative,
//            then the second conversion T -> V is also OK.
//
//            QED
//
//            Note: we didn't use (ii) for the proof, but it is informative
//            regardless, as clarification of when can (c) occur, and as
//            concrete examples of T and U to help follow the proof.
//

// In the functions below, if the template type T or U is 'bool', then a warning
// may be emitted because using '<' or '>' with bool is generally bad practice.
// However, in our case:
// - it's safe to do it
// - it's more concise/readable than specializing the templates for 'bool'
// So we simply silence the warning.
//
#if defined(VGC_CORE_COMPILER_MSVC)
#  pragma warning(push)
#  pragma warning(disable: 4804) // unsafe use of type 'bool' in operation
#endif

// int_cast from from U to T when:
// - U and T are both signed or both unsigned
// - The range of T includes the range of U.
//
template <typename T, typename U>
typename std::enable_if<
    std::is_integral<T>::value &&
    std::is_integral<U>::value &&
    std::is_signed<T>::value == std::is_signed<U>::value &&
    tmax_<T>::value >= tmax_<U>::value, // (1)
    T>::type
int_cast(U value) {
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U and T are both signed
// - The range of T does not include the range of U.
//
template <typename T, typename U>
typename std::enable_if<
    std::is_integral<T>::value &&
    std::is_integral<U>::value &&
    std::is_signed<T>::value &&
    std::is_signed<U>::value &&
    tmax_<T>::value < tmax_<U>::value, // (1)
    T>::type
int_cast(U value) {
    if (value < tmin_<T>::value || // (1)
        value > tmax_<T>::value) { // (1)
        internal::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U and T are both unsigned
// - The range of T does not include the range of U.
//
template <typename T, typename U>
typename std::enable_if<
    std::is_integral<T>::value &&
    std::is_integral<U>::value &&
    std::is_unsigned<T>::value &&
    std::is_unsigned<U>::value &&
    tmax_<T>::value < tmax_<U>::value, // (1)
    T>::type
int_cast(U value) {
    if (value > tmax_<T>::value) { // (1)
        internal::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U is unsigned and T is signed
// - The range of T includes the range of U
//
template <typename T, typename U>
typename std::enable_if<
    std::is_integral<T>::value &&
    std::is_integral<U>::value &&
    std::is_signed<T>::value &&
    std::is_unsigned<U>::value &&
    tmax_<T>::value >= tmax_<U>::value, // (2)
    T>::type
int_cast(U value) {
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U is unsigned and T is signed
// - The range of T does not include the range of U
//
template <typename T, typename U>
typename std::enable_if<
    std::is_integral<T>::value &&
    std::is_integral<U>::value &&
    std::is_signed<T>::value &&
    std::is_unsigned<U>::value &&
    tmax_<T>::value < tmax_<U>::value, // (2)
    T>::type
int_cast(U value) {
    if (value > tmax_<T>::value) { // (2)
        internal::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U is signed and T is unsigned
// - The range of T includes the positive range of U
//
template <typename T, typename U>
typename std::enable_if<
    std::is_integral<T>::value &&
    std::is_integral<U>::value &&
    std::is_unsigned<T>::value &&
    std::is_signed<U>::value &&
    tmax_<T>::value >= tmax_<U>::value, // (2)
    T>::type
int_cast(U value) {
    if (value < 0) { // 0 is promoted to int => (1)
        internal::throwNegativeIntegerError<T>(value);
    }
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U is signed and T is unsigned
// - The range of T does not include the positive range of U
//
template <typename T, typename U>
typename std::enable_if<
    std::is_integral<T>::value &&
    std::is_integral<U>::value &&
    std::is_unsigned<T>::value &&
    std::is_signed<U>::value &&
    tmax_<T>::value < tmax_<U>::value, // (2)
    T>::type
int_cast(U value) {
    if (value < 0) { // 0 is promoted to int => (1)
        internal::throwNegativeIntegerError<T>(value);
    }
    else if (value > tmax_<T>::value) { // (2)
        internal::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

#if defined(VGC_CORE_COMPILER_MSVC)
#  pragma warning(pop)
#endif

/// Sets the given bool, character, integer or floating-point number to zero.
///
/// This function is called by vgc::core::zero<T>(), and can be overloaded for
/// custom types.
///
/// \sa zero<T>()
///
template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value>::type
setZero(T& x)
{
    x = T();
}

/// Returns a zero-initialized value for the given type.
///
/// ```cpp
/// auto x = vgc::core::zero<int>();                  // int(0)
/// auto y = vgc::core::zero<double>();               // double(0.0)
/// auto v = vgc::core::zero<vgc::geometry::Vec2d>(); // Vec2d(0.0, 0.0)
/// ```
///
/// This function relies on ADL (= argument-dependent lookup), calling the
/// unqualified setZero(x) function under the hood. Therefore, you can
/// specialize zero() by overloading the setZero(x) function in the same
/// namespace of your custom type:
///
/// ```cpp
/// namespace foo {
///
/// struct A {
///     int m;
///     A() {}
/// };
///
/// void setZero(A& a)
/// {
///     a.m = 0;
/// }
///
/// } // namespace foo
///
/// A a1;                        // a1.m == uninitialized
/// A a2 = A();                  // a2.m == uninitialized
/// A a3 = vgc::core::zero<A>(); // a3.m == 0
/// ```
///
/// This function is primarily intended to be used in template functions. If
/// you know the type, prefer to use more readable ways to zero-initialize:
///
/// ```cpp
/// double x = 0.0;
/// vgc::geometry::Vec2d v(0.0, 0.0);
/// ```
///
/// \sa setZero(T& x)
///
/// More info on default/value/zero-initialization:
/// - https://en.cppreference.com/w/cpp/language/default_initialization
/// - https://en.cppreference.com/w/cpp/language/value_initialization
/// - https://stackoverflow.com/questions/29765961/default-value-and-zero-initialization-mess
/// - vgc/core/tests/test_zero.cpp
///
template<typename T>
T zero()
{
    T res;
    setZero(res);
    return res;
}

namespace internal {

// Computes the difference `a - b`, but where two infinities of the same sign
// are considered exactly equal, so their difference is zero.
//
// ```cpp
// double inf = std::numeric_limits<double>::infinity();
// std::cout << inf - inf << std::endl;         // "nan"
// std::cout << infdiff(inf, inf) << std::endl; // "0"
// }
// ```
//
template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
infdiff(T a, T b)
{
    return (a == b) ? zero<T>() : (a - b);
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
isClose(T a, T b, T relTol, T absTol)
{
    T diff = std::fabs(infdiff(a, b));
    if (diff == std::numeric_limits<T>::infinity()) {
        return false; // opposite infinities or finite/infinite mismatch
    }
    else {
        return diff <= std::fabs(relTol * b) ||
               diff <= std::fabs(relTol * a) ||
               diff <= absTol;
    }
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
isNear(T a, T b, T absTol)
{
    T diff = std::fabs(infdiff(a, b));
    if (diff == std::numeric_limits<T>::infinity()) {
        return false; // opposite infinities or finite/infinite mismatch
    }
    else {
        return diff <= absTol;
    }
}

} // namespace internal

/// Returns whether two float values are almost equal within some relative
/// tolerance. For example, set `relTol` to `0.05f` for testing if two values
/// are almost equal within a 5% tolerance.
///
/// ```cpp
/// float relTol = 0.05f;
///
/// vgc::core::isClose(101.0f,  103.0f,  relTol); // true: 103.0  <= 101.0  + 5%
/// vgc::core::isClose(1.01f,   1.03f,   relTol); // true: 1.03   <= 1.01   + 5%
/// vgc::core::isClose(0.0101f, 0.0103f, relTol); // true: 0.0103 <= 0.0101 + 5%
///
/// vgc::core::isClose(101.0f,  108.0f,  relTol); // false: 108.0  > 101.0  + 5%
/// vgc::core::isClose(1.01f,   1.08f,   relTol); // false: 1.08   > 1.01   + 5%
/// vgc::core::isClose(0.0101f, 0.0108f, relTol); // false: 0.0108 > 0.0101 + 5%
///
/// vgc::core::isClose(1e-50f, 0.0f, relTol); // false: 1e-50f > 0.0 + 5%
/// ```
///
/// If you need an absolute tolerance (which is especially important if one the
/// given values could be exactly zero), you should typically use `isNear()`
/// instead:
///
/// ```cpp
/// float absTol = 0.05f;
///
/// vgc::core::isNear(101.0f,  103.0f,  absTol); // false: 103.0  - 101.0  >  0.05
/// vgc::core::isNear(1.01f,   1.03f,   absTol); // true:  1.03   - 1.01   <= 0.05
/// vgc::core::isNear(0.0101f, 0.0103f, absTol); // true:  0.0103 - 0.0101 <= 0.05
///
/// vgc::core::isNear(101.0f,  108.0f,  absTol); // false: 108.0  - 101.0  >  0.05
/// vgc::core::isNear(1.01f,   1.08f,   absTol); // false: 1.08   - 1.01   >  0.05
/// vgc::core::isNear(0.0101f, 0.0108f, absTol); // true:  0.0108 - 0.0101 <= 0.05
///
/// vgc::core::isNear(1e-50f, 0.0f, relTol); // true:  1e-50 - 0.0 <= 0.05
/// ```
///
/// If you need both a relative and an absolute tolerance (which should rarely
/// be the case), then you can use the fourth argument of `isClose()`, which is
/// `0.0f` by default:
///
/// ```cpp
/// float relTol = 0.05f;
/// float absTol = 0.05f;
///
/// vgc::core::isClose(101.0f,  103.0f,  relTol, absTol); // true: 103.0  <= 101.0  + 5%
/// vgc::core::isClose(1.01f,   1.03f,   relTol, absTol); // true: 1.03   <= 1.01   + 5%
/// vgc::core::isClose(0.0101f, 0.0103f, relTol, absTol); // true: 0.0103 <= 0.0101 + 5%
///
/// vgc::core::isClose(101.0f,  108.0f,  relTol, absTol); // false: difference is > 5% and > 0.05
/// vgc::core::isClose(1.01f,   1.08f,   relTol, absTol); // false: difference is > 5% and > 0.05
/// vgc::core::isClose(0.0101f, 0.0108f, relTol, absTol); // true:  0.0108 - 0.0101 <= 0.05
///
/// vgc::core::isClose(1e-50f, 0.0f, relTol, absTol); // true:  1e-50 - 0.0 <= 0.05
/// ```
///
/// The default `relTol` is `1e-5f`, which tests whether the two values are
/// equal within about 5 decimal significant digits (floats have a precision of
/// approximately 7 decimal digits). If you provide your own `relTol`, it must
/// be strictly positive, otherwise the behavior is undefined.
///
/// If you need to test for exact equality, you can use `isNear(a, b, 0.0f)`,
/// which safely accepts a tolerance of exactly zero.
///
/// Note how `isClose()` is appropriate for comparing whether non-zero values
/// have the same magnitude and share several most significant digits. It works
/// well for either very small or very large non-zero values. However, it is
/// not suitable to use when one of the compared value could be exactly zero,
/// in which case you probably should be using `isNear()` instead.
///
/// Both the `isClose()` and `isNear()` functions return true when comparing
/// two infinities of the same sign, return false when comparing two infinities
/// of opposite sign, return false when comparing an infinite value with a
/// finite value, and return false if any of the compared value is NaN,
/// including when both `a` and `b` are NaN.
///
/// If all values are finite, and the tolerances are positive (as they should,
/// otherwise the behavior is undefined), this function is equivalent to:
///
/// ```cpp
/// abs(b-a) <= max(relTol * max(abs(a), abs(b)), absTol)
/// ```
///
/// This function has the same behavior as the builtin Python function
/// `math.isclose()`.
///
VGC_CORE_API
inline bool isClose(float a, float b, float relTol = 1e-5f, float absTol = 0.0f)
{
    return internal::isClose(a, b, relTol, absTol);
}

/// Returns whether two double values are almost equal within some relative
/// tolerance. Please see the documentation of the `float` overload for
/// details.
///
/// Note that this overload has a default `relTol` of `1e-9`, which tests
/// whether the two values are equal within about 9 decimal significant digits
/// (doubles have a precision of approximately 15 decimal digits).
///
VGC_CORE_API
inline bool isClose(double a, double b, double relTol = 1e-9, double absTol = 0.0)
{
    return internal::isClose(a, b, relTol, absTol);
}

/// Returns whether the absolute difference between two float values is within
/// the given tolerance.
///
/// ```cpp
/// float absTol = 0.05f;
/// vgc::core::isNear(42.00f, 42.04f,  absTol); // true
/// vgc::core::isNear(42.00f, 42.06f,  absTol); // false
/// ```
///
/// The given `absTol` must be positive, otherwise the behavior is undefined.
/// If you need to test for exact equality, you can use `isNear(a, b, 0.0f)`.
///
/// There is no default value for `absTol`, because the appropriate tolerance
/// is specific to each use case, and there is no "one size fits all" value.
/// For example, if you are comparing whether two widgets have nearly the same
/// size in virtual pixels, a tolerance of 0.1 is probably enough. But if you
/// are comparing whether two opacity values in the range [0, 1] are nearly
/// equal, you probably want a tolerance less than 0.01.
///
/// If you need a relative tolerance rather than an absolute tolerance, you
/// should use `isClose()` instead. Please refer to the documentation of
/// `isClose()` for a detailed comparison of when you need `isClose()` and when
/// you need `isNear()`, and a detailed description of how both functions
/// handle infinite and NaN values.
///
/// If all values are finite, and the given tolerance is positive (as it
/// should, otherwise the behavior is undefined), this function is equivalent
/// to:
///
/// ```cpp
/// abs(a-b) <= absTol;
/// ```
///
VGC_CORE_API
inline bool isNear(float a, float b, float absTol)
{
    return internal::isNear(a, b, absTol);
}

/// Returns whether the absolute difference between two double values is within
/// the given tolerance. Please see the documentation of the `float` overload
/// for details.
///
VGC_CORE_API
inline bool isNear(double a, double b, double absTol)
{
    return internal::isNear(a, b, absTol);
}

/// Returns the given value clamped to the interval [min, max]. If max < min,
/// then a warning is issued and the value is clamped to [max, min] instead.
///
template<typename T>
const T& clamp(const T& value, const T& min, const T& max)
{
    if (max < min) {
        warning()
            << "Warning: vgc::core::clamp("
            << "value=" << value << ", min=" << min << ", max=" << max << ") "
            << "called with max < min\n";
        return (value < max) ? max : (min < value) ? min : value;
    }
    else {
        return (value < min) ? min : (max < value) ? max : value;
    }
}

/// Returns the next representable floating point. This is equivalent to:
///
/// ```cpp
/// std::nextafter(x, vgc::core::tmax_<FloatType>::value);
/// ```
///
template <typename FloatType>
typename std::enable_if<std::is_floating_point<FloatType>::value, FloatType>::type
nextafter(FloatType x)
{
    return std::nextafter(x, vgc::core::tmax_<FloatType>::value);
}

/// Returns the previous representable floating point. This is equivalent to:
///
/// ```cpp
/// std::nextafter(x, -vgc::core::tmax_<FloatType>::value);
/// ```
///
template <typename FloatType>
typename std::enable_if<std::is_floating_point<FloatType>::value, FloatType>::type
nextbefore(FloatType x)
{
    return std::nextafter(x, -vgc::core::tmax_<FloatType>::value);
}

/// Converts the given floating point `x` to an integer type using floor.
///
/// If the given `x` is larger (resp. smaller) than the maximum (resp. minimum)
/// integer representable by the output type, then IntegerOverflowError is
/// raised.
///
/// Otherwise, this function is equivalent to:
///
/// \code
/// static_cast<T>(std::floor(x))
/// \endcode
///
/// Note that this function never raises NegativeIntegerError, even when
/// calling, say, `ifloor<unsigned int>(-1.0)`. In this case, the function
/// still raises IntegerOverflowError. The reason is that NegativeIntegerError
/// is categorized as a logic error, and is reserved for when a signed integer
/// type holds a negative integer, but shouldn't. IntegerOverflowError,
/// however, is categorized as a runtime error, which is more appropriate for
/// floating point to integer conversions, since floating points are subject to
/// rounding errors which are less predictible than arithmetic on integers.
///
template <typename IntType, typename FloatType>
typename std::enable_if<
    std::is_floating_point<FloatType>::value &&
    std::is_integral<IntType>::value, IntType>::type
ifloor(FloatType x)
{
    constexpr IntType tmini = tmin_<IntType>::value;
    constexpr IntType tmaxi = tmax_<IntType>::value;
    constexpr FloatType tminf = static_cast<FloatType>(tmin_<IntType>::value);
    constexpr FloatType tmaxf = static_cast<FloatType>(tmax_<IntType>::value);
    // Note: the outer "if" should be optimized out at compile time based on FloatType and IntType
    if constexpr (tmaxf < 1 + tmaxf) { // all IntType integers representable as FloatType
        if (x < tminf) {
            throw IntegerOverflowError(core::format(
                "Call to vgc::core::ifloor<{}>({:.1f}) overflows ({}Min = {})",
                int_typename<IntType>(), x, int_typename<IntType>(), tmini));
        }
        else if (x >= 1 + tmaxf) {
            throw IntegerOverflowError(core::format(
                "Call to vgc::core::ifloor<{}>({:.1f}) overflows ({}Max = {})",
                int_typename<IntType>(), x, int_typename<IntType>(), tmaxi));
        }
        else {
            return static_cast<IntType>(std::floor(x));
        }
    }
    else { // nextafter(tmaxf) - tmaxf > 1
        if (x < tminf) {
            throw IntegerOverflowError(core::format(
                "Call to vgc::core::ifloor<{}>({:.1f}) overflows ({}Min = {})",
                int_typename<IntType>(), x, int_typename<IntType>(), tmini));
        }
        else if (x >= tmaxf) { // See test_arithmetic.cpp for why "+1" is unnecessary
            throw IntegerOverflowError(core::format(
                "Call to vgc::core::ifloor<{}>({:.1f}) overflows ({}Max = {})",
                int_typename<IntType>(), x, int_typename<IntType>(), tmaxi));
        }
        else {
            return static_cast<IntType>(std::floor(x));
        }
    }
}

/// Maps a double \p x in the range [0, 1] to an 8-bit unsigned integer in
/// the range [0..255]. More precisely, the returned value is the integer in
/// [0..255] which is closest to 255*x.
///
VGC_CORE_API
inline UInt8 double01ToUint8(double x) {
    double y = std::round(vgc::core::clamp(x, 0.0, 1.0) * 255.0);
    return static_cast<UInt8>(y);
}

/// Maps an integer \p x in the range [0..255] to a double in the range [0, 1].
/// If the integer is not initially in the range [0..255], then it is first
/// clamped to this range.
///
VGC_CORE_API
inline double uint8ToDouble01(Int x) {
    return vgc::core::clamp(x, Int(0), Int(255)) / 255.0;
}

/// Small epsilon value under which two doubles are considered
/// indistinguishable.
///
const double epsilon = 1e-10;

/// Double-precision pi.
///
/// Note: C++ 20 has std::numbers::pi
///
const double pi = 3.141592653589793238463;

} // namespace core

} // namespace vgc

#endif // VGC_CORE_ARITHMETIC_H
