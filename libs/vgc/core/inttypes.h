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

#ifndef VGC_CORE_INTTYPES_H
#define VGC_CORE_INTTYPES_H

#include <cstdint>
#include <limits>

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

namespace internal {

// We need this additional level of indirection to workaround a bug in MSVC.
// See: https://stackoverflow.com/questions/59743372
//
// Once we require C++14, we could use a more elegant solution based on
// variable templates:
//
//   template<typename T>
//   constexpr T type_max = (std::numeric_limits<T>::max)();
//
// Once we require C++17, we could use an even more elegant solution based on
// "if constexpr" rather than SFINAE.
//
// Note: The extra parentheses around std::numeric_limits<T>::min/max avoid
// potential problems with min/max macros in WinDef.h.
//
template<typename T>
struct type_max {
    static constexpr T value = (std::numeric_limits<T>::max)();
};
template<typename T>
struct type_min {
    static constexpr T value = (std::numeric_limits<T>::min)();
};

} // namespace internal

/// Maximum value of an Int.
///
constexpr Int IntMax = internal::type_max<Int>::value;

/// Maximum value of an Int8.
///
constexpr Int8 Int8Max = internal::type_max<Int8>::value;

/// Maximum value of an Int16.
///
constexpr Int16 Int16Max = internal::type_max<Int16>::value;

/// Maximum value of an Int32.
///
constexpr Int32 Int32Max = internal::type_max<Int32>::value;

/// Maximum value of an Int64.
///
constexpr Int64 Int64Max = internal::type_max<Int64>::value;

/// Maximum value of a UInt.
///
constexpr UInt UIntMax = internal::type_max<UInt>::value;

/// Maximum value of a UInt8.
///
constexpr UInt8 UInt8Max = internal::type_max<UInt8>::value;

/// Maximum value of a UInt16.
///
constexpr UInt16 UInt16Max = internal::type_max<UInt16>::value;

/// Maximum value of a UInt32.
///
constexpr UInt32 UInt32Max = internal::type_max<UInt32>::value;

/// Maximum value of a UInt64.
///
constexpr UInt64 UInt64Max = internal::type_max<UInt64>::value;

/// Minimum value of an Int.
///
constexpr Int IntMin = internal::type_min<Int>::value;

/// Minimum value of an Int8.
///
constexpr Int8 Int8Min = internal::type_min<Int8>::value;

/// Minimum value of an Int16.
///
constexpr Int16 Int16Min = internal::type_min<Int16>::value;

/// Minimum value of an Int32.
///
constexpr Int32 Int32Min = internal::type_min<Int32>::value;

/// Minimum value of an Int64.
///
constexpr Int64 Int64Min = internal::type_min<Int64>::value;

/// Minimum value of a UInt.
///
constexpr UInt UIntMin = internal::type_min<UInt>::value;

/// Minimum value of a UInt8.
///
constexpr UInt8 UInt8Min = internal::type_min<UInt8>::value;

/// Minimum value of a UInt16.
///
constexpr UInt16 UInt16Min = internal::type_min<UInt16>::value;

/// Minimum value of a UInt32.
///
constexpr UInt32 UInt32Min = internal::type_min<UInt32>::value;

/// Minimum value of a UInt64.
///
constexpr UInt64 UInt64Min = internal::type_min<UInt64>::value;

} // namespace vgc

#endif // VGC_CORE_INTTYPES_H
