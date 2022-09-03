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

#include <pybind11/operators.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/range1d.h>
#include <vgc/geometry/range1f.h>

namespace {

template<typename T>
struct Range1_ {};

template<>
struct Range1_<float> {
    using type = vgc::geometry::Range1f;
};

template<>
struct Range1_<double> {
    using type = vgc::geometry::Range1d;
};

template<typename T>
using Range1 = typename Range1_<T>::type;

template<typename T>
void wrap_range(py::module& m, const std::string& name, T relTol) {

    using TRange1 = Range1<T>;

    py::class_<TRange1>(m, name.c_str())

        .def(py::init<>())
        .def(py::init<TRange1>())
        .def(py::init<T, T>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<TRange1>(s); }))
        .def(
            py::init([](T position, T size) {
                return TRange1::fromPositionSize(position, size);
            }),
            "position"_a,
            "size"_a)

        .def_property_readonly_static(
            "empty", [](py::object) -> TRange1 { return TRange1::empty; })
        .def("isEmpty", &TRange1::isEmpty)
        .def("normalize", &TRange1::normalize)
        .def("normalized", &TRange1::normalized)

        .def_property("position", &TRange1::position, &TRange1::setPosition)
        .def_property("size", &TRange1::size, &TRange1::setSize)
        .def_property("pMin", &TRange1::pMin, &TRange1::setPMin)
        .def_property("pMax", &TRange1::pMax, &TRange1::setPMax)

        .def("isClose", &TRange1::isClose, "other"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("isNear", &TRange1::isNear, "other"_a, "absTol"_a)

        .def(py::self == py::self)
        .def(py::self != py::self)

        .def(
            "unitedWith",
            py::overload_cast<const TRange1&>(&TRange1::unitedWith, py::const_))
        .def("unitedWith", py::overload_cast<T>(&TRange1::unitedWith, py::const_))
        .def("uniteWith", py::overload_cast<const TRange1&>(&TRange1::uniteWith))
        .def("uniteWith", py::overload_cast<T>(&TRange1::uniteWith))
        .def("intersectedWith", &TRange1::intersectedWith)
        .def("intersectWith", &TRange1::intersectWith)
        .def("intersects", &TRange1::intersects)
        .def(
            "contains", py::overload_cast<const TRange1&>(&TRange1::contains, py::const_))
        .def("contains", py::overload_cast<T>(&TRange1::contains, py::const_))

        .def("__repr__", [](const TRange1& m) { return vgc::core::toString(m); });
}

} // namespace

void wrap_range(py::module& m) {
    wrap_range<double>(m, "Range1d", 1e-9);
    wrap_range<float>(m, "Range1f", 1e-5f);
}
