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

#include <vgc/core/format.h>
#include <vgc/core/wraps/common.h>

void wrap_format(py::module& m) {
    py::enum_<vgc::core::TimeUnit>(m, "TimeUnit")
        .value("Seconds", vgc::core::TimeUnit::Seconds)
        .value("Milliseconds", vgc::core::TimeUnit::Milliseconds)
        .value("Microseconds", vgc::core::TimeUnit::Microseconds)
        .value("Nanoseconds", vgc::core::TimeUnit::Nanoseconds);

    m.def(
        "secondsToString",
        &vgc::core::secondsToString,
        "t"_a,
        "unit"_a = vgc::core::TimeUnit::Seconds,
        "decimals"_a = 0);
}
