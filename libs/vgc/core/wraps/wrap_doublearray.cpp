// Copyright 2018 The VGC Developers
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

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <vgc/core/doublearray.h>

namespace py = pybind11;
using This = vgc::core::DoubleArray;

// Note: the python wrappers for the VGC array types are designed to provide an
// interface as consistent as possible with Python lists, not with their C++
// counterparts. Methods part of the C++ API which are redundant with pythonic
// functionality are not provided in Python, unless they provide significantly
// better performance (e.g., DoubleArray([0] * 1000) vs DoubleArray(1000)).

// TODO Support slicing and other Python list methods. See:
//        pybind11/tests/test_sequences_and_iterators.cpp
//
//      Complete interface by taking inspiration from NumPy:
//        https://docs.scipy.org/doc/numpy/user/quickstart.html
//        https://docs.scipy.org/doc/numpy/user/basics.creation.html
//        https://docs.scipy.org/doc/numpy/reference/routines.array-creation.html
//        https://docs.scipy.org/doc/numpy/user/basics.indexing.html
//
// Note 1: unlike Python lists, Numpy arrays don't copy when slicing:
//   https://docs.scipy.org/doc/numpy/user/quickstart.html#view-or-shallow-copy
//   It may be a good idea to have the same behaviour with VGC arrays.
//
// Note 2: NumPy supports uninitialized arrays, see np.empty(). It is still
//   unclear whether VGC arrays should support this, see comment in
//   doublearray.h
//
// Note 3: Automatic conversion of STL containers by pybind11 always create
//   copies. Read this before including <pybind11/stl.h>:
//   http://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html#making-opaque-types

void wrap_doublearray(py::module& m)
{
    py::class_<This>(m, "DoubleArray")

        .def(py::init<>())
        .def(py::init([](int size) { return This(size, 0); } ))
        .def(py::init([](int size, double value) { return This(size, value); } ))
        .def(py::init([](py::sequence s) { This res; for (auto x : s) res.append(x.cast<double>()); return res; } ))
        .def(py::init<const This&>())

        .def("__getitem__", [](const This& a, int i) { return a.at(i); })
        .def("__setitem__", [](This& a, int i, double value) { a.at(i) = value; })
        .def("__len__", &This::size)

        .def("append", &This::append)

        .def(py::self == py::self)
        .def(py::self != py::self)

        .def("__repr__", [](const This& a) { return toString(a); })
    ;
}
