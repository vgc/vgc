// Copyright 2021 The VGC Developers
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

#ifndef VGC_DOM_VALUE_H
#define VGC_DOM_VALUE_H

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/enum.h>
#include <vgc/core/format.h>
#include <vgc/core/sharedconst.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/path.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::dom {

class Value;

/// \enum vgc::dom::ValueType
/// \brief Specifies the type of an attribute Value
///
/// Although W3C DOM attribute values are always strings, VGC DOM attribute
/// values are typed, and this class enumerates all allowed types. The type of
/// a given attribute Value can be retrieved at runtime via Value::type().
///
/// In order to write out compliant XML files, we define conversion rules
/// from/to strings in order to correcty encode and decode the types of
/// attributes.
///
/// Typically, in the XML file, types are not explicitly specified, since
/// built-in element types (e.g., "path") have built-in attributes (e.g.,
/// "positions") with known built-in types (in this case, "Vec2dArray").
/// Therefore, the VGC parser already knows what types all of these
/// built-in attributes have.
///
/// However, it is also allowed to add custom data to any element, and the type
/// of this custom data must be encoded in the XML file by following a specific
/// naming scheme. More specifically, the name of custom attributes must be
/// data-<t>-<name>, where <name> is a valid XML attribute name chosen by the
/// user (which cannot be equal to any of the builtin attribute names of this
/// element), and <t> is a one-letter code we call "type-hint". This type-hint
/// allows the parser to unambiguously deduce the type of the attribute from
/// the XML string value. The allowed type-hints are:
///
/// - c: color
/// - d: 64bit floating point
/// - i: 32bit integer
/// - s: string
///
/// For example, if you wish to store a custom sequence of indices (that is,
/// some data of type IntArray) into a given path, you can do as follows:
///
/// \code
/// <path
///     data-i-myIndices="[12, 42, 10]"
/// />
/// \endcode
///
/// If you wish to store a custom sequence of 2D coordinates (Vec2dArray):
///
/// \code
/// <path
///     data-d-myCoords="[(0, 0), (12, 42), (10, 34)]"
/// />
/// \endcode
///
/// Note how in the example above, the type "Vec2dArray" is automatically
/// deduced from the type-hint 'd' and the XML string value. However, without
/// the type-hint, the parser wouldn't be able to determine whether the
/// attribute has the type Vec2dArray or Vec2iArray.
///
/// XXX What if it's an empty array? data-d-myData="[]"? How to determine the
/// type from the type-hint? -> maybe we should forget about this "type-hint"
/// idea altogether, and just have authors always give the full type:
/// data-Vec2dArray-myCoords="[]"
///
enum class ValueType {
    None,
    Invalid,
    String,
    StringId,
    Int,
    IntArray,
    Double,
    DoubleArray,
    Color,
    ColorArray,
    Vec2d,
    Vec2dArray,
    Path,
    PathArray,
    VGC_ENUM_ENDMAX
};
VGC_DOM_API
VGC_DECLARE_ENUM(ValueType)

struct NoneValue : std::monostate {};
struct InvalidValue : std::monostate {};

