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

#ifndef VGC_CORE_FLAGS_H
#define VGC_CORE_FLAGS_H

#include <vgc/core/templateutil.h>

namespace vgc::core {

template<typename Enum>
class Flags {
public:
    using EnumType = Enum;
    using UnderlyingType = std::underlying_type_t<Enum>;

    constexpr Flags() noexcept = default;

    constexpr Flags(Enum v) noexcept
        : v_(v) {
    }

    constexpr UnderlyingType toUnderlying() const noexcept {
        return ::vgc::core::toUnderlying(v_);
    }

    explicit constexpr operator bool() const noexcept {
        return toUnderlying() != 0;
    }

    constexpr bool operator!() const noexcept {
        return toUnderlying() == 0;
    }

    constexpr void clear() noexcept {
        v_ = static_cast<Enum>(0);
    }

    /// Returns whether this set of flags contains the given `flag`.
    ///
    /// If `flag` has more than one bit set to 1, this function returns true if `this` has all these bits set to 1.
    /// If `flag` has no bits set to 1, this function always returns true.
    ///
    /// This is equivalent to `(*this & flag) == flag`.
    ///
    constexpr bool has(Enum flag) const noexcept {
        return hasAll(flag);
    }

    constexpr bool isEmpty() const noexcept {
        return toUnderlying() == 0;
    }

    constexpr bool hasAny(Flags flags) const noexcept {
        return (toUnderlying() & flags.toUnderlying()) != 0;
    }

    constexpr bool hasAll(Flags flags) const noexcept {
        return (toUnderlying() & flags.toUnderlying()) == flags.toUnderlying();
    }

    constexpr Flags& set(Flags flags) noexcept {
        v_ = static_cast<Enum>(toUnderlying() | flags.toUnderlying());
        return *this;
    }

    constexpr Flags& unset(Flags flags) noexcept {
        v_ = static_cast<Enum>(toUnderlying() & ~flags.toUnderlying());
        return *this;
    }

    constexpr Flags& toggle(Flags flags) noexcept {
        v_ = static_cast<Enum>(toUnderlying() ^ flags.toUnderlying());
        return *this;
    }

    constexpr Flags& toggleAll() noexcept {
        v_ = static_cast<Enum>(~toUnderlying());
        return *this;
    }

    constexpr Flags& mask(Flags flags) noexcept {
        v_ = static_cast<Enum>(toUnderlying() & flags.toUnderlying());
        return *this;
    }

    friend inline constexpr Flags operator|(Flags a, Flags b) noexcept {
        return Flags(static_cast<Enum>(a.toUnderlying() | b.toUnderlying()));
    }

    friend inline constexpr Flags operator&(Flags a, Flags b) noexcept {
        return Flags(static_cast<Enum>(a.toUnderlying() & b.toUnderlying()));
    }

    friend inline constexpr Flags operator^(Flags a, Flags b) noexcept {
        return Flags(static_cast<Enum>(a.toUnderlying() ^ b.toUnderlying()));
    }

    constexpr Flags operator~() const noexcept {
        return Flags(static_cast<Enum>(~toUnderlying()));
    }

    constexpr Flags& operator|=(Flags b) noexcept {
        v_ = static_cast<Enum>(toUnderlying() | b.toUnderlying());
        return *this;
    }

    constexpr Flags& operator&=(Flags b) noexcept {
        v_ = static_cast<Enum>(toUnderlying() & b.toUnderlying());
        return *this;
    }

    constexpr Flags& operator^=(Flags b) noexcept {
        v_ = static_cast<Enum>(toUnderlying() ^ b.toUnderlying());
        return *this;
    }

    friend inline constexpr bool operator==(Flags a, Flags b) noexcept {
        return a.toUnderlying() == b.toUnderlying();
    }

    friend inline constexpr bool operator!=(Flags a, Flags b) noexcept {
        return a.toUnderlying() != b.toUnderlying();
    }

private:
    Enum v_ = {};
};

} // namespace vgc::core

template<typename Enum>
struct std::hash<vgc::core::Flags<Enum>> {
    std::size_t operator()(const vgc::core::Flags<Enum>& p) const noexcept {
        return std::hash<vgc::core::Flags<Enum>::UnderlyingType>()(p.toUnderlying());
    }
};

#define VGC_DEFINE_FLAGS_BINARY_OPERATOR(Enum, Op)                                       \
    inline constexpr ::vgc::core::Flags<Enum> operator Op(Enum a, Enum b) noexcept {     \
        return ::vgc::core::Flags<Enum>(a) Op ::vgc::core::Flags<Enum>(b);               \
    }

#define VGC_DEFINE_FLAGS_BINARY_OPERATORS(Enum)                                          \
    VGC_DEFINE_FLAGS_BINARY_OPERATOR(Enum, |)                                            \
    VGC_DEFINE_FLAGS_BINARY_OPERATOR(Enum, &)                                            \
    VGC_DEFINE_FLAGS_BINARY_OPERATOR(Enum, ^)

#define VGC_DEFINE_FLAGS_BITWISE_NEGATION_OPERATOR(Enum)                                 \
    inline constexpr ::vgc::core::Flags<Enum> operator~(Enum value) noexcept {           \
        return ~::vgc::core::Flags<Enum>(value);                                         \
    }

#define VGC_DEFINE_FLAGS_OPERATORS(Enum)                                                 \
    VGC_DEFINE_FLAGS_BINARY_OPERATORS(Enum)                                              \
    VGC_DEFINE_FLAGS_BITWISE_NEGATION_OPERATOR(Enum)

#define VGC_DEFINE_FLAGS_ALIAS(Enum, FlagsTypeName)                                      \
    using FlagsTypeName = ::vgc::core::Flags<Enum>;

#define VGC_DEFINE_FLAGS(FlagsTypeName, EnumTypeName)                                    \
    VGC_DEFINE_FLAGS_OPERATORS(EnumTypeName)                                             \
    VGC_DEFINE_FLAGS_ALIAS(EnumTypeName, FlagsTypeName)

#endif // VGC_CORE_FLAGS_H
