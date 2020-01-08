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

#include <vgc/core/wraps/common.h>
#include <vgc/dom/exceptions.h>

void wrap_exceptions(py::module& m)
{
    py::module core = py::module::import("vgc.core");
    py::object coreLogicError = core.attr("LogicError");
    py::object coreRuntimeError = core.attr("RuntimeError");

    auto logicError = py::register_exception<vgc::dom::LogicError>(m, "LogicError", coreLogicError.ptr());
    auto notAliveError = py::register_exception<vgc::dom::NotAliveError>(m, "NotAliveError", logicError.ptr());
    auto wrongDocumentError = py::register_exception<vgc::dom::WrongDocumentError>(m, "WrongDocumentError", logicError.ptr());
    auto hierarchyRequestError = py::register_exception<vgc::dom::HierarchyRequestError>(m, "HierarchyRequestError", logicError.ptr());
    auto wrongChildTypeError = py::register_exception<vgc::dom::WrongChildTypeError>(m, "WrongChildTypeError", hierarchyRequestError.ptr());
    auto secondRootElementError = py::register_exception<vgc::dom::SecondRootElementError>(m, "SecondRootElementError", hierarchyRequestError.ptr());
    auto childCycleError = py::register_exception<vgc::dom::ChildCycleError>(m, "ChildCycleError", hierarchyRequestError.ptr());
    auto replaceDocumentError = py::register_exception<vgc::dom::ReplaceDocumentError>(m, "ReplaceDocumentError", hierarchyRequestError.ptr()); 

    auto runtimeError = py::register_exception<vgc::dom::RuntimeError>(m, "RuntimeError", coreRuntimeError.ptr());
    auto parseError = py::register_exception<vgc::dom::ParseError>(m, "ParseError", runtimeError.ptr());
    auto xmlSyntaxError = py::register_exception<vgc::dom::XmlSyntaxError>(m, "XmlSyntaxError", parseError.ptr());
    auto vgcSyntaxError = py::register_exception<vgc::dom::VgcSyntaxError>(m, "VgcSyntaxError", parseError.ptr());
    auto fileError = py::register_exception<vgc::dom::FileError>(m, "FileError", runtimeError.ptr());
}
