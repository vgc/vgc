// Copyright 2020 The VGC Developers
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

#include <pybind11/pybind11.h>
#include <vgc/dom/xmlformattingstyle.h>

namespace py = pybind11;
using vgc::dom::XmlIndentStyle;
using vgc::dom::XmlFormattingStyle;

void wrap_xmlformattingstyle(py::module& m)
{
    py::enum_<XmlIndentStyle>(m, "XmlIndentStyle")
        .value("Spaces", XmlIndentStyle::Spaces)
        .value("Tabs", XmlIndentStyle::Tabs)
    ;

    py::class_<XmlFormattingStyle>(m, "XmlFormattingStyle")
        .def(py::init<>())
        .def_readwrite("indentStyle", &XmlFormattingStyle::indentStyle)
        .def_readwrite("indentSize", &XmlFormattingStyle::indentSize)
        .def_readwrite("attributeIndentSize", &XmlFormattingStyle::attributeIndentSize)
    ;
}
