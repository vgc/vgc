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

#ifndef VGC_CORE_COMPILER_H
#define VGC_CORE_COMPILER_H

/// \file vgc/core/compiler.h
/// \brief Defines macros indicating which compiler is being used.
///

#if defined(__clang__)
#    define VGC_CORE_COMPILER_CLANG
#    define VGC_CORE_COMPILER_CLANG_MAJOR __clang_major__
#    define VGC_CORE_COMPILER_CLANG_MINOR __clang_minor__
#    define VGC_CORE_COMPILER_CLANG_PATCHLEVEL __clang_patchlevel__
#elif defined(__GNUC__) || defined(__GNUG__)
#    define VGC_CORE_COMPILER_GCC
#    define VGC_CORE_COMPILER_GCC_MAJOR __GNUC__
#    define VGC_CORE_COMPILER_GCC_MINOR __GNUC_MINOR__
#    define VGC_CORE_COMPILER_GCC_PATCHLEVEL __GNUC_PATCHLEVEL__
#elif defined(__INTEL_COMPILER)
#    define VGC_CORE_COMPILER_INTEL
#elif defined(_MSC_VER)
#    define VGC_CORE_COMPILER_MSVC
#    define VGC_CORE_COMPILER_MSVC_VERSION _MSC_VER
#endif

#ifndef NDEBUG
#    define VGC_DEBUG_BUILD
#endif

#if defined(VGC_CORE_COMPILER_CLANG) || defined(VGC_CORE_COMPILER_GCC)
#    define VGC_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(VGC_CORE_COMPILER_MSVC)
#    define VGC_FORCE_INLINE inline __forceinline
#else
#    define VGC_FORCE_INLINE inline
#endif

#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard) >= 201907L
#    define VGC_NODISCARD(msg) [[nodiscard(msg)]]
#else
#    define VGC_NODISCARD(msg) [[nodiscard]]
#endif

#endif // VGC_CORE_COMPILER_H
