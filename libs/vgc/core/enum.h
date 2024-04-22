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

// We use this base class for everything that does not need to be templated and
// can therefore be moved to the .cpp file.
//
struct VGC_CORE_API EnumDataBase {

    TypeId id;

    std::string fullTypeName;  // "vgc::ui::Key"
    std::string shortTypeName; // "Key"

    std::string unknownItemFullName;   // "vgc::ui::Key::Unknown_Key"
    std::string unknownItemShortName;  // "Unknown_Key"
    std::string unknownItemPrettyName; // "Unknown Key"

    Array<std::string> fullNames;   // "vgc::ui::Key::Digit0"
    Array<std::string> shortNames;  // "Digit0"
    Array<std::string> prettyNames; // "0"

    // Note: we store the data in separate arrays (rather than a single
    // Array<ItemData>) and with redundant info (common prefix for all
    // fullNames) in order to facilitate iteration without the need for proxy
    // iterators. This approach is typically safer (stable string addresses),
    // increases debuggability, and works better with parallelization libs
    // (sometimes not supporting proxy iterators).

    EnumDataBase(TypeId id, std::string_view prettyFunction);
    virtual ~EnumDataBase() = 0;

    void addItemBase(std::string_view shortName, std::string_view prettyName);
};

template<typename EnumType>
struct VGC_CORE_API EnumData : EnumDataBase {

    Array<EnumType> values;
    std::unordered_map<EnumType, Int> valueToIndex;

    EnumData(TypeId id, std::string_view prettyFunction)
        : EnumDataBase(id, prettyFunction) {
    }

    void
    addItem(EnumType value, std::string_view shortName, std::string_view prettyName) {
        Int index = values.length();
        valueToIndex[value] = index;
        values.append(value);
        addItemBase(shortName, prettyName);
    }

    std::optional<Int> getIndex(EnumType value) const {
        auto search = valueToIndex.find(value);
        if (search != valueToIndex.end()) {
            return search->second;
        }
        else {
            return std::nullopt;
        }
    }
};

} // namespace detail

class Enum;

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

    using StringArrayView = ArrayView<std::string>;

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
            ::vgc::core::detail::EnumData<Enum> data(::vgc::core::typeId<EnumType>(), pf);

/// Defines an enumerator of a scoped enum. See `Enum` for more details.
///
#define VGC_ENUM_ITEM(name, prettyName)                                                  \
            data.addItem(EnumType::name, VGC_PP_STR(name), prettyName);

/// Ends the definition of a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM_END()                                                            \
            return data;                                                                 \
        };                                                                               \
        static auto data = createData();                                                 \
        return data;                                                                     \
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
