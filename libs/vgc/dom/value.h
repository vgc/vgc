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

#include <functional> // function
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
public:
    using FmtBufferContext = fmt::buffer_context<char>;
    using FmtBufferIterator = FmtBufferContext::iterator;

    core::TypeId typeId;
    bool hasPaths;
    void (*copy)(const ValueData&, ValueData&);
    void (*move)(ValueData&, ValueData&);
    void (*destroy)(ValueData&);
    bool (*equal)(const ValueData&, const ValueData&);
    bool (*less)(const ValueData&, const ValueData&);
    Value (*getArrayItemWrapped)(const ValueData&, Int);
    void (*write)(const ValueData&, core::StringWriter&);
    Value (*readAs)(core::StringReader&);
    FmtBufferIterator (*format)(const ValueData&, FmtBufferContext&);
    void (*visitPaths)(ValueData&, const std::function<void(Path&)>&);
    void (*visitPathsConst)(const ValueData&, const std::function<void(const Path&)>&);

    // Constructors can't have explicit template arguments,
    // so we use this instead for type deduction.
    //
    template<typename T>
    struct Tag {};

    template<typename T>
    ValueTypeInfo(Tag<T>)
        : typeId(                // Comma operator:
            (check_<T>(),        // - first do the static checks
             core::typeId<T>())) // - then get and assign the typeId
        , hasPaths(dom::hasPaths<T>)
        , copy(&copy_<T>)
        , move(&move_<T>)
        , destroy(&destroy_<T>)
        , equal(&equal_<T>)
        , less(&less_<T>)
        , getArrayItemWrapped(&getArrayItemWrapped_<T>)
        , write(&write_<T>)
        , readAs(&readAs_<T>)
        , format(&format_<T>)
        , visitPaths(&visitPaths_<T>)
        , visitPathsConst(&visitPathsConst_<T>) {
    }