namespace detail {

template<ValueType valueType>
struct ValueTypeTraits {};

template<typename T>
struct ValueTypeTraitsFromType {};

#define VGC_DOM_VALUETYPE_TYPE_(enumerator, Type_)                                       \
    template<>                                                                           \
    struct ValueTypeTraits<ValueType::enumerator> {                                      \
        using Type = Type_;                                                              \
    };                                                                                   \
    template<>                                                                           \
    struct ValueTypeTraitsFromType<Type_> {                                              \
        static constexpr ValueType value = ValueType::enumerator;                        \
        static constexpr size_t index = core::toUnderlying(value);                       \
        using Type = Type_;                                                              \
    };
// clang-format off
VGC_DOM_VALUETYPE_TYPE_(None,          NoneValue)
VGC_DOM_VALUETYPE_TYPE_(Invalid,       InvalidValue)
VGC_DOM_VALUETYPE_TYPE_(String,        std::string)
VGC_DOM_VALUETYPE_TYPE_(StringId,      core::StringId)
VGC_DOM_VALUETYPE_TYPE_(Int,           Int)
VGC_DOM_VALUETYPE_TYPE_(IntArray,      core::SharedConstIntArray)
VGC_DOM_VALUETYPE_TYPE_(Double,        double)
VGC_DOM_VALUETYPE_TYPE_(DoubleArray,   core::SharedConstDoubleArray)
VGC_DOM_VALUETYPE_TYPE_(Color,         core::Color)
VGC_DOM_VALUETYPE_TYPE_(ColorArray,    core::SharedConstColorArray)
VGC_DOM_VALUETYPE_TYPE_(Vec2d,         geometry::Vec2d)
VGC_DOM_VALUETYPE_TYPE_(Vec2dArray,    geometry::SharedConstVec2dArray)
VGC_DOM_VALUETYPE_TYPE_(Path,          dom::Path)
VGC_DOM_VALUETYPE_TYPE_(PathArray,     dom::SharedConstPathArray)
// clang-format on
#undef VGC_DOM_VALUETYPE_TYPE_

template<typename Seq>
struct ValueVariant_;

template<size_t... Is>
struct ValueVariant_<std::index_sequence<Is...>> {
    using Type =
        std::variant<typename ValueTypeTraits<static_cast<ValueType>(Is)>::Type...>;
};

using ValueVariant =
    typename ValueVariant_<std::make_index_sequence<VGC_ENUM_COUNT(ValueType)>>::Type;

static_assert(std::is_copy_constructible_v<ValueVariant>);
static_assert(std::is_copy_assignable_v<ValueVariant>);
static_assert(std::is_move_constructible_v<ValueVariant>);
static_assert(std::is_move_assignable_v<ValueVariant>);

template<typename T, typename SFINAE = void>
struct IsValidValueType : std::false_type {};

template<typename T>
struct IsValidValueType<T, core::RequiresValid<typename ValueTypeTraitsFromType<T>::Type>>
    : std::true_type {};

template<typename T>
inline constexpr bool isValidValueType = IsValidValueType<T>::value;

template<typename T>
using ValueMakeSharedConstIfValid =
    std::conditional_t<isValidValueType<core::SharedConst<T>>, core::SharedConst<T>, T>;

} // namespace detail

template<typename T>
inline constexpr bool isCompatibleValueType =
    detail::isValidValueType<detail::ValueMakeSharedConstIfValid<T>>;

/// \class vgc::dom::Value
/// \brief Holds the value of an attribute
///
class VGC_DOM_API Value {
public:
    /// Constructs an empty value, that is, whose ValueType is None.
    ///
    constexpr Value()
        : var_(NoneValue{}) {
    }

    /// Returns a const reference to an empty value. This is useful for instance
    /// for optional values or to simply express non-initialized or null.
    ///
    static const Value& none();

    /// Returns a const reference to an invalid value. This is useful for error
    /// handling in methods that must return a `Value` by const reference.
    ///
    static const Value& invalid();

    /// Constructs a `Value` holding a `std::string`.
    ///
    Value(std::string_view string)
        : var_(std::string(string)) {
    }

    /// Constructs a `Value` holding a `std::string`.
    ///
    Value(std::string&& string)
        : var_(std::move(string)) {
    }

    /// Constructs a `Value` holding a `std::string`.
    ///
    Value(core::StringId stringId)
        : var_(stringId) {
    }

    /// Constructs a `Value` holding an `Int`.
    ///
    Value(Int value)
        : var_(value) {
    }

    /// Constructs a `Value` holding a shared const array of `Int`.
    ///
    Value(core::IntArray intArray)
        : var_(std::in_place_type<core::SharedConstIntArray>, std::move(intArray)) {
    }

    /// Constructs a `Value` holding a shared const array of `Int`.
    ///
    Value(core::SharedConstIntArray intArray)
        : var_(std::move(intArray)) {
    }

