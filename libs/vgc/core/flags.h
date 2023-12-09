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

/// \class vgc::core::Flags
/// \brief Stores a combination of enum values.
///
/// Traditionally, in C or C++, a combination of enum values are stored
/// directly as either an instance of the enum type, or as the underlying
/// integer type of the enum type. Here is an example using a C++ scoped enum:
///
/// ```cpp
/// enum class Button : int {
///     NoButton = 0x0,
///     Left     = 0x1,
///     Right    = 0x2,
///     Middle   = 0x4
/// };
///
/// int buttons = static_cast<int>(Button::Left) | static_cast<int>(Button::Right);
///
/// if (buttons & static_cast<int>(Button::Left)) {
///     print("The left button is pressed.");
/// }
/// ```
///
/// The class `Flags<Enum>` does the exact same thing under the hood, but
/// allows you to express it in a more type-safe and readable way:
///
/// ```cpp
/// enum class Button : int {
///     NoButton = 0x0,
///     Left     = 0x1,
///     Right    = 0x2,
///     Middle   = 0x4
/// };
/// VGC_DEFINE_FLAGS(Buttons, Button)
///
/// Buttons buttons = Button::Left | Button::Right;
///
/// if (buttons.has(Button::Left)) {
///     print("The left button is pressed.");
/// }
/// ```
///
/// In the code above, the macro `VGC_DEFINE_FLAGS(Buttons, Button)` defines
/// `Buttons` to be an alias for `Flags<Button>`, and enables all bitwise
/// operators on `Button`, each returning a `Flags<Button>`.
///
/// In addition to traditionnal bitwise operators, the class `Flags<Enum>`
/// provides:
///
/// - Convenient setters to modify in-place the combination of flags, such as:
/// `set(flags)`, `unset(flags)`, `toggle(flags)`, `toggleAll(flags)`,
/// `mask(flags)`, and `clear()`.
///
/// - Convenient getters to test whether flags are set, such as: `has(flag)`,
/// `hasAny(flags)`, `hasAll(flags)`, and `clear()`.
///
/// Using these methods is usually recommended over using the bitwise
/// operators, since they express more explicitly the intended operation.
///
/// The method `toUnderlying()` can be used to get the underlying integer that
/// stores the combination of flags.
///
template<typename Enum>
class Flags {
public:
    using EnumType = Enum;
    using UnderlyingType = std::underlying_type_t<Enum>;

    /// Creates a zero-initialized `Flags<Enum>`, that is, with all flags unset.
    ///
    constexpr Flags() noexcept = default;

    /// Creates a `Flags<Enum>` with all flags unset except the given `flag`.
    ///
    /// This enables implicit conversions from `Enum` to `Flags<Enum>`.
    ///
    constexpr Flags(Enum flag) noexcept
        : v_(flag) {
    }

    /// Creates a `Flags<Enum>` with only the given `flags` set.
    ///
    constexpr Flags(std::initializer_list<Enum> flags) noexcept
        : v_() {

        UnderlyingType x = {};
        for (const Enum& flag : flags) {
            x |= ::vgc::core::toUnderlying(flag);
        }
        v_ = static_cast<Enum>(x);
    }

    /// Returns the underlying integer that stores this combination of flags.
    ///
    constexpr UnderlyingType toUnderlying() const noexcept {
        return ::vgc::core::toUnderlying(v_);
    }

    /// Returns whether at least one flag is set.
    ///
    /// This is equivalent to `toUnderlying() != 0`.
    ///
    constexpr explicit operator bool() const noexcept {
        return toUnderlying() != 0;
    }

    /// Returns whether none of the flags are set.
    ///
    /// This is equivalent to `toUnderlying() == 0`.
    ///
    constexpr bool operator!() const noexcept {
        return toUnderlying() == 0;
    }

    /// Unsets all the flags.
    ///
    /// This is equivalent to setting the underlying integer to 0.
    ///
    constexpr void clear() noexcept {
        v_ = static_cast<Enum>(0);
    }

    /// Returns whether the given `flag` is set.
    ///
    /// This function is meant to be used with a single flag, but if `flag`
    /// does in fact have more than one bit set then it behaves like
    /// `hasAll(flag)`. However, we recommend in this case to explictly use
    /// `hasAll()` or `hasAny()` for better readability.
    ///
    /// This is equivalent to `(*this & flag) == flag`.
    ///
    constexpr bool has(Enum flag) const noexcept {
        return hasAll(flag);
    }

    /// Returns whether none of the flags are set.
    ///
    /// This is equivalent to `toUnderlying() == 0`.
    ///
    constexpr bool isEmpty() const noexcept {
        return toUnderlying() == 0;
    }

