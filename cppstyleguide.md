# VGC C++ Style Guide

If something is not addressed here, follow the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
If something is not addressed there, favor readability.

VGC uses clang-format to automatically format the code. See the `.clang-format` file in the `libs` and `apps` directory. We recommend to set up your IDE such that it automatically calls `clang-format` on save.

## Header Files

Headers should be self-contained (i.e., they must be compilable by themselves).
This is enforced by including each header first in their corresponding cpp file.

Headers should be guarded with:
```cpp
#ifndef VGC_LIBNAME_HEADERNAME_H
#define VGC_LIBNAME_HEADERNAME_H

...

#endif // VGC_LIBNAME_HEADERNAME_H
```

Try to avoid forward declarations as much as possible.

Always use angle brackets and the full path for header includes: `#include <vgc/libname/headername.h>`

Organize headers in the following groups, and include them in the following order:
- If in a `*.cpp` file: Corresponding `.h` file
- Standard library headers
- Headers from third-party libraries
- VGC headers
- VGC `detail` headers

Separate groups with a blank line. Within each group, order alphabetically.
Note that clang-format reorders automatically, so make sure to separate groups by a blank line even
if there is only one header in a given group.

```cpp
// In lineedit.cpp:

#include <vgc/ui/lineedit.h>

#include <algorithm>

#include <QClipboard>
#include <QGuiApplication>

#include <vgc/core/array.h>
#include <vgc/core/colors.h>
#include <vgc/core/performancelog.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>
```

Don't rely on includes from other headers. For example, if you need both
`vgc::geometry::Curve` and `vgc::geometry::Vec2d`, include both
`<vgc/geometry/curve.h>` and `<vgc/geometry/vec2d.h>`.

Never use `using lib::foo;` or `using namespace lib;` in a header file at
global or namespace scope (where `lib` is any namespace, including `std`,
`vgc`, `vgc::core`, `Eigen`, etc.). In rare cases, you may use such statements
at function scope, or in a .cpp file, but always preferring the more specific
`using lib::foo;`. An important use case is to take advantage of ADL (=
argument-dependent lookup), such as in `using std::swap;`.

## Scoping

Always scope public API under two namespaces, and comment closing bracket for readability. Do not indent within the namespaces:

```cpp
namespace vgc::mylib {

class MyClass {
    ...
};

void myFreeFunction();

} // namespace vgc::mylib
```

In .cpp files, prefer using unnamed namespaces rather than static functions
for defining things that aren't visible to other translation units:

```cpp
namespace {

void helperFunction_() {
    doSomething();
}

struct HelperStruct_ {
    int x;
    int y;
};

} // namespace
```

In .h files, encapsulate implementation details in a `detail` namespace.
There is no need to use the `_` suffix for names in the `detail` namespace:

```cpp
namespace detail {

struct HelperStruct {
    int x;
    int y;
};

} // namespace detail
```

Internal .h files may be placed in a separate `detail` folder:
`<vgc/mylib/detail/foo.h>`.

Prefer `enum class` rather than C-style `enum`.

## Naming

Naming matters: if you add functions/classes to the public API, expect a discussion about their names.

Prefer long descriptive names than abbreviations, but use common sense:
if a name is expected to be used extremely often (e.g., Vec2d), then shortening makes sense.

Capitalization:
```
libraryname
headerfilename.h
cppfilename.cpp
ClassName
publicFunctionName()
protectedFunctionName()  (underscore allowed for "unsafe" protected methods)
privateFunctionName_()
privateMemberVariableName_
localVariable
VGC_MYLIB_PUBLICMACRO
VGC_MYLIB_PRIVATEMACRO_
enum class EnumName {
    EnumValue1,
    EnumValue2
};
```

When using acronyms, do not capitalize the whole acronym. In order words, use `XmlClass` instead of `XMLClass`.

## Formatting

Indent with 4 spaces. No tabs. Wrap lines at 90 characters.

Function declarations:

```cpp
void byPtr(Foo* foo);
void byRef(Foo& foo);
void byConstPtr(const Foo* foo);
void byConstRef(const Foo& foo);
void byRValue(Foo&& foo);
```

Function definition:

```cpp
void someFunction() {
    doSomething();
    doSomethingElse();
}
```

```cpp
void fuctionWithoutBody() {
}
```

If the function params do not all fit in one line, then each should be in a separate line,
and the function body (if there is one) should be separated with a blank line:

```cpp
void functionWithLongParams(
    int longParam1,
    int longParam2,
    int longParam3) {
    
    doSomething();
}
```

If the function signature is on a single line and the function body doesn't
have any blank lines, then do not add a blank line between the function signature 
and its body:

```cpp
void functionSignatureFitsInOneLine(int x, int y) {
    noBlankLinesHere(x);
    noBlankLinesHere(y);
}
```

Otherwise (i.e., if the functions params do not fit in one line, or if there are blank
lines in the function body), then you should add a blank line (otherwise, the first "block"
in the body looks like function params).

```cpp
void functionWhoseBodyHasNewlines() {   

    // First pass
    doSomething();
    doSomethingElse();
    
    // Second pass
    redoSomething();
    redoSomethingElse();
}
```

