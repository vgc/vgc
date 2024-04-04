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

#ifndef VGC_CORE_DEFS_H
#define VGC_CORE_DEFS_H

/// \file vgc/core/defs.h
/// \brief Basic general-purpose macro definitions
///
/// Note that by design, this header is guaranteed to be transitively included
/// by any of the `<vgc/.../api.h>` file, and therefore there is no need to
/// include `<vgc/core/defs.h>` if such `api.h` is already included.

/// \def VGC_COMPILER_CLANG
/// Indicates that the current compiler is Clang.
///
/// \def VGC_COMPILER_CLANG_MAJOR
/// Indicates the major version of Clang. Defined only if `VGC_COMPILER_CLANG` is.
///
/// \def VGC_COMPILER_CLANG_MINOR
/// Indicates the minor version of Clang. Defined only if `VGC_COMPILER_CLANG` is.
///
/// \def VGC_COMPILER_CLANG_PATCHLEVEL
/// Indicates the patchlevel version of Clang. Defined only if `VGC_COMPILER_CLANG` is.
///
/// \def VGC_COMPILER_GCC
/// Indicates that the current compiler is GCC.
///
/// \def VGC_COMPILER_GCC_MAJOR
/// Indicates the major version of GCC. Defined only if `VGC_COMPILER_GCC` is.
///
/// \def VGC_COMPILER_GCC_MINOR
/// Indicates the minor version of GCC. Defined only if `VGC_COMPILER_GCC` is.
///
/// \def VGC_COMPILER_GCC_PATCHLEVEL
/// Indicates the patchlevel version of GCC. Defined only if `VGC_COMPILER_GCC` is.
///
/// \def VGC_COMPILER_INTEL
/// Indicates that the current compiler is the Intel C/C++ Compiler.
///
/// \def VGC_COMPILER_MSVC
/// Indicates that the current compiler is MSVC.
///
/// \def VGC_COMPILER_MSVC_VERSION
/// Indicates the version of MSVC. Defined only if `VGC_COMPILER_MSVC_VERSION` is.
///
#if defined(__clang__)
#    define VGC_COMPILER_CLANG
#    define VGC_COMPILER_CLANG_MAJOR __clang_major__
#    define VGC_COMPILER_CLANG_MINOR __clang_minor__
#    define VGC_COMPILER_CLANG_PATCHLEVEL __clang_patchlevel__
#elif defined(__GNUC__) || defined(__GNUG__)
#    define VGC_COMPILER_GCC
#    define VGC_COMPILER_GCC_MAJOR __GNUC__
#    define VGC_COMPILER_GCC_MINOR __GNUC_MINOR__
#    define VGC_COMPILER_GCC_PATCHLEVEL __GNUC_PATCHLEVEL__
#elif defined(__INTEL_COMPILER)
#    define VGC_COMPILER_INTEL
#elif defined(_MSC_VER)
#    define VGC_COMPILER_MSVC
#    define VGC_COMPILER_MSVC_VERSION _MSC_VER
#endif

/// \def VGC_DEBUG_BUILD
/// Indicates that this is a "Debug" build.
///
#ifndef NDEBUG
#    define VGC_DEBUG_BUILD
#endif

/// \def VGC_OS_WINDOWS
/// Indicates that the target platform is Windows.
///
/// \def VGC_OS_MACOS
/// Indicates that the target platform is macOS.
///
/// \def VGC_OS_LINUX
/// Indicates that the target platform is Linux.
///
#if defined(_WIN32)
#    define VGC_OS_WINDOWS
#elif defined(__APPLE__)
#    include "TargetConditionals.h"
#    if TARGET_IPHONE_SIMULATOR
#        error "Unsupported platform: iPhone simulator"
#    elif TARGET_OS_IPHONE
#        error "Unsupported platform: iPhone"
#    elif TARGET_OS_MAC
#        define VGC_OS_MACOS
#    else
#        error "Unsupported platform: unknown Apple platform"
#    endif
#elif __linux__
#    define VGC_OS_LINUX
#elif __unix__
#    error "Unsupported platform: unknown Unix platform"
#elif defined(_POSIX_VERSION)
#    error "Unsupported platform: unknown Posix platform"
#else
#    error "Unsupported platform: unknown platform"
#endif

