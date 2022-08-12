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

#ifndef VGC_CORE_DLL_H
#define VGC_CORE_DLL_H

/// \file vgc/core/dll.h
/// \brief Defines helper macros that help for defining symbol visibility when
///        compiling shared libraries.
///
/// The VGC codebase is made of several modules ("core", "dom", etc.). Each
/// module is meant to be compiled and linked as a shared library (e.g.,
/// libvgccore.so, libvgcdom.so, etc.), which is the behavior you get by
/// compiling using the provided CMake build system.
///
/// In order to decrease load time of the shared library, improve runtime
/// performance, and reduce the chance of name collisions, it is important to
/// only export in the shared object the symbols which are part of the public
/// API of the library. For more details, please refer to:
///
/// - https://gcc.gnu.org/wiki/Visibility
/// - https://www.akkadia.org/drepper/dsohowto.pdf
///
/// To achieve this, each library should have a header file `vgc/foo/api.h`
/// with the following:
///
/// \code
/// #ifndef VGC_FOO_API_H
/// #define VGC_FOO_API_H
///
/// #include <vgc/core/dll.h>
///
/// #if defined(VGC_FOO_STATIC)
/// #    define VGC_FOO_API
/// #    define VGC_FOO_API_HIDDEN
/// #else
/// #    if defined(VGC_FOO_EXPORTS)
/// #        define VGC_FOO_API VGC_CORE_DLL_EXPORT
/// #    else
/// #        define VGC_FOO_API VGC_CORE_DLL_IMPORT
/// #    endif
/// #    define VGC_FOO_API_HIDDEN VGC_CORE_DLL_HIDDEN
/// #endif
///
/// #endif // VGC_FOO_API_H
/// \endcode
///
/// All classes in the public API should be declared as such:
///
/// \code
/// class VGC_FOO_API Foo {
///     // ...
/// };
/// \endcode
///
/// And all free functions in the public API should be declared as such:
///
/// \code
/// VGC_FOO_API
/// void foo();
/// \endcode
///
/// Note that vgc_add_library() from the provided CMake build system
/// takes care of using the -fvisibility=hidden option when compiling on
/// Linux and MacOS. This ensures that no symbols are exported by default
/// unless explicitly marked with VGC_FOO_API. This matches the behaviour
/// of Windows DLLs, therefore avoiding potential surprises when developing
/// on Linux then porting to Windows.
///
/// # How does it work?
///
/// On Windows, when compiling a DLL with the Microsoft Visual C++ Compiler
/// (MSVC), no symbols are exported by default. The symbols which we want to be
/// part of the public API should be explicitly marked with
/// `__declspec(dllexport)` when compiling the library, and marked with
/// `__declspec(dllimport)` when using the library.
///
/// However, we don't want to use different C++ header files when compiling
/// versus using the library! In order to solve this problem, the provided
/// CMake builds system defines `VGC_FOO_EXPORTS` when compiling the `foo`
/// library, but does not define it when simply using the library. This allows
/// us to define the following macros:
///
/// \code
/// #if defined(VGC_FOO_EXPORTS)
/// #    define VGC_FOO_API __declspec(dllexport)
/// #else
/// #    define VGC_FOO_API __declspec(dllimport)
/// \endcode
///
/// Then simply use the following in our header files:
///
/// \code
/// class VGC_FOO_API Foo {
///     // ...
/// };
/// \endcode
///
/// When you are compiling the module `foo`, the above code expands to:
///
/// \code
/// class __declspec(dllexport) Foo {
///     // ...
/// };
/// \endcode
///
/// But when you are compiling another module `bar` which includes a
/// header file from `foo`, it expands to the following instead:
///
/// \code
/// class __declspec(dllimport) Foo {
///     // ...
/// };
/// \endcode
///
/// However, things are slightly more complex. For example, still on Windows,
/// but compiling with GCC or Clang, the syntax is different: we should use
/// `__attribute__((dllexport))` instead of `__declspec(dllexport)` (same for
/// `dllimport`). The syntax is also different when compiling on Linux or
/// MacOS. This is why, instead of defining the following:
///
/// \code
/// #if defined(VGC_FOO_EXPORTS)
/// #    define VGC_FOO_API __declspec(dllexport)
/// #else
/// #    define VGC_FOO_API __declspec(dllimport)
/// \endcode
///
/// You should instead use `VGC_CORE_DLL_EXPORT` and `VGC_CORE_DLL_IMPORT`
/// provided in this file, like so:
///
/// \code
/// #if defined(VGC_FOO_EXPORTS)
/// #    define VGC_FOO_API VGC_CORE_DLL_EXPORT
/// #else
/// #    define VGC_FOO_API VGC_CORE_DLL_IMPORT
/// \endcode
///
/// In addition, what if a client desires to compile `foo` as a static library
/// instead of a shared library? In this case, `VGC_FOO_API` should expands to
/// nothing instead, and we allow clients to switch to this mode by defining
/// `VGC_FOO_STATIC` using their build system:
///
/// \code
/// #if defined(VGC_FOO_STATIC)
/// #    define VGC_FOO_API
/// #else
/// #    if defined(VGC_FOO_EXPORTS)
/// #        define VGC_FOO_API VGC_CORE_DLL_EXPORT
/// #    else
/// #        define VGC_FOO_API VGC_CORE_DLL_IMPORT
/// #    endif
/// #endif
/// \endcode
///
/// The macro `VGC_FOO_API_HIDDEN` is defined following the same idea.
///

#include <vgc/core/compiler.h>
#include <vgc/core/os.h>

#if defined(VGC_CORE_OS_WINDOWS)
#    if defined(VGC_CORE_COMPILER_GCC) && VGC_CORE_COMPILER_GCC_MAJOR >= 4               \
        || defined(VGC_CORE_COMPILER_CLANG)
#        define VGC_CORE_DLL_EXPORT __attribute__((dllexport))
#        define VGC_CORE_DLL_IMPORT __attribute__((dllimport))
#        define VGC_CORE_DLL_HIDDEN
#    else
#        define VGC_CORE_DLL_EXPORT __declspec(dllexport)
#        define VGC_CORE_DLL_IMPORT __declspec(dllimport)
#        define VGC_CORE_DLL_HIDDEN
#    endif
#    define VGC_CORE_DLL_EXPORT_EXCEPTION
#    define VGC_CORE_DLL_IMPORT_EXCEPTION
#elif defined(VGC_CORE_COMPILER_GCC) && VGC_CORE_COMPILER_GCC_MAJOR >= 4                 \
    || defined(VGC_CORE_COMPILER_CLANG)
#    define VGC_CORE_DLL_EXPORT __attribute__((visibility("default")))
#    define VGC_CORE_DLL_IMPORT __attribute__((visibility("default")))
#    define VGC_CORE_DLL_HIDDEN __attribute__((visibility("hidden")))
#    define VGC_CORE_DLL_EXPORT_EXCEPTION VGC_CORE_DLL_EXPORT
#    define VGC_CORE_DLL_IMPORT_EXCEPTION VGC_CORE_DLL_IMPORT
#else
#    define VGC_CORE_DLL_EXPORT
#    define VGC_CORE_DLL_IMPORT
#    define VGC_CORE_DLL_HIDDEN
#    define VGC_CORE_DLL_EXPORT_EXCEPTION
#    define VGC_CORE_DLL_IMPORT_EXCEPTION
#endif

#endif // VGC_CORE_DLL_H
