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

#include <vgc/geometry/range1d.h>
#include <vgc/geometry/range1f.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

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

    using This = Range1<T>;

    vgc::core::wraps::Class<This>(m, name.c_str())

        .def(py::init<>())
        .def(py::init<This>())
        .def(py::init<T, T>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<This>(s); }))
        .def(
            py::init([](T position, T size) {
                return This::fromPositionSize(position, size);
            }),
            "position"_a,
            "size"_a)

        .def_property_readonly_static(
            "empty", [](py::object) -> This { return This::empty; })
        .def("isEmpty", &This::isEmpty)
        .def("normalize", &This::normalize)
        .def("normalized", &This::normalized)

        .def_property("position", &This::position, &This::setPosition)
        .def_property("size", &This::size, &This::setSize)
        .def_property("pMin", &This::pMin, &This::setPMin)
        .def_property("pMax", &This::pMax, &This::setPMax)

        .def("isClose", &This::isClose, "other"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("isNear", &This::isNear, "other"_a, "absTol"_a)

        .def(py::self == py::self)
        .def(py::self != py::self)

        .def("unitedWith", py::overload_cast<const This&>(&This::unitedWith, py::const_))
        .def("unitedWith", py::overload_cast<T>(&This::unitedWith, py::const_))
        .def("uniteWith", py::overload_cast<const This&>(&This::uniteWith))
        .def("uniteWith", py::overload_cast<T>(&This::uniteWith))
        .def("intersectedWith", &This::intersectedWith)
        .def("intersectWith", &This::intersectWith)
        .def("intersects", &This::intersects)
        .def("contains", py::overload_cast<const This&>(&This::contains, py::const_))
        .def("contains", py::overload_cast<T>(&This::contains, py::const_))

        .def("__repr__", [](const This& m) { return vgc::core::toString(m); });
}

} // namespace

void wrap_range(py::module& m) {
    wrap_range<double>(m, "Range1d", 1e-9);
    wrap_range<float>(m, "Range1f", 1e-5f);
}
