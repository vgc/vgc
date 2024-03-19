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
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/enum.h>
#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>
#include <vgc/core/parse.h>
#include <vgc/core/sharedconst.h>
#include <vgc/core/stringid.h>
#include <vgc/core/typeid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/path.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Element);
class Value;

using StreamReader = core::StringReader;
using StreamWriter = core::StringWriter;

/*
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
    NoneOrPath,
    PathArray,
    Custom,
    VGC_ENUM_ENDMAX
};
VGC_DOM_API
VGC_DECLARE_ENUM(ValueType)

*/

struct NoneValue : std::monostate {};
struct InvalidValue : std::monostate {};

/// \class vgc::dom::NoneOr
/// \brief Used to extend a type to support "none".
///
template<typename T>
class NoneOr : public std::optional<T> {
private:
    using Base = std::optional<T>;

public:
    using Base::Base;

    template<typename U = T>
    constexpr explicit NoneOr(U&& value)
        : Base(std::forward<U>(value)) {
    }

    using A = NoneOr;

    template<typename U>
    NoneOr& operator=(U&& x) {
        Base::operator=(std::forward<U>(x));
        return *this;
    }

    template<class U>
    friend constexpr bool operator==(const NoneOr<U>& lhs, const NoneOr<U>& rhs) {
        return operator==(
            static_cast<const std::optional<U>&>(lhs),
            static_cast<const std::optional<U>&>(rhs));
    }

    template<class U>
    friend constexpr bool operator!=(const NoneOr<U>& lhs, const NoneOr<U>& rhs) {
        return operator!=(
            static_cast<const std::optional<U>&>(lhs),
            static_cast<const std::optional<U>&>(rhs));
    }

    template<class U>
    friend constexpr bool operator<(const NoneOr<U>& lhs, const NoneOr<U>& rhs) {
        return operator<(
            static_cast<const std::optional<U>&>(lhs),
            static_cast<const std::optional<U>&>(rhs));
    }
};

class Value;

using FormatterBufferCtx = fmt::buffer_context<char>;
using FormatterBufferIterator = decltype(std::declval<FormatterBufferCtx>().out());

class PathUpdateData {
public:
    PathUpdateData() = default;

    const std::unordered_map<core::Id, core::Id>& copiedElements() const {
        return copiedElements_;
    }

    void addCopiedElement(core::Id oldInternalId, core::Id newInternalId) {
        copiedElements_[oldInternalId] = newInternalId;
    }

    const core::Array<core::Id>& absolutePathChangedElements() const {
        return absolutePathChangedElements_;
    }

    void addAbsolutePathChangedElement(core::Id internalId) {
        if (!absolutePathChangedElements_.contains(internalId)) {
            absolutePathChangedElements_.append(internalId);
        }
    }

private:
    std::unordered_map<core::Id, core::Id> copiedElements_;
    core::Array<core::Id> absolutePathChangedElements_;
};

template<typename T>
struct ValueTraits {
    static constexpr bool hasPaths = false;
};

// This should be specialized by custom types internally storing
// `vgc::dom::Path` data so they can update their in case an
// referenced element is moved or its ID changes.
//
// ```cpp
// namespace my::lib {
//
// class Foo {
//     ...
// };
//
// template<>
// constexpr bool hasVgcDomPaths<Foo>() {
//     return true;
// }
//
// } // namespace my::lib
// ```
//
// Note: we do not use a type trait for this so that we can rely on
// Argument-Dependent Lookup, instead of forcing people to specialize the trait
// in the vgc::dom namespace.
//
template<typename T>
constexpr bool hasVgcDomPaths(const T*) {
    return false;
}

#define VGC_DOM_DECLARE_HAS_PATHS(T)                                                     \
    template<>                                                                           \
    constexpr bool hasVgcDomPaths<T>(const T*) {                                         \
        return true;                                                                     \
    }

namespace detail {

class ValueData {
public:
    ValueData() = default;
    ~ValueData() = default;

    ValueData(ValueData&& other) {
        data_ = other.data_;
        other.data_ = nullptr;
    }

