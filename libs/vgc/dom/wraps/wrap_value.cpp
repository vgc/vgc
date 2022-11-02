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

#include <vgc/core/format.h>
#include <vgc/dom/value.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

template<typename T>
void defineValueComparisonMethods(vgc::core::wraps::Class<vgc::dom::Value>& c) {
    c.def(py::self == T());
    c.def(T() == py::self);
    c.def(py::self != T());
    c.def(T() != py::self);
}

void wrap_value(py::module& m) {

    using This = vgc::dom::Value;

    using ValueType = vgc::dom::ValueType;
    py::enum_<ValueType>(m, "ValueType")
        .value("None", ValueType::None)
        .value("Invalid", ValueType::Invalid)
        .value("String", ValueType::String)
        .value("Int", ValueType::Int)
        .value("IntArray", ValueType::IntArray)
        .value("Double", ValueType::Double)
        .value("DoubleArray", ValueType::DoubleArray)
        .value("Color", ValueType::Color)
        .value("ColorArray", ValueType::ColorArray)
        .value("Vec2d", ValueType::Vec2d)
        .value("Vec2dArray", ValueType::Vec2dArray)
        .value("Path", ValueType::Path)
        .value("PathArray", ValueType::PathArray);

    vgc::core::wraps::Class<This> c(m, "Value");
    c.def(py::init<>())
        .def(py::init<std::string>())
        .def(py::init<vgc::Int>())
        .def(py::init<vgc::core::Array<vgc::Int>>())
        .def(py::init<double>())
        .def(py::init<vgc::core::Array<double>>())
        .def(py::init<vgc::core::Color>())
        .def(py::init<vgc::core::Array<vgc::core::Color>>())
        .def(py::init<vgc::geometry::Vec2d>())
        .def(py::init<vgc::core::Array<vgc::geometry::Vec2d>>())
        .def(py::init<vgc::dom::Path>())
        .def(py::init<vgc::dom::PathArray>())

        .def_property_readonly_static("none", [](py::object) { return This::none(); })
        .def_property_readonly_static(
            "invalid", [](py::object) { return This::invalid(); })

        .def_property_readonly("type", &This::type)
        .def("isValid", &This::isValid)
        .def("clear", &This::clear)
        .def("getItemWrapped", &This::getItemWrapped)
        .def_property_readonly("arrayLength", &This::arrayLength)

        .def("getString", &This::getString)
        .def("set", py::overload_cast<std::string>(&This::set))
        .def("getStringId", &This::getStringId)
        .def("set", py::overload_cast<vgc::core::StringId>(&This::set))
        .def("getInt", &This::getInt)
        .def("set", py::overload_cast<vgc::Int>(&This::set))
        .def("getIntArray", &This::getIntArray)
        .def("set", py::overload_cast<const vgc::core::SharedConstIntArray&>(&This::set))
        .def("set", py::overload_cast<vgc::core::IntArray>(&This::set))
        .def("getDouble", &This::getDouble)
        .def("set", py::overload_cast<double>(&This::set))
        .def("getDoubleArray", &This::getDoubleArray)
        .def(
            "set",
            py::overload_cast<const vgc::core::SharedConstDoubleArray&>(&This::set))
        .def("set", py::overload_cast<vgc::core::DoubleArray>(&This::set))
        .def("getColor", &This::getColor)
        .def("set", py::overload_cast<const vgc::core::Color&>(&This::set))
        .def("getColorArray", &This::getColorArray)
        .def(
            "set", py::overload_cast<const vgc::core::SharedConstColorArray&>(&This::set))
        .def("set", py::overload_cast<vgc::core::ColorArray>(&This::set))
        .def("getVec2d", &This::getVec2d)
        .def("set", py::overload_cast<const vgc::geometry::Vec2d&>(&This::set))
        .def("getVec2dArray", &This::getVec2dArray)
        .def(
            "set",
            py::overload_cast<const vgc::geometry::SharedConstVec2dArray&>(&This::set))
        .def("set", py::overload_cast<vgc::geometry::Vec2dArray>(&This::set))
        .def("getPath", &This::getPath)
        .def("set", py::overload_cast<vgc::dom::Path>(&This::set))
        .def("getPathArray", &This::getPathArray)
        .def("set", py::overload_cast<vgc::dom::PathArray>(&This::set))

        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        //.def(py::self > py::self)
        //.def(py::self <= py::self)
        //.def(py::self >= py::self)

        .def(
            "__str__",
            [](const This& a) -> std::string {
                fmt::memory_buffer mbuf;
                fmt::format_to(std::back_inserter(mbuf), "{}", a);
                return std::string(mbuf.begin(), mbuf.size());
            })

        .def(
            "__repr__",
            [](const This& a) -> std::string {
                return a.visit([&](auto&& arg) -> std::string {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, vgc::dom::NoneValue>) {
                        return "vgc.dom.Value.none";
                    }
                    else if constexpr (std::is_same_v<T, vgc::dom::InvalidValue>) {
                        return "vgc.dom.Value.invalid";
                    }
                    else {
                        py::object obj = py::cast(&arg);
                        std::string r = py::cast<std::string>(obj.attr("__repr__")());
                        return vgc::core::format("vgc.dom.Value({})", r);
                    }
                });
            })

        ;

    defineValueComparisonMethods<std::string>(c);
    defineValueComparisonMethods<vgc::core::StringId>(c);
    defineValueComparisonMethods<vgc::Int>(c);
    defineValueComparisonMethods<vgc::core::IntArray>(c);
    defineValueComparisonMethods<double>(c);
    defineValueComparisonMethods<vgc::core::DoubleArray>(c);
    defineValueComparisonMethods<vgc::core::Color>(c);
    defineValueComparisonMethods<vgc::core::ColorArray>(c);
    defineValueComparisonMethods<vgc::geometry::Vec2d>(c);
    defineValueComparisonMethods<vgc::geometry::Vec2dArray>(c);
}
