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

#ifndef VGC_CORE_INT_H
#define VGC_CORE_INT_H

/// \file vgc/core/int.h
/// \brief Defines integer types and integer-related utilities.
///
/// In this header, we define various integer types used throughout the C++ VGC
/// API and implementation, such as vgc::Int, vgc::UInt, vgc::Int64, etc.
///
/// # Rationale
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
/// For convenience and consistency, we also specify typedefs (e.g.,
/// vgc::Int32) for the fixed width integer types provided in the standard
/// <cstdint> header (e.g., std::int32_t).
///
/// # vgc::Int vs vgc::UInt
///
/// Throughout VGC, we use and recommend using the signed integer vgc::Int for
/// most purposes, including array sizes and indices. We recognize that this is
/// somewhat non-idiomatic C++, given that the STL uses unsigned int for sizes
/// and indices. However, many authors of the STL themselves acknowledge that
/// using unsigned integers was a mistake, see:
///
/// http://channel9.msdn.com/Events/GoingNative/2013/Interactive-Panel-Ask-Us-Anything
/// (at 12min, 42min, and 63min)
///
/// In particular, type casting between signed and unsigned integers is
/// error-prone and produces less-readable code. Therefore, we prefer using
/// only one integer type, and since we can't avoid signed integers (index
/// offsets), using signed integers is the most obvious choice.
///
/// Also, note that unsigned integers use modulo arithmetic which may lead to
/// subtle and hard to detect bugs, for example in loops with decreasing
/// indices or in expressions involving substractions.
///
/// Finally, using signed integers makes the C++ API more consistent with the
/// Python API, since Python integers are always signed.
///
/// In order to alleviate the fact that the STL does use unsigned integers, we
/// provide our own containers which wrap the STL containers and perform the
/// type casts for you, along with additional bound checks.
///
/// # Why not vgc::core::Int?
///
/// You may have noticed that the VGC integer types are defined directly in the
/// vgc namespace (e.g., vgc::Int), rather than in the nested vgc::core
/// namespace where they more naturally belong, since they are defined in
/// <vgc/core/int.h>.
///
/// This is the only exception of our namespace rules, justified by the
/// extremely widespread use of these types, and the fact that they are not
/// wrapped in Python anyway (we want all Python types to be defined within
/// submodules, e.g., vgc.core.Vec2d).
///
/// # Integer type casting
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

// Note: the actual definitions of integer types and their limits are in a
// separate header to prevent a circular dependency with stringutil.h:
//
//         int.h
//           |
//           v
//      stringutil.h
//           |
//           v
//       inttypes.h
//

#include <type_traits>

#include <vgc/core/exceptions.h>
#include <vgc/core/inttypes.h>
#include <vgc/core/stringutil.h>

namespace vgc {

/// Returns a human-readable name for integer types.
///
/// Examples:
///
/// \code
/// std::cout << vgc::int_typename<vgc::Int8>()     << std::end; // => "Int8"
/// std::cout << vgc::int_typename<vgc::UInt16>()   << std::end; // => "UInt16"
/// std::cout << vgc::int_typename<vgc::Int>()      << std::end; // => "Int64" (or "Int32" if VGC_CORE_USE_32BIT_INT is defined)
/// std::cout << vgc::int_typename<bool>()          << std::end; // => "Bool"
/// std::cout << vgc::int_typename<char>()          << std::end; // => "Char"
/// std::cout << vgc::int_typename<signed char>()   << std::end; // => "Int8"
/// std::cout << vgc::int_typename<unsigned char>() << std::end; // => "UInt8"
/// std::cout << vgc::int_typename<short>()         << std::end; // => "Int16" (on most platforms)
/// std::cout << vgc::int_typename<int>()           << std::end; // => "Int32" (on most platforms)
/// std::cout << vgc::int_typename<long>()          << std::end; // => "Int32" or "Int64" (on most platforms)
/// std::cout << vgc::int_typename<long long>()     << std::end; // => "Int64" (on most platforms)
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
    return "Cannot convert " + int_typename<U>() + "(" +
            core::toString(value) + ") to type " + int_typename<T>();
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

// int_cast from from U to T when:
// - U and T are both signed or both unsigned
// - The range of T includes the range of U.
//
template <typename T, typename U>
typename std::enable_if<
    std::is_integral<T>::value &&
    std::is_integral<U>::value &&
    std::is_signed<T>::value == std::is_signed<U>::value &&
    internal::type_max<T>::value >= internal::type_max<U>::value, // (1)
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
    internal::type_max<T>::value < internal::type_max<U>::value, // (1)
    T>::type
int_cast(U value) {
    if (value < internal::type_min<T>::value || // (1)
        value > internal::type_max<T>::value) { // (1)
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
    internal::type_max<T>::value < internal::type_max<U>::value, // (1)
    T>::type
int_cast(U value) {
    if (value > internal::type_max<T>::value) { // (1)
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
    internal::type_max<T>::value >= internal::type_max<U>::value, // (2)
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
    internal::type_max<T>::value < internal::type_max<U>::value, // (2)
    T>::type
int_cast(U value) {
    if (value > internal::type_max<T>::value) { // (2)
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
    internal::type_max<T>::value >= internal::type_max<U>::value, // (2)
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
    internal::type_max<T>::value < internal::type_max<U>::value, // (2)
    T>::type
int_cast(U value) {
    if (value < 0) { // 0 is promoted to int => (1)
        internal::throwNegativeIntegerError<T>(value);
    }
    else if (value > internal::type_max<T>::value) { // (2)
        internal::throwIntegerOverflowError<T>(value);
    }
    return static_cast<T>(value);
}

} // namespace vgc

#endif // VGC_CORE_INT_H