    ValueData& operator=(ValueData&& other) {
        if (this != &other) {
            data_ = other.data_;
            other.data_ = nullptr;
        }
        return *this;
    }

    // Owner must use the appropriate function pointers to copy
    ValueData(const ValueData&) = delete;
    ValueData& operator=(const ValueData&) = delete;

    template<typename T>
    ValueData(T&& x)
        : data_(static_cast<void*>(new std::decay_t<T>(std::forward<T>(x)))) {
    }

    template<typename T>
    static ValueData create(T&& x) {
        return ValueData(std::move(x));
    }

    template<typename T>
    static ValueData create(const T& x) {
        return ValueData(x);
    }

    template<typename T>
    void destroy() {
        delete static_cast<T*>(data_);
        data_ = nullptr;
    }

    template<typename T>
    T& get() {
        return *static_cast<T*>(data_);
    }

    template<typename T>
    const T& get() const {
        return *static_cast<const T*>(data_);
    }

private:
    // For now we don't do Small-Value Optimization: values are always dynamically
    // allocated. If in the future we want SVO, we could do that by modifying
    // ValueData.
    //
    void* data_ = nullptr;
};

// Stores meta data and function pointers to operate on type-erased data.
// This is essentially a hand-written virtual table.
//
class VGC_DOM_API ValueTypeInfo {
private:
    // This is needed because core::TypeId is not default-constructible
    ValueTypeInfo(core::TypeId typeId)
        : typeId(typeId) {
    }

public:
    core::TypeId typeId;
    core::StringId name;
    bool hasPaths;

    void (*copy)(const ValueData&, ValueData&);
    void (*move)(ValueData&, ValueData&);
    void (*destroy)(ValueData&);

    bool (*equal)(const ValueData&, const ValueData&);
    bool (*less)(const ValueData&, const ValueData&);

    Value (*getArrayItemWrapped)(const ValueData&, Int);

    void (*read)(ValueData&, StreamReader& in);
    void (*write)(const ValueData&, StreamWriter& out);

    FormatterBufferIterator (*format)(const ValueData&, FormatterBufferCtx& ctx);

    void (*preparePathsForUpdate)(const ValueData&, const Element* owner);
    void (*updatePaths)(const ValueData&, const Element* owner, const PathUpdateData&);

    template<typename T>
    static ValueTypeInfo create() {
        ValueTypeInfo info(core::typeId<T>());
        info.name = core::StringId(typeid(T).name());
        info.hasPaths = vgc::dom::ValueTraits<T>::hasPaths;
        info.copy = &copy_<T>;
        info.move = &move_<T>;
        info.destroy = &destroy_<T>;
        info.equal = &equal_<T>;
        info.less = &less_<T>;
        info.getArrayItemWrapped = &getArrayItemWrapped_<T>;
        info.read = &read_<T>;
        info.write = &write_<T>;
        info.format = &format_<T>;
        info.preparePathsForUpdate = &preparePathsForUpdate_<T>;
        info.updatePaths = &updatePaths_<T>;
        return info;
    }

private:
    template<typename T>
    static void copy_(const ValueData& from, ValueData& to) {
        to = ValueData::create<T>(from.get<T>());
    }

    template<typename T>
    static void move_(ValueData& from, ValueData& to) {
        to = ValueData::create<T>(std::move(from.get<T>()));
    }

    template<typename T>
    static void destroy_(ValueData& d) {
        d.destroy<T>();
    }

    template<typename T>
    static bool equal_(const ValueData& d1, const ValueData& d2) {
        return d1.get<T>() == d2.get<T>();
    }

    template<typename T>
    static bool less_(const ValueData& d1, const ValueData& d2) {
        return d1.get<T>() < d2.get<T>();
    }

    // This function is defined further in this file due to cyclic dependency with Value
    template<typename T>
    static Value getArrayItemWrapped_(const ValueData& d, Int index);

    template<typename T>
    static void read_(ValueData& d, StreamReader& in) {
        using vgc::core::readTo;
        readTo(d.get<T>(), in);
    }

