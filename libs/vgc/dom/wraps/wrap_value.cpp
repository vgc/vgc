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

#include <unordered_map>

#include <vgc/core/format.h>
#include <vgc/dom/detail/pyvalue.h>
#include <vgc/dom/value.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

// Supported types as Value
#include <vgc/core/color.h>
#include <vgc/dom/path.h>
#include <vgc/geometry/vec2d.h>

template<typename T>
void defineValueComparisonMethods(vgc::core::wraps::Class<vgc::dom::Value>& c) {
    c.def(py::self == T());
    c.def(T() == py::self);
    c.def(py::self != T());
    c.def(T() != py::self);
}

// XXX: How to publicize this in vgc::dom::wraps, so that other wrapper
//      modules can register their own types? The question is how to get
//      the reference to the Value class from other modules.
//
template<typename T>
void registerValue(
    vgc::core::wraps::Class<vgc::dom::Value>& c,
    std::string_view pyTypeName) {

    vgc::dom::detail::registerPyValue<T>(pyTypeName);
    py::implicitly_convertible<T, vgc::dom::Value>();
    defineValueComparisonMethods<T>(c);
}

void wrap_value(py::module& m) {

    using This = vgc::dom::Value;

    vgc::core::wraps::Class<This> c(m, "Value");

    // Default constructor: creates a None value
    c.def(py::init<>());

    // Explicitly create a None or Invalid value
    c.def_property_readonly_static("none", [](py::object) { return This::none(); });
    c.def_property_readonly_static("invalid", [](py::object) { return This::invalid(); });

    c.def(py::init([](py::handle h) { return vgc::dom::detail::toValue(h); }));

    c.def("toPyObject", [](const This& self) {
        return vgc::dom::detail::toPyObject(self);
    });

    c.def("clear", &This::clear);

    c.def("isNone", &This::isNone);
    c.def("isValid", &This::isValid);
    c.def("hasValue", &This::hasValue);

    // Note: typeId is not currently wrapped.
    // As a workaround, Python users can use `type(v.toPyObject())`.
    //
    // TODO?
    // c.def_property_readonly("typeId", &This::typeId)

    c.def("getArrayItemWrapped", &This::getArrayItemWrapped);
    // TODO: other array methods

    c.def(py::self == py::self);
    c.def(py::self != py::self);
    c.def(py::self < py::self);
    //c.def(py::self > py::self);
    //c.def(py::self <= py::self);
    //c.def(py::self >= py::self);

    c.def("__str__", [](const This& self) -> std::string {
        fmt::memory_buffer mbuf;
        fmt::format_to(std::back_inserter(mbuf), "{}", self);
        return std::string(mbuf.begin(), mbuf.size());
    });

    c.def("__repr__", [](const This& self) -> std::string {
        if (self.has<vgc::dom::NoneValue>()) {
            return "vgc.dom.Value.none";
        }
        else if (self.has<vgc::dom::InvalidValue>()) {
            return "vgc.dom.Value.invalid";
        }
        else {
            // XXX Something faster avoiding creating a Python object copy?
            py::object obj = vgc::dom::detail::toPyObject(self);
            std::string r = py::cast<std::string>(obj.attr("__repr__")());
            return vgc::core::format("vgc.dom.Value({})", r);
        }
    });

    registerValue<vgc::Int>(c, "int");
    registerValue<double>(c, "float");
    registerValue<std::string>(c, "str");

    // StringId?

    // XXX How to handle possible N to 1 mappings, e.g.:
    //
    // registerValue<float>(c, "float"); // Conflicts with <double>("float")
    //
    // One solution might simply be to provide more fine-grained versions of
    // vgc::dom::detail::registerPyValue(), allowing to specify only the
    // `toPyObject` or the `toValue` function.
    //
    // This way, we could specify N to 1 mappings from Python to C++ and N to 1
    // mappings from C++ to Python

    registerValue<vgc::core::Color>(c, "vgc.core.Color");

    registerValue<vgc::core::IntArray>(c, "vgc.core.IntArray");
    registerValue<vgc::core::DoubleArray>(c, "vgc.core.DoubleArray");
    registerValue<vgc::core::ColorArray>(c, "vgc.core.ColorArray");

    registerValue<vgc::geometry::Vec2d>(c, "vgc.geometry.Vec2d");
    registerValue<vgc::geometry::Vec2dArray>(c, "vgc.geometry.Vec2dArray");

    registerValue<vgc::dom::Path>(c, "vgc.dom.Path");
    registerValue<vgc::dom::PathArray>(c, "vgc.dom.PathArray");
}
