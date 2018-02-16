# VGC C++ Style Guide

Try to follow these guidelines as much as reasonable. Don't overthink it (too much). 
If something is not addressed here, follow the 
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
If something is not addressed there, favour readability.
In case of doubt, follow your heart :)

## Header Files

Headers should be self-contained.

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

Don't rely on other includes from other headers. For example, if you need both 
`vgc::geometry::Curve` and `vgc::geometry::Vec2d`, include both 
`<vgc/geometry/curve.h>` and `<vgc/geometry/vec2d.h>`

Never use `using somenamespace::SomeClass;` in a header file, and more
importantly never use `using somenamespace` in a header file. Yes, this is annoying, 
and I hate it too. Please blame C++, not me.

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

Prefer long descriptive names than abbreviations, but use common sense: 
if a name is expected to be used extremely often (e.g., Vec2d), then shortening makes sense.

Don't overthink naming for anything which is private. However, expect a discussion during
a pull request for all names which are added to the public API.

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
VGC_MYLIB_PRIVATEMACRO
enum class EnumName {
    EnumValue1,
    EnumValue2
};
```

## Formatting

Indent with 4 spaces. No tabs.

Good: `Foo* foo` , `Foo& foo`, `const Foo* foo` , `const Foo& foo`,

Bad: `Foo *foo`, `Foo const& foo`

Curly brackets:

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

```
class TypicalClass
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
// Or even a one liner for short one-liners
int getX() { return x_; }
```

## Other

Use UTF-8 encoding for all std::strings.
