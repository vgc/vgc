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

#include <vgc/core/wraps/common.h>
#include <vgc/dom/value.h>

using This = vgc::dom::Value;

using vgc::dom::ValueType;

void wrap_value(py::module& m) {
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
        .value("Vec2dArray", ValueType::Vec2dArray);

    py::class_<This>(m, "Value")
        .def(py::init<>())
        .def(py::init<std::string>())
        .def(py::init<vgc::Int>())
        .def(py::init<vgc::core::Array<vgc::Int>>())
        .def(py::init<double>())
        .def(py::init<vgc::core::Array<double>>())
        .def(py::init<vgc::core::Color>())
        .def(py::init<vgc::core::Array<vgc::core::Color>>())
        .def(py::init<vgc::geometry::Vec2d>())
        .def(py::init<vgc::core::Array<vgc::geometry::Vec2d>>())
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
        .def("set", py::overload_cast<vgc::core::Array<vgc::Int>>(&This::set))
        .def("getDouble", &This::getDouble)
        .def("set", py::overload_cast<double>(&This::set))
        .def("getDoubleArray", &This::getDoubleArray)
        .def("set", py::overload_cast<vgc::core::Array<double>>(&This::set))
        .def("getColor", &This::getColor)
        .def("set", py::overload_cast<const vgc::core::Color&>(&This::set))
        .def("getColorArray", &This::getColorArray)
        .def("set", py::overload_cast<vgc::core::Array<vgc::core::Color>>(&This::set))
        .def("getVec2d", &This::getVec2d)
        .def("set", py::overload_cast<const vgc::geometry::Vec2d&>(&This::set))
        .def("getVec2dArray", &This::getVec2dArray)
        .def("set", py::overload_cast<vgc::core::Array<vgc::geometry::Vec2d>>(&This::set))
        .def("__str__", [](const This& self) -> std::string {
            fmt::memory_buffer mbuf;
            mbuf.append(std::string_view("<Value: "));
            std::string s;
            vgc::core::StringWriter sw(s);
            self.writeTo(sw);
            mbuf.append(s);
            mbuf.push_back('>');
            return std::string(mbuf.begin(), mbuf.size());
        });
}