    template<typename T>
    static void write_(const ValueData& d, StreamWriter& out) {
        using vgc::core::write;
        write(out, d.get<T>());
    }

    template<typename T>
    static FormatterBufferIterator format_(const ValueData& d, FormatterBufferCtx& ctx) {
        return core::formatTo(ctx.out(), "{}", d.get<T>());
    }

    template<typename T>
    static void preparePathsForUpdate_(const ValueData&, const Element*) {
        // TODO: Use Traits
    }

    template<typename T>
    static void updatePaths_(const ValueData&, const Element*, const PathUpdateData&) {
        // TODO: Use Traits
    }
};

// This is just a wrapper for `ValueTypeInfo*` that reimplements the equality
// operator: it tests for equality of `info->name` rather than equality of
// the pointer address, because pointer equality may not be guaranteed across
// shared library boundaries.
//
class VGC_DOM_API ValueTypeId {
public:
    ValueTypeId(const ValueTypeInfo* info)
        : info_(info) {
    }

    const ValueTypeInfo& info() const {
        return *info_;
    }

    bool operator==(const ValueTypeId& other) const {
        return info_->name == other.info_->name;
    }

    bool operator!=(const ValueTypeId& other) const {
        return !(*this == other);
    }

private:
    const ValueTypeInfo* info_;
};

template<typename T>
ValueTypeId valueTypeId() {
    // Each DLL may have its own &info address, but their info.name is equal
    static const ValueTypeInfo info = ValueTypeInfo::create<T>();
    return ValueTypeId(&info);
}

} // namespace detail

/*
class VGC_DOM_API AbstractValue {
public:
    virtual ~AbstractValue();

    virtual std::unique_ptr<AbstractValue> copy() const = 0;
    virtual std::unique_ptr<AbstractValue> move() const = 0;

    virtual bool compareEqual(const AbstractValue& rhs) const = 0;
    virtual bool compareLess(const AbstractValue& rhs) const = 0;

    virtual void read(StreamReader& in) = 0;
    virtual void write(StreamWriter& out) const = 0;

    virtual FormatterBufferIterator format(FormatterBufferCtx& ctx) const = 0;

    virtual void preparePathsForUpdate(const Element* owner) const = 0;
    virtual void updatePaths(const Element* owner, const PathUpdateData& data) = 0;
};

// XXX Use CRTP?

template<typename T>
class ArithmeticValue : public AbstractValue {
public:
    virtual ~ArithmeticValue() = default;

    explicit ArithmeticValue(const T& value) noexcept
        : value_(value) {
    }

    explicit ArithmeticValue(T&& value) noexcept
        : value_(std::move(value)) {
    }

    const T& value() const {
        return value_;
    }

    std::unique_ptr<AbstractValue> copy() const override {
        return std::make_unique<ArithmeticValue>(value_);
    }

    std::unique_ptr<AbstractValue> move() const override {
        return std::make_unique<ArithmeticValue>(std::move(value_));
    }

    bool compareEqual(const AbstractValue& rhs) const override {
        const auto& derived = static_cast<const ArithmeticValue&>(rhs);
        return value_ == derived.value_;
    }

    bool compareLess(const AbstractValue& rhs) const override {
        const auto& derived = static_cast<const ArithmeticValue&>(rhs);
        return value_ < derived.value_;
    }

    void read(StreamReader& in) override {
        using vgc::core::readTo;
        readTo(value_, in);
    }

    void write(StreamWriter& out) const override {
        using vgc::core::write;
        write(out, value_);
    }

    FormatterBufferIterator format(FormatterBufferCtx& ctx) const override {
        return core::formatTo(ctx.out(), "{}", value_);
    }

    void preparePathsForUpdate(const Element*) const override {
    }

    void updatePaths(const Element*, const PathUpdateData&) override {
    }

private:
    T value_;
};
*/

namespace detail {

template<typename T>
inline constexpr bool isValueType = !std::is_same_v<std::decay_t<T>, Value>;

} // namespace detail

class VGC_DOM_API Value {
public:
    // TODO: make this noexcept and allocation free: should just keep
    // the pointers as nullptr
    Value()
        : Value(NoneValue{}) {
    }

