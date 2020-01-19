# VGC C++ Style Guide

If something is not addressed here, follow the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
If something is not addressed there, favor readability.

## Header Files

Headers should be self-contained (i.e., they must be compilable by themselves).
This is enforced by including each header first in their corresponding cpp file.

Headers should be guarded with:
```
#ifndef VGC_LIBNAME_HEADERNAME_H
#define VGC_LIBNAME_HEADERNAME_H

...

#endif // VGC_LIBNAME_HEADERNAME_H
```

Try to avoid forward declarations as much as possible.

Always use angle brackets and the full path for header includes: `#include <vgc/libname/headername.h>`

In header files, include other header files in this order:
- Standard library headers
- Headers from third-party libraries
- VGC headers
- VGC internal headers

In a .cpp file, include its corresponding .h first, then follow the same order as above.

Within each category of includes (e.g., "VGC headers"), order alphabetically.

Don't rely on includes from other headers. For example, if you need both
`vgc::geometry::Curve` and `vgc::geometry::Vec2d`, include both
`<vgc/geometry/curve.h>` and `<vgc/geometry/vec2d.h>`. Exception: there is no
need to explicitly include headers which are already in
`<vgc/core/innercore.h>`.

Never use `using lib::foo;` or `using namespace lib;` in a header file at
global or namespace scope (where `lib` is any namespace, including `std`,
`vgc`, `vgc::core`, `Eigen`, etc.). In rare cases, you may use such statements
at function scope, or in a .cpp file, but always preferring the more specific
`using lib::foo;`. An important use case is to take advantage of ADL (=
argument-dependent lookup), such as in `using std::swap;`.

## Scoping

Always scope public API under two namespaces, and comment closing bracket for readability. Do not indent within the namespaces:

```
namespace vgc {
namespace mylib {

class MyClass
{
    ...
};

void myFreeFunction();

} // namespace mylib
} // namespace vgc
```

In .cpp files, prefer using unnamed namespaces rather than static functions
for defining things that aren't visible to other translation units:

```
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

In .h files, encapsulate implementation details in an `internal` namespace.
There is no need to use the `_` suffix for names in the `internal` namespace:

```
namespace internal {

struct HelperStruct {
    int x;
    int y;
};

} // namespace internal
```

Internal .h files may be placed in a separate `internal` folder:
`<vgc/mylib/internal/foo.h>`.


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

## Formatting

Indent with 4 spaces. No tabs.

Function declarations and definitions:

```
void byPtr(Foo* foo);
void byRef(Foo* foo);
void byConstPtr(const Foo* foo);
void byConstRef(const Foo& foo);
```

```
void functionDefinition()
{
    thisIsPreferred();
}
```

```
void butForShortInlineFunctions() {
    thisIsOkayToo();
}
```

```
// Or even a one liner if fits within 80 characters
int getX() { return x_; }
```

Conditional blocks:

```
if (condition) {
    thisIs();
    thePreferredStyle();
}
else {
    forConditional();
    blocks();
}
```

```
for (int i = 0; i < n; ++i) {
    doSomething(i);
}
```

```
for (Node* child : node->children()) {
    doSomething(child);
}
```

```
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

```
if (veryShortOneLiner1) thisIsOkay();
if (veryShortOneLiner2) whenMoreReadable();
if (veryShortOneLiner3) inRareCases();
if (veryShortOneLiner4) whereYouHave();
if (veryShortOneLiner5) manyOfThem();
```

```
if (longBody ||
    multilineCondition)
{
    putBracketOnItsOwnLine();
}
```

Class definitions:

```
class TypicalClass : public BaseClass
{
public:
    void thisIsPreferred();

private:
    int becauseOneMoreLines_;
    int doesntChangeMuch_;
};
```

```
class ShortClassOrStruct {
    ThisIs fine;
};
```

## Templates

Use "typename" rather than "class" for template parameters. Format angle
brackets `<>` as you would parenthesis in a function declaration.

```
template<typename T>
T add(const T& x, const T& y)
{
    return x + y;
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
