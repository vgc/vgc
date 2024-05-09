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

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <vgc/core/algorithms.h> // hashCombine
#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>
#include <vgc/core/preprocessor.h>
#include <vgc/core/templateutil.h>
#include <vgc/core/typeid.h>

namespace vgc::core {

class EnumValue;

namespace detail {

// Note: We have chosen to store type-erased enum values by casting them to a
// 64bit integer, which means that we do not support enum types whose
// underlying type is larger than 64bit. This is unlikely to ever be an issue,
// since as of this writing (e.g., up to C++23), such enum types are not
// portably supported anyway. Example with Clang:
//
//   enum Foo { _65bitValue = 0x1ffff'ffff'ffff'ffff };
//
//   error: integer literal is too large to be represented in any integer type.

struct EnumValueInfo {
    UInt64 value;           //  static_cast<UInt64>(vgc::ui::Key::Digit0)
    std::string fullName;   //  "vgc::ui::Key::Digit0"
    std::string shortName;  //  "Digit0"
    std::string prettyName; //  "0"
};

// Base class storing most of the meta-info of a given EnumType.
//
class VGC_CORE_API EnumTypeInfo_ {
public:
    core::TypeId typeId;
    bool isRegistered = false;

    std::string unknownItemFullName;   // "vgc::ui::Key::Unknown_Key"
    std::string unknownItemShortName;  // "Unknown_Key"
    std::string unknownItemPrettyName; // "Unknown Key"

    // This is where the actual per-enumerator data is stored. We allocate each
    // EnumValueInfo separately on the heap to ensure that it has a stable
    // address, and therefore that string_views to the stored strings stay
    // valid when valueInfo grows (or if this EnumTypeInfo_ is moved).
    //
    Array<std::unique_ptr<EnumValueInfo>> valueInfo;

    // These maps make it possible to quickly find the appropriate
    // EnumValueInfo from the enum value or the enum short name.
    //
    std::unordered_map<UInt64, Int> valueToIndex;
    std::unordered_map<std::string_view, Int> shortNameToIndex;

    // These are convenient arrays (redundant with valueInfo) facilitating
    // iteration without the need for proxy iterators. This approach increases
    // debuggability and works better with parallelization libs (sometimes not
    // supporting proxy iterators).
    //
    Array<EnumValue> enumValues;
    Array<std::string_view> fullNames;
    Array<std::string_view> shortNames;
    Array<std::string_view> prettyNames;

    EnumTypeInfo_(TypeId id);
    virtual ~EnumTypeInfo_() = 0;

    // EnumTypeInfo_ is non-copyable (because it stores unique_ptrs).
    // We need to explicitly mark it non-copyable otherwise it doesn't compile on MSVC.
    EnumTypeInfo_(const EnumTypeInfo_&) = delete;
    EnumTypeInfo_& operator=(const EnumTypeInfo_&) = delete;

    // Returns the index corresponding to the given enum value, if any.
    //
    std::optional<Int> getIndex(UInt64 value) const;

    // Returns the index corresponding to the given enum short name, if any.
    //
    std::optional<Int> getIndexFromShortName(std::string_view shortName) const;

protected:
    void addValue_(UInt64 value, std::string_view shortName, std::string_view prettyName);
};

// Derived template class providing a non-type-erased API.
//
// For example, this allows iteration over all enum values stored as their
// original types instead of casted as a 64bit integer.
//
template<typename TEnum, VGC_REQUIRES(std::is_enum_v<TEnum>)>
class EnumTypeInfo : public EnumTypeInfo_ {
public:
    Array<TEnum> values;

    EnumTypeInfo(TypeId id)
        : EnumTypeInfo_(id) {
    }

    void addValue(TEnum value, std::string_view shortName, std::string_view prettyName) {
        addValue_(static_cast<UInt64>(value), shortName, prettyName);
        values.append(value);
    }

