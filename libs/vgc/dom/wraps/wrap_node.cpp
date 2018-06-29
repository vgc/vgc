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
#include <vgc/dom/node.h>

namespace py = pybind11;
using namespace py::literals;

using This = vgc::dom::Node;
using Holder = vgc::dom::NodeSharedPtr;

using vgc::dom::NodeType;

void wrap_node(py::module& m)
{
    py::enum_<NodeType>(m, "NodeType")
        .value("Element", NodeType::Element)
        .value("Document", NodeType::Document)
    ;

    py::class_<This, Holder>(m, "Node")
        // Note: Node has no public constructor
        .def_property_readonly("nodeType", &This::nodeType)
        .def("appendChild", &This::appendChild, py::return_value_policy::reference)
    ;
}
