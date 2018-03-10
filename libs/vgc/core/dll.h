// Copyright 2017 The VGC Developers
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

#ifndef VGC_CORE_DLL_H
#define VGC_CORE_DLL_H

/// \file vgc/core/dll.h
/// \brief Defines helper macros that help for defining symbol visibility when
///        compiling shared libraries.
///
/// See https://gcc.gnu.org/wiki/Visibility
///

#include <vgc/core/compiler.h>
#include <vgc/core/os.h>

#if defined(VGC_CORE_OS_WINDOWS)
#   if defined(VGC_CORE_COMPILER_GCC) && VGC_CORE_COMPILER_GCC_MAJOR >= 4 || defined(VGC_CORE_COMPILER_CLANG)
#       define VGC_CORE_DLL_EXPORT __attribute__((dllexport))
#       define VGC_CORE_DLL_IMPORT __attribute__((dllimport))
#       define VGC_CORE_DLL_HIDDEN
#   else
#       define VGC_CORE_DLL_EXPORT __declspec(dllexport)
#       define VGC_CORE_DLL_IMPORT __declspec(dllimport)
#       define VGC_CORE_DLL_HIDDEN
#   endif
#elif defined(VGC_CORE_COMPILER_GCC) && VGC_CORE_COMPILER_GCC_MAJOR >= 4 || defined(VGC_CORE_COMPILER_CLANG)
#   define VGC_CORE_DLL_EXPORT __attribute__((visibility("default")))
#   define VGC_CORE_DLL_IMPORT __attribute__((visibility("default")))
#   define VGC_CORE_DLL_HIDDEN __attribute__((visibility("hidden")))
#else
#   define VGC_CORE_DLL_EXPORT
#   define VGC_CORE_DLL_IMPORT
#   define VGC_CORE_DLL_HIDDEN
#endif

#endif // VGC_CORE_DLL_H