    std::optional<Int> getIndex(TEnum value) const {
        return getIndex(static_cast<UInt64>(value));
    }
};

} // namespace detail

/// Type trait for `isRegisteredEnum`.
///
template<typename TEnum, typename SFINAE = void>
struct IsRegisteredEnum : std::false_type {};

template<typename TEnum>
struct IsRegisteredEnum<
    TEnum,
    RequiresValid<decltype(initEnumTypeInfo_((detail::EnumTypeInfo<TEnum>*)(0)))>>
    : std::true_type {};

/// Checks whether `TEnum` is an enumeration type that has been registered
/// via `VGC_DECLARE_ENUM` and `VGC_DEFINE_ENUM`.
///
template<typename TEnum>
inline constexpr bool isRegisteredEnum = IsRegisteredEnum<TEnum>::value;

namespace detail {

using EnumTypeInfoFactory = std::function<const EnumTypeInfo_*()>;

// Creates and returns an `EnumTypeInfo_` with the given `factory`, unless an
// `EnumTypeInfo_` with the given `typeId` has already been created, in which
// case the pre-existing `EnumTypeInfo_` is returned instead. This ensures
// uniqueness of the `EnumTypeInfo_*` even across shared library boundary.
//
VGC_CORE_API const EnumTypeInfo_*
getOrCreateEnumTypeInfo(TypeId typeId, EnumTypeInfoFactory factory);

template<typename TEnum>
const EnumTypeInfo<TEnum>* getOrCreateEnumTypeInfo() {
    TypeId typeId = core::typeId<TEnum>();
    const EnumTypeInfo_* info_ = getOrCreateEnumTypeInfo(typeId, [typeId]() {
        auto info = new EnumTypeInfo<TEnum>(typeId); // leaks intentionally
        if constexpr (isRegisteredEnum<TEnum>) {
            info->isRegistered = true;
            initEnumTypeInfo_(info); // found via ADL
        }
        return info;
    });
    return static_cast<const EnumTypeInfo<TEnum>*>(info_);
}

template<typename TEnum>
const EnumTypeInfo<TEnum>* enumTypeInfo() {
    static const EnumTypeInfo<TEnum>* info = detail::getOrCreateEnumTypeInfo<TEnum>();
    return info;
}

} // namespace detail

template<typename T>
using EnumArrayView = const Array<T>&;

using EnumStringArrayView = EnumArrayView<std::string_view>;

/// \class vgc::core::EnumType
/// \brief Represents the type of an enum value.
///
/// ```cpp
/// EnumType type = enumType<vgc::ui::Key>();
/// ```
///
class VGC_CORE_API EnumType {
public:
    /// Returns the unqualified type name of this `EnumType`.
    ///
    /// ```cpp
    /// enumType<vgc::ui::Key>().shortName()`; // => "Key"
    /// ```
    ///
    std::string_view shortName() const {
        return info_->typeId.name();
    }

    /// Returns the fully-qualified type name of this `EnumType`.
    ///
    /// ```cpp
    /// enumType<vgc::ui::Key>().shortName()`; // => "vgc::ui::Key"
    /// ```
    ///
    std::string_view fullName() const {
        return info_->typeId.fullName();
    }

    // TODO: prettyName?

    /// Iterates over all the registered `EnumValue` of this `EnumType`,
    /// in the same order as defined by `VGC_DEFINE_ENUM`.
    ///
    /// Returns an empty sequence if this is not a registered enum.
    ///
    /// \sa `core::enumValues<TEnum>()`.
    ///
    EnumArrayView<EnumValue> values() const {
        return info_->enumValues;
    }

    /// Iterates over all the short names of the registered values of this
    /// `EnumType`.
    ///
    EnumStringArrayView shortNames() const {
        return info_->shortNames;
    }

    /// Iterates over all the full names of the registered values of this
    /// `EnumType`.
    ///
    EnumStringArrayView fullNames() const {
        return info_->fullNames;
    }

    /// Iterates over all the pretty names of the registered values of this
    /// `EnumType`.
    ///
    EnumStringArrayView prettyNames() const {
        return info_->prettyNames;
    }

    /// Converts the given enumerator's `shortName` (e.g., "Digit0")
    /// to its corresponding value, if any.
    ///
    /// Return `std::nullopt` if there is no registered value with the given
    /// `shortName` for this `EnumType`.
    ///
    /// \sa `core::enumFromShortName<TEnum>()`.
    ///
    std::optional<EnumValue> fromShortName(std::string_view shortName) const;

    /// Returns whether this `EnumType` is equal to `other`.
    ///
    bool operator==(const EnumType& other) const {
        return info_ == other.info_;
    }

    /// Returns whether this `EnumType` is different from `other`.
    ///
    bool operator!=(const EnumType& other) const {
        return info_ != other.info_;
    }

