# VGC C++ Style Guide

Try to follow these guidelines as much as reasonable. Don't overthink it (too much). 
If something is not addressed here, use the 
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

#endif  // VGC_LIBNAME_HEADERNAME_H
```

Try to avoid forward declaration as much as possible.

Always use angle brackets and the full path for header includes: `#include <vgc/libname/headername.h>`

In header files, include other header files in this order: std, Qt, otherthirdlibs, vgc

In `vgc/mylib/foo.cpp`, include `<vgc/mylib/foo.h>` first, then all other header files as specified above.

Within each "group" (e.g., Qt includes), order alphabetically.

Don't rely on other headers includes. That is, if you need both 
`vgc::geometry::Curve` and `vgc::geometry::Vec2d`, include both 
`<vgc/geometry/curve.h>` and `<vgc/geometry/vec2d.h>`

Never use `using somenamespace::SomeClass;` in a header file, and more
importantly never use `using somenamespace` in a header file. Yes, this is annoying, 
and I hate it too. Please blame C++, not me.

## Scoping

Always scope under two namespaces: `vgc::mylib::MyClass`.

Use unnamed namespaces in .cpp files for defining anything that isn't visible to other translation units.

Prefer `enum class` rather than C-style `enum`.

## Naming

Prefer long descriptive names than abbreviation, but use common sense: 
if a name is expected to be used a lot (e.g., Vec2d), then shortening makes sense.

Capitalization convention:

`libraryname`

`headerfilename.h`

`cppfilename.cpp`

`ClassName`

`publicFunctionName()`

`privateFunctionName_()`

`privateMemberVariableName_`.

```
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
if (justOneLine)
    thisIsOkay();
```

```
if (oneShortLine1) thisIsEvenOkay();
if (oneShortLine2) especially();
if (oneShortLine3) ifMoreReadable();
if (oneShortLine4) andThereAre();
if (oneShortLine5) manyOfThem();
```

```
if (typicalCase) {
    useThis();
    likeThat();
}
else {
    andThis();
    andThat();
}
```

```
if (longBody ||
    multilineCondition)
{
     putBracketOnItsOwnLine();
}
```

```
class ShortClassOrStruct {
    ThisIs fine;
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
void functionDefinition()
{
    preferThis();
}
```

```
void butForShortInlineFunctions() {
    thisIsOkayToo();
}
```

```
// Or even a one liner for obvious inline getters
int getX() { return x_; }
```

## Other

Use UTF-8 encoding for all std::strings.
