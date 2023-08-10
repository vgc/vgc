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

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/format.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/sharedconst.h>

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
template<typename This>
vgc::Int wrapArrayIndex(This& a, vgc::Int i) {
    if (a.isEmpty()) {
        throw IndexError(format("Array index {} out of range (the array is empty)", i));
    }
    else if (i < -a.length() || i > a.length() - 1) {
        throw IndexError(format(
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

namespace detail {

template<typename T>
bool operator==(const Array<T>& a, py::sequence s) {
    using SeqElement = decltype(*s.begin());
    if (a.length() == int_cast<Int>(s.size())) {
        auto cmpEqual = [](const T& ae, const SeqElement& se) { //
            return ae == se.cast<T>();
        };
        return std::equal(a.begin(), a.end(), s.begin(), cmpEqual);
    }
    return false;
};

template<typename T>
struct PySeqElementCompLess {
    using SeqElement = decltype(*std::declval<py::sequence>().begin());
    bool operator()(const T& ae, const SeqElement& se) { //
        return ae < se.cast<T>();
    };
    bool operator()(const SeqElement& se, const T& ae) { //
        return se.cast<T>() < ae;
    };
};

template<typename T>
bool operator<(const Array<T>& a, py::sequence s) {
    using SeqElement = decltype(*s.begin());
    return std::lexicographical_compare(
        a.begin(), a.end(), s.begin(), s.end(), PySeqElementCompLess<T>());
};

template<typename T>
bool operator<(py::sequence s, const Array<T>& a) {
    using SeqElement = decltype(*s.begin());
    return std::lexicographical_compare(
        s.begin(), s.end(), a.begin(), a.end(), PySeqElementCompLess<T>());
};

} // namespace detail

// Defines most methods required to wrap a given Array<T> type.
//
template<typename T, bool isParseable>
void defineArrayCommonMethods(Class<core::Array<T>>& c, std::string fullName) {
    using ArrayType = core::Array<T>;
    using It = typename ArrayType::iterator;
    using rvp = py::return_value_policy;

    c.def(py::init<>());
    c.def(py::init<const ArrayType&>());
    c.def(py::init<Int>());
    c.def(py::init<Int, const T&>());

    if constexpr (isParseable) {
        c.def(py::init([](const std::string& s) { return parse<ArrayType>(s); }));
    }

    c.def("__getitem__", [](const ArrayType& a, Int i) {
        vgc::Int j = wrapArrayIndex(a, i);
        return a.getUnchecked(j);
    });

    c.def("__setitem__", [](ArrayType& a, Int i, const T& value) {
        vgc::Int j = wrapArrayIndex(a, i);
        a.getUnchecked(j) = value;
    });

    c.def("__len__", &ArrayType::length);

    c.def(
        "__iter__",
        [](ArrayType& a) {
            return py::make_iterator<rvp::reference_internal, It, It, T&>(
                a.begin(), a.end());
        },
        py::keep_alive<0, 1>());

    c.def("__contains__", [](ArrayType& a, const T& value) { return a.contains(value); });

    c.def("index", static_cast<Int (ArrayType::*)(const T&) const>(&ArrayType::index));

    c.def("prepend", py::overload_cast<const T&>(&ArrayType::prepend));
    c.def("append", py::overload_cast<const T&>(&ArrayType::append));
    c.def("insert", [](ArrayType& a, vgc::Int i, const T& x) {
        vgc::Int j = wrapArrayIndex(a, i);
        return a.insert(j, x);
    });

    c.def("pop", py::overload_cast<>(&ArrayType::pop));
    c.def("pop", [](ArrayType& a, vgc::Int i) {
        vgc::Int j = wrapArrayIndex(a, i);
        return a.pop(j);
    });

    c.def(py::self == py::self);
    c.def(py::self != py::self);
    c.def(py::self < py::self);
    c.def(py::self > py::self);
    c.def(py::self <= py::self);
    c.def(py::self >= py::self);

    using detail::operator==;
    using detail::operator<;

    c.def(
        "__eq__",
        [](const ArrayType& a, const py::sequence& s) { return a == s; },
        py::is_operator());
    c.def(
        "__eq__",
        [](const py::sequence& s, const ArrayType& a) { return a == s; },
        py::is_operator());
    c.def(
        "__neq__",
        [](const ArrayType& a, const py::sequence& s) { return !(a == s); },
        py::is_operator());
    c.def(
        "__neq__",
        [](const py::sequence& s, const ArrayType& a) { return !(a == s); },
        py::is_operator());
    c.def(
        "__lt__",
        [](const ArrayType& a, const py::sequence& s) { return a < s; },
        py::is_operator());
    c.def(
        "__lt__",
        [](const py::sequence& s, const ArrayType& a) { return s < a; },
        py::is_operator());
    c.def(
        "__gt__",
        [](const ArrayType& a, const py::sequence& s) { return s < a; },
        py::is_operator());
    c.def(
        "__gt__",
        [](const py::sequence& s, const ArrayType& a) { return a < s; },
        py::is_operator());
    c.def(
        "__le__",
        [](const ArrayType& a, py::sequence s) { return !(s < a); },
        py::is_operator());
    c.def(
        "__le__",
        [](py::sequence s, const ArrayType& a) { return !(a < s); },
        py::is_operator());
    c.def(
        "__ge__",
        [](const ArrayType& a, py::sequence s) { return !(a < s); },
        py::is_operator());
    c.def(
        "__ge__",
        [](py::sequence s, const ArrayType& a) { return !(s < a); },
        py::is_operator());

    c.def("__str__", [](const ArrayType& a) { return toString(a); });

    c.def("__repr__", [fullName = fullName](const ArrayType& a) {
        py::object pyStr = py::cast(toString(a));
        std::string pyStrRepr = py::cast<std::string>(pyStr.attr("__repr__")());
        return vgc::core::format("{}({})", fullName, pyStrRepr);
    });
}

// Defines most methods required to wrap a given SharedConstArray<T> type.
//
template<typename T, bool isParseable>
void defineSharedConstArrayCommonMethods(
    Class<core::SharedConstArray<T>>& c,
    std::string fullName) {

    using ArrayType = core::Array<T>;
    using It = typename ArrayType::const_iterator;
    using SharedConstArrayType = core::SharedConstArray<T>;
    using rvp = py::return_value_policy;

    defineSharedConstCommonMethods(c);

    c.def(py::init<const ArrayType&>());
    c.def(py::init<Int, const T&>());

    if constexpr (isParseable) {
        c.def(py::init([](const std::string& s) {
            return SharedConstArrayType(vgc::core::parse<ArrayType>(s));
        }));
    }

    c.def("__getitem__", [](const SharedConstArrayType& a, Int i) {
        vgc::Int j = wrapArrayIndex(a.get(), i);
        return a.get().getUnchecked(j);
    });

    c.def("__len__", [](const SharedConstArrayType& a) { return a.get().length(); });

    c.def(
        "__iter__",
        [](const SharedConstArrayType& a) {
            return py::make_iterator<rvp::reference_internal, It, It, const T&>(
                a.get().begin(), a.get().end());
        },
        // Keep object alive while iterator exists.
        py::keep_alive<0, 1>());

    c.def("__contains__", [](const SharedConstArrayType& a, const T& value) {
        return a.get().contains(value);
    });

    c.def("index", [](const SharedConstArrayType& a, const T& v) {
        return a.get().index(v);
    });

    c.def(py::self == py::self);
    c.def(py::self != py::self);
    c.def(py::self < py::self);
    c.def(py::self > py::self);
    c.def(py::self <= py::self);
    c.def(py::self >= py::self);

    ArrayType array = {};
    c.def(py::self == array);
    c.def(array == py::self);
    c.def(py::self != array);
    c.def(array != py::self);
    c.def(py::self < array);
    c.def(array < py::self);
    c.def(py::self > array);
    c.def(array > py::self);
    c.def(py::self <= array);
    c.def(array <= py::self);
    c.def(py::self >= array);
    c.def(array >= py::self);

    using detail::operator==;
    using detail::operator<;

    c.def(
        "__eq__",
        [](const SharedConstArrayType& a, py::sequence s) { return a.get() == s; },
        py::is_operator());
    c.def(
        "__eq__",
        [](py::sequence s, const SharedConstArrayType& a) { return a.get() == s; },
        py::is_operator());
    c.def(
        "__neq__",
        [](const SharedConstArrayType& a, py::sequence s) { return !(a.get() == s); },
        py::is_operator());
    c.def(
        "__neq__",
        [](py::sequence s, const SharedConstArrayType& a) { return !(a.get() == s); },
        py::is_operator());
    c.def(
        "__lt__",
        [](const SharedConstArrayType& a, py::sequence s) { return a.get() < s; },
        py::is_operator());
    c.def(
        "__lt__",
        [](py::sequence s, const SharedConstArrayType& a) { return s < a.get(); },
        py::is_operator());
    c.def(
        "__gt__",
        [](const SharedConstArrayType& a, py::sequence s) { return s < a.get(); },
        py::is_operator());
    c.def(
        "__gt__",
        [](py::sequence s, const SharedConstArrayType& a) { return a.get() < s; },
        py::is_operator());
    c.def(
        "__le__",
        [](const SharedConstArrayType& a, py::sequence s) { return !(s < a.get()); },
        py::is_operator());
    c.def(
        "__le__",
        [](py::sequence s, const SharedConstArrayType& a) { return !(a.get() < s); },
        py::is_operator());
    c.def(
        "__ge__",
        [](const SharedConstArrayType& a, py::sequence s) { return !(a.get() < s); },
        py::is_operator());
    c.def(
        "__ge__",
        [](py::sequence s, const SharedConstArrayType& a) { return !(s < a.get()); },
        py::is_operator());

    c.def("__str__", [](const SharedConstArrayType& a) { return toString(a.get()); });

    c.def("__repr__", [fullName = fullName](const SharedConstArrayType& a) {
        py::object pyStr = py::cast(toString(a.get()));
        std::string pyStrRepr = py::cast<std::string>(pyStr.attr("__repr__")());
        return vgc::core::format("{}({})", fullName, pyStrRepr);
    });
}

template<typename T, bool isParseable = true>
void wrap_array(py::module& m, const std::string& valueTypeName) {
    using ArrayType = core::Array<T>;
    using SharedConstArrayType = core::SharedConstArray<T>;

    std::string moduleFullName = py::cast<std::string>(m.attr("__name__"));

    std::string arrayTypeName = valueTypeName + "Array";
    Class<ArrayType> c1(m, arrayTypeName.c_str());
    vgc::core::wraps::defineArrayCommonMethods<T, isParseable>(
        c1, vgc::core::format("{}.{}", moduleFullName, arrayTypeName));
    c1.def(py::init([](py::sequence s) {
        ArrayType res;
        for (auto x : s) {
            res.append(x.cast<T>());
        }
        return res;
    }));

    py::implicitly_convertible<py::sequence, ArrayType>();

    std::string sharedConstArrayTypeName = std::string("SharedConst") + arrayTypeName;
    Class<SharedConstArrayType> c2(m, sharedConstArrayTypeName.c_str());
    vgc::core::wraps::defineSharedConstArrayCommonMethods<T, isParseable>(
        c2, vgc::core::format("{}.{}", moduleFullName, sharedConstArrayTypeName));
}

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_ARRAY_H