    /// Returns `true` if and only if at least one of the given `flags` is set.
    ///
    /// If `flags` has no flag set, then this function always returns `false`.
    ///
    /// This is equivalent to `(*this & flags) != 0`.
    ///
    constexpr bool hasAny(Flags flags) const noexcept {
        return (toUnderlying() & flags.toUnderlying()) != 0;
    }

    /// Returns `true` if and only if all the given `flags` are set.
    ///
    /// If `flags` has no flag set, then this function always returns `true`.
    ///
    /// This is equivalent to `(*this & flags) == flags`.
    ///
    constexpr bool hasAll(Flags flags) const noexcept {
        return (toUnderlying() & flags.toUnderlying()) == flags.toUnderlying();
    }

    /// Sets all the given `flags` to 1.
    ///
    /// This is equivalent to `*this = *this | flags`.
    ///
    /// Returns `*this` for convenience (method chaining).
    ///
    constexpr Flags& set(Flags flags) noexcept {
        v_ = static_cast<Enum>(toUnderlying() | flags.toUnderlying());
        return *this;
    }

    /// Sets all the given `flags` to 0.
    ///
    /// This is equivalent to `*this = *this & ~flags`.
    ///
    /// Returns `*this` for convenience (method chaining).
    ///
    constexpr Flags& unset(Flags flags) noexcept {
        v_ = static_cast<Enum>(toUnderlying() & ~flags.toUnderlying());
        return *this;
    }

    /// Toggles (or "flips") all the given `flags`.
    ///
    /// This is equivalent to `*this = *this ^ flags`.
    ///
    /// Returns `*this` for convenience (method chaining).
    ///
    constexpr Flags& toggle(Flags flags) noexcept {
        v_ = static_cast<Enum>(toUnderlying() ^ flags.toUnderlying());
        return *this;
    }

    /// Toggles (or "flips") all the bits in this combination of flags.
    ///
    /// This is equivalent to `*this = ~(*this)`.
    ///
    /// Returns `*this` for convenience (method chaining).
    ///
    constexpr Flags& toggleAll() noexcept {
        v_ = static_cast<Enum>(~toUnderlying());
        return *this;
    }

    /// Applies the given `flags` as a mask over this combination of flags.
    ///
    /// This is equivalent to `*this = *this & flags`.
    ///
    /// Returns `*this` for convenience (method chaining).
    ///
    constexpr Flags& mask(Flags flags) noexcept {
        v_ = static_cast<Enum>(toUnderlying() & flags.toUnderlying());
        return *this;
    }

    /// Returns the bitwise OR between two combinations of flags.
    ///
    friend constexpr Flags operator|(Flags a, Flags b) noexcept {
        return Flags(static_cast<Enum>(a.toUnderlying() | b.toUnderlying()));
    }

    /// Returns the bitwise AND between two combinations of flags.
    ///
    friend constexpr Flags operator&(Flags a, Flags b) noexcept {
        return Flags(static_cast<Enum>(a.toUnderlying() & b.toUnderlying()));
    }

    /// Returns the bitwise XOR between two combinations of flags.
    ///
    friend constexpr Flags operator^(Flags a, Flags b) noexcept {
        return Flags(static_cast<Enum>(a.toUnderlying() ^ b.toUnderlying()));
    }

    /// Returns the bitwise NOT of this combination of flags.
    ///
    constexpr Flags operator~() const noexcept {
        return Flags(static_cast<Enum>(~toUnderlying()));
    }

    /// Self-assigns the result of the bitwise OR with the `other` flags.
    ///
    constexpr Flags& operator|=(Flags other) noexcept {
        v_ = static_cast<Enum>(toUnderlying() | other.toUnderlying());
        return *this;
    }

    /// Self-assigns the result of the bitwise AND with the `other` flags.
    ///
    constexpr Flags& operator&=(Flags other) noexcept {
        v_ = static_cast<Enum>(toUnderlying() & other.toUnderlying());
        return *this;
    }

    /// Self-assigns the result of the bitwise XOR with the `other` flags.
    ///
    constexpr Flags& operator^=(Flags other) noexcept {
        v_ = static_cast<Enum>(toUnderlying() ^ other.toUnderlying());
        return *this;
    }

    /// Returns whether two combinations of flags are equal.
    ///
    friend constexpr bool operator==(Flags a, Flags b) noexcept {
        return a.toUnderlying() == b.toUnderlying();
    }

    /// Returns whether two combinations of flags are different.
    ///
    friend constexpr bool operator!=(Flags a, Flags b) noexcept {
        return a.toUnderlying() != b.toUnderlying();
    }

private:
    Enum v_ = {};
};

} // namespace vgc::core