    /// Constructs a `Value` holding a `double`.
    ///
    Value(double value)
        : var_(value) {
    }

    /// Constructs a `Value` holding a shared const array of `double`.
    ///
    Value(core::DoubleArray doubleArray)
        : var_(std::in_place_type<core::SharedConstDoubleArray>, std::move(doubleArray)) {
    }

    /// Constructs a `Value` holding a shared const array of `double`.
    ///
    Value(core::SharedConstDoubleArray doubleArray)
        : var_(std::move(doubleArray)) {
    }

    /// Constructs a `Value` holding a `Color`.
    ///
    Value(core::Color color)
        : var_(std::move(color)) {
    }

    /// Constructs a `Value` holding a shared const array of `Color`.
    ///
    Value(core::ColorArray colorArray)
        : var_(std::in_place_type<core::SharedConstColorArray>, std::move(colorArray)) {
    }

    /// Constructs a `Value` holding a shared const array of `Color`.
    ///
    Value(core::SharedConstColorArray colorArray)
        : var_(std::move(colorArray)) {
    }

    /// Constructs a `Value` holding a `Vec2d`.
    ///
    Value(geometry::Vec2d vec2d)
        : var_(vec2d) {
    }

    /// Constructs a `Value` holding a shared const array of `Vec2d`.
    ///
    Value(geometry::Vec2dArray vec2dArray)
        : var_(
            std::in_place_type<geometry::SharedConstVec2dArray>,
            std::move(vec2dArray)) {
    }

    /// Constructs a `Value` holding a shared const array of `Vec2d`.
    ///
    Value(geometry::SharedConstVec2dArray vec2dArray)
        : var_(std::move(vec2dArray)) {
    }

    /// Constructs a `Value` holding a `dom::Path`.
    ///
    Value(dom::Path path)
        : var_(path) {
    }

    /// Constructs a `Value` holding a shared const array of `dom::Path`.
    ///
    Value(dom::PathArray pathArray)
        : var_(std::in_place_type<dom::SharedConstPathArray>, std::move(pathArray)) {
    }

    /// Constructs a `Value` holding a shared const array of `dom::Path`.
    ///
    Value(dom::SharedConstPathArray pathArray)
        : var_(std::move(pathArray)) {
    }

    /// Returns the ValueType of this Value.
    ///
    ValueType type() const {
        return static_cast<ValueType>(var_.index());
    }

    /// Returns whether this `Value` is Valid, that is, whether type() is not
    /// ValueType::Invalid, which means that it does hold one of the correct
    /// values.
    ///
    bool isValid() const {
        return type() != ValueType::Invalid;
    }

    bool hasValue() const {
        return type() > ValueType::Invalid;
    }

    bool isNone() const {
        return type() == ValueType::None;
    }

    /// Stops holding any Value. This makes this `Value` empty.
    ///
    void clear();

    /// Returns the item held by the container in this `Value` at the given `index`
    /// wrapped. This returns an empty value if `type() != ValueType::Array..`.
    ///
    Value getItemWrapped(Int index) {
        return visit([&, index = index](auto&& arg) -> Value {
            using Type = std::decay_t<decltype(arg)>;
            using UnsharedType = core::RemoveSharedConst<Type>;
            if constexpr (core::isArray<UnsharedType>) {
                const UnsharedType& x = arg;
                return Value(x.getWrapped(index));
            }
            else {
                return Value();
            }
        });
    }

    /// Returns the length of the held array.
    /// Returns 0 if this value is not an array.
    ///
    Int arrayLength() {
        return visit([&](auto&& arg) -> Int {
            using Type = std::decay_t<decltype(arg)>;
            using UnsharedType = core::RemoveSharedConst<Type>;
            if constexpr (core::isArray<UnsharedType>) {
                const UnsharedType& x = arg;
                return x.length();
            }
            else {
                return 0;
            }
        });
    }

    /// Returns the string held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::String`.
    ///
    const std::string& getString() const {
        return std::get<std::string>(var_);
    }

