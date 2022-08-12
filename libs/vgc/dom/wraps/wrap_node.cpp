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
#include <vgc/dom/node.h>

using This = vgc::dom::Node;
using Holder = vgc::dom::NodePtr;
using Parent = vgc::core::Object;

using vgc::dom::Node;
using vgc::dom::NodeType;

void wrap_node(py::module& m) {
    py::enum_<NodeType>(m, "NodeType")
        .value("Element", NodeType::Element)
        .value("Document", NodeType::Document);

    vgc::core::wraps::wrapObjectCommon<This>(m, "Node");

    py::class_<This, Holder, Parent>(m, "Node")
        // Note: Node has no public constructor
        .def_property_readonly("document", &This::document)
        .def_property_readonly("nodeType", &This::nodeType)
        .def("remove", &This::remove)
        .def_property_readonly("parent", &This::parent)
        .def_property_readonly("firstChild", &This::firstChild)
        .def_property_readonly("lastChild", &This::lastChild)
        .def_property_readonly("previousSibling", &This::previousSibling)
        .def_property_readonly("nextSibling", &This::nextSibling)
        .def_property_readonly("children", &This::children)
        .def("canReparent", &This::canReparent)
        .def("reparent", &This::reparent)
        .def("canReplace", &This::canReplace)
        .def("replace", &This::replace)
        .def("isDescendant", &This::isDescendant);
}
