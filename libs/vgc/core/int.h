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
/// \brief Provides short type aliases for integer types (i64, etc.)
///
/// This header defines the following fixed-width signed integer types using
/// 2's complement representation (respectively, 8-bit, 16-bit, 32-bit, and
/// 63-bit):
///
/// \code
/// vgc::i8
/// vgc::i16
/// vgc::i32
/// vgc::i64
/// \endcode
///
/// And the following fixed-width unsigned integer types (respectively, 8-bit,
/// 16-bit, 32-bit, and 63-bit):
///
/// \code
/// vgc::u8
/// vgc::u16
/// vgc::u32
/// vgc::u64
/// \endcode
///
/// These are simply type aliases of the corresponding integer types provided
/// by the C++ standard header <cstdint>, such as std::int64_t, but with
/// shorter names for convenience and readability.
///
/// For increased convenience, note that these types are defined directly in
/// the vgc namespace, not in the nested vgc::core namespace. This is an
/// exception, which was justified by the extremely widespread use of these
/// types, and the fact that they are not wrapped in Python.
///
/// The recommendantion for VGC libraries is to use i64 by default for all
/// integers, including values which are not supposed to be negative, such as
/// sizes and indices.
///

#include <cstdint>

namespace vgc {

/// The 8-bit signed integer type. Prefer using i64 in almost all situations.
///
using i8 = std::int8_t;

/// The 16-bit signed integer type. Prefer using i64 in almost all situations.
///
using i16 = std::int16_t;

/// The 32-bit signed integer type. Prefer using i64 in almost all situations.
///
using i32 = std::int32_t;

/// The 64-bit signed integer type. This is the preferred integer type for all
/// purposes in the VGC codebase, including for integers which are not supposed
/// to be negative, such as container sizes, indices, etc. In other words, we
/// prefer consistency and minimizing the risk of integer overflow over
/// performance.
///
using i64 = std::int64_t;

/// The 8-bit unsigned integer type. Prefer using i64 in almost all situations.
///
using u8 = std::uint8_t;

/// The 16-bit unsigned integer type. Prefer using i64 in almost all
/// situations.
///
using u16 = std::uint16_t;

/// The 32-bit unsigned integer type. Prefer using i64 in almost all
/// situations.
///
using u32 = std::uint32_t;

/// The 64-bit unsigned integer type. Prefer using i64 in almost all
/// situations.
///
using u64 = std::uint64_t;

} // namespace vgc

#endif // VGC_CORE_INT_H