    /// Returns a const reference to an empty value. This is useful for instance
    /// for optional values or to simply express non-initialized or null.
    ///
    static const Value& none();

    /// Returns a const reference to an invalid value. This is useful for error
    /// handling in methods that must return a `Value` by const reference.
    ///
    static const Value& invalid();

    Value(const Value& other)
        : typeId_(other.typeId_) {

        info_().copy(other.data_, data_);
    }

    Value(Value&& other) noexcept
        : typeId_(other.typeId_)
        , data_(std::move(other.data_)) {
    }

    // This constructor is intentionally implicit
    template<typename T, VGC_REQUIRES(detail::isValueType<T>)>
    Value(T&& x)
        : typeId_(detail::valueTypeId<std::decay_t<T>>())
        , data_(std::forward<T>(x)) {
    }

    ~Value() {
        info_().destroy(data_);
    }

    core::TypeId type() const {
        return info_().typeId;
    }

    Value& operator=(const Value& other) {
        if (&other != this) {
            info_().destroy(data_);
            typeId_ = other.typeId_;
            info_().copy(other.data_, data_);
        }
        return *this;
    }

    Value& operator=(Value&& other) noexcept {
        if (&other != this) {
            info_().destroy(data_);
            typeId_ = other.typeId_;
            data_ = std::move(other.data_);
        }
        return *this;
    }

    // TODO:
    // template<typename T, VGC_REQUIRES(detail::isValueType<T>)>
    // Value& operator=(T&& other) noexcept;
    //
    // Currently, `value = Vec2d(1, 2)` works through the implicit:
    // 1. Value(Vec2d&&)
    // 2. Value::operator=(Value&&)
    //
    // But by implementing operator=(T&&), we can probably avoid allocations
    // when the current type of the Value matches the target type.

    template<typename T>
    bool has() const {
        return typeId_ == detail::valueTypeId<T>();

        // Note: using std::decay_t<T> would be incorrect here, since this is
        // used to know whether we can static_cast<T*>() the data.
    }

    template<typename T>
    const T* getIf() const {
        if (has<T>()) {
            return &data_.get<T>();
        }
        else {
            return nullptr;
        }
    }

    template<typename T>
    const T& get() const {
        if (has<T>()) {
            return data_.get<T>();
        }
        else {
            throw core::LogicError("Bad vgc::dom::Value cast.");
        }
    }

    template<typename T>
    const T& getUnchecked() const {
        return data_.get<T>();
    }

    bool isNone() const { // XXX: Rename to isEmpty()?
        return has<NoneValue>();
    }

    bool isValid() const {
        return !has<InvalidValue>();
    }

    bool hasValue() const {
        return isValid() && !isNone();
    }

    Value getArrayItemWrapped(Int index) const {
        return info_().getArrayItemWrapped(data_, index);
    }

    friend bool operator==(const Value& lhs, const Value& rhs) {
        return lhs.typeId_ == rhs.typeId_ //
               && lhs.info_().equal(lhs.data_, rhs.data_);
    }

    friend bool operator!=(const Value& lhs, const Value& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const Value& lhs, const Value& rhs) {
        return lhs.typeId_ == rhs.typeId_ //
               && lhs.info_().less(lhs.data_, rhs.data_);
    }

    // Reads as a T, where T is the type currently held by this Value.
    //
    // TODO: read<T> variant?
    //
    void read(StreamReader& in) {
        info_().read(data_, in);
    }

    // Parses as a T, where T is the type currently held by this Value.
    //
    // TODO: parse<T> variant?
    //
    void parse(const std::string& s) {
        StreamReader sr(s);
        read(sr);
    }

    void write(StreamWriter& out) const {
        info_().write(data_, out);
    }

    FormatterBufferIterator format(FormatterBufferCtx& ctx) const {
        return info_().format(data_, ctx);
    }

private:
    detail::ValueTypeId typeId_;
    detail::ValueData data_;

    const detail::ValueTypeInfo& info_() const {
        return typeId_.info();
    }

    friend Element;

