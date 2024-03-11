// Copyright 2021 The VGC Developers
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

#include <vgc/core/format.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/vec3d.h>
#include <vgc/geometry/vec3f.h>
#include <vgc/geometry/vec4d.h>
#include <vgc/geometry/vec4f.h>

#include <vgc/core/wraps/array.h>
#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/wraps/vec.h>

namespace {

template<typename T>
constexpr T relTol_();

template<>
constexpr float relTol_() {
    return 1e-5f;
}

template<>
constexpr double relTol_() {
    return 1e-9;
}

template<typename TVec>
TVec normalizedOrThrow(const TVec& v) {
    bool isNormalizable;
    TVec res = v.normalized(&isNormalizable);
    if (!isNormalizable) {
        throw py::value_error("The vector is not normalizable.");
    }
    return res;
}

template<typename TVec>
void wrap_vecArray(py::module& m, const std::string& name) {
    using ArrayType = vgc::core::Array<TVec>;
    using SharedConstArrayType = vgc::core::SharedConstArray<TVec>;

    std::string moduleFullName = py::cast<std::string>(m.attr("__name__"));

    std::string arrayTypeName = name + "Array";
    vgc::core::wraps::Class<ArrayType> c1(m, arrayTypeName.c_str());
    vgc::core::wraps::defineArrayCommonMethods<TVec, true>(
        c1, vgc::core::format("{}.{}", moduleFullName, arrayTypeName));
    c1.def(py::init([name](py::sequence s) {
        ArrayType res;
        for (auto it : s) {
            auto t = py::reinterpret_borrow<py::tuple>(it);
            res.append(vgc::geometry::wraps::vecFromTuple<TVec>(t));
        }
        return res;
    }));

    std::string sharedConstArrayTypeName = std::string("SharedConst") + arrayTypeName;
    vgc::core::wraps::Class<SharedConstArrayType> c2(m, sharedConstArrayTypeName.c_str());
    vgc::core::wraps::defineSharedConstArrayCommonMethods<TVec, true>(
        c2, vgc::core::format("{}.{}", moduleFullName, sharedConstArrayTypeName));
}

template<int dimension, typename T>
void wrap_vec(py::module& m, const std::string& name) {

    using TVec = vgc::geometry::Vec<dimension, T>;

    vgc::core::wraps::Class<TVec> cvec(m, name.c_str());

    // Default constructor and copy constructor
    cvec.def(py::init<>());
    cvec.def(py::init<TVec>());

    // Constructor with explicit initialization of all elements
    if constexpr (dimension == 2) {
        cvec.def(py::init<T, T>());
    }
    if constexpr (dimension == 3) {
        cvec.def(py::init<T, T, T>());
    }
    else if constexpr (dimension == 4) {
        cvec.def(py::init<T, T, T, T>());
    }

    // Constructor from string (parse)
    cvec.def(py::init([](const std::string& s) { return vgc::core::parse<TVec>(s); }));

    // Constructor from any Python object implementing the Sequence protocol
    // (tuple, list, numpy array, etc.)
    //
    // Important: this must be defined after the string overload, otherwise it
    // would take precendence since a string implements the Sequence protocol.
    //
    cvec.def(py::init(
        [](py::sequence s) { return vgc::geometry::wraps::vecFromSequence<TVec>(s); }));

    // Enable implicit conversions from Python tuples to Vec types.
    //
    // This allows to use the `(x, y)` syntax for all functions
    // that expect a Vec type, for example:
    //
    //   m = Mat2d((1, 2), (3, 4))
    //   r = Rect2d(position=(1, 2), size=(2, 3))
    //
    // which would otherwise have to be written as:
    //
    //   m = Mat2d(Vec2d(1, 2), Vec2d(3, 4))
    //   r = Rect2d(position=Vec2d(1, 2), size=Vec2d(2, 3))
    //
    py::implicitly_convertible<py::tuple, TVec>();

    // Index-based getter and setter
    cvec.def(
            "__getitem__",
            [](const TVec& v, int i) {
                if (i < 0 || i >= dimension) {
                    throw py::index_error();
                }
                return v[i];
            })
        .def("__setitem__", [](TVec& v, int i, T x) {
            if (i < 0 || i >= dimension) {
                throw py::index_error();
            }
            v[i] = x;
        });

    // Implements the Sequence protocol, which requires both __getitem__ (see
    // above) and __len__ (see below).
    //
    // For example, this allows for easy conversion to numpy arrays:
    //
    // ```python
    // from vgc.geometry import Vec2d
    // import numpy as np
    // v = Vec2d(1, 2)
    // a = np.array(v) # => array([1., 2.])
    // ```
    //
    // Note that __getitem__ alone is enough to make the Vec iterable (e.g.,
    // `for x in v:`), but numpy's array constructor requires the Sequence
    // protocol to work properly. Without __len__, the code above would result
    // in an np.array of size=1 and dtype=object, containing the Vec2d as
    // unique element.
    //
    cvec.def("__len__", [](const TVec&) { return dimension; });

    // Named getters and setters
    cvec.def_property("x", &TVec::x, &TVec::setX);
    cvec.def_property("y", &TVec::y, &TVec::setY);
    if constexpr (dimension >= 3) {
        cvec.def_property("z", &TVec::z, &TVec::setZ);
    }
    if constexpr (dimension >= 4) {
        cvec.def_property("w", &TVec::w, &TVec::setW);
    }

    // Overload of arithmetic operators
    auto self2 = py::self; // Fix https://github.com/pybind/pybind11/issues/1893
    cvec.def(py::self += py::self)
        .def(py::self + py::self)
        .def(+py::self)
        .def(self2 -= py::self)
        .def(py::self - py::self)
        .def(-py::self)
        .def(py::self *= T())
        .def(T() * py::self)
        .def(py::self * T())
        .def(py::self /= T())
        .def(py::self / T())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        .def(py::self <= py::self)
        .def(py::self > py::self)
        .def(py::self >= py::self);

    // Misc methods
    cvec.def("length", &TVec::length)
        .def("squaredLength", &TVec::squaredLength)
        .def("normalize", [](TVec& v) { v = normalizedOrThrow(v); })
        .def("normalized", [](const TVec& v) { return normalizedOrThrow(v); })
        .def("dot", &TVec::dot)
        .def("angle", py::overload_cast<const TVec&>(&TVec::angle, py::const_));

    // Vec2-specific methods
    if constexpr (dimension == 2) {
        cvec.def("orthogonalize", &TVec::orthogonalize)
            .def("orthogonalized", &TVec::orthogonalized)
            .def("det", &TVec::det)
            .def("angle", py::overload_cast<>(&TVec::angle, py::const_));
    }

    // Vec3-specific methods
    if constexpr (dimension == 3) {
        cvec.def("cross", &TVec::cross);
    }

    // Tests for almost-equality
    constexpr T relTol = relTol_<T>();
    cvec.def("isClose", &TVec::isClose, "b"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("allClose", &TVec::allClose, "b"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("isNear", &TVec::isNear, "b"_a, "absTol"_a)
        .def("allNear", &TVec::allNear, "b"_a, "absTol"_a);

    // Conversion to string
    cvec.def("__repr__", [](const TVec& v) { return vgc::core::toString(v); });

    // Wrap Array type
    wrap_vecArray<TVec>(m, name);
}

} // namespace

void wrap_vec(py::module& m) {
    wrap_vec<2, float>(m, "Vec2f");
    wrap_vec<2, double>(m, "Vec2d");
    wrap_vec<3, float>(m, "Vec3f");
    wrap_vec<3, double>(m, "Vec3d");
    wrap_vec<4, float>(m, "Vec4f");
    wrap_vec<4, double>(m, "Vec4d");
}
