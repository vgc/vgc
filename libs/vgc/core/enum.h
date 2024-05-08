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

} // namespace detail

/// \class vgc::core::EnumType
/// \brief Represents the type of an enum value.
///
/// ```cpp
/// EnumType type = enumType<vgc::ui::Key>();
/// ```
///
class VGC_CORE_API EnumType {
public:
    template<typename T>
    using ArrayView = const Array<T>&;

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

    /// Iterates over all the registered `EnumValue` of this `EnumType`,
    /// in the same order as defined by `VGC_DEFINE_ENUM`.
    ///
    /// Returns an empty sequence if this is not a registered enum.
    ///
    /// \sa `values<EnumType>()`.
    ///
    ArrayView<EnumValue> values() const {
        return info_->enumValues;
    }

    /// Converts the given enumerator's `shortName` (e.g., "Digit0")
    /// to its corresponding value, if any.
    ///
    /// Return `std::nullopt` if there is no registered value with the given
    /// `shortName` for this `EnumType`.
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
    static const detail::EnumTypeInfo_* info = detail::getOrCreateEnumTypeInfo<TEnum>();
    return EnumType(info);
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
    /*
    /// Creates an empty `EnumValue`.
    ///
    EnumValue()
        : typeId_(core::typeId<void>()) {
    }
    */

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
        // if (isEmpty()) {
        //     throw core::LogicError(
        //         "Attempting to get the stored value of an empty EnumValue.");
        // }
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

/*
/// \class vgc::core::Enum
/// \brief Provides runtime introspection for registered enum types
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
/// Once registered, you can use the static functions of this `Enum` class to
/// iterate over items or convert integer values from/to strings.
///
/// Examples:
///
/// ```cpp
/// // Iterate over all registered values of an enum type
/// for (foo::MyEnum value : Enum::values<foo::MyEnum>()) {
///     // ...
/// }
///
/// // Convert from an enum type to a string
/// print(Enum::shortTypeName<foo::MyEnum>()); // => "MyEnum"
/// print(Enum::fullTypeName<foo::MyEnum>()); // => "foo::MyEnum"
///
/// // Convert from an enum value to a string
/// print(Enum::shortName(foo::MyEnum::Value1));  // => "Value1"
/// print(Enum::fullName(foo::MyEnum::Value1));   // => "foo::MyEnum::Value1"
/// print(Enum::prettyName(foo::MyEnum::Value1)); // => "Value 1"
///
/// // Convert from a string to an enum value
/// if (auto value = Enum::fromShortName<foo::MyEnum>("Value1")) {
///     foo::MyEnum v = *value; // => foo::MyEnum::Value1
/// }
/// ```
///
/// All the functions in the example above are templated by an `EnumType`,
/// which should either be explicitly provided, or can be automatically deduced
/// (as is the case with `shortName(EnumType value)`, `fullName(EnumType
/// value)`, and `prettyName(EnumType value)`). A call to any of these
/// functions will only compile if `EnumType` is a registered enum type.
///
/// Alternatively, there also exists a non-templated version for some of these
/// functions, taking an extra `TypeId` argument, which always compiles but
/// typically returns an `std::optional` to handle the case where the enum type
/// is not registered.
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
class Enum {
public:
    template<typename T>
    using ArrayView = const Array<T>&;

    using StringArrayView = ArrayView<std::string_view>;

    /// Returns the unqualified type name of the given `EnumType`.
    ///
    /// ```cpp
    /// Enum::shortTypeName<vgc::ui::Key>()`; // => "Key"
    /// ```
    ///
    /// \sa `shortTypeName(TypeId)`,
    ///     `fullTypeName<EnumType>()`, `fullTypeName(TypeId)`.
    ///
    template<typename EnumType>
    static std::string_view shortTypeName() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.shortTypeName;
    }

    /// Returns the unqualified type name of an enum type given by its `TypeId`.
    ///
    /// Returns `std::nullopt` if there is no registered enum type corresponding
    /// to the given `TypeId`.
    ///
    /// ```cpp
    /// TypeId id = typeId<vgc::ui::Key>();
    /// if (auto name = Enum::shortTypeName(id)) {
    ///     print(*name); // => "Key"
    /// }
    /// ```
    ///
    /// \sa `shortTypeName<EnumType>()`,
    ///     `fullTypeName<EnumType>()`, `fullTypeName(TypeId)`.
    ///
    static std::optional<std::string_view> shortTypeName(TypeId enumTypeId) {
        if (auto data = detail::getEnumDataBase(enumTypeId)) { // [1]
            return data->shortTypeName;
        }
        else {
            return std::nullopt;
        }
        // [1] XXX: What if the enum type does have a corresponding
        // VGC_DECLARE/DEFINE_ENUM, but it is *not yet* registered due to lazy
        // initialization? (This applies to all other usages of TypeId in
        // Enum). This might be solved by implementing a new class EnumTypeId
        // (or simply EnumType?), similar to TypeId, but ensuring that EnumData
        // is initialized when an EnumTypeId is instanciated. Using such
        // EnumTypeId class instead of TypeId in EnumValue would also make it
        // possible to safely publicize EnumValue(EnumTypeId, UInt64 value).
    }

    /// Returns the fully-qualified type name of the given `EnumType`.
    ///
    /// ```cpp
    /// Enum::fullTypeName<vgc::ui::Key>()`; // => "vgc::ui::Key"
    /// ```
    ///
    /// \sa `fullTypeName(TypeId)`,
    ///     `shortTypeName<EnumType>()`, `shortTypeName(TypeId)`.
    ///
    /// Example: `"vgc::ui::Key"`.
    ///
    template<typename EnumType>
    static std::string_view fullTypeName() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.fullTypeName;
    }

    /// Returns the fully-qualified type name of an enum type given by its
    /// `TypeId`.
    ///
    /// Returns `std::nullopt` if there is no registered enum type corresponding
    /// to the given `TypeId`.
    ///
    /// ```cpp
    /// TypeId id = typeId<vgc::ui::Key>();
    /// if (auto name = Enum::fullTypeName(id)) {
    ///     print(*name); // => "vgc::ui::Key"
    /// }
    /// ```
    ///
    /// \sa `fullTypeName<EnumType>()`,
    ///     `shortTypeName<EnumType>()`, `shortTypeName(TypeId)`.
    ///
    static std::optional<std::string_view> fullTypeName(TypeId enumTypeId) {
        if (auto data = detail::getEnumDataBase(enumTypeId)) { // [1]
            return data->fullTypeName;
        }
        else {
            return std::nullopt;
        }
    }

    // TODO: prettyTypeName?

    /// Returns the sequence of all the registered values of the given `EnumType`.
    ///
    /// The sequence is in the same order as defined by `VGC_DEFINE_ENUM`.
    ///
    /// \sa `values(TypeId)`.
    ///
    template<typename EnumType>
    static ArrayView<EnumType> values() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.values;
    }

    /// Returns the sequence of all the registered `EnumValue` of an enum type
    /// given by its `TypeId`.
    ///
    /// The sequence is in the same order as defined by `VGC_DEFINE_ENUM`.
    ///
    /// Returns an empty sequence if there is no registered enum type
    /// corresponding to the given `TypeId`.
    ///
    /// \sa `values<EnumType>()`.
    ///
    static ArrayView<EnumValue> values(TypeId enumTypeId) {
        if (auto data = detail::getEnumDataBase(enumTypeId)) { // [1]
            return data->enumValues;
        }
        else {
            static Array<EnumValue> emptyArray;
            return emptyArray;
        }
    }

    /// Returns the sequence of unqualified names (e.g., "Digit0") of all the
    /// registered values of the given `EnumType`.
    ///
    /// The sequence is in the same order as defined by `VGC_DEFINE_ENUM`.
    ///
    /// \sa `fullNames()`, `prettyNames()`.
    ///
    template<typename EnumType>
    static StringArrayView shortNames() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.shortNames;
    }

    /// Returns the sequence of fully-qualified names (e.g., "Digit0") of all
    /// the registered values of the given `EnumType`.
    ///
    /// The sequence is in the same order as defined by `VGC_DEFINE_ENUM`.
    ///
    /// \sa `shortNames()`, `prettyNames()`.
    ///
    template<typename EnumType>
    static StringArrayView fullNames() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.fullNames;
    }

    /// Returns the sequence of pretty names (e.g., "Digit 0") of all the
    /// registered values of the given `EnumType`.
    ///
    /// The sequence is in the same order as defined by `VGC_DEFINE_ENUM`.
    ///
    /// \sa `shortNames()`, `fullNames()`.
    ///
    template<typename EnumType>
    static StringArrayView prettyNames() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.prettyNames;
    }

    // XXX return std::optional<std::string_view> instead of using the "unknown name"?
    // XXX remove in favor of EnumValue(EnumType value).shortName()?
    template<typename EnumType>
    static std::string_view shortName(EnumType value) {
        const detail::EnumData<EnumType>& data = enumData_(value);
        if (auto index = data.getIndex(value)) {
            return data.shortNames[*index];
        }
        else {
            return data.unknownItemShortName;
        }
    }

    // XXX return std::optional<std::string_view> instead of using the "unknown name"?
    // XXX remove in favor of EnumValue(EnumType value).fullName()?
    template<typename EnumType>
    static std::string_view fullName(EnumType value) {
        const detail::EnumData<EnumType>& data = enumData_(value);
        if (auto index = data.getIndex(value)) {
            return data.fullNames[*index];
        }
        else {
            return data.unknownItemFullName;
        }
    }

    // XXX return std::optional<std::string_view> instead of using the "unknown name"?
    // XXX remove in favor of EnumValue(EnumType value).prettyName()?
    template<typename EnumType>
    static std::string_view prettyName(EnumType value) {
        const detail::EnumData<EnumType>& data = enumData_(value);
        if (auto index = data.getIndex(value)) {
            return data.prettyNames[*index];
        }
        else {
            return data.unknownItemPrettyName;
        }
    }

    /// Converts the given enumerator's `shortName` (e.g., "Digit0")
    /// to its corresponding value, if any.
    ///
    /// Return `std::nullopt` if there is no registered value with the given
    /// `shortName` for the given `EnumType`.
    ///
    /// \sa `fromShortName(TypeId, string_view)`.
    ///
    template<typename EnumType>
    static std::optional<EnumType> fromShortName(std::string_view shortName) {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        if (auto index = data.getIndexFromShortName(shortName)) {
            return data.value[*index];
        }
        else {
            return std::nullopt;
        }
    }

    /// Converts the given enumerator's `shortName` (e.g., "Digit0")
    /// to its corresponding value, if any.
    ///
    /// Return `std::nullopt` if there is no registered value with the given
    /// `shortName` for the given `enumTypeId`.
    ///
    /// \sa `fromShortName<EnumType>(string_view)`.
    ///
    static std::optional<EnumValue>
    fromShortName(TypeId enumTypeId, std::string_view shortName) {
        if (auto data = detail::getEnumDataBase(enumTypeId)) {
            if (auto index = data->getIndexFromShortName(shortName)) {
                // Note: Using the generally unsafe EnumValue(TypeId, UInt64)
                // constructor is safe in this case, because we know that the
                // enumerator data is already initialized, since we found it.
                return EnumValue(enumTypeId, data->valueInfo[*index]->value);
            }
            else {
                return std::nullopt;
            }
        }
        else {
            return std::nullopt;
        }
    }
};
*/

} // namespace vgc::core

// clang-format off

/// Declares a scoped enum. See `Enum` for more details.
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
