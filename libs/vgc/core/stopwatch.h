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

#ifndef VGC_CORE_STOPWATCH_H
#define VGC_CORE_STOPWATCH_H

#include <chrono>
#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>

namespace vgc::core {

/// \class vgc::core::Stopwatch
/// \brief A class to measure elapsed time.
///
class VGC_CORE_API Stopwatch {
public:
    /// Creates a Stopwatch. This automatically calls start().
    ///
    /// ```cpp
    /// vgc::core::Stopwatch t;
    /// doSomething();
    /// std::cout << "elpased time: " << t.elapsed() << "s\n";
    /// ```
    ///
    Stopwatch();

    /// Creates a Stopwatch without initializing it. If you use this
    /// constructor, you must manually call start before calling elapsed(),
    /// otherwise the result of elapsed() is undefined.
    ///
    ///  ```cpp
    /// vgc::core::Stopwatch t(vgc::core::NoInit{});
    /// t.start();
    /// doSomething();
    /// std::cout << "elpased time: " << t.elapsed() << "s\n";
    /// ```
    ///
    Stopwatch(NoInit) {
    }

    /// Starts this Stopwatch.
    ///
    void start();

    /// Restarts this Stopwatch and returns the elapsed time, in seconds, since
    /// this Stopwatch was created or last (re)started.
    ///
    double restart();

    /// Returns the elapsed time, in seconds, as a float, since this Stopwatch
    /// was created or last (re)started.
    ///
    double elapsed() const;

    /// Returns the elapsed time, in seconds, as an integer, since this
    /// Stopwatch was created or last (re)started.
    ///
    Int64 elapsedSeconds() const;

    /// Returns the elapsed time, in milliseconds, as an integer, since this
    /// Stopwatch was created or last (re)started.
    ///
    Int64 elapsedMilliseconds() const;

    /// Returns the elapsed time, in microseconds, as an integer, since this
    /// Stopwatch was created or last (re)started.
    ///
    Int64 elapsedMicroseconds() const;

    /// Returns the elapsed time, in nanoseconds, as an integer, since this
    /// Stopwatch was created or last (re)started.
    ///
    Int64 elapsedNanoseconds() const;

private:
    using Clock_ = std::chrono::steady_clock;
    using Time_ = Clock_::time_point;

    Time_ t_;
};

} // namespace vgc::core

#endif // VGC_CORE_STOPWATCH_H