    /// Sets this `Value` to the given string `s`.
    ///
    void set(std::string s) {
        var_ = std::move(s);
    }

    /// Returns the string identifier held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::StringId`.
    ///
    core::StringId getStringId() const {
        return std::get<core::StringId>(var_);
    }

    /// Sets this `Value` to the given string `s`.
    ///
    void set(core::StringId s) {
        var_ = std::move(s);
    }

    /// Returns the integer held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::Int`.
    ///
    Int getInt() const {
        return std::get<Int>(var_);
    }

    /// Sets this `Value` to the given integer `value`.
    ///
    void set(Int value) {
        var_ = value;
    }

    /// Returns the `core::SharedConstIntArray` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::IntArray`.
    ///
    const core::SharedConstIntArray& getIntArray() const {
        return std::get<core::SharedConstIntArray>(var_);
    }

    /// Sets this `Value` to the given shared const `intArray`.
    ///
    void set(core::SharedConstIntArray intArray) {
        emplace_(std::move(intArray));
    }

    /// Returns the double held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::Double`.
    ///
    double getDouble() const {
        return std::get<double>(var_);
    }

    /// Sets this `Value` to the given double `value`.
    ///
    void set(double value) {
        var_ = value;
    }

    /// Returns the `core::SharedConstDoubleArray` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::DoubleArray`.
    ///
    const core::SharedConstDoubleArray& getDoubleArray() const {
        return std::get<core::SharedConstDoubleArray>(var_);
    }

    /// Sets this `Value` to the given shared const `doubleArray`.
    ///
    void set(core::SharedConstDoubleArray doubleArray) {
        emplace_(std::move(doubleArray));
    }

    /// Returns the `core::Color` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::Color`.
    ///
    const core::Color& getColor() const {
        return std::get<core::Color>(var_);
    }

    /// Sets this `Value` to the given `color`.
    ///
    void set(const core::Color& color) {
        var_ = color;
    }

    /// Returns the `core::SharedConstColorArray` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::ColorArray`.
    ///
    const core::SharedConstColorArray& getColorArray() const {
        return std::get<core::SharedConstColorArray>(var_);
    }

    /// Sets this `Value` to the given shared const `colorArray`.
    ///
    void set(core::SharedConstColorArray colorArray) {
        emplace_(std::move(colorArray));
    }

    /// Returns the `geometry::Vec2d` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::Vec2d`.
    ///
    const geometry::Vec2d& getVec2d() const {
        return std::get<geometry::Vec2d>(var_);
    }

    /// Sets this `Value` to the given `vec2d`.
    ///
    void set(const geometry::Vec2d& vec2d) {
        var_ = vec2d;
    }

    /// Returns the `geometry::SharedConstVec2dArray` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::Vec2dArray`.
    ///
    const geometry::SharedConstVec2dArray& getVec2dArray() const {
        return std::get<geometry::SharedConstVec2dArray>(var_);
    }

    /// Sets this `Value` to the given shared const `vec2dArray`.
    ///
    void set(geometry::SharedConstVec2dArray vec2dArray) {
        emplace_(std::move(vec2dArray));
    }

    /// Returns the `dom::Path` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::Path`.
    ///
    const dom::Path& getPath() const {
        return std::get<dom::Path>(var_);
    }

    /// Sets this `Value` to the given `path`.
    ///
    void set(dom::Path path) {
        var_ = std::move(path);
    }

    /// Returns the `dom::SharedConstPathArray` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::PathArray`.
    ///
    const dom::SharedConstPathArray& getPathArray() const {
        return std::get<dom::SharedConstPathArray>(var_);
    }

    /// Sets this `Value` to the given shared const `pathArray`.
    ///
    void set(dom::SharedConstPathArray pathArray) {
        emplace_(std::move(pathArray));
    }

