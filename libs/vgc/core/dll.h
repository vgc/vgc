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
/// The VGC codebase is made of several libraries ("core", "geometry", etc.).
/// Each library is meant to be compiled and linked as a shared library (e.g.,
/// libvgccore.so, libvgcgeometry.so, etc.), which is the behavior you get by
/// default when compiling using the provided CMake build system.
///
/// In order to decrease load time of the shared library, improve runtime
/// performance, and reduce the chance of name collisions, it is important to
/// only export in the shared object the symbols which are part of the public
/// API of the library. For more details, please refer to:
///
/// - https://gcc.gnu.org/wiki/Visibility
/// - https://www.akkadia.org/drepper/dsohowto.pdf
///
/// To achieve this, each library `foo` has a header file `vgc/foo/api.h`
/// which defines the following macros:
///
/// - `VGC_FOO_API`: specifies that a class or function should be exported
/// - `VGC_FOO_HIDDEN`: specifies that a class method shouldn't be exported
/// - `VGC_FOO_API_EXCEPTION`: same as `VGC_FOO_API` but for exception classes
///
/// In addition, the file `vgc/foo/api.h` provides macros to declare and define
/// explicit class/struct/union template instantiations that should be
/// exported:
///
/// - `VGC_FOO_API_DECLARE_TEMPLATE_CLASS`
/// - `VGC_FOO_API_DECLARE_TEMPLATE_STRUCT`
/// - `VGC_FOO_API_DECLARE_TEMPLATE_UNION`
///
/// - `VGC_FOO_API_DEFINE_TEMPLATE_CLASS`
/// - `VGC_FOO_API_DEFINE_TEMPLATE_STRUCT`
/// - `VGC_FOO_API_DEFINE_TEMPLATE_UNION`
///
/// All of these macros are explained in more details below.
///
/// Note that `vgc_add_library()` from the provided CMake build system takes
/// care of using the `-fvisibility=hidden` option when compiling on Linux and
/// MacOS. This ensures that no symbols are exported by default unless
/// explicitly marked with `VGC_FOO_API`, etc. This matches the behaviour of
/// Windows DLLs, therefore avoiding potential surprises when developing on
/// Linux then compiling on Windows.
///
/// # Exporting a free function
///
/// The macro `VGC_FOO_API` can be used to specify that a free function should
/// be exported:
///
/// ```cpp
/// VGC_FOO_API
/// void foo();
/// ```
///
/// By default, a free function is not exported unless specified otherwise.
///
/// Note that free functions marked `inline` never need to be exported to be
/// usable in other libraries or applications.
///
/// # Exporting a class
///
/// The macro `VGC_FOO_API` can be used to specify that all methods of a class
/// should be exported:
///
/// ```cpp
/// class VGC_FOO_API Foo {
///     // ...
/// };
/// ```
///
/// Importantly, this also export type info (RTTI), which is typically required
/// to be able to wrap abstract classes in Python.
///
/// If for some reason you desire not to export a specific method, you can use
/// `VGC_FOO_HIDDEN`:
///
/// ```cpp
/// class VGC_FOO_API Foo {
///     void bar(); // exported
///
///     VGC_FOO_HIDDEN
///     void baz(); // not exported
/// };
/// ```
///
/// # Exporting an exception class
///
/// Exception classes are a bit special and require special markup. You
/// should use `VGC_FOO_API_EXCEPTION` to export an exception class:
///
/// ```cpp
/// class VGC_FOO_API_EXCEPTION FooError : public std::logic_error {
///     // ...
/// };
///
/// class VGC_FOO_API_EXCEPTION BarError : public vgc::core::LogicError {
///     // ...
/// };
/// ```
///
/// # Exporting function templates
///
/// Similarly to inline functions, function templates do not need to be
/// exported to be usable in other libraries or applications.
///
/// However, if desired (for example to speed up compilation or reduce object
/// size), it is possible to define explicit function template instantiations,
/// and export these, using the following syntax:
///
/// ```cpp
/// // In foo.h
///
/// template<typename T>
/// T foo() {
///    // ...
/// }
///
/// extern template VGC_FOO_API
/// float foo<float>();
///
/// // In foo.cpp
///
/// template
/// float foo<float>();
/// ```
///
/// Note that the template arguments can be omitted if they can be deduced from
/// the function arguments:
///
/// ```cpp
/// // In foo.h
///
/// template<typename T>
/// T foo(T x) {
///    // ...
/// }
///
/// extern template VGC_FOO_API
/// float foo(float);
///
/// // In foo.cpp
///
/// template
/// float foo(float);
/// ```
///
/// For more info on explicit function template instantiation, see:
/// - https://en.cppreference.com/w/cpp/language/function_template
///
/// # Exporting class templates
///
/// Similarly to function templates, class templates (or struct templates, or
/// union templates) do not need to be exported to be usable in other libraries
/// or applications.
///
/// However, if desired (for example to speed up compilation or reduce object
/// size), it is possible to define explicit class template instantiations and
/// export these, using the following syntax:
///
/// ```cpp
/// // In foo.h
///
/// template<typename T>
/// class Foo {
///    // ...
/// }
///
/// VGC_FOO_API_DECLARE_TEMPLATE_CLASS(Foo<float>);
///
/// // In foo.cpp
///
/// VGC_FOO_API_DEFINE_TEMPLATE_CLASS(Foo<float>);
/// ```
///
/// For more info on explicit class template instantiation, see:
/// - https://en.cppreference.com/w/cpp/language/class_template
///
/// # How to create a new library?
///
/// When creating a new library `foo`, you should create a `vgc/foo/api.h` file
/// with the following content:
///
/// ```cpp
/// #ifndef VGC_FOO_API_H
/// #define VGC_FOO_API_H
///
/// #include <vgc/core/dll.h>
///
/// // clang-format off
///
/// #if defined(VGC_FOO_STATIC)
/// #  define VGC_FOO_API                          VGC_DLL_STATIC
/// #  define VGC_FOO_API_HIDDEN                   VGC_DLL_STATIC_HIDDEN
/// #  define VGC_FOO_API_DECLARE_TEMPLATE(k, ...) VGC_DLL_STATIC_DECLARE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE(k, ...)  VGC_DLL_STATIC_DEFINE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_EXCEPTION                VGC_DLL_STATIC_EXCEPTION
/// #elif defined(VGC_FOO_EXPORTS)
/// #  define VGC_FOO_API                          VGC_DLL_EXPORT
/// #  define VGC_FOO_API_HIDDEN                   VGC_DLL_EXPORT_HIDDEN
/// #  define VGC_FOO_API_DECLARE_TEMPLATE(k, ...) VGC_DLL_EXPORT_DECLARE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE(k, ...)  VGC_DLL_EXPORT_DEFINE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_EXCEPTION                VGC_DLL_EXPORT_EXCEPTION
/// #else
/// #  define VGC_FOO_API                          VGC_DLL_IMPORT
/// #  define VGC_FOO_API_HIDDEN                   VGC_DLL_IMPORT_HIDDEN
/// #  define VGC_FOO_API_DECLARE_TEMPLATE(k, ...) VGC_DLL_IMPORT_DECLARE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE(k, ...)  VGC_DLL_IMPORT_DEFINE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_EXCEPTION                VGC_DLL_IMPORT_EXCEPTION
/// #endif
/// #define VGC_FOO_API_DECLARE_TEMPLATE_CLASS(...)  VGC_FOO_API_DECLARE_TEMPLATE(class, __VA_ARGS__)
/// #define VGC_FOO_API_DECLARE_TEMPLATE_STRUCT(...) VGC_FOO_API_DECLARE_TEMPLATE(struct, __VA_ARGS__)
/// #define VGC_FOO_API_DECLARE_TEMPLATE_UNION(...)  VGC_FOO_API_DECLARE_TEMPLATE(union, __VA_ARGS__)
/// #define VGC_FOO_API_DEFINE_TEMPLATE_CLASS(...)   VGC_FOO_API_DEFINE_TEMPLATE(class, __VA_ARGS__)
/// #define VGC_FOO_API_DEFINE_TEMPLATE_STRUCT(...)  VGC_FOO_API_DEFINE_TEMPLATE(struct, __VA_ARGS__)
/// #define VGC_FOO_API_DEFINE_TEMPLATE_UNION(...)   VGC_FOO_API_DEFINE_TEMPLATE(union, __VA_ARGS__)
///
/// #endif // VGC_FOO_API_H
/// ```
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
/// ```cpp
/// #if defined(VGC_FOO_EXPORTS)
/// #    define VGC_FOO_API __declspec(dllexport)
/// #else
/// #    define VGC_FOO_API __declspec(dllimport)
/// #endif
/// ```
///
/// Then simply use the following in our header files:
///
/// ```cpp
/// class VGC_FOO_API Foo {
///     // ...
/// };
/// ```
///
/// When you are compiling the module `foo`, the above code expands to:
///
/// ```cpp
/// class __declspec(dllexport) Foo {
///     // ...
/// };
/// ```
///
/// But when you are compiling another module `bar` which includes a
/// header file from `foo`, it expands to the following instead:
///
/// ```cpp
/// class __declspec(dllimport) Foo {
///     // ...
/// };
/// ```
///
/// However, things are slightly more complex. For example, still on Windows,
/// but compiling with GCC or Clang, the syntax is different: we should use
/// `__attribute__((dllexport))` instead of `__declspec(dllexport)` (same for
/// `dllimport`). The syntax is also different when compiling on Linux or
/// MacOS. This is why, instead of defining the following:
///
/// ```cpp
/// #if defined(VGC_FOO_EXPORTS)
/// #    define VGC_FOO_API __declspec(dllexport)
/// #else
/// #    define VGC_FOO_API __declspec(dllimport)
/// #endif
/// ```
///
/// You should instead use `VGC_DLL_EXPORT` and `VGC_DLL_IMPORT`
/// provided in this file, like so:
///
/// ```cpp
/// #if defined(VGC_FOO_EXPORTS)
/// #    define VGC_FOO_API VGC_DLL_EXPORT
/// #else
/// #    define VGC_FOO_API VGC_DLL_IMPORT
/// #endif
/// ```
///
/// In addition, what if a client desires to compile `foo` as a static library
/// instead of a shared library? In this case, `VGC_FOO_API` should expands to
/// nothing instead, and we allow clients to switch to this mode by defining
/// `VGC_FOO_STATIC` using their build system:
///
/// ```cpp
/// #if defined(VGC_FOO_STATIC)
/// #    define VGC_FOO_API VGC_DLL_STATIC
/// #else
/// #    if defined(VGC_FOO_EXPORTS)
/// #        define VGC_FOO_API VGC_DLL_EXPORT
/// #    else
/// #        define VGC_FOO_API VGC_DLL_IMPORT
/// #    endif
/// #endif
/// ```
///
/// The macro `VGC_FOO_API_HIDDEN` is defined following the same idea.
///
/// Unfortunately, compiler are inconsistent with respect to how class
/// templates instantiations should be annotated in order to be exported. This
/// is why we also define special macros for these. For more details, see:
/// https://github.com/vgc/vgc/pull/1896.
///

