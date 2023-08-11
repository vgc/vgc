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
#include <vgc/dom/node.h>

#include <vgc/core/wraps/array.h>
#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/object.h>

void wrap_node(py::module& m) {

    using This = vgc::dom::Node;

    using StringId = vgc::core::StringId;

    using NodeType = vgc::dom::NodeType;
    py::enum_<NodeType>(m, "NodeType")
        .value("Element", NodeType::Element)
        .value("Document", NodeType::Document);

    vgc::core::wraps::wrapObjectCommon<This>(m, "Node");
    vgc::core::wraps::wrap_array<This*, false>(m, "Node");

    vgc::core::wraps::ObjClass<This>(m, "Node")
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
        .def("isDescendantOf", &This::isDescendantOf)
        .def("ancestors", &This::ancestors)
        .def("lowestCommonAncestorWith", &This::lowestCommonAncestorWith, "other"_a)
        .def(
            "getElementFromPath",
            &This::getElementFromPath,
            "path"_a,
            "tagNameFilter"_a = StringId())
        .def(
            "getValueFromPath",
            &This::getValueFromPath,
            "path"_a,
            "tagNameFilter"_a = StringId());

    m.def(
        "lowestCommonAncestor",
        [](const vgc::core::Array<vgc::dom::Node*>& nodes) {
            return vgc::dom::lowestCommonAncestor(nodes);
        },
        "nodes"_a);
}
