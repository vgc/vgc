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

#include <vgc/core/exceptions.h>
#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/exceptions.h>

void wrap_exceptions(py::module& m) {
    VGC_CORE_WRAP_BASE_EXCEPTION(core, LogicError);
    VGC_CORE_WRAP_EXCEPTION(core, NegativeIntegerError, m, LogicError);
    VGC_CORE_WRAP_EXCEPTION(core, IndexError, m, LogicError);
    VGC_CORE_WRAP_EXCEPTION(core, LengthError, m, LogicError);
    VGC_CORE_WRAP_EXCEPTION(core, NullError, m, LogicError);
    VGC_CORE_WRAP_EXCEPTION(core, NotAliveError, m, LogicError);
    VGC_CORE_WRAP_EXCEPTION(core, NotAChildError, m, LogicError);

    VGC_CORE_WRAP_BASE_EXCEPTION(core, RuntimeError);
    VGC_CORE_WRAP_EXCEPTION(core, ParseError, m, RuntimeError);
    VGC_CORE_WRAP_EXCEPTION(core, RangeError, m, RuntimeError);
    VGC_CORE_WRAP_EXCEPTION(core, IntegerOverflowError, m, RangeError);
    VGC_CORE_WRAP_EXCEPTION(core, FileError, m, RuntimeError);
}
