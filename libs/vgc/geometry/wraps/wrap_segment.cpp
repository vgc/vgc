// Copyright 2024 The VGC Developers
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

#include <vgc/geometry/segment2d.h>
#include <vgc/geometry/segment2f.h>
#include <vgc/geometry/vec.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/wraps/vec.h>

using vgc::geometry::Segment2d;
using vgc::geometry::Segment2f;
using vgc::geometry::SegmentIntersectionType;
using vgc::geometry::Vec;

namespace {

template<typename T>
struct Segment2_ {};

template<>
struct Segment2_<float> {
    using type = Segment2f;
};

template<>
struct Segment2_<double> {
    using type = Segment2d;
};

template<typename T>
using Segment2 = typename Segment2_<T>::type;

void wrap_segmentIntersectionType(py::module& m) {
    py::enum_<SegmentIntersectionType>(m, "SegmentIntersectionType")
        .value("Empty", SegmentIntersectionType::Empty)
        .value("Point", SegmentIntersectionType::Point)
        .value("Segment", SegmentIntersectionType::Segment);
}

template<typename T>
void wrap_segment(py::module& m, const std::string& name) {

    // Fix https://github.com/pybind/pybind11/issues/1893
    auto self2 = py::self;

    using Segment2x = Segment2<T>;
    using Vec2x = Vec<2, T>;
    using Inter2x = typename Segment2x::IntersectionType;

    // Wrap Segment2xIntersection.
    //
    std::string intersectionName = name + "Intersection";
    vgc::core::wraps::Class<Inter2x>(m, intersectionName.c_str())
        .def(py::init<>())
        .def(py::init<Vec2x, T, T>())
        .def(py::init<Vec2x, Vec2x, T, T, T, T>())
        .def_property_readonly("type", &Inter2x::type)
        .def_property_readonly("p", &Inter2x::p)
        .def_property_readonly("q", &Inter2x::q)
        .def_property_readonly("s1", &Inter2x::s1)
        .def_property_readonly("t1", &Inter2x::t1)
        .def_property_readonly("s2", &Inter2x::s2)
        .def_property_readonly("t2", &Inter2x::t2)
        .def(py::self == py::self)
        .def(py::self != py::self);

    m.def(
        "segmentIntersect",
        py::overload_cast<const Vec2x&, const Vec2x&, const Vec2x&, const Vec2x&>(
            &vgc::geometry::segmentIntersect));

    // Wrap Segment2x.
    //
    vgc::core::wraps::Class<Segment2x>(m, name.c_str())

        .def(py::init<>())
        .def(py::init<Vec2x, Vec2x>())
        .def(py::init<T, T, T, T>())
        .def(py::init<Segment2x>())
        .def(
            py::init([](const std::string& s) { return vgc::core::parse<Segment2x>(s); }))

        .def(
            "__getitem__",
            [](const Segment2x& t, int i) {
                if (i < 0 || i >= 2) {
                    throw py::index_error();
                }
                return t[i];
            })

        .def(
            "__setitem__",
            [](Segment2x& t, int i, const Vec2x& v) {
                if (i < 0 || i >= 2) {
                    throw py::index_error();
                }
                t[i] = v;
            })

        .def_property(
            "a", &Segment2x::a, py::overload_cast<const Vec2x&>(&Segment2x::setA))
        .def_property(
            "b", &Segment2x::b, py::overload_cast<const Vec2x&>(&Segment2x::setB))

        .def("intersect", &Segment2x::intersect)

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

        .def("isDegenerate", &Segment2x::isDegenerate)

        .def("__repr__", [](const Segment2x& t) { return vgc::core::toString(t); });
}

} // namespace

void wrap_segment(py::module& m) {
    wrap_segmentIntersectionType(m);
    wrap_segment<double>(m, "Segment2d");
    wrap_segment<float>(m, "Segment2f");
}
