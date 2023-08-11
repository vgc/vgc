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

#include <vgc/dom/document.h>
#include <vgc/dom/element.h>

#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/object.h>

void wrap_document(py::module& m) {
    using This = vgc::dom::Document;
    using XmlFormattingStyle = vgc::dom::XmlFormattingStyle;
    vgc::core::wraps::ObjClass<This>(m, "Document")
        .def(py::init([]() { return This::create(); }))
        .def_static("open", &This::open)
        .def_property_readonly("rootElement", &This::rootElement)
        .def("save", &This::save, "filePath"_a, "style"_a = XmlFormattingStyle())
        .def_static(
            "copy",
            [](const vgc::core::Array<vgc::dom::Node*>& nodes) {
                return This::copy(nodes);
            },
            "nodes"_a)
        .def_static("paste", &This::paste, "document"_a, "parent"_a);
}