    void preparePathsForUpdate_(const Element* owner) const {
        info_().preparePathsForUpdate(data_, owner);
    }

    void updatePaths_(const Element* owner, const PathUpdateData& data) {
        info_().updatePaths(data_, owner, data);
    }
};

namespace detail {

template<typename T>
Value ValueTypeInfo::getArrayItemWrapped_(const ValueData& d, Int index) {
    using T_ = core::RemoveSharedConst<T>;
    if constexpr (core::isArray<T_>) {
        const T_& x = d.get<T>(); // c.f. `SharedConst<T>::operator const T&()`
        return Value(x.getWrapped(index));
    }
    else {
        return Value();
    }
}

} // namespace detail

/*
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
VGC_DOM_VALUETYPE_TYPE_(Path,          Path)
VGC_DOM_VALUETYPE_TYPE_(NoneOrPath,    NoneOr<Path>)
VGC_DOM_VALUETYPE_TYPE_(PathArray,     PathArray)
VGC_DOM_VALUETYPE_TYPE_(Custom,        CustomValueHolder)
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
struct IsValueVariantAlternative : std::false_type {};

template<typename T>
struct IsValueVariantAlternative<
    T,
    core::RequiresValid<typename ValueTypeTraitsFromType<T>::Type>> : std::true_type {};

template<typename T>
inline constexpr bool isValueVariantAlternative = IsValueVariantAlternative<T>::value;

} // namespace detail

*/

/*
template<typename T>
inline constexpr bool isValueConstructibleFrom =
    (detail::isValueVariantAlternative<T>                       //
     || detail::isValueVariantAlternative<core::SharedConst<T>> //
     || std::is_base_of_v<CustomValue, T>);
*/

template<typename T>
inline constexpr bool isValueConstructibleFrom = true;

