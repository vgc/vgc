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

#include <vgc/dom/exceptions.h>

#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/exceptions.h>

void wrap_exceptions(py::module& m) {
    py::module core = py::module::import("vgc.core");

    VGC_CORE_WRAP_EXCEPTION(dom, LogicError, core, LogicError);
    VGC_CORE_WRAP_EXCEPTION(dom, WrongDocumentError, m, LogicError);
    VGC_CORE_WRAP_EXCEPTION(dom, HierarchyRequestError, m, LogicError);
    VGC_CORE_WRAP_EXCEPTION(dom, WrongChildTypeError, m, HierarchyRequestError);
    VGC_CORE_WRAP_EXCEPTION(dom, SecondRootElementError, m, HierarchyRequestError);
    VGC_CORE_WRAP_EXCEPTION(dom, ChildCycleError, m, HierarchyRequestError);
    VGC_CORE_WRAP_EXCEPTION(dom, ReplaceDocumentError, m, HierarchyRequestError);

    VGC_CORE_WRAP_EXCEPTION(dom, RuntimeError, core, RuntimeError);
    VGC_CORE_WRAP_EXCEPTION(dom, ParseError, m, RuntimeError);
    VGC_CORE_WRAP_EXCEPTION(dom, XmlSyntaxError, m, ParseError);
    VGC_CORE_WRAP_EXCEPTION(dom, VgcSyntaxError, m, ParseError);
    VGC_CORE_WRAP_EXCEPTION(dom, FileError, m, RuntimeError);
}
