# VGC Python Wrappers Guide

(Note: This is an early stage document and it might evolve after more thoughts
and use cases. Also, currently, not all C++ API is wrapped, not because we don't
want to but because it has not been done yet.)

Unless in rare exceptions, all C++ code should have Python wrappers.

Exceptions include:
- C++ utilities for writing python wrappers
- C++ constructs that have no pythonic-equivalent (e.g., type erasures)

## Wrappers vs. Bindings

Terminology is sometimes confusing. We try to use the term "wrapper" to mean the
C++ code that wraps C++ API into Python by using pybind11 (that is,
`wrap_myfile.h` is a wrapper). This wrapper is compiled into a Python module,
which we call the "Python bindings" of the C++ API. But it's possible that these
terms are used interchangeably, we do not guarantee this documentation to be
consistent ;)

## pybind11

We use pybind11 for writing the python wrappers. The [official
documentation](http://pybind11.readthedocs.io) is excellent, so we won't repeat
it here.

Typically, when writing a new C++ class/function, try to find an existing
similar class/function and follow the same style for its Python wrappers. In
case you can't find such similar class/function, ask for help and/or refer to
the pybind11 documentation.

In the rest of this guide, we document the architecture, design decisions, and
guidelines which are specific to VGC.

## Architecture

Every VGC library is located in a folder `<vgc-source-dir>/libs/vgc/foobar/`, and
its corresponding Python wrappers are located in the subfolder
`<vgc-source-dir>/libs/vgc/foobar/wraps/`. Example:

```
foobar/
 |- CMakeLists.txt
 |- bar.h
 |- bar.cpp
 |- foo.h
 |- foo.cpp
 +- wraps/
     |- CMakeLists.txt
     |- module.cpp
     |- wrap_bar.cpp
     +- wrap_foo.cpp
```

Typically, there should be one `wrap_*.cpp` file for each C++ header/source
pair, but this is not a strong requirement.

The `foobar` C++ library is compiled into a shared library (e.g.,
`libvgcfoobar.so`) located in `<vgc-build-dir>/lib/`.

The Python wrappers for the `foobar` C++ library are compiled into a Python
module (e.g., `foobar.cpython-36m-x86_64-linux-gnu.so`) located in
`<vgc-build-dir>/python/vgc/`.

Therefore, in order to use the Python bindings of `foobar`, simply add
`<vgc-build-dir>/python` to your `PYTHONPATH` or `sys.path`, and import the
library symbols as `from vgc.foobar import Foo, Bar`. Note that the Python
interpreter embedded in VGC apps automatically adds `<vgc-build-dir>/python` to
`sys.path`.

## Wrapping C++ Classes with Value Semantics

In C++, it is usually a good practice to design classes to be used with *value
semantics*. For example, a vgc::core::Vec2d can be allocated on the stack, and
writing `a = b` creates a copy of your `b` into `a`:

```
using vgc::core::Vec2d;
Vec2d v1(0, 0);
Vec2d v2 = v1;
v2[0] = 42;
std::cout << v1[0]; // -> 0
```

By contrast, in Python, all objects are used with *pointer semantics*. For
example, when writing `a = b`, `a` does not become a copy of `b`, but another
name for the object referenced by `b`:

```
from vgc.core import Vec2d
v1 = Vec2d(0, 0)
v2 = v1
v2[0] = 42;
print(v1[0]); // -> 42
```

This divergence of semantics between the C++ code and its Python "equivalent"
may seem confusing or even error-prone. But in fact, it makes sense and follows
the philosophy of each language: Python programmers won't be surprised by the
Python behavior, and C++ programmers won't be surprised by the C++ behavior.

Such behavior is obtained by wrapping `Vec2d` using `py::class_<Vec2d>` with no
template options:

```
py::class_<Vec2d>(m, "Vec2d")
    .def(py::init<>())
    // ...
;
```

## Wrapping C++ Classes with Pointer Semantics

Despite value semantics being generally preferrable in C++, many node-based data
structures (for example: linked-lists, trees, scene graphs, hierarchies of GUI
widgets, etc.) are well represented in C++ by allocating `Node` objects
dynamically. Doing so, the address of each `Node` object can be conveniently
used as its "identity": it is guaranteed to be unique and never change.

The lifetime of the nodes is typically controlled by a manager class
(say, `Graph`), for example using `std::unique_ptr<Node>`,
`std::shared_ptr<Node>`, or other mechanisms. Clients of the API refer to nodes
using raw pointers `Node*`, non-owning smart pointers such as
`std::weak_ptr<Node>`, or any other [handle
mechanism](https://isocpp.org/wiki/faq/references#what-is-a-handle) such as
`std::list<T>::iterator` or [CGAL
handles](https://doc.cgal.org/latest/Circulator/classHandle.html).

One important aspect of such data structures is that it is generally desired
that "observers" do NOT extend the lifetime of nodes. For example, when erasing
an element in a `std::list`, the element is immediately deleted even if some
clients hold iterators to the elements (in which case the iterators become
invalid). This is why observers should generally not hold `shared_ptr`, but
rather non-owning pointers/references/handles.

In the VGC codebase, we choose to consistently use
`std::shared_ptr<Node>` to express owning pointers (even for unique ownership),
and `Node*` to express non-owning pointers. Such class `Node` meant to be used with
pointer semantics should derive (directly or indirectly) from
`vgc::core::Object`, for example:

```
#include <vgc/core/object.h>
#include <vgc/mylib/api.h>

namespace vgc {
namespace mylib {

VGC_CORE_DECLARE_PTRS(Node);

class VGC_MYLIB_API Node: public core::Object // or any class inheriting Object
{
public:
    VGC_CORE_OBJECT(Node)

    Node* otherNode() { return otherNode_.get(); }

private:
    NodeSharedPtr otherNode_;
};

} // namespace mylib
} // namespace vgc
```

And they should be wrapped like so:

```
using This = vgc::mylib::Node;
using Holder = vgc::mylib::NodeSharedPtr
using Parent = vgc::core::Object; // or any class inheriting Object

py::class_<This, Holder, Parent>(m, "Node")
    .def(py::init<>())
    .def_property_readonly("otherNode", &This::otherNode, py::return_value_policy::reference)
    // ...
;
```

The [return value
policy](https://pybind11.readthedocs.io/en/stable/advanced/functions.html#return-value-policies)
makes sure that Python does not take ownership of the returned `Node*` which
would extend its lifetime. Also, note that `vgc::core::Object` derives from
`std::enable_shared_from_this`, which makes it possible for Python to properly
share ownership if necessary, as suggested by the [pybind11 documentation about
shared
pointers](https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html#std-shared-ptr).
Also, it allows C++ observers to promote the raw pointer to a `std::weak_ptr` if
necessary.

## Parent Class

The parent class of any given class can be specified as a [template parameter of
py::class_](
http://pybind11.readthedocs.io/en/stable/classes.html#inheritance-and-automatic-upcasting).
However, for this to work, this requires pybind11 to already know about the
parent class. Therefore, in `module.cpp`, the python wrappers of parent classes
must appear before the derived classes. We recommend to document these
dependencies using comments for better maintainability.

## Default Arguments

Consider the following C++ method with a default argument:

```
#include "foo.h"
class Bar {
    void bar(const Foo& foo = Foo());
};
```

You would typically wrap it in `wrap_bar.cpp` as such:

```
py::class_<Bar>("Bar")
    .def("bar", &Bar::bar, "foo"_a = Foo())
```

However, as per the
[pybind11 documentation](http://pybind11.readthedocs.io/en/stable/advanced/functions.html#default-arguments-revisited),
this requires pybind11 to already know about `Foo` via a prior instantiation of
`py::class_<Foo>`, or an exception will be thrown when importing the Python
module.

In other words, it is important to wrap `Foo` before `Bar` in your module definition:

```
PYBIND11_MODULE(foobar, m) {
    wrap_foo(m);
    wrap_bar(m);
}
```

This is a bit unfortunate as we would prefer to keep the list alphabetical and
not worry about dependency issues. The alternative is not to use default
arguments for the wrappers, and instead use [function overloads or generic
py::args arguments](https://github.com/pybind/pybind11/issues/1218#issuecomment-353700297).
An even strongly opinionated alternative is not to use default
arguments even in the C++ API, and use overloads instead, as discussed in the
[Google C++ Style
Guide](https://google.github.io/styleguide/cppguide.html#Default_Arguments).

Currently, we consider that it is okay to use default arguments both in the C++
API and in the wrapped Python API, since both are well-known useful features of
both languages, making the API and its documentation more readable. Therefore,
just make sure to place wrappers of types used as default parameters first, and
document why:

```
PYBIND11_MODULE(foobar, m) {
    // Used as default arguments. See pythonwrappersguide.md#default-arguments.
    wrap_foo(m);

    // Never used as default arguments
    wrap_bar(m);
}
```

Keep the 2nd category alphabetical, and whenever relevant, document the
dependencies between the wrappers in the 1st category using comments.

Finally, one word of caution: in C++, default parameters are evaluated at call
time, while in Python, there are [evaluated at definition
time](http://docs.python-guide.org/en/latest/writing/gotchas/#mutable-default-arguments).
Therefore, be extra careful to keep default parameters immutable in C++ in
order to have consistency between the C++ and the Python API.