/*
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
    Value(const std::string& string)
        : var_(string) {
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

    /// Constructs a `Value` holding a `Path`.
    ///
    Value(Path path)
        : var_(path) {
    }

    /// Constructs a `Value` holding a `NoneOr<Path>`.
    ///
    Value(NoneOr<Path> noneOrPath)
        : var_(std::move(noneOrPath)) {
    }

    /// Constructs a `Value` holding an array of `Path`.
    ///
    Value(PathArray pathArray)
        : var_(std::move(pathArray)) {
    }

    /// Constructs a `Value` holding a clone of the given `customValue`.
    ///
    Value(const CustomValue& customValue) {
        var_ = detail::CustomValueHolder(customValue.clone());
    }

    /// Constructs a `Value` holding a moved clone of the given `customValue`.
    ///
    Value(const CustomValue&& customValue) {
        var_ = detail::CustomValueHolder(customValue.clone(true));
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

    /// Sets this `Value` to the given `intArray`.
    ///
    void set(core::IntArray intArray) {
        emplace_(core::SharedConstIntArray(std::move(intArray)));
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

    /// Sets this `Value` to the given `doubleArray`.
    ///
    void set(core::DoubleArray doubleArray) {
        emplace_(core::SharedConstDoubleArray(std::move(doubleArray)));
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

    /// Sets this `Value` to the given `colorArray`.
    ///
    void set(core::ColorArray colorArray) {
        emplace_(core::SharedConstColorArray(std::move(colorArray)));
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

    /// Sets this `Value` to the given `vec2dArray`.
    ///
    void set(geometry::Vec2dArray vec2dArray) {
        emplace_(geometry::SharedConstVec2dArray(std::move(vec2dArray)));
    }

    /// Sets this `Value` to the given shared const `vec2dArray`.
    ///
    void set(geometry::SharedConstVec2dArray vec2dArray) {
        emplace_(std::move(vec2dArray));
    }

    /// Returns the `Path` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::Path`.
    ///
    const Path& getPath() const {
        return std::get<Path>(var_);
    }

    /// Sets this `Value` to the given `path`.
    ///
    void set(Path path) {
        var_ = std::move(path);
    }

    /// Returns the `NoneOr<Path>` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::NoneOrPath`.
    ///
    const NoneOr<Path>& getNoneOrPath() const {
        return std::get<NoneOr<Path>>(var_);
    }

    /// Sets this `Value` to the given `path`.
    ///
    void set(NoneOr<Path> noneOrPath) {
        var_ = std::move(noneOrPath);
    }

    /// Returns the `SharedConstPathArray` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::PathArray`.
    ///
    const PathArray& getPathArray() const {
        return std::get<PathArray>(var_);
    }

    /// Sets this `Value` to the given shared const `pathArray`.
    ///
    void set(PathArray pathArray) {
        emplace_(std::move(pathArray));
    }

    /// Returns the `CustomValue*` held by this `Value`.
    /// The behavior is undefined if `type() != ValueType::Custom`.
    ///
    const CustomValue* getCustomValuePtr() const {
        return std::get<detail::CustomValueHolder>(var_).get();
    }

    /// Sets this `Value` to a clone of the given `customValue`.
    ///
    void set(const CustomValue& customValue) {
        emplace_(detail::CustomValueHolder(customValue.clone()));
    }

    /// Sets this `Value` to a moved clone of the given `customValue`.
    ///
    void set(const CustomValue&& customValue) {
        emplace_(detail::CustomValueHolder(customValue.clone(true)));
    }

    template<typename Visitor>
    // constexpr // clang errors with "inline function is not defined".
    decltype(std::invoke(std::declval<Visitor>(), std::declval<NoneValue>()))
    visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), var_);
    }

    /// Note: For a held value of type `SharedConst<U>`, both `has<SharedConst<U>>()`
    /// and `has<U>()` return true. And for a held value of type `CustomType*`,
    /// returns whether `dynamic_cast<T>(value) != nullptr`.
    ///
    template<typename T>
    constexpr bool has() const {
        using U = std::decay_t<T>;
        if constexpr (std::is_base_of_v<CustomValue, U>) {
            if (type() == ValueType::Custom) {
                return dynamic_cast<T>(std::get<detail::CustomValueHolder>(var_))
                       != nullptr;
            }
            return false;
        }
        else if constexpr (detail::isValueVariantAlternative<core::SharedConst<U>>) {
            return var_.index()
                   == detail::ValueTypeTraitsFromType<core::SharedConst<U>>::index;
        }
        else {
            static_assert(detail::isValueVariantAlternative<U>);
            return var_.index() == detail::ValueTypeTraitsFromType<U>::index;
        }
    }

    /// Note: For a held value of type `SharedConst<U>`, both `get<SharedConst<U>>()`
    /// and `get<U>()` are defined and return `const U&`.
    ///
    template<typename T>
    const T& get() const {
        using U = std::decay_t<T>;
        if constexpr (detail::isValueVariantAlternative<core::SharedConst<U>>) {
            return std::get<core::SharedConst<U>>(var_);
        }
        else if constexpr (std::is_base_of_v<CustomValue, U>) {
            if (type() == ValueType::Custom) {
                // assumes p is never null
                CustomValue* p = std::get<detail::CustomValueHolder>(var_).get();
                T* casted = dynamic_cast<T*>(p);
                if (casted == nullptr) {
                    throw std::bad_variant_access();
                }
                return *casted;
            }
            throw std::bad_variant_access();
        }
        else {
            static_assert(detail::isValueVariantAlternative<U>);
            return std::get<U>(var_);
        }
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
        using U = std::decay_t<T>;
        if constexpr (detail::isValueVariantAlternative<U>) {
            if (a.has<U>()) {
                return a.get<U>() == b;
            }
        }
        else if constexpr (detail::isValueVariantAlternative<core::SharedConst<U>>) {
            if (a.has<core::SharedConst<U>>()) {
                return a.get<core::SharedConst<U>>() == b;
            }
        }
        else if constexpr (std::is_base_of_v<CustomValue, U>) {
            if (a.type() == ValueType::Custom) {
                // assumes p is never null
                const CustomValue* p = a.getCustomValuePtr();
                // TODO: what about b == p ?
                return p->compareEqual(&b);
            }
        }
        return false;
    }

    template<typename T>
    friend constexpr bool operator==(const T& a, const Value& b) noexcept {
        return (b == a);
    }

    template<typename T>
    friend constexpr bool operator!=(const Value& a, const T& b) noexcept {
        return !(a == b);
    }

    template<typename T>
    friend constexpr bool operator!=(const T& a, const Value& b) noexcept {
        return !(b == a);
    }

    private : constexpr explicit Value(InvalidValue x)
        : var_(x) {
    }

    template<typename T>
    void emplace_(T&& value) {
        using U = std::decay_t<T>;
        static_assert(detail::isValueVariantAlternative<U>);
        var_.emplace<U>(std::forward<T>(value));
    }

    friend Element;

    void preparePathsForUpdate_(const Element* owner) const;
    void updatePaths_(const Element* owner, const PathUpdateData& data);

    detail::ValueVariant var_;
};
*/

