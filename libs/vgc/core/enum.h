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

// We use this base class for everything that does not need to be templated and
// can therefore be moved to the .cpp file.
//
struct VGC_CORE_API EnumDataBase {

    EnumDataBase(const EnumDataBase&) = delete;
    EnumDataBase& operator=(const EnumDataBase&) = delete;

    EnumDataBase(EnumDataBase&&) = default;
    EnumDataBase& operator=(EnumDataBase&&) = default;

    TypeId typeId;

    std::string fullTypeName;  // "vgc::ui::Key"
    std::string shortTypeName; // "Key"

    std::string unknownItemFullName;   // "vgc::ui::Key::Unknown_Key"
    std::string unknownItemShortName;  // "Unknown_Key"
    std::string unknownItemPrettyName; // "Unknown Key"

    std::unordered_map<UInt64, Int> valueToIndex;

    // We use pointers to ensure the string_views stay valid when valueData grows.
    Array<std::unique_ptr<EnumValueData>> valueData;
    Array<std::string_view> fullNames;
    Array<std::string_view> shortNames;
    Array<std::string_view> prettyNames;

    // Note: we redundantly store the data in separate arrays in order to
    // facilitate iteration without the need for proxy iterators. This approach
    // increases debuggability and works better with parallelization libs
    // (sometimes not supporting proxy iterators).

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
};

VGC_CORE_API
void registerEnumDataBase(const EnumDataBase& data);

VGC_CORE_API
const EnumDataBase& getEnumDataBase(TypeId typeId);

inline const EnumValueData* getEnumValueData(TypeId typeId, UInt64 value) {
    const EnumDataBase& dataBase = getEnumDataBase(typeId);
    if (auto index = dataBase.getIndexBase(value)) {
        return dataBase.valueData[*index].get();
    }
    else {
        return nullptr;
    }
}

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

/// \class vgc::core::EnumValue
/// \brief Stores any Enum value in a type-safe way.
///
class EnumValue {
private:
    struct EmptyTag {};

public:
    /// Creates an empty `EnumValue`.
    ///
    EnumValue()
        : data_(nullptr) {
    }

    /// Creates an `EnumValue` from the given enumerator value.
    ///
    // Note: This constructor intentionally allows implicit conversions.
    //
    template<typename EnumType, VGC_REQUIRES(std::is_enum_v<EnumType>)>
    EnumValue(EnumType value)
        : data_(detail::getEnumValueData(
              core::typeId<EnumType>(),
              static_cast<UInt64>(value))) {

        // Note: alternatively, we could store the typeId and the UInt64 value
        // as data members, and defer the call to getEnumValueData() to when
        // it's needed. However, this would double the sizeof(EnumValue), and
        // be slower if the data is needed more than once. But it would be
        // faster is the data is never needed. If we defer the call to
        // getEnumValueData() but keep it in cache, this would triple the
        // sizeof(EnumValue) and would need thread-safey considerations.
        //
        // However, note that our current approach doesn't allow storing
        // values which have not been registered.
        //
        // So it's a tradeoff, and we haven't yet do serious benchmarking to
        // know what's best.
    }

    /// Returns whether the `EnumValue` is empty.
    ///
    bool isEmpty() const {
        return data_ == nullptr;
    }

    /// Returns the `TypeId` of the type of the stored enumerator value, if any.
    ///
    std::optional<TypeId> typeId() const {
        if (data_) {
            return data_->typeId;
        }
        else {
            return std::nullopt;
        }
    }

    /// Returns whether the `EnumValue` stores an enumerator value
    /// of the given `EnumType`.
    ///
    template<typename EnumType>
    bool has() const {
        return typeId() == core::typeId<EnumType>();
    }

    /// Returns the stored value as an `EnumType`.
    ///
    /// Throws an exception if the `EnumType` is empty or if the stored
    /// enumerator value is not of type `EnumType`.
    ///
    template<typename EnumType>
    EnumType get() {
        if (!data_) {
            throw core::LogicError(
                "Attempting to get the stored value of an empty EnumValue.");
        }
        TypeId requestedType = core::typeId<EnumType>();
        if (data_->typeId != requestedType) {
            throw core::LogicError(core::format(
                "Mismatch between stored EnumValue type ({}) and requested type ({}).",
                data_->typeId.name(),
                requestedType.name()));
        }
        return static_cast<EnumType>(data_->value);
    }

    /// Returns the stored value as an `EnumType`.
    ///
    /// The behavior is undefined if the `EnumType` is empty or if the stored
    /// enumerator value is not of type `EnumType`.
    ///
    template<typename EnumType>
    EnumType getUnchecked() {
        return static_cast<EnumType>(data_->value);
    }

