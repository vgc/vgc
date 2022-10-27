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

#include <vgc/core/compiler.h>
#include <vgc/core/format.h>
#include <vgc/core/templateutil.h>

namespace vgc::core {

namespace detail {

// Extracts the fully-qualified name of the enum class (e.g., `vgc::ui::Key`)
// from the platform-dependent VGC_PRETTY_FUNCTION output of enumData_, e.g.:
//
// `const class vgc::core::detail::EnumData &__cdecl vgc::ui::enumData_(enum vgc::ui::Key)`
//
inline std::string_view fullEnumClassName(std::string_view enumDataPrettyFunction) {

    const auto& s = enumDataPrettyFunction;

    // Find closing parenthesis
    size_t j = s.size();
    if (j == 0) {
        return "";
    }
    --j;
    while (j > 0 && s[j] != ')') {
        --j;
    }

    // Find opening parenthesis or whitespace
    size_t i = j;
    while (i > 0 && s[i] != '(' && s[i] != ' ') {
        --i;
    }
    ++i;

    // Return substring if there is a match, otherwise return the empty string
    if (j > i) {
        return s.substr(i, j - i);
    }
    else {
        return "";
    }
}

// Stores strings related to an enum item
class EnumData {
public:
    EnumData(
        std::string_view fullEnumClassName,    // "vgc::ui::Key"
        std::string_view shortEnumItemName,    // "Digit0"
        std::string_view enumItemPrettyName) { // "0"

        fullEnumItemName_ = fullEnumClassName;
        fullEnumItemName_.append("::");
        fullEnumItemName_.append(shortEnumItemName);
        enumItemPrettyName_ = enumItemPrettyName;
        prefixSize_ = fullEnumItemName_.size() - shortEnumItemName.size();
    }

    std::string_view shortName() const {
        return std::string_view(fullEnumItemName_).substr(prefixSize_);
    }

    std::string_view fullName() const {
        return fullEnumItemName_;
    }

    std::string_view prettyName() const {
        return enumItemPrettyName_;
    }

private:
    std::string fullEnumItemName_;   // "vgc::ui::Key::Digit0"
    std::string enumItemPrettyName_; // "0"
    size_t prefixSize_;              // "vgc::ui::Key::".size()
};

} // namespace detail

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
    template<typename EnumType>
    static std::string_view shortName(EnumType item) {
        return enumData_(item).shortName();
    }

    template<typename EnumType>
    static std::string_view fullName(EnumType item) {
        return enumData_(item).fullName();
    }

    template<typename EnumType>
    static std::string_view prettyName(EnumType item) {
        return enumData_(item).prettyName();
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
    const ::vgc::core::detail::EnumData& enumData_(Enum value);

/// Starts the definition of a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM_BEGIN(Enum)                                                      \
    const ::vgc::core::detail::EnumData& enumData_(Enum value) {                         \
        using E = Enum;                                                                  \
        using S = ::vgc::core::detail::EnumData;                                         \
        using Map = std::unordered_map<E, S>;                                            \
        std::string pf = VGC_PRETTY_FUNCTION;                                            \
        static const std::string_view fecn = ::vgc::core::detail::fullEnumClassName(pf); \
        static const S unknown =                                                         \
            S(fecn, "Unknown_" #Enum, "Unknown " #Enum);                                 \
        static auto createMap = []() {                                                   \
            Map map;

/// Defines an enumerator of a scoped enum. See `Enum` for more details.
///
#define VGC_ENUM_ITEM(name, prettyName)                                                  \
            map.insert({E::name, S(fecn, VGC_PP_STR(name), prettyName)});

/// Ends the definition of a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM_END()                                                            \
            return map;                                                                  \
        };                                                                               \
        static const Map map = createMap();                                              \
        auto search = map.find(value);                                                   \
        if (search != map.end()) {                                                       \
            return search->second;                                                       \
        }                                                                                \
        else {                                                                           \
            return unknown;                                                              \
        }                                                                                \
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
#define VGC_ENUM_COUNT(enum_) static_cast<std::underlying_type_t<enum_>>(enum_::End_)

// clang-format on

#endif // VGC_CORE_ENUM_H
