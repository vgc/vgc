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

using std::string_view;
using vgc::Int;

namespace {

void wrap_XmlEventType(py::module& m) {
    using This = vgc::core::XmlEventType;
    py::enum_<This>(m, "XmlEventType")
        .value("None_", This::None)
        .value("StartDocument", This::StartDocument)
        .value("EndDocument", This::EndDocument)
        .value("StartElement", This::StartElement)
        .value("EndElement", This::EndElement)
        .value("Characters", This::Characters)
        .value("Comment", This::Comment)
        .value("ProcessingInstruction", This::ProcessingInstruction);
}

void wrap_XmlStreamAttributeView(py::module& m) {
    using This = vgc::core::XmlStreamAttributeView;
    vgc::core::wraps::Class<This>(m, "XmlStreamAttributeView")
        .def_property_readonly("name", &This::name)
        .def_property_readonly("value", &This::value)
        .def_property_readonly("rawText", &This::rawText)
        .def_property_readonly("leadingWhitespace", &This::leadingWhitespace)
        .def_property_readonly("separator", &This::separator)
        .def_property_readonly("rawValue", &This::rawValue)
        .def_property_readonly("quotationMark", &This::quotationMark);
}

void wrap_XmlStringReader(py::module& m) {
    using py::const_;
    using py::overload_cast;
    using This = vgc::core::XmlStreamReader;
    vgc::core::wraps::Class<This>(m, "XmlStreamReader")
        .def(py::init<std::string_view>())
        .def_static("fromFile", &This::fromFile)
        .def("readNext", &This::readNext)
        .def_property_readonly("eventType", &This::eventType)
        .def_property_readonly("rawText", &This::rawText)
        .def_property_readonly("hasXmlDeclaration", &This::hasXmlDeclaration)
        .def_property_readonly("xmlDeclaration", &This::xmlDeclaration)
        .def_property_readonly("version", &This::version)
        .def_property_readonly("encoding", &This::encoding)
        .def_property_readonly("isEncodingSet", &This::isEncodingSet)
        .def_property_readonly("isStandalone", &This::isStandalone)
        .def_property_readonly("isStandaloneSet", &This::isStandaloneSet)
        .def_property_readonly("name", &This::name)
        .def_property_readonly("characters", &This::characters)
        // TODO: wrap attributes() (requires to write python wrappers for core::Span)
        .def_property_readonly("numAttributes", &This::numAttributes)
        .def("attribute", overload_cast<Int>(&This::attribute, const_))
        .def("attribute", overload_cast<string_view>(&This::attribute, const_))
        .def("attributeName", &This::attributeName)
        .def("attributeValue", overload_cast<Int>(&This::attributeValue, const_))
        .def("attributeValue", overload_cast<string_view>(&This::attributeValue, const_))
        .def_property_readonly(
            "processingInstructionTarget", &This::processingInstructionTarget)
        .def_property_readonly(
            "processingInstructionData", &This::processingInstructionData);

    // TODO: ensures everything works as expected with string_view, especially
    // when given as argument, e.g., attributeValue(attrName)
    // https://zpz.github.io/blog/python-cpp-pybind11-stringview/
}

} // namespace

void wrap_xml(py::module& m) {
    wrap_XmlEventType(m);
    wrap_XmlStreamAttributeView(m);
    wrap_XmlStringReader(m);
}
