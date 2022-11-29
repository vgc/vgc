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

#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/rect2f.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>
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

    using This = Rect2<T>;
    using Vec2x = Vec2<T>;

    using vgc::geometry::wraps::vecFromTuple;

    vgc::core::wraps::Class<This>(m, name.c_str())

        .def(py::init<>())
        .def(py::init<Vec2x, Vec2x>())
        .def(py::init<T, T, T, T>())
        .def(py::init<This>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<This>(s); }))

        .def(
            py::init([](const Vec2x& position, const Vec2x& size) {
                return This::fromPositionSize(position, size);
            }),
            "position"_a,
            "size"_a)
        .def(
            py::init([](const Vec2x& position, py::tuple size) {
                return This::fromPositionSize(position, vecFromTuple<Vec2x>(size));
            }),
            "position"_a,
            "size"_a)
        .def(
            py::init([](py::tuple position, const Vec2x& size) {
                return This::fromPositionSize(vecFromTuple<Vec2x>(position), size);
            }),
            "position"_a,
            "size"_a)
        .def(
            py::init([](py::tuple position, py::tuple size) {
                return This::fromPositionSize(
                    vecFromTuple<Vec2x>(position), vecFromTuple<Vec2x>(size));
            }),
            "position"_a,
            "size"_a)

        .def_property_readonly_static(
            "empty", [](py::object) -> This { return This::empty; })
        .def("isEmpty", &This::isEmpty)
        .def("isDegenerate", &This::isDegenerate)
        .def("normalize", &This::normalize)
        .def("normalized", &This::normalized)

        .def_property(
            "position",
            &This::position,
            py::overload_cast<const Vec2x&>(&This::setPosition))
        .def_property(
            "size", &This::size, py::overload_cast<const Vec2x&>(&This::setSize))
        .def_property("x", &This::x, &This::setX)
        .def_property("y", &This::y, &This::setY)
        .def_property("width", &This::width, &This::setWidth)
        .def_property("height", &This::height, &This::setHeight)

        .def_property(
            "pMin", &This::pMin, py::overload_cast<const Vec2x&>(&This::setPMin))
        .def_property(
            "pMax", &This::pMax, py::overload_cast<const Vec2x&>(&This::setPMax))
        .def_property("xMin", &This::xMin, &This::setXMin)
        .def_property("yMin", &This::yMin, &This::setYMin)
        .def_property("xMax", &This::xMax, &This::setXMax)
        .def_property("yMax", &This::yMax, &This::setYMax)

        .def("corner", py::overload_cast<vgc::Int>(&This::corner, py::const_))
        .def("corner", py::overload_cast<vgc::Int, vgc::Int>(&This::corner, py::const_))

        .def("isClose", &This::isClose, "other"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("isNear", &This::isNear, "other"_a, "absTol"_a)
        .def("allNear", &This::allNear, "other"_a, "absTol"_a)

        .def(py::self == py::self)
        .def(py::self != py::self)

        .def("unitedWith", py::overload_cast<const This&>(&This::unitedWith, py::const_))
        .def("unitedWith", py::overload_cast<const Vec2x&>(&This::unitedWith, py::const_))
        .def("uniteWith", py::overload_cast<const This&>(&This::uniteWith))
        .def("uniteWith", py::overload_cast<const Vec2x&>(&This::uniteWith))
        .def("intersectedWith", &This::intersectedWith)
        .def("intersectWith", &This::intersectWith)
        .def("intersects", &This::intersects)
        .def("contains", py::overload_cast<const This&>(&This::contains, py::const_))
        .def("contains", py::overload_cast<const Vec2x&>(&This::contains, py::const_))
        .def("contains", py::overload_cast<T, T>(&This::contains, py::const_))

        .def("__repr__", [](const This& m) { return vgc::core::toString(m); });
}

} // namespace

void wrap_rect(py::module& m) {
    wrap_rect<double>(m, "Rect2d", 1e-9);
    wrap_rect<float>(m, "Rect2f", 1e-5f);
}
