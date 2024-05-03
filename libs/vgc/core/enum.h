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

namespace detail {

struct EnumValueData {
    TypeId typeId;          //  core::typeId<vgc::ui::Key>()
    UInt64 value;           //  static_cast<UInt64>(vgc::ui::Key::Digit0)
    std::string fullName;   //  "vgc::ui::Key::Digit0"
    std::string shortName;  //  "Digit0"
    std::string prettyName; //  "0"

    // Note: Not supporting enum types larger than 64bit is unlikely to ever be
    // an issue, since (at least up to C++23), such enum types are not portably
    // supported anyway. Example with Clang:
    //
    //   enum Foo { _65bitValue = 0x1ffff'ffff'ffff'ffff };
    //
    //   error: integer literal is too large to be represented in any integer type.
};

struct EnumDataBase;

VGC_CORE_API
void registerEnumDataBase(const EnumDataBase& data);

VGC_CORE_API
const EnumDataBase* getEnumDataBase(TypeId typeId);

VGC_CORE_API
const EnumValueData* getEnumValueData(TypeId typeId, UInt64 value);

} // namespace detail

/// Type trait for `isRegisteredEnum`.
///
template<typename EnumType, typename SFINAE = void>
struct IsRegisteredEnum : std::false_type {};

template<typename EnumType>
struct IsRegisteredEnum<EnumType, RequiresValid<decltype(enumData_(EnumType()))>>
    : std::true_type {};

/// Checks whether `EnumType` is an enumeration type that has been registered
/// via `VGC_DECLARE_ENUM` and `VGC_DEFINE_ENUM`.
///
template<typename EnumType>
inline constexpr bool isRegisteredEnum = IsRegisteredEnum<EnumType>::value;

class Enum;

/// \class vgc::core::EnumValue
/// \brief Stores any Enum value in a type-safe way.
///
class EnumValue {
public:
    /// Creates an empty `EnumValue`.
    ///
    EnumValue()
        : typeId_(core::typeId<void>()) {
    }

    /// Creates an `EnumValue` from the given enumerator value.
    ///
    // Note: This constructor intentionally allows implicit conversions.
    //
    template<typename EnumType, VGC_REQUIRES(std::is_enum_v<EnumType>)>
    EnumValue(EnumType value)
        : typeId_(core::typeId<EnumType>())
        , value_(static_cast<UInt64>(value)) {

        if constexpr (isRegisteredEnum<EnumType>) {

            // Ensures that the global EnumData<EnumType> is initialized by
            // making a call to `enumData_()` now. This is important since the
            // data is lazy-initialized, and this constructor might be our only
            // opportunity to call `enumData_()` before other methods such as
            // `EnumValue::shortName()` are called, which require the data to
            // have already been initialized (we cannot initialize it there
            // because we do not have access to `EnumType` anymore).
            //
            const auto& init = enumData_(value);
            VGC_UNUSED(init);
        }

        // Note: alternatively, we could call getEnumValueData() here and only
        // store the returned pointer as data member. This would make
        // `EnumValue` 64bit instead of 128bit, but would only work for
        // registered enums, and would not support enum values outside of the
        // registered values (so wouldn't work for flags). Therefore, for more
        // genericity, we have chosen the current approach.
    }

    /// Returns whether the `EnumValue` is empty.
    ///
    bool isEmpty() const {
        return typeId_ == core::typeId<void>();
    }

    /// Returns the `TypeId` of the type of the stored enumerator value.
    ///
    /// Returns `typeId<void>()` if `isEmpty()` is true.
    ///
    TypeId typeId() const {
        return typeId_;
    }

    /// Returns whether the `EnumValue` stores an enumerator value
    /// of the given `EnumType`.
    ///
    template<typename EnumType>
    bool has() const {
        return typeId_ == core::typeId<EnumType>();
    }

    /// Returns the stored value as an `EnumType`.
    ///
    /// Throws an exception if the `EnumType` is empty or if the stored
    /// enumerator value is not of type `EnumType`.
    ///
    template<typename EnumType>
    EnumType get() {
        if (isEmpty()) {
            throw core::LogicError(
                "Attempting to get the stored value of an empty EnumValue.");
        }
        TypeId requestedType = core::typeId<EnumType>();
        if (typeId_ != requestedType) {
            throw core::LogicError(core::format(
                "Mismatch between stored EnumValue type ({}) and requested type ({}).",
                typeId_.name(),
                requestedType.name()));
        }
        return static_cast<EnumType>(value_);
    }

