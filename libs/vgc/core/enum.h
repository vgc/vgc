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

#include <vgc/core/format.h>
#include <vgc/core/templateutil.h>

namespace vgc::core {

namespace detail {

// Parses VGC_PRETTY_FUNCTION of enumData_() to get fully-qualified enum name
// as string.
inline std::string_view fullEnum(std::string_view enumDataFunctionName) {

    const auto& s = enumDataFunctionName;

    // Find closing parenthesis
    size_t j = s.size();
    while (j > 0 && s[j] != ')') {
        --j;
    }

    // Find opening parenthesis or whitespace
    size_t i = j;
    while (i > 0 && s[i] != '(' && s[i] != ' ') {
        --i;
    }
    ++i;

    if (j > i) {
        return s.substr(i, j - i);
    }
    else {
        return "";
    }
}

// Stores strings related to an enumerator of an `enum` or `enum class`
class EnumData {
public:
    EnumData(
        std::string_view fullEnum,
        std::string_view /*enum_*/,
        std::string_view name,
        std::string_view prettyName) {

        fullName_ = fullEnum;
        fullName_.append("::");
        fullName_.append(name);
        prettyName_ = prettyName;
        prefixSize_ = fullName_.size() - name.size();
    }

    std::string_view shortName() const {
        return std::string_view(fullName_).substr(prefixSize_);
    }

    std::string_view fullName() const {
        return fullName_;
    }

    std::string_view prettyName() const {
        return prettyName_;
    }

private:
    size_t prefixSize_;
    std::string fullName_;
    std::string prettyName_;
};

} // namespace detail

/// \class vgc::core::Enum
/// \brief Query metadata about enums
///
/// Strings related to a scoped enum can be defined via the following function:
///
/// ```cpp
/// // In the .h file
///
/// namespace lib::foo {
/// enum class MyEnum {
///     Value1,
///     Value2
/// };
/// }
///
/// VGC_DECLARE_ENUM((lib, foo), MyEnum)
///
/// // In the .cpp file
///
/// VGC_DEFINE_ENUM(
///     (lib, foo),
///     MyEnum,
///     (Value1, "Value 1"),
///     (Value2, "Value 2"))
/// ```
///
/// Once defined, this allow you to query strings via the following static
/// functions of `vgc::core::Enum`:
///
/// - `shortName()`: for example, `Value1`
/// - `fullName()`: for example, `lib::foo::MyEnum::Value1`
/// - `prettyName()`: for example, `Value `
///
/// Note that due to technical reasons (template specialization of
/// `fmt::formatter`), the macros `VGC_DECLARE_ENUM` and `VGC_DEFINE_ENUM` must
/// appear at the global namespace scope.
///
/// Also note that due to compiler macro limitations, `VGC_DEFINE_ENUM` only
/// supports up to 122 enumerators. If you need more, you can use the following
/// long-form version, which is a little more verbose but doesn't have limits
/// on the number of enumerators:
///
/// ```cpp
/// VGC_DEFINE_ENUM_BEGIN((lib, foo), MyEnum)
///     VGC_ENUMERATOR(Value1, "Value 1")
///     VGC_ENUMERATOR(Value2, "Value 2")
///     ...
///     VGC_ENUMERATOR(ValueN, "Value N")
/// VGC_DEFINE_ENUM_END()
/// ```
///
class Enum {
public:
    template<typename EnumType>
    static std::string_view shortName(EnumType enumerator) {
        return enumData_(enumerator).shortName();
    }

    template<typename EnumType>
    static std::string_view fullName(EnumType enumerator) {
        return enumData_(enumerator).fullName();
    }

    template<typename EnumType>
    static std::string_view prettyName(EnumType enumerator) {
        return enumData_(enumerator).prettyName();
    }
};

namespace detail {

template<typename EnumType>
struct EnumFormatter : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(EnumType v, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(Enum::fullName(v), ctx);
    }
};

} // namespace detail

} // namespace vgc::core

// specialize fmt formatter for all enum types that define an enumData_
// overload.

template<typename T>
struct fmt::formatter<T, char, vgc::core::RequiresValid<decltype(enumData_(T()))>>
    : ::vgc::core::detail::EnumFormatter<T> {};

// clang-format off

/// Declares a scoped enum. See `Enum` for more details.
///
#define VGC_DECLARE_ENUM(Enum)                                                           \
    const ::vgc::core::detail::EnumData& enumData_(Enum value);

/// Starts the definition of a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM_BEGIN(Enum)                                           \
    const ::vgc::core::detail::EnumData& enumData_(Enum value) {                         \
        using E = Enum;                                                                  \
        using S = ::vgc::core::detail::EnumData;                                         \
        using Map = std::unordered_map<E, S>;                                            \
        std::string fn = VGC_PRETTY_FUNCTION; \
        static const std::string_view fenum = ::vgc::core::detail::fullEnum(fn); \
        static const char* enum_ = VGC_PP_STR(Enum);                                     \
        static const S unknown =                                                         \
            S(fenum, enum_, "Unknown_" #Enum, "Unknown " #Enum);                    \
        static auto createMap = []() {                                                   \
            Map map;

/// Defines an enumerator of a scoped enum. See `Enum` for more details.
///
#define VGC_ENUMERATOR(name, prettyName)                                                 \
            map.insert({E::name, S(fenum, enum_, VGC_PP_STR(name), prettyName)});

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

// clang-format on

#define VGC_ENUMERATOR_(x, t)                                                            \
    VGC_ENUMERATOR(                                                                      \
        VGC_PP_EXPAND(VGC_PP_PAIR_FIRST t), VGC_PP_EXPAND(VGC_PP_PAIR_SECOND t))

#define VGC_DEFINE_ENUM_X_(Enum, ...)                                                    \
    VGC_DEFINE_ENUM_BEGIN(Enum)                                                          \
        VGC_PP_EXPAND(VGC_PP_FOREACH_X(VGC_ENUMERATOR_, Enum, __VA_ARGS__))              \
    VGC_DEFINE_ENUM_END()

/// Defines a scoped enum. See `Enum` for more details.
///
#define VGC_DEFINE_ENUM(...) VGC_PP_EXPAND(VGC_DEFINE_ENUM_X_(__VA_ARGS__, ~))

#endif // VGC_CORE_ENUM_H
