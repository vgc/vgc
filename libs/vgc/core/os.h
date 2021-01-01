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

#ifndef VGC_CORE_OS_H
#define VGC_CORE_OS_H

/// \file vgc/core/os.h
/// \brief Defines macros indicating which operating system is being used.
///
/// On Windows, we define VGC_CORE_OS_WINDOWS, on macOS, we define
/// VGC_CORE_OS_MACOS, and on Linux, we define VGC_CORE_OS_LINUX. Other
/// platforms are currently not supported and a compilation error will be
/// raised.
///

#if defined(_WIN32)
#    define VGC_CORE_OS_WINDOWS
#elif defined(__APPLE__)
#    include "TargetConditionals.h"
#    if TARGET_IPHONE_SIMULATOR
#        error "Unsupported platform: iPhone simulator"
#    elif TARGET_OS_IPHONE
#        error "Unsupported platform: iPhone"
#    elif TARGET_OS_MAC
#        define VGC_CORE_OS_MACOS
#    else
#        error "Unsupported platform: unknown Apple platform"
#    endif
#elif __linux__
#    define VGC_CORE_OS_LINUX
#elif __unix__
#    error "Unsupported platform: unknown Unix platform"
#elif defined(_POSIX_VERSION)
#    error "Unsupported platform: unknown Posix platform"
#else
#    error "Unsupported platform: unknown platform"
#endif

#endif // VGC_CORE_OS_H
