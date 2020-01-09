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
/// \brief Defines integer types used in VGC.
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

#include <cstdint>

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

#endif // VGC_CORE_INT_H
