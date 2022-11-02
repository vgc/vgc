// Copyright 2022 The VGC Developers
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

#ifndef VGC_CORE_WRAPS_ARRAY_H
#define VGC_CORE_WRAPS_ARRAY_H

#include <utility>

#include <vgc/core/array.h>
#include <vgc/core/format.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

namespace vgc::core::wraps {

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
// Note 2: Automatic conversion of STL containers by pybind11 always create
//   copies. Read this before including <pybind11/stl.h>:
//   http://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html#making-opaque-types
//
// Note 3: we're mimicking many of the things done in pybind11/stl_bind.h. See there for
// additional implementation notes.

// Converts from a valid Python Array index to a valid C++ Array index, such
// that for example -1 refers to the last element in the array. Raises
// IndexError if the input index is out of range in the Python sense.
//
template<typename ArrayType>
vgc::Int wrapArrayIndex(ArrayType& a, vgc::Int i) {
    if (a.isEmpty()) {
        throw vgc::core::IndexError(
            vgc::core::format("Array index {} out of range (the array is empty)", i));
    }
    else if (i < -a.length() || i > a.length() - 1) {
        throw vgc::core::IndexError(vgc::core::format(
            "Array index {} out of range [{}, {}] (array length is {})",
            i,
            -a.length(),
            a.length() - 1,
            a.length()));
    }
    else {
        if (i < 0) {
            i += a.length();
        }
        return i;
    }
}

// Defines most methods required to wrap a given Array<T> type.
//
template<typename ValueType>
void defineArrayCommonMethods(Class<core::Array<ValueType>>& c, std::string fullName) {
    using T = core::Array<ValueType>;
    using It = typename T::iterator;
    using rvp = py::return_value_policy;

    c.def(py::init<>());
    c.def(py::init<const T&>());
    c.def(py::init<Int>());
    c.def(py::init<Int, const ValueType&>());
    c.def(py::init([](const std::string& s) { return vgc::core::parse<T>(s); }));

    c.def("__getitem__", [](const T& a, Int i) {
        vgc::Int j = wrapArrayIndex(a, i);
        return a.getUnchecked(j);
    });

    c.def("__setitem__", [](T& a, Int i, const ValueType& value) {
        vgc::Int j = wrapArrayIndex(a, i);
        a.getUnchecked(j) = value;
    });

    c.def("__len__", &T::length);

    c.def(
        "__iter__",
        [](T& a) {
            return py::make_iterator<rvp::reference_internal, It, It, ValueType&>(
                a.begin(), a.end());
        },
        py::keep_alive<0, 1>());

    c.def("__contains__", [](T& a, const ValueType& value) { return a.contains(value); });

    c.def("index", static_cast<Int (T::*)(const ValueType&) const>(&T::index));

    c.def("prepend", py::overload_cast<const ValueType&>(&T::prepend));
    c.def("append", py::overload_cast<const ValueType&>(&T::append));
    c.def("insert", [](T& a, vgc::Int i, const ValueType& x) {
        vgc::Int j = wrapArrayIndex(a, i);
        return a.insert(j, x);
    });

    c.def("pop", py::overload_cast<>(&T::pop));
    c.def("pop", [](T& a, vgc::Int i) {
        vgc::Int j = wrapArrayIndex(a, i);
        return a.pop(j);
    });

    c.def(py::self == py::self);
    c.def(py::self != py::self);
    c.def(py::self < py::self);
    c.def(py::self > py::self);
    c.def(py::self <= py::self);
    c.def(py::self >= py::self);

    c.def("__str__", [](const T& a) { return toString(a); });

    c.def("__repr__", [fullName = fullName](const T& a) {
        py::object pyStr = py::cast(toString(a));
        std::string pyStrRepr = py::cast<std::string>(pyStr.attr("__repr__")());
        return vgc::core::format("{}({})", fullName, pyStrRepr);
    });
}

template<typename ValueType>
void wrap_array(py::module& m, const std::string& valueTypeName) {
    using T = ValueType;
    using ArrayType = core::Array<ValueType>;

    std::string moduleFullName = py::cast<std::string>(m.attr("__name__"));

    std::string arrayTypeName = valueTypeName + "Array";
    Class<ArrayType> c1(m, arrayTypeName.c_str());
    vgc::core::wraps::defineArrayCommonMethods(
        c1, vgc::core::format("{}.{}", moduleFullName, arrayTypeName));
    c1.def(py::init([](py::sequence s) {
        ArrayType res;
        for (auto x : s) {
            res.append(x.cast<T>());
        }
        return res;
    }));
}

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_ARRAY_H