    /// Returns whether this `EnumType` is less than `other`.
    ///
    bool operator<(const EnumType& other) const {
        return info_ < other.info_;
    }

private:
    const detail::EnumTypeInfo_* info_;

    template<typename T>
    friend EnumType enumType();

    friend struct std::hash<EnumType>;

    friend detail::EnumTypeInfo_;
    friend EnumValue;

    EnumType(const detail::EnumTypeInfo_* info)
        : info_(info) {
    }
};

/// Returns the `EnumType` of the given enum type.
///
/// ```
/// enum class Animal { Cat, Dog };
/// EnumType type = enumType<Animal>();
/// ```
///
template<typename TEnum>
EnumType enumType() {
    return EnumType(detail::enumTypeInfo<TEnum>());
}

} // namespace vgc::core

namespace std {

template<>
struct hash<vgc::core::EnumType> {
    std::size_t operator()(const vgc::core::EnumType& t) const {
        return std::hash<const vgc::core::detail::EnumTypeInfo_*>()(t.info_);
    }
};

} // namespace std

template<>
struct fmt::formatter<vgc::core::EnumType> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(vgc::core::EnumType t, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(t.fullName(), ctx);
    }
};

namespace vgc::core {

/// \class vgc::core::EnumValue
/// \brief Stores any Enum value in a type-safe way.
///
class VGC_CORE_API EnumValue {
public:
    // Creates an `EnumValue` given its `type` and underlying `value` as an
    // `UInt64`.
    //
    // This constructor is only meant for advanced use cases and should rarely
    // be needed: whenever possible, prefer using the constructor from an
    // actual C++ enum value, as it is typically more convenient and improves
    // type-safety.
    //
    // ```cpp
    // enumClass MyEnum {
    //     Foo = 0,
    //     Bar = 1
    // };
    //
    // // Using this constructor
    // EnumValue v1 = EnumValue(enumType<MyEnum>(), 0);
    //
    // // Using the type-safe constructor
    // EnumValue v2 = EnumValue(MyEnum::Foo);
    //
    // assert(v1 == v2);
    // ```
    //
    EnumValue(EnumType type, UInt64 value)
        : type_(type)
        , value_(value) {
    }

    /// Creates an `EnumValue` from the given enumerator value.
    ///
    // Note: This constructor intentionally allows implicit conversions.
    //
    template<typename TEnum, VGC_REQUIRES(std::is_enum_v<TEnum>)>
    EnumValue(TEnum value)
        : type_(enumType<TEnum>())
        , value_(static_cast<UInt64>(value)) {
    }

    /// Returns the `EnumType` of this `EnumValue`.
    ///
    EnumType type() const {
        return type_;
    }

    /// Returns whether the `EnumValue` stores an enumerator value
    /// of the given `TEnum` type.
    ///
    template<typename TEnum>
    bool has() const {
        return type_ == enumType<TEnum>();
    }

    /// Returns the stored value as a `TEnum`.
    ///
    /// Throws an exception if the `EnumValue` is empty or if the stored
    /// enumerator value is not of type `TEnum`.
    ///
    template<typename TEnum>
    TEnum get() const {
        EnumType requestedType = enumType<TEnum>();
        if (type_ != requestedType) {
            throw core::LogicError(core::format(
                "Mismatch between stored EnumValue type ({}) and requested type ({}).",
                type_.fullName(),
                requestedType.fullName()));
        }
        return static_cast<TEnum>(value_);
    }

    /// Returns the stored value as a `TEnum`.
    ///
    /// The behavior is undefined if the `EnumValue` is empty or if the stored
    /// enumerator value is not of type `TEnum`.
    ///
    template<typename TEnum>
    TEnum getUnchecked() const {
        return static_cast<TEnum>(value_);
    }

    /// Returns the unqualified name (e.g., "Digit0") of the stored enumerator
    /// value, if any.
    ///
    /// Otherwise, returns "NoValue".
    ///
    // TODO: Return format("{}({})", shortTypeName, value) instead of "NoValue".
    //       This requires returning something like `StringOrStringView`.
    //
    std::string_view shortName() const {
        if (auto info = getEnumValueInfo_()) {
            return info->shortName;
        }
        else {
            return "NoValue";
        }
    }

