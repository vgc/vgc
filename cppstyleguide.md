# VGC C++ Style Guide

If something is not addressed here, follow the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
If something is not addressed there, favor readability.

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

```cpp
namespace vgc::mylib

class MyClass
{
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

In .h files, encapsulate implementation details in an `internal` namespace.
There is no need to use the `_` suffix for names in the `internal` namespace:

```cpp
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

## Formatting

Indent with 4 spaces. No tabs.

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
void largeOrNonInlineFunction()
{
    thisIsPreferred();
}

void shortInlineFunction() {
    thisIsOkay();
}

void veryShortInlineFunction() { thisIsOkay(); }
```

Constructors:

```cpp
LargeOrNonInlineConstructor::LargeOrNonInlineConstructor(
    int veryLongParam1,
    int veryLongParam2) :
    
    BaseClass(),
    veryLongParam1_(veryLongParam1),
    veryLongParam2_(veryLongParam2)
{
    doSomething();
}

LargeOrNonInlineConstructor::LargeOrNonInlineConstructor(int shortParam1, int shortParam2) :
    BaseClass(),
    shortParam1_(shortParam1), shortParam2_(shortParam2)
{
    doSomething();
}

LargeOrNonInlineConstructor::LargeOrNonInlineConstructor(int shortParam1, int shortParam2) :
    shortParam1_(shortParam1), shortParam2_(shortParam2)
{
    doSomething();
}

NonInlineConstructorWithoutBody::NonInlineConstructorWithoutBody(int x) :
    x_(x)
{
    
}
    
InlineConstructorWithLongParams(int x, int y, int z, int w) :
    x_(x), y_(y), z_(z), w_(w)
{
    doSomething();
}

InlineConstructorWithLongParamsWithoutBody(int x, int y, int z, int w) :
    x_(x), y_(y), z_(z), w_(w) {}

InlineConstructorWithShortParams(int x) : x_(x) {
    doSomething();
    doSomethingElse();
}

InlineConstructorWithShortParamsAndShortBody(double x) : x_(x) { doSomething(); }

InlineConstructorWithShortParamsWithoutBody(double x) : x_(x) {}
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

if (veryShortOneLiner1) thisIsOkay();
if (veryShortOneLiner2) whenMoreReadable();
if (veryShortOneLiner3) inRareCases();
if (veryShortOneLiner4) whereYouHave();
if (veryShortOneLiner5) manyOfThem();

if (longBody ||
    multilineCondition)
{
    putBracketOnItsOwnLine();
}
```

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
    Flex(FlexDirection direction = FlexDirection::Row,
         FlexWrap wrap = FlexWrap::NoWrap);

public:
    // then create(...) static functions
    static FlexPtr create(
            FlexDirection direction = FlexDirection::Row,
            FlexWrap wrap = FlexWrap::NoWrap);
            
    // then other public methods
    FlexDirection direction() const { 
        return direction_;
    }
    
protected: // then other protected methods
    void onWidgetAdded(Object* child) override;
    void onWidgetRemoved(Object* child) override;

private:: // then private member variables and methods
    FlexDirection direction_;
    FlexWrap wrap_;
}
```

## Templates

Use "typename" rather than "class" for template parameters. Format angle
brackets `<>` as you would parenthesis in a function declaration.

```cpp
template<typename T>
T add(const T& x, const T& y)
{
    return x + y;
}
```

In general, try to conform to the style found in `vgc/core/templateutil.h`, `vgc/core/arithmetic.h` or `vgc/core/array.h`.

Examples:

```cpp
template<typename T, typename Enable = bool>
struct IsCallable : std::false_type {};

template<typename T>
struct IsCallable<T, RequiresValid<typename CallableTraits<T>::CallSig>> : std::true_type {};

template<typename T>
inline constexpr bool isCallable = IsCallable<T>::value;

template<typename T, Requires<std::is_arithmetic_v<T>> = true>
void setZero(T& x) {
    x = T();
}

template <typename T, typename U,
    Requires<
        std::is_integral_v<T> &&
        std::is_integral_v<U> &&
        std::is_signed_v<T> &&
        std::is_unsigned_v<U> &&
        tmax<T> >= tmax<U>
    > = true>
T int_cast(U value) {
    return static_cast<T>(value);
}

template<typename T>
class Array {
public:
    // ...
    
    template<typename InputIt, internal::RequiresInputIterator<InputIt> = true>
    void assign(InputIt first, InputIt last) {
        assignRange_(first, last);
    }
    
    // ...
};
```

However, we recognize that templated code can easily get complicated, so it's 
okay to deviate a little from the recommended style if it increases readability.

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
