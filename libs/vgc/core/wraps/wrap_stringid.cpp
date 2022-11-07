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

#include <string_view>

#include <vgc/core/stringid.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

void wrap_stringid(py::module& m) {
    using This = vgc::core::StringId;
    vgc::core::wraps::Class<This>(m, "StringId")
        .def(py::init<>())
        .def(py::init<std::string_view>())
        .def("isEmpty", &This::isEmpty)
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        .def(py::self == std::string_view())
        .def(std::string_view() == py::self)
        .def(py::self != std::string_view())
        .def(std::string_view() != py::self)
        .def("__str__", &This::string)
        .def("__repr__", [](const This& a) -> std::string {
            py::str s = py::cast(a.string());
            std::string r = py::cast<std::string>(s.attr("__repr__")());
            return vgc::core::format("vgc.core.StringId({})", r);
        });
}