#include <vgc/core/defs.h>

// The outer ifndef VGC_DLL_EXPORT allows client code (or a build system) to
// provide their own definition of VGC_DLL_EXPORT, VGC_DLL_IMPORT, etc., for
// example for a compiler that we do not support.

// Note: the `k` macro argument means "class-key". Can be `class`, `struct`, or `union`.
// See:
// - https://en.cppreference.com/w/cpp/language/class
// - https://en.cppreference.com/w/cpp/language/class_template

// clang-format off

#ifndef VGC_DLL_EXPORT
#  if defined(VGC_OS_WINDOWS)
#    if defined(VGC_COMPILER_MSVC)
#      define VGC_DLL_EXPORT                          __declspec(dllexport)
#      define VGC_DLL_IMPORT                          __declspec(dllimport)
#      define VGC_DLL_EXPORT_DECLARE_TEMPLATE(k, ...) extern template k __VA_ARGS__
#      define VGC_DLL_IMPORT_DECLARE_TEMPLATE(k, ...) extern template k VGC_DLL_IMPORT __VA_ARGS__
#      define VGC_DLL_EXPORT_DEFINE_TEMPLATE(k, ...)  template k VGC_DLL_EXPORT __VA_ARGS__
#      define VGC_DLL_IMPORT_DEFINE_TEMPLATE(k, ...)  template k __VA_ARGS__
#    else // GCC, Clang, and possibly others
#      define VGC_DLL_EXPORT                          __attribute__((dllexport))
#      define VGC_DLL_IMPORT                          __attribute__((dllimport))
#      define VGC_DLL_EXPORT_DECLARE_TEMPLATE(k, ...) extern template k VGC_DLL_EXPORT __VA_ARGS__
#      define VGC_DLL_IMPORT_DECLARE_TEMPLATE(k, ...) extern template k VGC_DLL_IMPORT __VA_ARGS__
#      define VGC_DLL_EXPORT_DEFINE_TEMPLATE(k, ...)  template k __VA_ARGS__
#      define VGC_DLL_IMPORT_DEFINE_TEMPLATE(k, ...)  template k __VA_ARGS__
#    endif
#    define VGC_DLL_EXPORT_HIDDEN
#    define VGC_DLL_IMPORT_HIDDEN
#    define VGC_DLL_EXPORT_EXCEPTION
#    define VGC_DLL_IMPORT_EXCEPTION
#  else // Linux or macOS with GCC or Clang, and possibly others
#    define VGC_DLL_EXPORT                          __attribute__((visibility("default")))
#    define VGC_DLL_IMPORT                          __attribute__((visibility("default")))
#    define VGC_DLL_EXPORT_HIDDEN                   __attribute__((visibility("hidden")))
#    define VGC_DLL_IMPORT_HIDDEN                   __attribute__((visibility("hidden")))
#    define VGC_DLL_EXPORT_DECLARE_TEMPLATE(k, ...) extern template k VGC_DLL_EXPORT __VA_ARGS__
#    define VGC_DLL_IMPORT_DECLARE_TEMPLATE(k, ...) extern template k VGC_DLL_IMPORT __VA_ARGS__
#    define VGC_DLL_EXPORT_DEFINE_TEMPLATE(k, ...)  template k __VA_ARGS__
#    define VGC_DLL_IMPORT_DEFINE_TEMPLATE(k, ...)  template k __VA_ARGS__
#    define VGC_DLL_EXPORT_EXCEPTION                VGC_DLL_EXPORT
#    define VGC_DLL_IMPORT_EXCEPTION                VGC_DLL_IMPORT
#  endif
#  define VGC_DLL_STATIC
#  define VGC_DLL_STATIC_DECLARE_TEMPLATE(k, ...) extern template k __VA_ARGS__
#  define VGC_DLL_STATIC_DEFINE_TEMPLATE(k, ...)  template k __VA_ARGS__
#  define VGC_DLL_STATIC_EXCEPTION
#  define VGC_DLL_STATIC_HIDDEN
#  define VGC_DLL_EXPORT_DECLARE_TEMPLATE_FUNCTION(...) extern template VGC_DLL_EXPORT __VA_ARGS__
#  define VGC_DLL_IMPORT_DECLARE_TEMPLATE_FUNCTION(...) extern template VGC_DLL_IMPORT __VA_ARGS__
#  define VGC_DLL_STATIC_DECLARE_TEMPLATE_FUNCTION(...) extern template VGC_DLL_STATIC __VA_ARGS__
#  define VGC_DLL_EXPORT_DEFINE_TEMPLATE_FUNCTION(...)  template __VA_ARGS__
#  define VGC_DLL_IMPORT_DEFINE_TEMPLATE_FUNCTION(...)  template __VA_ARGS__
#  define VGC_DLL_STATIC_DEFINE_TEMPLATE_FUNCTION(...)  template __VA_ARGS__
#endif

#endif // VGC_CORE_DLL_H
