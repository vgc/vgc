// Copyright 2023 The VGC Developers
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

#include <vgc/core/xml.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

namespace {

void wrap_XmlTokenType(py::module& m) {
    using This = vgc::core::XmlTokenType;
    py::enum_<This>(m, "XmlTokenType")
        .value("None", This::None)
        .value("Invalid", This::Invalid)
        .value("StartDocument", This::StartDocument)
        .value("EndDocument", This::EndDocument)
        .value("StartTag", This::StartTag)
        .value("EndTag", This::EndTag)
        .value("CharacterData", This::CharacterData)
        .value("Comment", This::Comment);
}

void wrap_XmlStringReader(py::module& m) {
    using This = vgc::core::XmlStreamReader;
    vgc::core::wraps::Class<This>(m, "XmlStreamReader")
        .def(py::init<std::string_view>())
        .def_static("fromFile", &This::fromFile)
        .def("readNext", &This::readNext)
        .def_property_readonly("tokenType", &This::tokenType)
        .def_property_readonly("rawText", &This::rawText)
        .def_property_readonly("characterData", &This::characterData);
}

} // namespace

void wrap_xml(py::module& m) {
    wrap_XmlTokenType(m);
    wrap_XmlStringReader(m);
}