/// Writes the given Value to the output stream.
///
template<typename OStream>
void write(OStream& out, const Value& v) {
    if constexpr (std::is_same_v<OStream, StreamWriter>) {
        v.write(out);
    }
    else {
        std::string tmp;
        StreamWriter sw(tmp);
        v.write(sw);
        write(out, tmp);
    }
}

/// Converts the given string into a Value. Raises vgc::dom::VgcSyntaxError if
/// the given string does not represent a `Value` of the given ValueType.
///
//VGC_DOM_API
//void parseValue(Value& value, const std::string& s);

template<typename OStream, typename T>
void write(OStream& out, const NoneOr<T>& v) {
    if (v.has_value()) {
        write(out, v.value());
    }
    else {
        write(out, "none");
    }
}

template<typename IStream, typename T>
void readTo(NoneOr<T>& v, IStream& in) {
    // check for "none"
    std::string peek = "????";
    Int numGot = 0;
    for (size_t i = 0; i < peek.size(); ++i) {
        if (in.get(peek[i])) {
            ++numGot;
        }
        else {
            break;
        }
    }
    if (peek == "none") {
        char c = 0;
        if (in.get(c)) {
            ++numGot;
            if (core::isWhitespace(c)) {
                core::skipWhitespaceCharacters(in);
                v.reset();
                return;
            }
        }
        else {
            v.reset();
            return;
        }
    }

    for (Int i = 0; i < numGot; ++i) {
        in.unget();
    }

    readTo(v.emplace(), in);
}

template<typename IStream>
void readTo(NoneValue&, IStream& in) {
    static std::string s = "none";
    core::skipWhitespaceCharacters(in);
    core::skipExpectedString(in, s);
    core::expectsEofOrWhitespaceCharacter(in);
}

template<typename IStream>
void readTo(InvalidValue&, IStream& in) {
    static std::string s = "invalid";
    core::skipWhitespaceCharacters(in);
    core::skipExpectedString(in, s);
    core::expectsEofOrWhitespaceCharacter(in);
}

template<typename OStream>
void write(OStream& out, const NoneValue&) {
    core::write(out, "none");
}

template<typename OStream>
void write(OStream& out, const InvalidValue&) {
    core::write(out, "invalid");
}

/*
template<typename OStream>
void write(OStream& out, const detail::CustomValueHolder& v) {
}

VGC_DOM_API
void readTo(detail::CustomValueHolder& v, StreamReader& in);
*/

} // namespace vgc::dom

template<typename T>
struct fmt::formatter<vgc::dom::NoneOr<T>> : fmt::formatter<T> {
    template<typename FormatContext>
    auto format(const vgc::dom::NoneOr<T>& v, FormatContext& ctx) -> decltype(ctx.out()) {
        if (v.has_value()) {
            return fmt::formatter<T>::format(v.value(), ctx);
        }
        else {
            return fmt::format_to(ctx.out(), "none");
        }
    }
};

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
struct fmt::formatter<vgc::dom::Value> : fmt::formatter<double> {
    template<typename FormatContext>
    auto format(const vgc::dom::Value& v, FormatContext& ctx) -> decltype(ctx.out()) {
        return v.format(ctx);
    }
};

#endif // VGC_DOM_VALUE_H
