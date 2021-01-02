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
#include <vgc/ui/style.h>

void wrap_StyleSheet(py::module& m)
{
    using This = vgc::ui::StyleSheet;
    using Holder = vgc::ui::StyleSheetPtr;
    using Parent = vgc::core::Object;

    py::class_<This, Holder, Parent>(m, "StyleSheet")
        .def(py::init([]() { return This::create(); } ))
        .def(py::init([](const std::string& s) { return This::create(s); } ))
    ;
}

void wrap_style(py::module& m)
{
    // Necessary to define inheritance across modules. See:
    // http://pybind11.readthedocs.io/en/stable/advanced/misc.html#partitioning-code-over-multiple-extension-modules
    py::module::import("vgc.core");

    wrap_StyleSheet(m);
}
