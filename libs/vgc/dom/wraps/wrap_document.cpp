// Copyright 2018 The VGC Developers
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
#include <vgc/dom/document.h>
#include <vgc/dom/element.h>

namespace py = pybind11;
using namespace py::literals;

using This = vgc::dom::Document;
using Holder = vgc::dom::DocumentSharedPtr;

using vgc::dom::XmlFormattingStyle;

void wrap_document(py::module& m)
{
    py::class_<This, Holder>(m, "Document")
        .def(py::init([]() { return This::make(); } ))
        .def_property("rootElement", &This::rootElement, &This::setRootElement)
        .def("save", &This::save, "filePath"_a, "style"_a = XmlFormattingStyle())
    ;
}