private:
    // Checks that T can be held as a Value. The intent is to provide nice
    // error messages rather than gnarly template instanciation errors.
    //
    template<typename T>
    static void check_() {

        constexpr bool defaultConstructible = std::is_default_constructible_v<T>;
        static_assert(
            defaultConstructible,
            "vgc::dom::Value cannot hold type T because it is not "
            "default-constructible.");

        constexpr bool copyConstructible = std::is_copy_constructible_v<T>;
        static_assert(
            copyConstructible,
            "vgc::dom::Value cannot hold type T because it is not "
            "copy-constructible.");
    }

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
    static void write_(const ValueData& d, core::StringWriter& out) {
        using vgc::core::write;
        write(out, d.get<T>());
    }

    // This function is defined further in this file due to cyclic dependency with Value
    template<typename T>
    static Value readAs_(core::StringReader& in);

    template<typename T>
    static FmtBufferIterator format_(const ValueData& d, FmtBufferContext& ctx) {
        return core::formatTo(ctx.out(), "{}", d.get<T>());
    }

    template<typename T>
    static void visitPaths_(ValueData& d, const std::function<void(Path&)>& fn) {
        if constexpr (dom::hasPaths<T>) {
            PathVisitor<T>::visit(d.get<T>(), fn);
        }
    }

    template<typename T>
    static void
    visitPathsConst_(const ValueData& d, const std::function<void(const Path&)>& fn) {
        if constexpr (dom::hasPaths<T>) {
            PathVisitor<T>::visit(d.get<T>(), fn);
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
    static const ValueTypeInfo perDllInfo = ValueTypeInfo::Tag<T>();
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
inline constexpr bool isNotValue = !std::is_same_v<std::decay_t<T>, Value>;

} // namespace detail

class VGC_DOM_API Value {
public:
    // TODO: make this noexcept and allocation free: should just keep
    // the pointers as nullptr
    Value()
        : Value(NoneValue{}) {
    }

    // This constructor is intentionally implicit
    template<typename T, VGC_REQUIRES(detail::isNotValue<T>)>
    Value(T&& x)
        : typeInfo_(detail::valueTypeInfo<std::decay_t<T>>())
        , data_(std::forward<T>(x)) {
    }

    Value(const Value& other)
        : typeInfo_(other.typeInfo_) {

        info_().copy(other.data_, data_);
    }

    Value(Value&& other) noexcept
        : typeInfo_(other.typeInfo_)
        , data_(std::move(other.data_)) {
    }

    Value& operator=(const Value& other) {
        if (&other != this) {
            info_().destroy(data_); // TODO: what if this Value owns the other value?
                // It would be better to keep-alive until the end of this function.
            typeInfo_ = other.typeInfo_;
            info_().copy(other.data_, data_);
        }
        return *this;
    }

    Value& operator=(Value&& other) noexcept {
        if (&other != this) {
            info_().destroy(data_); // Same comment as above
            typeInfo_ = other.typeInfo_;
            data_ = std::move(other.data_);
        }
        return *this;
    }

    ~Value() {
        info_().destroy(data_);
    }

    // TODO:
    // template<typename T, VGC_REQUIRES(detail::isNotValue<T>)>
    // Value& operator=(T&& other) noexcept;
    //
    // Currently, `value = Vec2d(1, 2)` works through the implicit:
    // 1. Value(Vec2d&&)
    // 2. Value::operator=(Value&&)
    //
    // But by implementing operator=(T&&), we can probably avoid allocations
    // when the current type of the Value matches the target type.
    //
    // Same comment as other operator=:
    // Beware of the case where we do for example:
    //
    // value = value.get<Vec2dArray>()[12];
    //
    // The `value` needs to keep the Vec2dArray alive long enough
    // until until it a copy of the Vec2d is stored in `value`.
    //

    /// Returns a const reference to an empty value. This is useful for instance
    /// for optional values or to simply express non-initialized or null.
    ///
    static const Value& none();

    /// Returns a const reference to an invalid value. This is useful for error
    /// handling in methods that must return a `Value` by const reference.
    ///
    static const Value& invalid();

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

    void clear() {
        if (!isNone()) {
            *this = NoneValue{};
        }
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

    // TODO: getArrayItem (non-wrapped), arrayLength, etc.

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

    template<typename OStream>
    friend void write(OStream& out, const Value& self) {
        if constexpr (std::is_same_v<OStream, core::StringWriter>) {
            self.info_().write(self.data_, out);
        }
        else {
            std::string s;
            core::StringWriter sw(s);
            self.info_().write(self.data_, sw);
            write(out, s);
        }
    }

    /// Reads from the input stream a value of type `T`, where `T` is the type
    /// currently held by `other`, and returns it as a `Value`.
    ///
    /// Note: the `Value` type intentionally does not implement the usual
    /// `void readTo(Value& v, IStream&)` interface, because:
    ///
    /// 1. Unless we use the type currently held by `v`, we would not know
    /// which type `T` to read from the input stream.
    ///
    /// 2. If we use the type currently held by `v` as type to read, then this
    /// means that the result of `readTo(v, in)` depends on the current state
    /// of `v`, which is unexcepted.
    ///
    /// 3. If we implement `readTo(v, in)` as above, then this means that both
    /// `core::read<Value>(in)` and `core::parse<Value>(in)` would also be
    /// available, but they would always attempt to read a `NoneValue`, which
    /// is unexpected. Indeed, they are basically implemented as `{ T x;
    /// readTo(x, in); return x; }`, and a default-constructed `Value` holds
    /// the type `NoneValue`.
    ///
    // TODO: Allow reading from an arbitrary IStream (instead of just a
    // StringReader) by automatically wrapping the IStream in a virtual class.
    // This would be essentially similar to visitPaths taking an std::function.
    //
    static Value readAs(const Value& other, core::StringReader& in) {
        return other.info_().readAs(in);
    }

    void visitPaths(const std::function<void(const Path&)>& fn) const {
        return info_().visitPathsConst(data_, fn);
    }

    void visitPaths(const std::function<void(Path&)>& fn) {
        return info_().visitPaths(data_, fn);
    }

private:
    const detail::ValueTypeInfo* typeInfo_;
    detail::ValueData data_;

    const detail::ValueTypeInfo& info_() const {
        return *typeInfo_;
    }

    friend ::fmt::formatter<Value>;
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
template<typename T>
Value ValueTypeInfo::readAs_(core::StringReader& in) {
    using vgc::core::readTo;
    T x;
    readTo(x, in);
    return Value(std::move(x));
}

} // namespace detail

} // namespace vgc::dom

template<>
struct fmt::formatter<vgc::dom::Value> : fmt::formatter<double> {
    template<typename FormatContext>
    auto format(const vgc::dom::Value& v, FormatContext& ctx) -> decltype(ctx.out()) {
        return v.info_().format(v.data_, ctx);
    }
};

#endif // VGC_DOM_VALUE_H