/// Implements `std::hash` for `vgc::core::Flags<Enum>`.
///
template<typename Enum>
struct std::hash<vgc::core::Flags<Enum>> {
    std::size_t operator()(const vgc::core::Flags<Enum>& p) const noexcept {
        return std::hash<typename vgc::core::Flags<Enum>::UnderlyingType>()(
            p.toUnderlying());
    }
};

/// Enables the given `Op` operator on `Enum`, returning a
/// `vgc::core::Flags<Enum>`.
///
/// \sa `VGC_DEFINE_FLAGS_BITWISE_NEGATION_OPERATOR`,
///     `VGC_DEFINE_FLAGS_BINARY_OPERATORS`,
///     `VGC_DEFINE_FLAGS_OPERATORS`.
///
#define VGC_DEFINE_FLAGS_BINARY_OPERATOR(Enum, Op)                                       \
    inline constexpr ::vgc::core::Flags<Enum> operator Op(Enum a, Enum b) noexcept {     \
        return ::vgc::core::Flags<Enum>(a) Op ::vgc::core::Flags<Enum>(b);               \
    }

/// Enables the bitwise OR, AND, and XOR operators on `Enum`, returning a
/// `vgc::core::Flags<Enum>`.
///
/// \sa `VGC_DEFINE_FLAGS_BITWISE_NEGATION_OPERATOR`,
///     `VGC_DEFINE_FLAGS_BINARY_OPERATOR`,
///     `VGC_DEFINE_FLAGS_OPERATORS`.
///
#define VGC_DEFINE_FLAGS_BINARY_OPERATORS(Enum)                                          \
    VGC_DEFINE_FLAGS_BINARY_OPERATOR(Enum, |)                                            \
    VGC_DEFINE_FLAGS_BINARY_OPERATOR(Enum, &)                                            \
    VGC_DEFINE_FLAGS_BINARY_OPERATOR(Enum, ^)

/// Enables the bitwise NOT operator on `Enum`, returning a
/// `vgc::core::Flags<Enum>`.
///
/// \sa `VGC_DEFINE_FLAGS_BINARY_OPERATOR`,
///     `VGC_DEFINE_FLAGS_BINARY_OPERATORS`,
///     `VGC_DEFINE_FLAGS_OPERATORS`.
///
#define VGC_DEFINE_FLAGS_BITWISE_NEGATION_OPERATOR(Enum)                                 \
    inline constexpr ::vgc::core::Flags<Enum> operator~(Enum value) noexcept {           \
        return ~::vgc::core::Flags<Enum>(value);                                         \
    }

/// Enables all bitwise operators on `Enum`, returning a
/// `vgc::core::Flags<Enum>`.
///
/// \sa `VGC_DEFINE_FLAGS_BITWISE_NEGATION_OPERATOR`,
///     `VGC_DEFINE_FLAGS_BINARY_OPERATOR`,
///     `VGC_DEFINE_FLAGS_BINARY_OPERATORS`,
///     `VGC_DEFINE_FLAGS`.
///
#define VGC_DEFINE_FLAGS_OPERATORS(Enum)                                                 \
    VGC_DEFINE_FLAGS_BINARY_OPERATORS(Enum)                                              \
    VGC_DEFINE_FLAGS_BITWISE_NEGATION_OPERATOR(Enum)

/// Defines `FlagsTypeName` to be an alias for `vgc::core::Flags<Enum>`, but
/// without enabling any bitwise operators on `Enum`.
///
/// \sa `VGC_DEFINE_FLAGS`.
///
#define VGC_DEFINE_FLAGS_ALIAS(FlagsTypeName, Enum)                                      \
    using FlagsTypeName = ::vgc::core::Flags<Enum>;

/// Defines `FlagsTypeName` to be an alias for `vgc::core::Flags<Enum>`, and
/// enables all bitwise operators on `Enum`, returning a
/// `vgc::core::Flags<Enum>`.
///
/// ```cpp
/// enum class Button : int {
///     NoButton = 0x0,
///     Left     = 0x1,
///     Right    = 0x2,
///     Middle   = 0x4
/// };
/// VGC_DEFINE_FLAGS(Buttons, Button)
///
/// Buttons buttons = Button::Left | Button::Right;
///
/// if (buttons.has(Button::Left)) {
///     print("The left button is pressed.");
/// }
/// ```
///
/// See documentation of `vgc::core::Flags<Enum>` for more details.
///
/// \sa `VGC_DEFINE_FLAGS_OPERATORS`,
///     `VGC_DEFINE_FLAGS_ALIAS`.
///
#define VGC_DEFINE_FLAGS(FlagsTypeName, Enum)                                            \
    VGC_DEFINE_FLAGS_OPERATORS(Enum)                                                     \
    VGC_DEFINE_FLAGS_ALIAS(FlagsTypeName, Enum)

#endif // VGC_CORE_FLAGS_H
