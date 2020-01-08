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
#include <vgc/core/exceptions.h>

void wrap_exceptions(py::module& m)
{
    auto logicError = py::register_exception<vgc::core::LogicError>(m, "LogicError");
    auto runtimeError = py::register_exception<vgc::core::RuntimeError>(m, "RuntimeError");
    auto parseError = py::register_exception<vgc::core::ParseError>(m, "ParseError", runtimeError.ptr());
    auto rangeError = py::register_exception<vgc::core::RangeError>(m, "RangeError", runtimeError.ptr());
}