    /// Returns the fully-qualified name (e.g., "vgc::ui::Key::Digit0") of the
    /// stored enumerator value, if any.
    ///
    /// Otherwise, returns "NoType::NoValue".
    ///
    // TODO: Return format("{}({})", fullTypeName, value) instead of "NoValue".
    //       This requires returning something like `StringOrStringView`.
    //
    std::string_view fullName() const {
        if (auto info = getEnumValueInfo_()) {
            return info->fullName;
        }
        else {
            return "NoType::NoValue";
        }
    }

    /// Returns the pretty name (e.g., "0") of the stored enumerator value, if
    /// any.
    ///
    /// Otherwise, returns "No Value".
    ///
    // TODO: Return format("{}({})", shortTypeName, value) instead of "NoValue".
    //       This requires returning something like `StringOrStringView`.
    //
    std::string_view prettyName() const {
        if (auto info = getEnumValueInfo_()) {
            return info->prettyName;
        }
        else {
            return "No Value";
        }
    }

    /// Returns whether the two `EnumValue` are equal, that is, if
    /// they have both same type and value.
    ///
    bool operator==(const EnumValue& other) const {
        return type_ == other.type_ && value_ == other.value_;
    }

    /// Returns whether the two `EnumValue` are different.
    ///
    bool operator!=(const EnumValue& other) const {
        return !(*this == other);
    }

    /// Compares the two `EnumValue`.
    ///
    bool operator<(const EnumValue& other) const {
        if (type_ == other.type_) {
            return value_ < other.value_;
        }
        else {
            return type_ < other.type_;
        }
    }

private:
    EnumType type_;
    UInt64 value_ = 0;

    // Note: alternatively, we could just store an EnumValueInfo*. This
    // would make `EnumValue` 64bit instead of 128bit, but would only work for
    // registered enums, and would not support enum values outside of the
    // registered values (so wouldn't work for flags). Therefore, for more
    // genericity, we have chosen the current approach.

    friend std::hash<EnumValue>;

    const detail::EnumValueInfo* getEnumValueInfo_() const {
        if (auto index = type_.info_->getIndex(value_)) {
            return type_.info_->valueInfo[*index].get();
        }
        return nullptr;
    }
};

} // namespace vgc::core

namespace std {

template<>
struct hash<vgc::core::EnumValue> {
    std::size_t operator()(const vgc::core::EnumValue& v) const {
        size_t res = 123456;
        vgc::core::hashCombine(res, v.type_, v.value_);
        return res;
    }
};

} // namespace std

template<>
struct fmt::formatter<vgc::core::EnumValue> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::core::EnumValue& v, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(v.fullName(), ctx);
    }
};

template<typename TEnum>
struct fmt::formatter< //
    TEnum,
    char,
    vgc::core::Requires<vgc::core::isRegisteredEnum<TEnum>>>

    : fmt::formatter<vgc::core::EnumValue> {

    template<typename FormatContext>
    auto format(TEnum value, FormatContext& ctx) {
        return fmt::formatter<vgc::core::EnumValue>::format(value, ctx);
    }
};

namespace vgc::core {

// Below, we define templated free functions which are similar to methods in
// `EnumType`, but return actual enum values instead of type-erased
// `EnumValue`.

/// Returns the sequence of all the registered values of the enum type
/// `TEnum`.
///
/// \sa `EnumType::values()`.
///
template<typename TEnum, VGC_REQUIRES(isRegisteredEnum<TEnum>)>
EnumArrayView<TEnum> enumValues() {
    auto info = detail::enumTypeInfo<TEnum>();
    return info->values;
}

/// Converts the given enumerator's `shortName` (e.g., "Digit0") to its
/// corresponding value, if any.
///
/// Return `std::nullopt` if there is no registered value with the given
/// `shortName` for the enum type `TEnum`.
///
/// \sa `EnumType::fromShortName()`.
///
template<typename TEnum, VGC_REQUIRES(isRegisteredEnum<TEnum>)>
std::optional<TEnum> enumFromShortName(std::string_view shortName) {
    auto info = detail::enumTypeInfo<TEnum>();
    if (auto index = info->getIndexFromShortName(shortName)) {
        return info->values[*index];
    }
    else {
        return std::nullopt;
    }
}

} // namespace vgc::core

// clang-format off