Constructor initializer lists:

Each initialization is on a separate line.
Use a blank line to separate the initializer list from the function body (if any).
If the function parameters do not fit on a single line,
use a blank line to separate the function parameters and the initializer list.

```cpp
SomeConstructor()
    : i_(0) {
}

SomeConstructor(int param1, int param2)
    : param1_(param1)
    , param2_(param2) {
}

SomeVeryLongConstructor(
    int veryLongParam1,
    int veryLongParam2)
    
    : veryLongParam1_(veryLongParam1)
    , veryLongParam2_(veryLongParam2) {
    
    doSomething();
}
```

Conditional blocks:

```cpp
if (condition) {
    thisIs();
    thePreferredStyle();
}
else {
    forIf();
    blocks();
}

while (condition) {
    thisIs();
    thePreferredStyle();
    forWhileBlocks();
}

do {
    thisIs();
    thePreferredStyle();
    forDoWhileBlocks();
} while (condition);

for (int i = 0; i < n; ++i) {
    doSomething(i);
}

for (Node* child : node->children()) {
    doSomething(child);
}

switch (value) {
case Enum::Value1:
    thisIs();
    thePreferredStyle();
    break;
case Enum::Value2:
    forSwitch();
    blocks();
    break;
default:
   break;
}
```

Conditional blocks without braces are forbidden. Conditional blocks that could fit in one line should still use 3 lines instead:

```
if (!valid()) {
    return;
}
```

In very exceptional cases, derogations to these rules can be made when it significantly 
improves readability, such as data-oriented code, in which case one surround the code 
with `// clang-format off` and `// clang-format on`.

Class definitions (typical layout):

```cpp
class VGC_MYLIB_API DerivedClass : public BaseClass {
public:
    void DerivedClass(); // constructors go first
    void foo();          // then public methods

protected:
    void bar();          // then protected methods
    
private:
    int x_;              // then private member variables
    int baz_();          // then private methods
};
```

Class definitions (special case of `vgc::core::Object`):

```cpp
class VGC_UI_API Flex : public Widget {
private: // the VGC_OBJECT macro goes first
    VGC_OBJECT(Flex, Widget)

protected: // then protected constructors                   
    Flex(FlexDirection direction = FlexDirection::Row);

public:
    // then create(...) static functions
    static FlexPtr create(FlexDirection direction = FlexDirection::Row);
            
    // then other public methods
    FlexDirection direction() const { 
        return direction_;
    }
    
protected: // then other protected methods
    void onWidgetAdded(Widget* child) override;
    void onWidgetRemoved(Widget* child) override;

private:: // then private member variables and methods
    FlexDirection direction_;
}
```

## Templates

Use "typename" rather than "class" for template parameters. Format angle
brackets `<>` as you would parenthesis in a function declaration.

```cpp
template<typename T>
T add(const T& x, const T& y) {
    return x + y;
}
```

Template metaprogramming code is more prone to become less readable when
formatted with `clang-format`, so use `clang-format off` whenever relevant.
In general, try to conform to the style found in `vgc/core/templateutil.h`, 
`vgc/core/arithmetic.h` or `vgc/core/array.h`, but it's okay to deviate a
little if it improves readability.

Examples of style you should try to adhere to:

```cpp
template<typename T, typename SFINAE = void>
struct IsForwardIterator : std::false_type {};

template<typename T>
struct IsForwardIterator<T, Requires<
    std::is_convertible_v<
        typename std::iterator_traits<T>::iterator_category,
        std::forward_iterator_tag>>>
    : std::true_type {};

template<typename T>
inline constexpr bool isForwardIterator = IsForwardIterator<T>::value;

template<typename T>
class Array {

    // ...

    template<typename IntType, VGC_REQUIRES(isSignedInteger<IntType>)>
    Array(IntType length, const T& value) {
        checkLengthForInit_(length);
        init_(length, value);
    }
};

template<typename T, typename U, VGC_REQUIRES(
    std::is_integral_v<T>
    && std::is_integral_v<U>
    && std::is_signed_v<T>
    && std::is_unsigned_v<U>
    && tmax<T> >= tmax<U>)> // (2)
T int_cast(U value) {
    return static_cast<T>(value);
}
```

## Polymorphic Classes

A class is said *polymorphic* if it declares or inherits at least one virtual method.

The destructor of any polymorphic class must be either public and virtual, or protected and
non-virtual, in order to ensure that deleting derived objects through pointers to base
is well-defined. Always define this destructor out-of-line (= in a *.cpp file), in order
to satisfy the following
[Clang's Coding Standard](http://llvm.org/docs/CodingStandards.html#provide-a-virtual-method-anchor-for-classes-in-headers):

> **Provide a Virtual Method Anchor for Classes in Headers**
> If a class is defined in a header file and has a vtable (either it has virtual methods
> or it derives from classes with virtual methods), it must always have at least one out-of-line
> virtual method in the class. Without this, the compiler will copy the vtable and RTTI into
> every .o file that #includes the header, bloating .o file sizes and increasing link times.

## Other

Use UTF-8 encoding for all std::strings.
