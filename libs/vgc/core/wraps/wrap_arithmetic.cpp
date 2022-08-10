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

#include <vgc/core/arithmetic.h>
#include <vgc/core/wraps/common.h>

void wrap_arithmetic(py::module& m) {
    // Note: we do wrap isClose despite the existence of Python's math.isclose,
    // for consistency with isNear and the fact that we define it for Vec too.

    m.def(
        "isClose",
        [](double a, double b, double relTol, double absTol) {
            return vgc::core::isClose(a, b, relTol, absTol);
        },
        "a"_a,
        "b"_a,
        "relTol"_a = 1e-9,
        "absTol"_a = 0.0);

    m.def(
        "isNear",
        [](double a, double b, double relTol) { return vgc::core::isNear(a, b, relTol); },
        "a"_a,
        "b"_a,
        "relTol"_a);
}