/// \def VGC_WARNING_PUSH
/// Makes a "push" to the stack of compiler warning options.
///
/// \def VGC_WARNING_POP
/// Makes a "pop" to the stack of compiler warning options.
///
/// \def VGC_WARNING_GCC_DISABLE
/// Disable a specific GCC or Clang warning. This should typically be surrounded
/// by `VGC_WARNING_PUSH` and `VGC_WARNING_POP`.
///
/// \def VGC_WARNING_MSVC_DISABLE
/// Disable a specific MSVC warning. This should typically be surrounded
/// by `VGC_WARNING_PUSH` and `VGC_WARNING_POP`.
///
#if defined(VGC_COMPILER_CLANG) || defined(VGC_COMPILER_GCC)
// clang-format off
// clang format bug: "_Pragma(#x)" -> "_Pragma(#    x)"
#    define VGC_DO_PRAGMA_(x) _Pragma(#x)
// clang-format on
#    define VGC_WARNING_PUSH VGC_DO_PRAGMA_(GCC diagnostic push)
#    define VGC_WARNING_POP VGC_DO_PRAGMA_(GCC diagnostic pop)
#    define VGC_WARNING_GCC_DISABLE_(option)                                             \
        VGC_DO_PRAGMA_(GCC diagnostic ignored #option)
#    define VGC_WARNING_GCC_DISABLE(wname) VGC_WARNING_GCC_DISABLE_(-W##wname)
#    define VGC_WARNING_MSVC_DISABLE(codes)
#elif defined(VGC_COMPILER_MSVC)
#    define VGC_WARNING_PUSH __pragma(warning(push))
#    define VGC_WARNING_POP __pragma(warning(pop))
#    define VGC_WARNING_GCC_DISABLE(wname)
#    define VGC_WARNING_MSVC_DISABLE(codes) __pragma(warning(disable : codes))
#else
#    define VGC_WARNING_PUSH
#    define VGC_WARNING_POP
#    define VGC_WARNING_GCC_DISABLE(wname)
#    define VGC_WARNING_MSVC_DISABLE(codes)
#endif

/// \def VGC_FORCE_INLINE
/// Ensures that a given function is not inlined, for example for benchmarking purposes.
///
#if defined(VGC_COMPILER_CLANG) || defined(VGC_COMPILER_GCC)
#    define VGC_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(VGC_COMPILER_MSVC)
#    define VGC_FORCE_INLINE inline __forceinline
#else
#    define VGC_FORCE_INLINE inline
#endif

/// \def VGC_NODISCARD
///
/// Marks a function as "nodiscard" with a message explaining why.
///
/// This is the same as `[[nodiscard(msg)]]` for compilers that support it (C++20),
/// otherwise it falls back to `[[nodiscard]]` (C++17).
///
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard) >= 201907L
#    define VGC_NODISCARD(msg) [[nodiscard(msg)]]
#else
#    define VGC_NODISCARD(msg) [[nodiscard]]
#endif

/// \def VGC_PRETTY_FUNCTION
/// Returns the name of the current function. This is a cross-platform
/// way to use the non-standard `__PRETTY_FUNCTION__`, `__FUNCSIG__`,
/// or `__FUNCTION__` available on some compilers.
///
#if defined(VGC_COMPILER_CLANG) || defined(VGC_COMPILER_GCC)
#    define VGC_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(VGC_COMPILER_MSVC)
#    define VGC_PRETTY_FUNCTION __FUNCSIG__
#else
#    define VGC_PRETTY_FUNCTION __FUNCTION__
#endif

#endif // VGC_CORE_DEFS_H
