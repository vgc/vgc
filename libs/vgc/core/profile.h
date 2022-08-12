// Copyright 2022 The VGC Developers
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

#ifndef VGC_CORE_PROFILE_H
#define VGC_CORE_PROFILE_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/compiler.h>
#include <vgc/core/preprocessor.h>

namespace vgc::core::detail {

class VGC_CORE_API ScopeProfiler {
public:
    ScopeProfiler(const char* name);
    ~ScopeProfiler();
    ScopeProfiler(const ScopeProfiler&) = delete;
    ScopeProfiler& operator=(const ScopeProfiler&) = delete;

private:
    const char* name_;
    Int correspondingIndex_;
};

} // namespace vgc::core::detail

/// Measures the time taken for executing a scope.
///
/// All measures are then printed via VGC_DEBUG_TMP, but only once the
/// outer-most measured scoped is closed, so that printing does not affect the
/// measurements.
///
///
/// ```cpp
/// void printHelloWorld() {
///     VGC_PROFILE_FUNCTION
///     {
///         VGC_PROFILE_SCOPE("hello")
///         std::cout << "hello";
///     }
///     {
///         VGC_PROFILE_SCOPE("world")
///         std::cout << "world" << std::endl;
///     }
/// }
///
/// int main() {
///     printHelloWorld();
/// }
/// ```
///
/// Possible output:
///
/// ```
/// [Thread 12216]
///                5us 100ns    void __cdecl printHelloWorld(void)
///                    700ns      hello
///                4us 300ns      world
/// ```
///
/// For now, usage of VGC_PROFILE_FUNCTION is not meant to be committed to the
/// git repository, and only used temporarily while optimizing something. In
/// the future, the stored measurements might be available via an API allowing
/// to display them in a more convenient way.
///
/// Note that the first measurement of a given scope or function may include
/// some initialization overhead up to 10 microseconds on some hardware. We
/// recommend to rely only on subsequent measurements, whose overheads are
/// typically less than 100 nanoseconds each.
///
#define VGC_PROFILE_SCOPE(name)                                                          \
    ::vgc::core::detail::ScopeProfiler VGC_PP_XCONCAT(VGC_PP_XCONCAT(vgcProfiler, __LINE__), _)(name);

/// Measures the time taken for executing a function.
/// See VGC_PROFILE_SCOPE for details.
///
#define VGC_PROFILE_FUNCTION VGC_PROFILE_SCOPE(VGC_PRETTY_FUNCTION)

#endif // VGC_CORE_PROFILE_H