    /// Returns the short name (e.g., "Digit0") of the stored enumerator value,
    /// if any.
    ///
    /// Otherwise, returns "NoValue".
    ///
    std::string_view shortName() const {
        if (data_) {
            return data_->shortName;
        }
        else {
            return "NoValue";
        }
    }

    /// Returns the long name (e.g., "vgc::ui::Key::Digit0") of the stored
    /// enumerator value, if any.
    ///
    /// Otherwise, returns "NoType::NoValue".
    ///
    std::string_view fullName() const {
        if (data_) {
            return data_->fullName;
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
        if (data_) {
            return data_->prettyName;
        }
        else {
            return "No Value";
        }
    }

    /// Returns whether the two `EnumValue` are equal, that is, if
    /// they have both same type and value.
    ///
    bool operator==(const EnumValue& other) const {
        return data_ == other.data_;
    }

    /// Returns whether the two `EnumValue` are different.
    ///
    bool operator!=(const EnumValue& other) const {
        return !(*this == other);
    }

    /// Compares the two `EnumValue`.
    ///
    bool operator<(const EnumValue& other) const {
        if (data_ && other.data_) {
            if (data_->typeId == other.data_->typeId) {
                return data_->value < other.data_->value;
            }
            else {
                return data_->typeId < other.data_->typeId;
            }
        }
        else {
            return data_ < other.data_;
        }
    }

private:
    const detail::EnumValueData* data_ = nullptr;

    friend std::hash<EnumValue>;
    friend fmt::formatter<EnumValue>;
};

} // namespace vgc::core

namespace std {

template<>
struct hash<vgc::core::EnumValue> {
    std::size_t operator()(const vgc::core::EnumValue& v) const {
        return std::hash<const vgc::core::detail::EnumValueData*>()(v.data_);
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

/// \class vgc::core::Enum
/// \brief Query metadata about registered enum items
///
/// In order to provide introspection and easy printing of enum items (=
/// "enumerator"), each enum item and its "pretty name" can be registered using
/// the `VGC_DECLARE_ENUM` and `VGC_DEFINE_ENUM` macros, as such:
///
/// ```cpp
/// // In the .h file
///
/// namespace vgc::foo {
///
/// enum class MyEnum {
///     Value1,
///     Value2
/// };
///
/// VGC_FOO_API
/// VGC_DECLARE_ENUM(MyEnum)
///
/// } // namespace lib::foo
///
///
/// // In the .cpp file
///
/// namespace vgc::foo {
///
/// VGC_DEFINE_ENUM(
///     MyEnum,
///     (Value1, "Value 1"),
///     (Value2, "Value 2"))
///
/// } // namespace lib::foo
///
/// ```
///
/// Once registered, you can query various strings about the enum items via the
/// following static functions of `vgc::core::Enum`:
///
/// - `Enum::shortName(item)`: for example, `"Value1"`
/// - `Enum::fullName(item)`: for example, `"vgc::foo::MyEnum::Value1"`
/// - `Enum::prettyName(item)`: for example, `"Value 1"`
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

    template<typename EnumType>
    static std::string_view shortTypeName() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.shortTypeName;
    }

    template<typename EnumType>
    static std::string_view fullTypeName() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.fullTypeName;
    }

    // TODO: prettyTypeName?

    template<typename EnumType>
    static ArrayView<EnumType> values() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.values;
    }

    template<typename EnumType>
    static StringArrayView shortNames() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.shortNames;
    }

    template<typename EnumType>
    static StringArrayView fullNames() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.fullNames;
    }

    template<typename EnumType>
    static StringArrayView prettyNames() {
        const detail::EnumData<EnumType>& data = enumData_(EnumType{});
        return data.prettyNames;
    }

    template<typename EnumType>
    static std::string_view shortName(EnumType item) {
        const detail::EnumData<EnumType>& data = enumData_(item);
        if (auto index = data.getIndex(item)) {
            return data.shortNames[*index];
        }
        else {
            return data.unknownItemShortName;
        }
    }

    template<typename EnumType>
    static std::string_view fullName(EnumType item) {
        const detail::EnumData<EnumType>& data = enumData_(item);
        if (auto index = data.getIndex(item)) {
            return data.fullNames[*index];
        }
        else {
            return data.unknownItemFullName;
        }
    }

    template<typename EnumType>
    static std::string_view prettyName(EnumType item) {
        const detail::EnumData<EnumType>& data = enumData_(item);
        if (auto index = data.getIndex(item)) {
            return data.prettyNames[*index];
        }
        else {
            return data.unknownItemPrettyName;
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
        static auto data = createData();                                      \
                                                             \
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
