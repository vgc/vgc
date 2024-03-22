// Copyright 2024 The VGC Developers
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

#ifndef VGC_DOM_DETAIL_PYVALUE_H
#define VGC_DOM_DETAIL_PYVALUE_H

// Note: this file is directly part of the dom library rather than just in the
// Python wrapper module because it needs to export symbols.

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <vgc/core/format.h>
#include <vgc/core/stringid.h>
#include <vgc/core/typeid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/value.h>

namespace vgc::dom::detail {

// Wrapper class that allows dom::Value to hold any Python object.
//
// This is used as fallback for when there is no corresponding C++ type
// registered for a given Python object.
//
class VGC_DOM_API AnyPyValue {
public:
    // Needs to be default-constructible for being heldable in a `Value`.
    // This is due to the implementation of readAs_(). Can lift this
    // restriction? Maybe there could be a constructor with a `ReadInit{}`
    // tag?
    AnyPyValue() {
    }

    AnyPyValue(py::handle h)
        : obj_(py::reinterpret_borrow<py::object>(h)) {
    }

    AnyPyValue(py::object obj)
        : obj_(obj) {
    }

    bool operator==(const AnyPyValue& other) const {
        return obj_.is(other.obj_);
    }

    bool operator<(const AnyPyValue& other) const {
        return obj_ < other.obj_; // XXX: Does this make sense?
    }

    py::object object() const {
        return obj_;
    }

    py::handle handle() const {
        return obj_;
    }

private:
    py::object obj_;
};

template<typename IStream>
void readTo(AnyPyValue&, IStream&) {

    // TODO: Use Python's type constructor from string, and emit a warning is
    // no construction from string exists.
    //
    // Rationale: In Python, converting from int to string is simply calling
    // `int("12")`, which hints that using a constructor from string is the
    // Pythonic way to do parsing. And we do implement such string constructors
    // for the Python version of IntArray, Vec2dArray, etc.
}

template<typename OStream>
void write(OStream& out, const AnyPyValue& v) {
    std::string s = py::str(v.handle());
    vgc::core::write(out, s);
}

} // namespace vgc::dom::detail

template<>
struct fmt::formatter<vgc::dom::detail::AnyPyValue> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::dom::detail::AnyPyValue& v, FormatContext& ctx) {
        std::string s = py::str(v.handle());
        return fmt::formatter<std::string_view>::format(s, ctx);
    }
};

namespace vgc::dom::detail {

// Convert from Python object to C++ dom::Value

using PyObjectToValueFn = Value (*)(py::handle);

template<typename T>
Value toValue(py::handle h) {
    return Value(py::cast<T>(h));
    // TODO: error handling?
}

// Convert from C++ dom::Value to Python object

using ValueToPyObjectFn = py::object (*)(const Value&);

template<typename T>
py::object toPyObject(const Value& value) {
    return py::cast(value.get<T>());
    // TODO: error handling?
}

VGC_DOM_API
void registerPyValue(
    std::string_view pyTypeName,
    core::TypeId typeId,
    PyObjectToValueFn toValue,
    ValueToPyObjectFn toPyObject);

/// Registers the C++ type T as the type that should be used as
/// `vgc::dom::Value` held type for a given Python type name.
///
/// For example, the following code registers `vgc::Int` as the C++ type to use
/// when a Python object of type `int` is assigned to a `vgc::dom::Value`:
///
/// ```cpp
/// registerPyValue<vgc::Int>("int");
/// ```
///
template<typename T>
void registerPyValue(std::string_view pyTypeName) {
    core::TypeId typeId = core::typeId<T>();
    auto toValue_ = &toValue<T>;
    auto toPyObject_ = &toPyObject<T>;
    detail::registerPyValue(pyTypeName, typeId, toValue_, toPyObject_);
}

/// Converts the given Python object to a dom::Value holding the most
/// appropriate C++ type.
///
VGC_DOM_API
Value toValue(py::handle h);

/// Converts the given C++ dom::Value to a Python object.
///
VGC_DOM_API
py::object toPyObject(const Value& value);

} // namespace vgc::dom::detail

#endif // VGC_DOM_DETAIL_PYVALUE_H
