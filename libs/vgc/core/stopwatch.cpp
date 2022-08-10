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

#include <vgc/core/stopwatch.h>

namespace vgc::core {

Stopwatch::Stopwatch() {
    start();
}

void Stopwatch::start() {
    t_ = Clock_::now();
}

double Stopwatch::restart() {
    Time_ t2 = Clock_::now();
    std::chrono::duration<double> seconds = t2 - t_;
    t_ = t2;
    return seconds.count();
}

double Stopwatch::elapsed() const {
    Time_ t2 = Clock_::now();
    std::chrono::duration<double> seconds = t2 - t_;
    return seconds.count();
}

Int64 Stopwatch::elapsedSeconds() const {
    Time_ t2 = Clock_::now();
    using Units = std::chrono::duration<Int64>;
    return std::chrono::duration_cast<Units>(t2 - t_).count();
}

Int64 Stopwatch::elapsedMilliseconds() const {
    Time_ t2 = Clock_::now();
    using Units = std::chrono::duration<Int64, std::milli>;
    return std::chrono::duration_cast<Units>(t2 - t_).count();
}

Int64 Stopwatch::elapsedMicroseconds() const {
    Time_ t2 = Clock_::now();
    using Units = std::chrono::duration<Int64, std::micro>;
    return std::chrono::duration_cast<Units>(t2 - t_).count();
}

Int64 Stopwatch::elapsedNanoseconds() const {
    Time_ t2 = Clock_::now();
    using Units = std::chrono::duration<Int64, std::nano>;
    return std::chrono::duration_cast<Units>(t2 - t_).count();
}

} // namespace vgc::core
