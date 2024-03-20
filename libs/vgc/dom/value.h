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

#include <string>
#include <type_traits> // decay, is_same

#include <vgc/core/array.h>
#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>
#include <vgc/core/parse.h>
#include <vgc/core/typeid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/path.h>

namespace vgc::dom {

struct NoneValue : std::monostate {};
struct InvalidValue : std::monostate {};

template<typename IStream>
void readTo(NoneValue&, IStream& in) {
    core::readExpectedWord(in, "none");
}

template<typename IStream>
void readTo(InvalidValue&, IStream& in) {
    core::readExpectedWord(in, "invalid");
}

template<typename OStream>
void write(OStream& out, const NoneValue&) {
    core::write(out, "none");
}

template<typename OStream>
void write(OStream& out, const InvalidValue&) {
    core::write(out, "invalid");
}

} // namespace vgc::dom

template<>
struct fmt::formatter<vgc::dom::NoneValue> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::dom::NoneValue&, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format("none", ctx);
    }
};

template<>
struct fmt::formatter<vgc::dom::InvalidValue> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::dom::InvalidValue&, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format("invalid", ctx);
    }
};

namespace vgc::dom {

class Node;
class Value;

using StreamReader = core::StringReader;
using StreamWriter = core::StringWriter;

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

    bool hasPaths;

    void (*copy)(const ValueData&, ValueData&);
    void (*move)(ValueData&, ValueData&);
    void (*destroy)(ValueData&);

    bool (*equal)(const ValueData&, const ValueData&);
    bool (*less)(const ValueData&, const ValueData&);

    Value (*getArrayItemWrapped)(const ValueData&, Int);

    void (*read)(ValueData&, StreamReader&);
    void (*write)(const ValueData&, StreamWriter&);

    FormatterBufferIterator (*format)(const ValueData&, FormatterBufferCtx&);

    void (*preparePathsForUpdate)(const ValueData&, const Node*);
    void (*updatePaths)(ValueData&, const Node*, const PathUpdateData&);

    template<typename T>
    static ValueTypeInfo create() {
        ValueTypeInfo info(core::typeId<T>());
        info.hasPaths = dom::hasPaths<T>;
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
    static void preparePathsForUpdate_(const ValueData& d, const Node* owner) {
        if constexpr (dom::hasPaths<T>) {
            PathTraits<T>::preparePathsForUpdate(d.get<T>(), owner);
        }
    }

    template<typename T>
    static void
    updatePaths_(ValueData& d, const Node* owner, const PathUpdateData& data) {
        if constexpr (dom::hasPaths<T>) {
            PathTraits<T>::updatePaths(d.get<T>(), owner, data);
        }
    }
};

// Inserts `info` in a registry of all known `ValueTypeInfo`, unless it was
// already registered. Returns a pointer to the `ValueTypeInfo` stored in the
// registry.
//
// This mechanism ensures uniqueness of the `valueTypeInfo<T>()` address even
// across shared library boundary, which makes it possible to have a very
// fast implementation of Value::has<T>().
//
// Indeed, on Windows, each DLL gets their own copy of static local variables
// that are defined in template functions, so a registry shared across DLL is
// necessary to achieve such uniqueness goal.
//
VGC_DOM_API
const ValueTypeInfo* registerValueTypeInfo(const ValueTypeInfo* info);

template<typename T>
const ValueTypeInfo* registerValueTypeInfo() {
    static const ValueTypeInfo perDllInfo = ValueTypeInfo::create<T>();
    return registerValueTypeInfo(&perDllInfo);
}

template<typename T>
const ValueTypeInfo* valueTypeInfo() {
    static const ValueTypeInfo* info = registerValueTypeInfo<T>();
    return info;
}

} // namespace detail

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
        : typeInfo_(other.typeInfo_) {

        info_().copy(other.data_, data_);
    }

    Value(Value&& other) noexcept
        : typeInfo_(other.typeInfo_)
        , data_(std::move(other.data_)) {
    }

    // This constructor is intentionally implicit
    template<typename T, VGC_REQUIRES(detail::isValueType<T>)>
    Value(T&& x)
        : typeInfo_(detail::valueTypeInfo<std::decay_t<T>>())
        , data_(std::forward<T>(x)) {
    }

    ~Value() {
        info_().destroy(data_);
    }

    Value& operator=(const Value& other) {
        if (&other != this) {
            info_().destroy(data_);
            typeInfo_ = other.typeInfo_;
            info_().copy(other.data_, data_);
        }
        return *this;
    }

    Value& operator=(Value&& other) noexcept {
        if (&other != this) {
            info_().destroy(data_);
            typeInfo_ = other.typeInfo_;
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

    /// Returns the `TypeId` of the held value.
    ///
    core::TypeId typeId() const {
        return info_().typeId;
    }

    /// Returns whether the held value is of type `T`.
    ///
    template<typename T>
    bool has() const {
        return typeInfo_ == detail::valueTypeInfo<T>();

        // Note: using std::decay_t<T> would be incorrect here, since this is
        // used to know whether we can static_cast<T*>() the data.
    }

    /// Returns the held value as a `const T*` if the held value is of type `T`. Otherwise, return `nullptr`
    ///
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
        return lhs.typeInfo_ == rhs.typeInfo_ //
               && lhs.info_().equal(lhs.data_, rhs.data_);
    }

    friend bool operator!=(const Value& lhs, const Value& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const Value& lhs, const Value& rhs) {
        return lhs.typeInfo_ == rhs.typeInfo_ //
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
    const detail::ValueTypeInfo* typeInfo_;
    detail::ValueData data_;

    const detail::ValueTypeInfo& info_() const {
        return *typeInfo_;
    }

    friend Element;

    void preparePathsForUpdate_(const Node* owner) const {
        info_().preparePathsForUpdate(data_, owner);
    }

    void updatePaths_(const Node* owner, const PathUpdateData& data) {
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

} // namespace vgc::dom

template<>
struct fmt::formatter<vgc::dom::Value> : fmt::formatter<double> {
    template<typename FormatContext>
    auto format(const vgc::dom::Value& v, FormatContext& ctx) -> decltype(ctx.out()) {
        return v.format(ctx);
    }
};

#endif // VGC_DOM_VALUE_H
