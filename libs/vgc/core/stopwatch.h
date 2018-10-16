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

#ifndef VGC_CORE_STOPWATCH_H
#define VGC_CORE_STOPWATCH_H

#include <chrono>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// \class vgc::core::Stopwatch
/// \brief A class to measure elapsed time.
///
/// Usage:
/// \code
/// Stopwatch t;
/// doSomething();
/// std::cout << "elpased time: " << t.elapsed() << "s\n";
/// \endcode
///
class VGC_CORE_API Stopwatch {
public:
    /// Creates a Stopwatch.
    ///
    Stopwatch();

    /// Restarts this Stopwatch and returns the elapsed time, in seconds, since
    /// this Stopwatch was created or last restarted.
    ///
    double restart();

    /// Returns the elapsed time, in seconds, since this Stopwatch was created
    /// or last restarted.
    ///
    double elapsed() const;

private:
    using Clock_ = std::chrono::steady_clock;
    using Time_ = Clock_::time_point;

    Time_ t_;
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_STOPWATCH_H
