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

In header files, include other header files in this order: std, Qt, otherthirdlibs, vgc

In cpp files, include corresponding header first, then all other header files as specified above.

Within each "group" (e.g., Qt includes), order alphabetically.

Don't rely on includes from other headers. For example, if you need both
`vgc::geometry::Curve` and `vgc::geometry::Vec2d`, include both
`<vgc/geometry/curve.h>` and `<vgc/geometry/vec2d.h>`

Never use `using somenamespace::SomeClass;` in a header file, and more
importantly never use `using somenamespace` in a header file.

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
namespace { void helperFunction_() {
    doSomething();
}}

namespace { struct HelperStruct_ {
    int x;
    int y;
};}
```

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
if (justOneStatement)
    thisIsOkayToo();
```

```
if (veryShortOneLiner1) evenThisIsOkay();
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
}
```

```
class ShortClassOrStruct {
    ThisIs fine;
}
```

## Other

Use UTF-8 encoding for all std::strings.
