// Copyright 2022 The VGC Developers
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

#ifndef VGC_CORE_ENUM_H
#define VGC_CORE_ENUM_H

#include <vgc/core/templateutil.h>

#define VGC_DEFINE_CROSS_SCOPED_ENUM_OPERATOR(EnumA, EnumB, Op)             \
    inline constexpr EnumA operator Op(EnumA a, EnumB b) noexcept {         \
        return static_cast<EnumA>(                                          \
            ::vgc::core::toUnderlying(a) Op ::vgc::core::toUnderlying(b));  \
    }                                                                       \
    inline EnumA& operator Op##=(EnumA& a, EnumB b) noexcept {              \
        a = static_cast<EnumA>(                                             \
            ::vgc::core::toUnderlying(a) Op ::vgc::core::toUnderlying(b));  \
        return a;                                                           \
    }

#define VGC_DEFINE_CROSS_SCOPED_ENUM_OPERATORS(EnumA, EnumB)    \
    VGC_DEFINE_CROSS_SCOPED_ENUM_OPERATOR(EnumA, EnumB, |)      \
    VGC_DEFINE_CROSS_SCOPED_ENUM_OPERATOR(EnumA, EnumB, &)      \
    VGC_DEFINE_CROSS_SCOPED_ENUM_OPERATOR(EnumA, EnumB, ^)

#define VGC_DEFINE_SCOPED_ENUM_OPERATOR(Enum, Op)           \
    VGC_DEFINE_CROSS_SCOPED_ENUM_OPERATOR(Enum, Enum, Op)

#define VGC_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS(Enum)        \
    VGC_DEFINE_CROSS_SCOPED_ENUM_OPERATORS(Enum, Enum)      \
    inline Enum operator~(Enum value) noexcept {            \
        return static_cast<Enum>(                           \
            ~::vgc::core::toUnderlying(value));             \
    }                                                       \
    inline bool operator!(Enum value) noexcept {            \
        return ::vgc::core::toUnderlying(value) == 0;       \
    }

#endif // VGC_CORE_ENUM_H
