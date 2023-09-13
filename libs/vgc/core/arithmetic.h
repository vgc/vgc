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

#include <cmath>   // fabs, nextafter, round, pow
#include <cstdint> // int32_t, etc.
#include <limits>  // min, max, infinity
#include <type_traits>

#include <vgc/core/api.h>
#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>
#include <vgc/core/logcategories.h>
#include <vgc/core/templateutil.h>

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

} // namespace vgc

namespace vgc::core {

/// The type trait `TMax<T>` defines a public member typedef `TMax<T>::value`
/// equal to the maximum finite value representable by the arithmetic type `T`.
///
/// This value is equivalent to `(std::numeric_limits<T>::max)()`.
///
/// \sa `tmax<T>`, `TMin<T>`, and `Infinity<T>`.
///
template<typename T>
struct TMax {
    static_assert(
        std::is_arithmetic_v<T>,
        "vgc::core::TMax<T> is only defined for arithmetic types.");
    static constexpr T value = (std::numeric_limits<T>::max)();
};

/// The type trait `TMin<T>` defines a public member typedef `TMin<T>::value`
/// equal to the minimum finite value representable by the arithmetic type `T`.
///
/// For [integral types](https://en.cppreference.com/w/cpp/types/is_integral)
/// (bool, char, int, etc.), this value is equivalent to
/// `std::numeric_limits<T>::min()`.
///
/// For [floating-point types](https://en.cppreference.com/w/cpp/types/is_floating_point)
/// (float, double, etc.), this value is equivalent to
/// `- (std::numeric_limits<T>::max)()`.
///
/// \sa `tmin<T>`, `TMax<T>`, and `SmallestNormal<T>`.
///
template<typename T, typename SFINAE = void>
struct TMin {
    static_assert(
        std::is_arithmetic_v<T>,
        "vgc::core::TMin<T> is only defined for arithmetic types.");
};
template<typename T>
struct TMin<T, Requires<std::is_integral_v<T>>> {
    static constexpr T value = (std::numeric_limits<T>::min)();
};
template<typename T>
struct TMin<T, Requires<std::is_floating_point_v<T>>> {
    static constexpr T value = -(std::numeric_limits<T>::max)();
};

/// The type trait `SmallestNormal<T>` defines a public member typedef
/// `SmallestNormal<T>::value` equal to the smallest non-zero positive normal
/// value representable by the floating point type `T`.
///
/// If `T` is a floating point type, this value is equivalent
/// to `std::numeric_limits<T>::min()`.
///
/// \sa `smallestNormal<T>` and `TMin<T>`.
///
template<typename T>
struct SmallestNormal {
    static_assert(
        std::is_floating_point_v<T>,
        "vgc::core::SmallestNormal<T> is only defined for floating point types.");
    static constexpr T value = (std::numeric_limits<T>::min)();
};

/// The type trait `Infinity<T>` defines a public member typedef
/// `Infinity<T>::value` equal to the value representing infinity for the
/// floating-point type `T`.
///
/// \sa `infinity<T>` and `TMax<T>`
///
template<typename T>
struct Infinity {
    static_assert(
        std::is_floating_point_v<T>,
        "vgc::core::Infinity<T> is only defined for floating point types.");
    static constexpr T value = std::numeric_limits<T>::infinity();
};

/// Maximum finite value representable by the arithmetic type `T`.
///
/// ```cpp
/// std::cout << vgc::core::tmax<bool>          << std::endl; // 1
/// std::cout << vgc::core::tmax<unsigned char> << std::endl; // 255
/// std::cout << vgc::core::tmax<uint32_t>      << std::endl; // 4294967295
/// std::cout << vgc::core::tmax<int32_t>       << std::endl; // 2147483647
/// std::cout << vgc::core::tmax<float>         << std::endl; // 3.40282e+38
/// std::cout << vgc::core::tmax<double>        << std::endl; // 1.79769e+308
/// ```
///
/// \sa `tmin<T>`, `infinity<T>`, `TMax<T>`, and the convenient aliases
///     `IntMax`, `Int32Max`, `UInt32Max`, `FloatMax`, etc.
///
template<typename T>
inline constexpr T tmax = TMax<T>::value;

/// Minimum finite value representable by the arithmetic type `T`.
///
/// ```cpp
/// std::cout << vgc::core::tmin<bool>          << std::endl; // 0
/// std::cout << vgc::core::tmin<unsigned char> << std::endl; // 0
/// std::cout << vgc::core::tmin<uint32_t>      << std::endl; // 0
/// std::cout << vgc::core::tmin<int32_t>       << std::endl; // -2147483648
/// std::cout << vgc::core::tmin<float>         << std::endl; // -3.40282e+38
/// std::cout << vgc::core::tmin<double>        << std::endl; // -1.79769e+308
/// ```
///
/// Note that for floating-point types, this is NOT the same as
/// `std::numeric_limits<T>::min()`, which instead returns the smallest positive
/// normal value.
///
/// \sa `tmax<T>`, `smallestNormal<T>`, `TMin<T>`, and the convenient aliases
///     `IntMin`, `Int32Min`, `UInt32Min`, `FloatMin`, etc.
///
template<typename T>
inline constexpr T tmin = TMin<T>::value;

/// Smallest non-zero positive normal value representable by the floating point
/// type `T`.
///
/// ```cpp
/// std::cout << vgc::core::smallestNormal<float>  << std::endl; // 1.17549e-38
/// std::cout << vgc::core::smallestNormal<double> << std::endl; // 2.22507e-308
/// ```
///
/// \sa `tmin<T>`, `SmallestNormal<T>`, `FloatSmallestNormal`, and `DoubleSmallestNormal`.
///
template<typename T>
inline constexpr T smallestNormal = SmallestNormal<T>::value;

/// Value representing infinity for the floating-point type `T`.
///
/// \sa `tmax<T>`, `Infinity<T>`, `FloatInfinity`, and `DoubleInfinity`.
///
template<typename T>
inline constexpr T infinity = Infinity<T>::value;

/// Maximum value of an Int.
///
inline constexpr Int IntMax = tmax<Int>;

/// Maximum value of an Int8.
///
inline constexpr Int8 Int8Max = tmax<Int8>;

/// Maximum value of an Int16.
///
inline constexpr Int16 Int16Max = tmax<Int16>;

/// Maximum value of an Int32.
///
inline constexpr Int32 Int32Max = tmax<Int32>;

/// Maximum value of an Int64.
///
inline constexpr Int64 Int64Max = tmax<Int64>;

/// Maximum value of a UInt.
///
inline constexpr UInt UIntMax = tmax<UInt>;

/// Maximum value of a UInt8.
///
inline constexpr UInt8 UInt8Max = tmax<UInt8>;

/// Maximum value of a UInt16.
///
inline constexpr UInt16 UInt16Max = tmax<UInt16>;

/// Maximum value of a UInt32.
///
inline constexpr UInt32 UInt32Max = tmax<UInt32>;

/// Maximum value of a UInt64.
///
inline constexpr UInt64 UInt64Max = tmax<UInt64>;

/// Maximum finite value of a float.
///
inline constexpr float FloatMax = tmax<float>;

/// Maximum finite value of a double.
///
inline constexpr double DoubleMax = tmax<double>;

/// Minimum value of an Int.
///
inline constexpr Int IntMin = tmin<Int>;

/// Minimum value of an Int8.
///
inline constexpr Int8 Int8Min = tmin<Int8>;

/// Minimum value of an Int16.
///
inline constexpr Int16 Int16Min = tmin<Int16>;

/// Minimum value of an Int32.
///
inline constexpr Int32 Int32Min = tmin<Int32>;

/// Minimum value of an Int64.
///
inline constexpr Int64 Int64Min = tmin<Int64>;

/// Minimum value of a UInt.
///
inline constexpr UInt UIntMin = tmin<UInt>;

/// Minimum value of a UInt8.
///
inline constexpr UInt8 UInt8Min = tmin<UInt8>;

/// Minimum value of a UInt16.
///
inline constexpr UInt16 UInt16Min = tmin<UInt16>;

/// Minimum value of a UInt32.
///
inline constexpr UInt32 UInt32Min = tmin<UInt32>;

/// Minimum value of a UInt64.
///
inline constexpr UInt64 UInt64Min = tmin<UInt64>;

/// Minimum finite value of a float.
///
/// Note: this is a very large negative number, and shouldn't be confused
/// with FLT_MIN or std::numeric_limits<float>::min(), which are instead
/// the smallest positive non-zero normal value.
///
/// \sa `FloatSmallestNormal`.
///
inline constexpr float FloatMin = tmin<float>;

/// Minimum finite value of a double.
///
/// Note: this is a very large negative number, and shouldn't be confused
/// with DBL_MIN or std::numeric_limits<double>::min(), which are instead
/// the smallest positive non-zero normal value.
///
/// \sa `DoubleSmallestNormal`.
///
inline constexpr double DoubleMin = tmin<double>;

/// Smallest non-zero positive normal value representable by a `float`.
///
/// \sa `FloatMin`.
///
inline constexpr float FloatSmallestNormal = smallestNormal<float>;

/// Smallest non-zero positive normal value representable by a `double`.
///
/// \sa `DoubleMin`.
///
inline constexpr double DoubleSmallestNormal = smallestNormal<double>;

/// Positive infinity value of a `float`.
///
inline constexpr float FloatInfinity = infinity<float>;

/// Positive infinity value of a `double`.
///
inline constexpr double DoubleInfinity = infinity<double>;

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

template<typename T, VGC_REQUIRES(std::is_integral_v<T>)>
inline std::string int_typename() {
    return "UnknownInt";
}

// clang-format off
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
// clang-format on

namespace detail {

template<typename T, typename U>
std::string intErrorReason(U value) {
    return core::format(
        "Cannot convert {}({}) to type {}", int_typename<U>(), value, int_typename<T>());
}

template<typename T, typename U>
void throwIntegerOverflowError(U value) {
    throw core::IntegerOverflowError(intErrorReason<T>(value));
}

template<typename T, typename U>
void throwNegativeIntegerError(U value) {
    throw core::NegativeIntegerError(intErrorReason<T>(value));
}

} // namespace detail

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
#    pragma warning(push)
#    pragma warning(disable : 4804) // unsafe use of type 'bool' in operation
#endif

// clang-format off

// int_cast from from U to T when:
// - U and T are both signed or both unsigned
// - The range of T includes the range of U.
//
template<typename T, typename U, VGC_REQUIRES(
    std::is_integral_v<T>
    && std::is_integral_v<U>
    && std::is_signed_v<T> == std::is_signed_v<U>
    && tmax<T> >= tmax<U>)> // (1)
T int_cast(U value) {
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U and T are both signed
// - The range of T does not include the range of U.
//
template<typename T, typename U, VGC_REQUIRES(
    std::is_integral_v<T>
    && std::is_integral_v<U>
    && std::is_signed_v<T>
    && std::is_signed_v<U>
    && tmax<T> < tmax<U>)>
T int_cast(U value) {
    if (value < tmin<T> || // (1)
        value > tmax<T>) { // (1)
        detail::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U and T are both unsigned
// - The range of T does not include the range of U.
//
template<typename T, typename U, VGC_REQUIRES(
    std::is_integral_v<T>
    && std::is_integral_v<U>
    && std::is_unsigned_v<T>
    && std::is_unsigned_v<U>
    && tmax<T> < tmax<U>)>
T int_cast(U value) {
    if (value > tmax<T>) { // (1)
        detail::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U is unsigned and T is signed
// - The range of T includes the range of U
//
template<typename T, typename U, VGC_REQUIRES(
    std::is_integral_v<T>
    && std::is_integral_v<U>
    && std::is_signed_v<T>
    && std::is_unsigned_v<U>
    && tmax<T> >= tmax<U>)> // (2)
T int_cast(U value) {
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U is unsigned and T is signed
// - The range of T does not include the range of U
//
template<typename T, typename U, VGC_REQUIRES(
    std::is_integral_v<T>
    && std::is_integral_v<U>
    && std::is_signed_v<T>
    && std::is_unsigned_v<U>
    && tmax<T> < tmax<U>)> // (2)
T int_cast(U value) {
    if (value > tmax<T>) { // (2)
        detail::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U is signed and T is unsigned
// - The range of T includes the positive range of U
//
template<typename T, typename U, VGC_REQUIRES(
    std::is_integral_v<T>
    && std::is_integral_v<U>
    && std::is_unsigned_v<T>
    && std::is_signed_v<U>
    && tmax<T> >= tmax<U>)> // (2)
T int_cast(U value) {
    if (value < 0) { // 0 is promoted to int => (1)
        detail::throwNegativeIntegerError<T>(value);
    }
    return static_cast<T>(value);
}

// int_cast from from U to T when:
// - U is signed and T is unsigned
// - The range of T does not include the positive range of U
//
template<typename T, typename U, VGC_REQUIRES(
    std::is_integral_v<T>
    && std::is_integral_v<U> 
    && std::is_unsigned_v<T>
    && std::is_signed_v<U>
    && tmax<T> < tmax<U>)> // (2)
T int_cast(U value) {
    if (value < 0) { // 0 is promoted to int => (1)
        detail::throwNegativeIntegerError<T>(value);
    }
    else if (value > tmax<T>) { // (2)
        detail::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

// clang-format on

#if defined(VGC_CORE_COMPILER_MSVC)
#    pragma warning(pop)
#endif

/// Sets the given bool, character, integer or floating-point number to zero.
///
/// This function is called by vgc::core::zero<T>(), and can be overloaded for
/// custom types.
///
/// \sa zero<T>()
///
template<typename T, VGC_REQUIRES(std::is_arithmetic_v<T>)>
void setZero(T& x) {
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
T zero() {
    T res;
    setZero(res);
    return res;
}

namespace detail {

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
template<typename T, VGC_REQUIRES(std::is_floating_point_v<T>)>
T infdiff(T a, T b) {
    return (a == b) ? zero<T>() : (a - b);
}

template<typename T, VGC_REQUIRES(std::is_floating_point_v<T>)>
bool isClose(T a, T b, T relTol, T absTol) {
    T diff = std::fabs(infdiff(a, b));
    if (diff == std::numeric_limits<T>::infinity()) {
        return false; // opposite infinities or finite/infinite mismatch
    }
    else {
        return diff <= std::fabs(relTol * b) || diff <= std::fabs(relTol * a)
               || diff <= absTol;
    }
}

template<typename T, VGC_REQUIRES(std::is_floating_point_v<T>)>
bool isNear(T a, T b, T absTol) {
    T diff = std::fabs(infdiff(a, b));
    if (diff == std::numeric_limits<T>::infinity()) {
        return false; // opposite infinities or finite/infinite mismatch
    }
    else {
        return diff <= absTol;
    }
}

} // namespace detail

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
inline bool isClose(float a, float b, float relTol = 1e-5f, float absTol = 0.0f) {
    return detail::isClose(a, b, relTol, absTol);
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
inline bool isClose(double a, double b, double relTol = 1e-9, double absTol = 0.0) {
    return detail::isClose(a, b, relTol, absTol);
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
inline bool isNear(float a, float b, float absTol) {
    return detail::isNear(a, b, absTol);
}

/// Returns whether the absolute difference between two double values is within
/// the given tolerance. Please see the documentation of the `float` overload
/// for details.
///
VGC_CORE_API
inline bool isNear(double a, double b, double absTol) {
    return detail::isNear(a, b, absTol);
}

/// Returns the given value clamped to the interval [min, max]. If max < min,
/// then a warning is issued and the value is clamped to [max, min] instead.
///
template<typename T>
constexpr const T&
clamp(const T& value, const TypeIdentity<T>& min, const TypeIdentity<T>& max) {
    if (max < min) {
        VGC_WARNING(
            LogVgcCore,
            "vgc::core::clamp(value={}, min={}, max={}) called with max < min.",
            value,
            min,
            max);
        return (value < max) ? max : (min < value) ? min : value;
    }
    else {
        return (value < min) ? min : (max < value) ? max : value;
    }
}

/// Returns the next representable floating point. This is equivalent to:
///
/// ```cpp
/// std::nextafter(x, vgc::core::tmax<FloatType>);
/// ```
///
template<typename FloatType, VGC_REQUIRES(std::is_floating_point_v<FloatType>)>
FloatType nextafter(FloatType x) {
    return std::nextafter(x, tmax<FloatType>);
}

/// Returns the previous representable floating point. This is equivalent to:
///
/// ```cpp
/// std::nextafter(x, -vgc::core::tmax<FloatType>);
/// ```
///
template<typename FloatType, VGC_REQUIRES(std::is_floating_point_v<FloatType>)>
FloatType nextbefore(FloatType x) {
    return std::nextafter(x, -tmax<FloatType>);
}

/// Unchecked casting between arithmetic types that may cause narrowing.
///
/// This is the same as `static_cast` but clarifies intent and makes it easy to
/// search in the codebase.
///
/// If you need to convert from a floating point type to an integer type, it is
/// recommended to use `trunc_cast()` instead, which is equivalent but clarifies
/// intent even more. If you need a different rounding behavior, see
/// `floor_cast()`, `ceil_cast()`, and `round_cast()`. If you need checked
/// casting, see `ifloor()`.
///
/// \sa `trunc_cast()`, `floor_cast()`, `ceil_cast()`, `round_cast()`, `ifloor()`.
///
template<
    typename T,
    typename U,
    VGC_REQUIRES(std::is_arithmetic_v<T>&& std::is_arithmetic_v<U>)>
constexpr T narrow_cast(U x) noexcept {
    return static_cast<T>(x);
}

/// Unchecked casting (possibly narrowing) from the given floating point `x`
/// into an integer type, using truncation behavior (rounds towards zero).
///
/// This is the same as `static_cast` but clarifies intent.
///
/// \sa `narrow_cast()`, `floor_cast()`, `ceil_cast()`, `round_cast()`.
///
template<
    typename IntType,
    typename FloatType,
    VGC_REQUIRES(std::is_integral_v<IntType>&& std::is_floating_point_v<FloatType>)>
constexpr IntType trunc_cast(FloatType x) noexcept {
    return static_cast<IntType>(x);
}

/// Unchecked casting (possibly narrowing) from the given floating point `x`
/// into an integer type, using floor behavior (rounds towards -infinity).
///
/// \sa `narrow_cast()`, `trunc_cast()`, `ceil_cast()`, `round_cast()`, `ifloor()`.
///
template<
    typename IntType,
    typename FloatType,
    VGC_REQUIRES(std::is_integral_v<IntType>&& std::is_floating_point_v<FloatType>)>
constexpr IntType floor_cast(FloatType x) noexcept {
    if (x >= 0) {
        return static_cast<IntType>(x);
    }
    else {
        // Note: In C++23, std::floor becomes constexpr.
        // We might then want to use the following (after performance comparison):
        // return static_cast<IntType>(std::floor(x));
        IntType yi = static_cast<IntType>(x);
        FloatType y = static_cast<FloatType>(yi);
        if (x == y) {
            return yi;
        }
        else {
            return yi - 1;
        }
    }
}

/// Unchecked casting (possibly narrowing) from the given floating point `x`
/// into an integer type, using ceil behavior (rounds towards +infinity).
///
/// \sa `narrow_cast()`, `trunc_cast()`, `floor_cast()`, `round_cast()`.
///
template<
    typename IntType,
    typename FloatType,
    VGC_REQUIRES(std::is_integral_v<IntType>&& std::is_floating_point_v<FloatType>)>
constexpr IntType ceil_cast(FloatType x) noexcept {
    if (x <= 0) {
        return static_cast<IntType>(x);
    }
    else {
        // Note: In C++23, std::ceil becomes constexpr.
        // We might then want to use the following (after performance comparison):
        // return static_cast<IntType>(std::ceil(x));
        IntType yi = static_cast<IntType>(x);
        FloatType y = static_cast<FloatType>(yi);
        if (x == y) {
            return yi;
        }
        else {
            return yi + 1;
        }
    }
}

/// Unchecked casting (possibly narrowing) from the given floating point `x`
/// into an integer type, using round behavior (rounds towards closest integer,
/// or away from 0 if exactly at 0.5 between two integers).
///
/// \sa `narrow_cast()`, `trunc_cast()`, `floor_cast()`, `ceil_cast()`.
///
template<
    typename IntType,
    typename FloatType,
    VGC_REQUIRES(std::is_integral_v<IntType>&& std::is_floating_point_v<FloatType>)>
constexpr IntType round_cast(FloatType x) noexcept {
    constexpr FloatType one = 1;
    constexpr FloatType half = one / 2;
    if (x >= 0) {
        return static_cast<IntType>(x + half);
    }
    else {
        return static_cast<IntType>(x - half);
    }
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
// clang-format off
template<typename IntType, typename FloatType,
    VGC_REQUIRES(std::is_floating_point_v<FloatType> && std::is_integral_v<IntType>)>
// clang-format on
IntType ifloor(FloatType x) {
    constexpr IntType tmini = tmin<IntType>;
    constexpr IntType tmaxi = tmax<IntType>;
    constexpr FloatType tminf = static_cast<FloatType>(tmin<IntType>);
    constexpr FloatType tmaxf = static_cast<FloatType>(tmax<IntType>);
    // Note: the outer "if" should be optimized out at compile time based on FloatType and IntType
    if constexpr (tmaxf < 1 + tmaxf) { // all IntType integers representable as FloatType
        if (x < tminf) {
            throw IntegerOverflowError(core::format(
                "Call to vgc::core::ifloor<{}>({:.1f}) overflows ({}Min = {})",
                int_typename<IntType>(),
                x,
                int_typename<IntType>(),
                tmini));
        }
        else if (x >= 1 + tmaxf) {
            throw IntegerOverflowError(core::format(
                "Call to vgc::core::ifloor<{}>({:.1f}) overflows ({}Max = {})",
                int_typename<IntType>(),
                x,
                int_typename<IntType>(),
                tmaxi));
        }
        else {
            return static_cast<IntType>(std::floor(x));
        }
    }
    else { // nextafter(tmaxf) - tmaxf > 1
        if (x < tminf) {
            throw IntegerOverflowError(core::format(
                "Call to vgc::core::ifloor<{}>({:.1f}) overflows ({}Min = {})",
                int_typename<IntType>(),
                x,
                int_typename<IntType>(),
                tmini));
        }
        else if (x >= tmaxf) { // See test_arithmetic.cpp for why "+1" is unnecessary
            throw IntegerOverflowError(core::format(
                "Call to vgc::core::ifloor<{}>({:.1f}) overflows ({}Max = {})",
                int_typename<IntType>(),
                x,
                int_typename<IntType>(),
                tmaxi));
        }
        else {
            return static_cast<IntType>(std::floor(x));
        }
    }
}

/// Returns a power of ten as a `float`.
///
/// This is equivalent to `std::pow(10.0f, static_cast<float>(exp))`, but much
/// faster for small values of `exp` (between -10 and 10) which are optimized
/// via a table lookup.
///
/// Note that the returned value is exact from 10^0 to 10^10, while none of the other
/// powers of ten can be exactly represented as a `float`.
///
VGC_CORE_API
float pow10f(Int exp);

/// Returns a power of ten as a `double`.
///
/// This is equivalent to `std::pow(10.0, static_cast<double>(exp))`, but much
/// faster for small values of `exp` (between -22 and 22) which are optimized
/// via a table lookup.
///
/// Note that the returned value is exact from 10^0 to 10^22, while none of the other
/// powers of ten can be exactly represented as a `double`.
///
VGC_CORE_API
double pow10d(Int exp);

/// Returns a power of ten as a floating point.
///
/// This is equivalent to:
///
/// ```cpp
/// std::pow(static_cast<FloatType>(10), static_cast<FloatType>(exp))
/// ```
///
/// but much faster for small values of `exp` which are optimized via a table lookup.
///
/// \sa `pow10f()`, `pow10d()`.
///
template<typename FloatType, VGC_REQUIRES(std::is_floating_point_v<FloatType>)>
FloatType pow10(Int exp) {
    // Generic implementation, e.g. for long double or extended floating-point types.
    return std::pow(static_cast<FloatType>(10), static_cast<FloatType>(exp));
}

// float specialization of pow10()
template<>
inline float pow10<float>(Int exp) {
    return pow10f(exp);
}

// double specialization of pow10()
template<>
inline double pow10<double>(Int exp) {
    return pow10d(exp);
}

/// \enum vgc::core::PrecisionMode
/// \brief Specifies a precision mode for rounding numbers.
///
/// \sa `Precision`, `round()`, `roundToDecimals()`, `roundToSignificantDigits()`.
///
enum class PrecisionMode : Int8 {

    /// Do not round, that is, keep numbers with their current precision.
    ///
    Unrestricted,

    /// Round to a specified number of base-10 fractional digits.
    ///
    Decimals,

    /// Round to a specified number of base-10 significant digits.
    ///
    SignificantDigits
};
// TODO? BinaryDecimals, BinarySignificantDigits

/// \class vgc::core::Precision
/// \brief Specifies a given precision mode and value for rounding numbers.
///
/// This class can be used to specify a given precision mode and value for
/// rounding numbers.
///
/// ```cpp
/// vgc::core::Precision precision(vgc::core::PrecisionMode::Decimals, 2);
/// float pi = 3.1415;
/// std::cout << vgc::core::round(pi, precision); // => 3.14
/// ```
///
/// \sa `PrecisionMode`, `round()`, `roundToDecimals()`, `roundToSignificantDigits()`.
///
class Precision {
public:
    /// Specifies a `Precision` with the given `mode` and `value`.
    ///
    constexpr Precision(PrecisionMode mode = PrecisionMode::Unrestricted, Int8 value = 0)
        : mode_(mode)
        , value_(value) {
    }

    /// Returns the `PrecisionMode` of this `Precision`.
    ///
    constexpr PrecisionMode mode() const {
        return mode_;
    }

    /// Sets the `PrecisionMode` of this `Precision`.
    ///
    constexpr void setMode(PrecisionMode mode) {
        mode_ = mode;
    }

    /// Returns the precision value of this `Precision`.
    ///
    constexpr Int8 value() const {
        return value_;
    }

    /// Sets the precision value of this precision `Precision`.
    ///
    constexpr void setValue(Int8 value) {
        value_ = value;
    }

    /// Returns whether the two precisons `p1` and `p2` are equal, that is,
    /// whether they have the same mode and the same value.
    ///
    friend bool operator==(Precision p1, Precision p2) {
        return p1.mode_ == p2.mode_ && p1.value_ == p2.value_;
    }

    /// Returns whether the two precisons `p1` and `p2` are different.
    ///
    friend bool operator!=(Precision p1, Precision p2) {
        return !(p1 == p2);
    }

    /// Compares the two precisions `p1` and `p2` using the lexicographic order
    /// on (`mode()`, `value()`).
    ///
    friend bool operator<(Precision p1, Precision p2) {
        // clang-format off
        return ( (p1.mode_ < p2.mode_) ||
                (!(p2.mode_ < p1.mode_) &&
                 ( (p1.value_ < p2.value_))));
        // clang-format on
    }

    /// Compares the two precisions `p1` and `p2` using the lexicographic order
    /// on (`mode()`, `value()`).
    ///
    friend bool operator<=(Precision p1, Precision p2) {
        return !(p2 < p1);
    }

    /// Compares the two precisions `p1` and `p2` using the lexicographic order
    /// on (`mode()`, `value()`).
    ///
    friend bool operator>(Precision p1, Precision p2) {
        return p2 < p1;
    }

    /// Compares the two precisions `p1` and `p2` using the lexicographic order
    /// on (`mode()`, `value()`).
    ///
    friend bool operator>=(Precision p1, Precision p2) {
        return !(p1 < p2);
    }

private:
    PrecisionMode mode_;
    Int8 value_;
};

/// Rounds the floating point `x` to the given number of base-10 fractional digits.
///
/// ```cpp
/// float pi = 3.14159;
/// std::cout << vgc::core::roundToDecimals(pi, 2); // => 3.14
/// std::cout << vgc::core::roundToDecimals(428.3, -1); // => 430.0
/// ```
///
/// \sa `round()`, `roundToDecimals()`, `roundToSignificantDigits()`.
///
template<typename FloatType, VGC_REQUIRES(std::is_floating_point_v<FloatType>)>
FloatType roundToDecimals(FloatType x, Int numDigits) {
    if (numDigits == 0) {
        return std::round(x);
    }
    else if (numDigits > 0) {
        FloatType s = pow10<FloatType>(numDigits); // exact for small numDigits
        return std::round(x * s) / s;
    }
    else {
        FloatType s = pow10<FloatType>(-numDigits); // exact for small (-numDigits)
        return std::round(x / s) * s;
    }
}

/// Rounds the floating point `x` to the given number of base-10 significant digits.
///
/// ```cpp
/// float pi = 3.14159;
/// float pj = 314.159;
/// std::cout << vgc::core::roundToSignificantDigits(pi, 2); // => 3.1
/// std::cout << vgc::core::roundToSignificantDigits(pj, 2); // => 310
/// ```
///
/// If `numDigits` is negative, this always returns 0.
///
/// If `numDigits` is equal to zero, this returns either 0 or round to the next
/// power of ten.
///
/// ```cpp
/// vgc::core::roundToSignificantDigits(583.14159, 4); // =>  583.1
/// vgc::core::roundToSignificantDigits(583.14159, 3); // =>  583.0
/// vgc::core::roundToSignificantDigits(583.14159, 2); // =>  580.0
/// vgc::core::roundToSignificantDigits(583.14159, 1); // =>  600.0
/// vgc::core::roundToSignificantDigits(583.14159, 0); // => 1000.0
/// vgc::core::roundToSignificantDigits(583.14159, 0); // =>    0.0
/// ```
///
template<typename FloatType, VGC_REQUIRES(std::is_floating_point_v<FloatType>)>
FloatType roundToSignificantDigits(FloatType x, Int numDigits) {

    // Fast return if x equals zero, which doesn't have a magnitude.
    //
    if (x == 0) {
        return x;
    }

    // Compute the "magnitude" of the number, that is, the highest
    // non-null power of ten in the decimal representation of the number.
    //
    // Example: 876 = 8 x 10^2 + 7 x 10^1 + 6 x 10^0, so its magnitude is 2.
    //
    // Desired behavior at the boundary between magnitudes:
    //
    // magnitude(99)  == 1
    // magnitude(100) == 2
    // magnitude(101) == 2
    //
    // Note: we might be able to implement a faster version not using log10,
    // since we do not need all the decimals provided by log10 (we only need
    // the integer part).
    //
    Int magnitude = static_cast<Int>(std::floor(std::log10(std::abs(x))));

    // Deduce how many decimals we need to round the number to.
    //
    return roundToDecimals(x, numDigits - magnitude - 1);
}

/// Rounds the floating point `x` to the given `precision`.
///
template<typename FloatType, VGC_REQUIRES(std::is_floating_point_v<FloatType>)>
FloatType round(FloatType x, Precision precision) {
    switch (precision.mode()) {
    case PrecisionMode::Unrestricted:
        return x;
    case PrecisionMode::Decimals:
        return roundToDecimals(x, precision.value());
    case PrecisionMode::SignificantDigits:
        return roundToSignificantDigits(x, precision.value());
    }
    return x;
}

/// Linearly interpolates floating point values using the following
/// formula: `(1 - t) * a + t * b`.
///
template<typename FloatType, VGC_REQUIRES(std::is_floating_point_v<FloatType>)>
FloatType
fastLerp(core::TypeIdentity<FloatType> a, core::TypeIdentity<FloatType> b, FloatType t) {
    return (1 - t) * a + t * b;
}

/// Linearly interpolates custom type values using the following
/// formula: `(1 - t) * a + t * b`, for types that support these operations.
///
template<
    typename ValueType,
    typename FloatType,
    VGC_REQUIRES(std::is_floating_point_v<FloatType> && !std::is_arithmetic_v<ValueType>)>
ValueType
fastLerp(const ValueType& a, const ValueType& b, FloatType t) {
    return (1 - t) * a + t * b;
}

/// \struct vgc::core::NoInit
/// \brief Tag to select a function overload that doesn't perform initialization.
///
/// `NoInit` is a tag-like structure used to select a function overload
/// (typically, a constructor) that doesn't perform initialization.
///
/// Example:
///
/// ```cpp
/// vgc::geometry::Vec2d v;                          // (0, 0)
/// vgc::geometry::Vec2d v();                        // (0, 0)
/// vgc::geometry::Vec2d v{};                        // (0, 0)
/// vgc::geometry::Vec2d v(vgc::core::noInit);       // (?, ?)
///
/// vgc::core::Array<int> a(3);                      // [0, 0, 0]
/// vgc::core::Array<int> a(3, vgc::core::noInit);   // [?, ?, ?]
/// ```
///
struct VGC_CORE_API NoInit {};
inline constexpr NoInit noInit = {};

/// \struct vgc::core::UncheckedInit
/// \brief Tag to select a function overload that doesn't perform checks.
///
/// `UncheckedInit` is a tag-like structure used to select a function overload
/// (typically, a constructor) that doesn't perform checks.
///
struct VGC_CORE_API UncheckedInit {};
inline constexpr UncheckedInit uncheckedInit = {};

/// Small epsilon value under which two doubles are considered
/// indistinguishable.
///
inline constexpr double epsilon = 1e-10;

/// Double-precision pi.
///
/// Note: C++ 20 has std::numbers::pi
///
inline constexpr double pi = 3.141592653589793238463;

} // namespace vgc::core

#endif // VGC_CORE_ARITHMETIC_H
