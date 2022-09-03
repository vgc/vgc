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

#include <pybind11/operators.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/geometry/wraps/vec.h>

namespace {

template<typename T>
struct Rect2_ {};

template<>
struct Rect2_<float> {
    using type = vgc::geometry::Rect2f;
};

template<>
struct Rect2_<double> {
    using type = vgc::geometry::Rect2d;
};

template<typename T>
using Rect2 = typename Rect2_<T>::type;

template<typename T>
struct Vec2_ {};

template<>
struct Vec2_<float> {
    using type = vgc::geometry::Vec2f;
};

template<>
struct Vec2_<double> {
    using type = vgc::geometry::Vec2d;
};

template<typename T>
using Vec2 = typename Vec2_<T>::type;

template<typename T>
void wrap_rect(py::module& m, const std::string& name, T relTol) {

    using TRect2 = Rect2<T>;
    using TVec2 = Vec2<T>;

    using vgc::geometry::wraps::vecFromTuple;

    py::class_<TRect2>(m, name.c_str())

        .def(py::init<>())
        .def(py::init<TVec2, TVec2>())
        .def(py::init<T, T, T, T>())
        .def(py::init<TRect2>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<TRect2>(s); }))

        .def(
            py::init([](const TVec2& position, const TVec2& size) {
                return TRect2::fromPositionSize(position, size);
            }),
            "position"_a,
            "size"_a)
        .def(
            py::init([](const TVec2& position, py::tuple size) {
                return TRect2::fromPositionSize(position, vecFromTuple<TVec2>(size));
            }),
            "position"_a,
            "size"_a)
        .def(
            py::init([](py::tuple position, const TVec2& size) {
                return TRect2::fromPositionSize(vecFromTuple<TVec2>(position), size);
            }),
            "position"_a,
            "size"_a)
        .def(
            py::init([](py::tuple position, py::tuple size) {
                return TRect2::fromPositionSize(
                    vecFromTuple<TVec2>(position), vecFromTuple<TVec2>(size));
            }),
            "position"_a,
            "size"_a)

        .def_property_readonly_static(
            "empty", [](py::object) -> TRect2 { return TRect2::empty; })
        .def("isEmpty", &TRect2::isEmpty)
        .def("normalize", &TRect2::normalize)
        .def("normalized", &TRect2::normalized)

        .def_property(
            "position",
            &TRect2::position,
            py::overload_cast<const TVec2&>(&TRect2::setPosition))
        .def_property(
            "size", &TRect2::size, py::overload_cast<const TVec2&>(&TRect2::setSize))
        .def_property("x", &TRect2::x, &TRect2::setX)
        .def_property("y", &TRect2::y, &TRect2::setY)
        .def_property("width", &TRect2::width, &TRect2::setWidth)
        .def_property("height", &TRect2::height, &TRect2::setHeight)

        .def_property(
            "pMin", &TRect2::pMin, py::overload_cast<const TVec2&>(&TRect2::setPMin))
        .def_property(
            "pMax", &TRect2::pMax, py::overload_cast<const TVec2&>(&TRect2::setPMax))
        .def_property("xMin", &TRect2::xMin, &TRect2::setXMin)
        .def_property("yMin", &TRect2::yMin, &TRect2::setYMin)
        .def_property("xMax", &TRect2::xMax, &TRect2::setXMax)
        .def_property("yMax", &TRect2::yMax, &TRect2::setYMax)

        .def("isClose", &TRect2::isClose, "other"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("isNear", &TRect2::isNear, "other"_a, "absTol"_a)
        .def("allNear", &TRect2::allNear, "other"_a, "absTol"_a)

        .def(py::self == py::self)
        .def(py::self != py::self)

        .def(
            "unitedWith",
            py::overload_cast<const TRect2&>(&TRect2::unitedWith, py::const_))
        .def(
            "unitedWith",
            py::overload_cast<const TVec2&>(&TRect2::unitedWith, py::const_))
        .def("uniteWith", py::overload_cast<const TRect2&>(&TRect2::uniteWith))
        .def("uniteWith", py::overload_cast<const TVec2&>(&TRect2::uniteWith))
        .def("intersectedWith", &TRect2::intersectedWith)
        .def("intersectWith", &TRect2::intersectWith)
        .def("intersects", &TRect2::intersects)
        .def("contains", py::overload_cast<const TRect2&>(&TRect2::contains, py::const_))
        .def("contains", py::overload_cast<const TVec2&>(&TRect2::contains, py::const_))
        .def("contains", py::overload_cast<T, T>(&TRect2::contains, py::const_))

        .def("__repr__", [](const TRect2& m) { return vgc::core::toString(m); });
}

} // namespace

void wrap_rect(py::module& m) {
    wrap_rect<double>(m, "Rect2d", 1e-9);
    wrap_rect<float>(m, "Rect2f", 1e-5f);
}
