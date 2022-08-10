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

#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/object.h>
#include <vgc/dom/document.h>
#include <vgc/dom/element.h>

using This = vgc::dom::Element;
using Parent = vgc::dom::Node;

using vgc::dom::Document;
using vgc::dom::Element;

void wrap_element(py::module& m) {
    vgc::core::wraps::ObjClass<This>(m, "Element")
        .def_create<This*, Document*, const std::string&>()
        .def_create<This*, Element*, const std::string&>()
        //.def(py::init([](Document* parent, const std::string& name) { return This::create(parent, name); } ))
        //.def(py::init([](Element* parent, const std::string& name) { return This::create(parent, name); } ))
        .def_property_readonly(
            "name", [](const This& self) { return self.name().string(); });
}
