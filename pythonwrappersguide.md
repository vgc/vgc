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
    # Used as default arguments
    wrap_foo(m);

    # Never used as default arguments
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