    /// Returns the stored value as an `EnumType`.
    ///
    /// The behavior is undefined if the `EnumType` is empty or if the stored
    /// enumerator value is not of type `EnumType`.
    ///
    template<typename EnumType>
    EnumType getUnchecked() {
        return static_cast<EnumType>(value_);
    }

    /// Returns the unqualified name (e.g., "Digit0") of the stored enumerator
    /// value, if any.
    ///
    /// Otherwise, returns "NoValue".
    ///
    std::string_view shortName() const {
        if (auto data = getEnumValueData_()) {
            return data->shortName;
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
        if (auto data = getEnumValueData_()) {
            return data->fullName;
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
        if (auto data = getEnumValueData_()) {
            return data->prettyName;
        }
        else {
            return "No Value";
        }
    }

    /// Returns whether the two `EnumValue` are equal, that is, if
    /// they have both same type and value.
    ///
    bool operator==(const EnumValue& other) const {
        return typeId_ == other.typeId_ && value_ == other.value_;
    }

    /// Returns whether the two `EnumValue` are different.
    ///
    bool operator!=(const EnumValue& other) const {
        return !(*this == other);
    }

    /// Compares the two `EnumValue`.
    ///
    bool operator<(const EnumValue& other) const {
        if (typeId_ == other.typeId_) {
            return value_ < other.value_;
        }
        else {
            return typeId_ < other.typeId_;
        }
    }

private:
    TypeId typeId_;
    UInt64 value_ = 0;

    friend std::hash<EnumValue>;
    friend fmt::formatter<EnumValue>;

    const detail::EnumValueData* getEnumValueData_() const {
        return detail::getEnumValueData(typeId_, value_);
    }

    // For access to the EnumValue(TypeId, UInt64) constructor.
    // Will not be needed anymore if/when we publicize it (see comment below)
    friend Enum;
    friend detail::EnumDataBase;

    // Creates an `EnumValue` given the `typeId` of the enum type and the
    // `underlyingValue` of the enumerator as an `UInt64`.
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
    // EnumValue v1 = EnumValue(typeId<MyEnum>(), 0);
    //
    // // Using the type-safe constructor
    // EnumValue v2 = EnumValue(MyEnum::Foo);
    //
    // assert(v1 == v2);
    // ```
    //
    // Also, note that when using this constructor, then the `shortName()`,
    // `fullName()`, and `prettyName()` may not return correct values as they
    // may not be properly initialized yet, especially at program startup.
    // Using the type-safe constructor instead ensures proper initialization.
    //
    // XXX Publicize, or is it too dangerous?
    //
    // XXX Is it possible/desirable to modify the EnumData implementation to
    // enforce at-least-partial initialization from this constructor? Indeed,
    // while it is impossible to initialize `EnumData<EnumType>::values` here,
    // it might be possible to at least initialize the `EnumDataBase` instance
    // (including shortName, etc.), for example by actually storing it in the
    // global registry separately from the EnumData<EnumType> instance.
    // Instead, we currently have EnumData<EnumType> inherit EnumDataBase, and
    // instanciated as a function static variable discovered via ADL (reason
    // why we need the actual type to be able to call this function), the
    // registry only pointing to this instance. But maybe even partial
    // initialization is impossible, e.g.: how can we find the init function to
    // call without being able to rely on ADL, while keeping the
    // VGC_DECLARE/DEFINE_ENUM macros in the same namespace as the enum type?
    // Should we instead use an actual global variable rather than a function
    // static variable, which would enforce initialization when the dynamic
    // library is loaded rather than at first use? Possibly using a Schwarz
    // counter for more initialization order safety?
    //
    // => The points above might be solvable with a new EnumTypeId class (or
    //    just `EnumType`), see comment [1] in Enum class.
    //
    EnumValue(core::TypeId typeId, UInt64 underlyingValue)
        : typeId_(typeId)
        , value_(underlyingValue) {
    }
};

} // namespace vgc::core

namespace std {

template<>
struct hash<vgc::core::EnumValue> {
    std::size_t operator()(const vgc::core::EnumValue& v) const {
        size_t res = 123456;
        vgc::core::hashCombine(res, v.typeId_, v.value_);
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

namespace vgc::core {

namespace detail {

// We use this base class for everything that does not need to be templated and
// can therefore be moved to the .cpp file.
//
struct VGC_CORE_API EnumDataBase {

    // EnumDataBase is non-copyable (because it stores unique_ptrs).
    // We need to explicitly mark it non-copyable otherwise it doesn't compile on MSVC.
    EnumDataBase(const EnumDataBase&) = delete;
    EnumDataBase& operator=(const EnumDataBase&) = delete;

    TypeId typeId;

    std::string fullTypeName;  // "vgc::ui::Key"
    std::string shortTypeName; // "Key"

    std::string unknownItemFullName;   // "vgc::ui::Key::Unknown_Key"
    std::string unknownItemShortName;  // "Unknown_Key"
    std::string unknownItemPrettyName; // "Unknown Key"

    // This is where the actual per-enumerator data is stored. We use pointers
    // to ensure that any string_view to the stored strings stay valid when
    // valueData grows, or when this EnumDataBase is moved.
    //
    Array<std::unique_ptr<EnumValueData>> valueData;

    // These maps make it possible to quickly find the appropriate
    // EnumValueData from the enum value or the enum short name.
    //
    std::unordered_map<UInt64, Int> valueToIndex;
    std::unordered_map<std::string_view, Int> shortNameToIndex;

    // These are convenient arrays (redundant with valueData) facilitating
    // iteration without the need for proxy iterators. This approach increases
    // debuggability and works better with parallelization libs (sometimes not
    // supporting proxy iterators).
    //
    Array<EnumValue> enumValues;
    Array<std::string_view> fullNames;
    Array<std::string_view> shortNames;
    Array<std::string_view> prettyNames;

    EnumDataBase(TypeId id, std::string_view prettyFunction);
    virtual ~EnumDataBase() = 0;

    void
    addItemBase(UInt64 value, std::string_view shortName, std::string_view prettyName);

    std::optional<Int> getIndexBase(UInt64 value) const {
        auto search = valueToIndex.find(value);
        if (search != valueToIndex.end()) {
            return search->second;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<Int> getIndexFromShortName(std::string_view shortName) const {
        auto search = shortNameToIndex.find(shortName);
        if (search != shortNameToIndex.end()) {
            return search->second;
        }
        else {
            return std::nullopt;
        }
    }
};

template<typename EnumType>
struct EnumData : EnumDataBase {

    Array<EnumType> values;

    EnumData(TypeId id, std::string_view prettyFunction)
        : EnumDataBase(id, prettyFunction) {
    }

    void
    addItem(EnumType value, std::string_view shortName, std::string_view prettyName) {
        addItemBase(static_cast<UInt64>(value), shortName, prettyName);
        values.append(value);
    }

    std::optional<Int> getIndex(EnumType value) const {
        return getIndexBase(static_cast<UInt64>(value));
    }
};

} // namespace detail

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
                return EnumValue(enumTypeId, data->valueData[*index]->value);
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

} // namespace vgc::core

// clang-format off

// Partial specialization of fmt::formatter for registered enum types. This
// relies on SFINAE and ADL: we SFINAE-test whether enumData_(item) is valid
// code, which is true for registered enum types since the function enumData_()
// is found via Argument-Dependent Lookup. Note that for this reason, we cannot
// move the function enumData_() to a separate namespace, e.g., `detail`.
//
template<typename EnumType>
struct fmt::formatter<
        EnumType, char,
        vgc::core::RequiresValid<decltype(enumData_(EnumType()))>>
    : fmt::formatter<std::string_view> {

    template<typename FormatContext>
    auto format(EnumType item, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(
            vgc::core::Enum::fullName(item), ctx);
    }
};

/// Declares a scoped enum. See `Enum` for more details.
///
#define VGC_DECLARE_ENUM(Enum)                                                           \
    const ::vgc::core::detail::EnumData<Enum>& enumData_(Enum value);

/// Starts the definition of a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM_BEGIN(Enum)                                                      \
    const ::vgc::core::detail::EnumData<Enum>& enumData_(Enum) {                         \
        using EnumType = Enum;                                                           \
        static ::std::string pf = VGC_PRETTY_FUNCTION;                                   \
        static auto createData = []() {                                                  \
            auto data = new ::vgc::core::detail::EnumData<Enum>(                         \
                ::vgc::core::typeId<EnumType>(), pf);                                    \
            registerEnumDataBase(*data);

/// Defines an enumerator of a scoped enum. See `Enum` for more details.
///
#define VGC_ENUM_ITEM(name, prettyName)                                                  \
            data->addItem(EnumType::name, VGC_PP_STR(name), prettyName);

/// Ends the definition of a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM_END()                                                            \
            return data;                                                                 \
        };                                                                               \
        static auto data = createData();                                                 \
        return *data;                                                                    \
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