/// \brief Provides runtime introspection for a given enum type
///
/// In order to support iteration over items of an enum type, and support
/// conversion from an enum integer value to a string (and vice-versa), any
/// enum type can be *registered* using the `VGC_DECLARE_ENUM` and
/// `VGC_DEFINE_ENUM` macros, as such:
///
/// ```cpp
/// // In the .h file
///
/// namespace foo {
///
/// enum class MyEnum {
///     Value1,
///     Value2
/// };
///
/// FOO_API
/// VGC_DECLARE_ENUM(MyEnum)
///
/// } // namespace foo
///
///
/// // In the .cpp file
///
/// namespace foo {
///
/// VGC_DEFINE_ENUM(
///     MyEnum,
///     (Value1, "Value 1"),
///     (Value2, "Value 2"))
///
/// } // namespace foo
///
/// ```
///
/// Note that due to compiler limits (maximum number of macro arguments
/// allowed), `VGC_DEFINE_ENUM` only supports up to 122 enum items. If you
/// need more, you can use the following long-form version, which is a little
/// more verbose but doesn't have limits on the number of enum items:
///
/// ```cpp
/// VGC_DEFINE_ENUM_BEGIN(MyEnum)
///     VGC_ENUM_ITEM(Value1, "Value 1")
///     VGC_ENUM_ITEM(Value2, "Value 2")
///     ...
///     VGC_ENUM_ITEM(ValueN, "Value N")
/// VGC_DEFINE_ENUM_END()
/// ```
///
/// Once registered, you can use the following functions to iterate over items
/// or convert integer values from/to strings.
///
/// Examples:
///
/// ```cpp
/// // Iterate over all registered values of an enum type
/// for (foo::MyEnum value : enumValues<foo::MyEnum>()) {
///     // ...
/// }
///
/// // Convert from an enum type to a string
/// print(enumType<foo::MyEnum>().shortName()); // => "MyEnum"
/// print(enumType<foo::MyEnum>().fullName()); // => "foo::MyEnum"
///
/// // Convert from an enum value to a string
/// print(EnumValue(foo::MyEnum::Value1).shortName());  // => "Value1"
/// print(EnumValue(foo::MyEnum::Value1).fullName());   // => "foo::MyEnum::Value1"
/// print(EnumValue(foo::MyEnum::Value1).prettyName()); // => "Value 1"
///
/// // Convert from a string to an enum value
/// if (auto value = enumType<foo::MyEnum>().fromShortName("Value1")) {
///     foo::MyEnum v = *value; // => foo::MyEnum::Value1
/// }
/// ```
///
#define VGC_DECLARE_ENUM(TEnum)                                                          \
    void initEnumTypeInfo_(::vgc::core::detail::EnumTypeInfo<TEnum>* info);

/// Starts the definition of a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM_BEGIN(TEnum)                                                     \
    void initEnumTypeInfo_(::vgc::core::detail::EnumTypeInfo<TEnum>* info) {             \
        using TEnum_ = TEnum;

/// Defines an enumerator of a scoped enum. See `Enum` for more details.
///
#define VGC_ENUM_ITEM(name, prettyName)                                                  \
        info->addValue(TEnum_::name, VGC_PP_STR(name), prettyName);

/// Ends the definition of a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM_END()                                                            \
    }

#define VGC_ENUM_ITEM_(x, t)                                                             \
    VGC_ENUM_ITEM(                                                                       \
        VGC_PP_EXPAND(VGC_PP_PAIR_FIRST t),                                              \
        VGC_PP_EXPAND(VGC_PP_PAIR_SECOND t))

#define VGC_DEFINE_ENUM_X_(Enum, ...)                                                    \
    VGC_DEFINE_ENUM_BEGIN(Enum)                                                          \
        VGC_PP_EXPAND(VGC_PP_FOREACH_X(VGC_ENUM_ITEM_, Enum, __VA_ARGS__))               \
    VGC_DEFINE_ENUM_END()

/// Defines a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM(...) VGC_PP_EXPAND(VGC_DEFINE_ENUM_X_(__VA_ARGS__, ~))

#define VGC_ENUM_ENDMAX End_, Max_ = End_ - 1
#define VGC_ENUM_COUNT(enum_) (static_cast<Int>(enum_::Max_) + 1)

// clang-format on

#endif // VGC_CORE_ENUM_H
