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

#include <vgc/core/wraps/array.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/wraps/vec.h>

namespace {

template<typename This>
This normalizedOrThrow(const This& v) {
    bool isNormalizable;
    This res = v.normalized(&isNormalizable);
    if (!isNormalizable) {
        throw py::value_error("The vector is not normalizable.");
    }
    return res;
}

template<
    typename This,
    typename T,
    VGC_REQUIRES(std::is_same_v<typename This::ScalarType, T>)>
void wrap_vec2x(py::module& m, const std::string& thisTypeName, T relTol) {

    // Fix https://github.com/pybind/pybind11/issues/1893
    auto self2 = py::self;

    py::class_<This>(m, thisTypeName.c_str())

        .def(py::init<>())
        .def(py::init<T, T>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<This>(s); }))
        .def(py::init(
            [](py::tuple t) { return vgc::geometry::wraps::vecFromTuple<This>(t); }))
        .def(py::init<This>())

        .def(
            "__getitem__",
            [](const This& v, int i) {
                if (i < 0 || i >= 2) {
                    throw py::index_error();
                }
                return v[i];
            })
        .def(
            "__setitem__",
            [](This& v, int i, T x) {
                if (i < 0 || i >= 2) {
                    throw py::index_error();
                }
                v[i] = x;
            })

        .def_property("x", &This::x, &This::setX)
        .def_property("y", &This::y, &This::setY)

        .def(py::self += py::self)
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
        .def(py::self >= py::self)

        .def("length", &This::length)
        .def("squaredLength", &This::squaredLength)

        .def("normalize", [](This& v) { v = normalizedOrThrow(v); })
        .def("normalized", [](const This& v) { return normalizedOrThrow(v); })
        .def("orthogonalize", &This::orthogonalize)
        .def("orthogonalized", &This::orthogonalized)
        .def("dot", &This::dot)
        .def("det", &This::det)
        .def("angle", &This::angle)
        .def("isClose", &This::isClose, "b"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("allClose", &This::allClose, "b"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("isNear", &This::isNear, "b"_a, "absTol"_a)
        .def("allNear", &This::allNear, "b"_a, "absTol"_a)

        .def("__repr__", [](const This& v) { return vgc::core::toString(v); });
}

template<typename This>
void wrap_2darray(py::module& m, const std::string& valueTypeName) {
    using T = typename This::value_type; // Example: Vec2d
    using U = typename T::ScalarType;    // Example: double
    std::string thisTypeName = valueTypeName + "Array";
    py::class_<This> c(m, thisTypeName.c_str());
    vgc::core::wraps::defineArrayCommonMethods(c);
    c.def(py::init([valueTypeName](py::sequence s) {
        This res;
        for (auto it : s) {
            auto t = py::reinterpret_borrow<py::tuple>(it);
            if (t.size() != 2) {
                throw py::value_error(
                    "Tuple length must be 2 for conversion to " + valueTypeName);
            }
            res.append(T(t[0].cast<U>(), t[1].cast<U>()));
        }
        return res;
    }));
}

} // namespace

void wrap_vec(py::module& m) {
    wrap_vec2x<vgc::geometry::Vec2d>(m, "Vec2d", 1e-9);
    wrap_vec2x<vgc::geometry::Vec2f>(m, "Vec2f", 1e-5f);
    wrap_2darray<vgc::geometry::Vec2dArray>(m, "Vec2d");
    wrap_2darray<vgc::geometry::Vec2fArray>(m, "Vec2f");
}
