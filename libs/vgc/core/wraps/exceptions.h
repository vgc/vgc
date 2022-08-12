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

#ifndef VGC_CORE_WRAPS_EXCEPTIONS_H
#define VGC_CORE_WRAPS_EXCEPTIONS_H

#include <vgc/core/wraps/common.h>

// Wraps an exception base class.
//
// ```cpp
// VGC_CORE_WRAP_BASE_EXCEPTION(core, LogicError);
// ```
//
#define VGC_CORE_WRAP_BASE_EXCEPTION(libname, ErrorType)                                 \
    py::register_exception<vgc::libname::ErrorType>(m, #ErrorType)

// Wraps an exception class deriving from another exception class. If the
// parent exception is from the same module, simply pass it `m`, otherwise, you
// must import beforehand the module in whic the parent Exception is defined.
//
// ```cpp
// VGC_CORE_WRAP_EXCEPTION(core, IndexError, m, LogicError);
//
// py::module core = py::module::import("vgc.core");
// VGC_CORE_WRAP_EXCEPTION(dom, LogicError, core, LogicError);
// ```
//
#define VGC_CORE_WRAP_EXCEPTION(libname, ErrorType, parentmodule, ParentErrorType)       \
    py::register_exception<vgc::libname::ErrorType>(                                     \
        m, #ErrorType, parentmodule.attr(#ParentErrorType).ptr())

#endif // VGC_CORE_WRAPS_EXCEPTIONS_H