    template<typename Visitor>
    /*constexpr*/ // clang errors with "inline function is not defined".
    decltype(std::invoke(std::declval<Visitor>(), std::declval<NoneValue>()))
    visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), var_);
    }

    /// Note: For a held value of type `SharedConst<U>`, both `has<SharedConst<U>>()`
    /// and `has<U>()` return true.
    ///
    template<typename T>
    constexpr bool has() const {
        using Type = detail::ValueMakeSharedConstIfValid<std::decay_t<T>>;
        static_assert(detail::isValidValueType<Type>);
        return var_.index() == detail::ValueTypeTraitsFromType<Type>::index;
    }

    /// Note: For a held value of type `SharedConst<U>`, both `get<SharedConst<U>>()`
    /// and `get<U>()` are defined and return `const T&`.
    ///
    template<typename T>
    const T& get() const {
        using Type = detail::ValueMakeSharedConstIfValid<std::decay_t<T>>;
        static_assert(detail::isValidValueType<Type>);
        return std::get<Type>(var_);
    }

    friend constexpr bool operator==(const Value& a, const Value& b) noexcept {
        return a.var_ == b.var_;
    }

    friend constexpr bool operator!=(const Value& a, const Value& b) noexcept {
        return a.var_ != b.var_;
    }

    friend constexpr bool operator<(const Value& a, const Value& b) noexcept {
        return a.var_ < b.var_;
    }

    //friend constexpr bool operator>(const Value& a, const Value& b) noexcept {
    //    return a.var_ > b.var_;
    //}
    //
    //friend constexpr bool operator<=(const Value& a, const Value& b) noexcept {
    //    return a.var_ <= b.var_;
    //}
    //
    //friend constexpr bool operator>=(const Value& a, const Value& b) noexcept {
    //    return a.var_ >= b.var_;
    //}

    template<typename T>
    friend constexpr bool operator==(const Value& a, const T& b) noexcept {
        using Type = detail::ValueMakeSharedConstIfValid<std::decay_t<T>>;
        return a.has<Type>() && a.get<Type>() == b;
    }

    template<typename T>
    friend constexpr bool operator==(const T& a, const Value& b) noexcept {
        using Type = detail::ValueMakeSharedConstIfValid<std::decay_t<T>>;
        return b.has<Type>() && a == b.get<Type>();
    }

    template<typename T>
    friend constexpr bool operator!=(const Value& a, const T& b) noexcept {
        return !(a == b);
    }

    template<typename T>
    friend constexpr bool operator!=(const T& a, const Value& b) noexcept {
        return !(a == b);
    }

private:
    explicit constexpr Value(InvalidValue x)
        : var_(x) {
    }

    template<typename T>
    void emplace_(T&& value) {
        using Type = detail::ValueMakeSharedConstIfValid<std::decay_t<T>>;
        static_assert(detail::isValidValueType<Type>);
        var_.emplace<Type>(std::forward<T>(value));
    }

    detail::ValueVariant var_;
};

/// Writes the given Value to the output stream.
///
template<typename OStream>
void write(OStream& out, const Value& v) {
    v.visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, NoneValue>) {
            write(out, "None");
        }
        else if constexpr (std::is_same_v<T, InvalidValue>) {
            write(out, "Invalid");
        }
        else {
            write(out, arg);
        }
    });
}

/// Converts the given string into a Value. Raises vgc::dom::VgcSyntaxError if
/// the given string does not represent a `Value` of the given ValueType.
///
VGC_DOM_API
Value parseValue(const std::string& s, ValueType t);

} // namespace vgc::dom

template<>
struct fmt::formatter<vgc::dom::NoneValue> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::dom::NoneValue&, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format("None", ctx);
    }
};

template<>
struct fmt::formatter<vgc::dom::InvalidValue> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::dom::InvalidValue&, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format("Invalid", ctx);
    }
};

template<>
struct fmt::formatter<vgc::dom::Value> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::dom::Value& v, FormatContext& ctx) -> decltype(ctx.out()) {
        return v.visit([&](auto&& arg) {
            return fmt::format_to(ctx.out(), "{}", std::forward<decltype(arg)>(arg));
        });
    }
};

#endif // VGC_DOM_VALUE_H
