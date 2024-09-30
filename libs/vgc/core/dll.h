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
/// explicit function/class/struct/union template instantiations that should be
/// exported:
///
/// - `VGC_FOO_API_DECLARE_TEMPLATE_FUNCTION`
/// - `VGC_FOO_API_DECLARE_TEMPLATE_CLASS`
/// - `VGC_FOO_API_DECLARE_TEMPLATE_STRUCT`
/// - `VGC_FOO_API_DECLARE_TEMPLATE_UNION`
///
/// - `VGC_FOO_API_DEFINE_TEMPLATE_FUNCTION`
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
/// Importantly, this also exports type info (RTTI), which is typically
/// required to be able to wrap abstract classes in Python.
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
/// VGC_FOO_API_DECLARE_TEMPLATE_FUNCTION(float foo<float>());
///
/// // In foo.cpp
///
/// VGC_FOO_API_DEFINE_TEMPLATE_FUNCTION(float foo<float>());
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
/// VGC_FOO_API_DECLARE_TEMPLATE_FUNCTION(float foo(float));
///
/// // In foo.cpp
///
/// VGC_FOO_API_DEFINE_TEMPLATE_FUNCTION(float foo(float));
/// ```
///
/// For more info on explicit function template instantiations, see:
/// - https://en.cppreference.com/w/cpp/language/function_template
///
/// # Exporting class/struct/union templates
///
/// Similarly to function templates, class templates (or struct templates, or
/// union templates) do not need to be exported to be usable in other libraries
/// or applications.
///
/// However, if desired (for example to speed up compilation or reduce object
/// size), it is possible to define explicit class/struct/union template
/// instantiations and export these, using the following syntax:
///
/// ```cpp
/// // In foo.h
///
/// template<typename T>
/// class Foo { /* ... */ };
///
/// template<typename T>
/// struct Bar { /* ... */ };
///
/// template<typename T>
/// union Baz { /* ... */ };
///
/// VGC_FOO_API_DECLARE_TEMPLATE_CLASS(Foo<float>);
/// VGC_FOO_API_DECLARE_TEMPLATE_STRUCT(Bar<float>);
/// VGC_FOO_API_DECLARE_TEMPLATE_UNION(Baz<float>);
///
/// // In foo.cpp
///
/// VGC_FOO_API_DEFINE_TEMPLATE_CLASS(Foo<float>);
/// VGC_FOO_API_DEFINE_TEMPLATE_STRUCT(Bar<float>);
/// VGC_FOO_API_DEFINE_TEMPLATE_UNION(Baz<float>);
/// ```
///
/// For more info on explicit class/struct/union template instantiations, see:
/// - https://en.cppreference.com/w/cpp/language/class_template
///
/// # Exporting enums
///
/// Enums (either `enum` or `enum class`) do not need to be exported to be
/// usable in other libraries or applications. No special markup should be
/// used.
///
/// However, enum meta-data provided by `VGC_DECLARE_ENUM` and
/// `VGC_DEFINE_ENUM` should be exported by marking it like this:
///
/// ```cpp
/// // In foo.h
///
/// enum class MyEnum { /* ... */ };
///
/// VGC_FOO_API
/// VGC_DECLARE_ENUM(MyEnum)
///
/// // In foo.cpp
///
/// VGC_DEFINE_ENUM(MyEnum, /* ... */)
/// ```
///
/// # How to create a new library?
///
/// When creating a new library `vgc/foo`, you should create a `vgc/foo/api.h`
/// file with the following content:
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
/// #  define VGC_FOO_API                                VGC_DLL_STATIC
/// #  define VGC_FOO_API_HIDDEN                         VGC_DLL_STATIC_HIDDEN
/// #  define VGC_FOO_API_DECLARE_TEMPLATE(k, ...)       VGC_DLL_STATIC_DECLARE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE(k, ...)        VGC_DLL_STATIC_DEFINE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DECLARE_TEMPLATE_FUNCTION(...) VGC_DLL_STATIC_DECLARE_TEMPLATE_FUNCTION(__VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE_FUNCTION(...)  VGC_DLL_STATIC_DEFINE_TEMPLATE_FUNCTION(__VA_ARGS__)
/// #  define VGC_FOO_API_EXCEPTION                      VGC_DLL_STATIC_EXCEPTION
/// #elif defined(VGC_FOO_EXPORTS)
/// #  define VGC_FOO_API                                VGC_DLL_EXPORT
/// #  define VGC_FOO_API_HIDDEN                         VGC_DLL_EXPORT_HIDDEN
/// #  define VGC_FOO_API_DECLARE_TEMPLATE(k, ...)       VGC_DLL_EXPORT_DECLARE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE(k, ...)        VGC_DLL_EXPORT_DEFINE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DECLARE_TEMPLATE_FUNCTION(...) VGC_DLL_EXPORT_DECLARE_TEMPLATE_FUNCTION(__VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE_FUNCTION(...)  VGC_DLL_EXPORT_DEFINE_TEMPLATE_FUNCTION(__VA_ARGS__)
/// #  define VGC_FOO_API_EXCEPTION                      VGC_DLL_EXPORT_EXCEPTION
/// #else
/// #  define VGC_FOO_API                                VGC_DLL_IMPORT
/// #  define VGC_FOO_API_HIDDEN                         VGC_DLL_IMPORT_HIDDEN
/// #  define VGC_FOO_API_DECLARE_TEMPLATE(k, ...)       VGC_DLL_IMPORT_DECLARE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE(k, ...)        VGC_DLL_IMPORT_DEFINE_TEMPLATE(k, __VA_ARGS__)
/// #  define VGC_FOO_API_DECLARE_TEMPLATE_FUNCTION(...) VGC_DLL_IMPORT_DECLARE_TEMPLATE_FUNCTION(__VA_ARGS__)
/// #  define VGC_FOO_API_DEFINE_TEMPLATE_FUNCTION(...)  VGC_DLL_IMPORT_DEFINE_TEMPLATE_FUNCTION(__VA_ARGS__)
/// #  define VGC_FOO_API_EXCEPTION                      VGC_DLL_IMPORT_EXCEPTION
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
/// Essentially, the macro `VGC_FOO_API` expands to the appropriate
/// compiler-dependent symbol visibility attribute (such as
/// `__declspec(dllexport)`, `__declspec(dllimport)`, and
/// `__attribute__((visibility("default")))`), based on whether the library is
/// being compiled or is being used by another library.
///
/// For example, on Windows, when compiling the `foo` library as a shared
/// library (DLL) with the Microsoft Visual C++ Compiler (MSVC), the following
/// code:
///
/// ```cpp
/// class VGC_FOO_API Foo {
///     // ...
/// };
/// ```
///
/// expands to:
///
/// ```cpp
/// class __declspec(dllexport) Foo {
///     // ...
/// };
/// ```
///
/// But when compiling another library or application that uses the `foo`
/// library, it instead expands to:
///
/// ```cpp
/// class __declspec(dllimport) Foo {
///     // ...
/// };
/// ```
///
/// And if compiling the `foo` library as a static library, it expands to:
///
/// ```cpp
/// class Foo {
///     // ...
/// };
/// ```
///
/// This behavior is made possible by the provided CMake build system that
/// defines `VGC_FOO_EXPORTS` when compiling `foo` as a shared library, defines
/// `VGC_FOO_STATIC` when compiling `foo` as a static library, but defines none
/// of them when simply using the library. This allows `VGC_FOO_API` to be
/// defined differently in these three scenarios. A simplified implementation
/// that only works for MSVC would look like this:
///
/// ```cpp
/// #if defined(VGC_FOO_STATIC)
/// #    define VGC_FOO_API
/// #elif defined(VGC_FOO_EXPORTS)
/// #    define VGC_FOO_API __declspec(dllexport)
/// #else
/// #    define VGC_FOO_API __declspec(dllimport)
/// #endif
/// ```
///
/// In practice, different compilers and/or target platforms require the use of
/// different attributes and possibly at different places (e.g., for template
/// classes).
///
/// This is why in `vgc/foo/api.h`, we prefer not to explicitly use
/// compiler-dependent attributes such as `__declspec(dllexport)`, but instead
/// use the intermediate macros `VGC_DLL_...` (defined in `vgc/core/dll.h`),
/// like so:
///
/// ```cpp
/// #if defined(VGC_FOO_STATIC)
/// #    define VGC_FOO_API VGC_DLL_STATIC
/// #elif defined(VGC_FOO_EXPORTS)
/// #    define VGC_FOO_API VGC_DLL_EXPORT
/// #else
/// #    define VGC_FOO_API VGC_DLL_IMPORT
/// #endif
/// ```
///
/// For example, on Windows, `VGC_DLL_EXPORT` expands to
/// `__declspec(dllexport)` when compiling with MSVC, but it expands to
/// `__attribute__((dllexport))` when compiling with Clang or GCC. And it
/// expands to `__attribute__((visibility("default")))` when compiling on macOS
/// or Linux with Clang or GCC.
///
/// This design makes it possible to easily add support for different compilers
/// or platforms by only updating `vgc/core/dll.h`, without having to change all
/// the `vgc/foo/api.h` files.
///
/// All the other macros (`VGC_FOO_API_DECLARE_TEMPLATE_CLASS`, etc.) are
/// defined following the same idea. They take care of adding the appropriate
/// keywords (e.g., `extern template`) and attributes (e.g.,
/// `__declspec(dllexport)`) at the appropriate places, depending on whether
/// the library is being compiled or being used, and depending on the current
/// compiler and target platform.
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
