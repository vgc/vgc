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

#include <vgc/geometry/segmentintersector2.h>

#include <vgc/core/wraps/array.h>
#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

using vgc::geometry::Segment2;
using vgc::geometry::SegmentIntersection2;
using vgc::geometry::SegmentIntersectionType;
using vgc::geometry::SegmentIntersector2;
using vgc::geometry::Vec2;

namespace {

// Note: In C++, SegmentIntersector2d::Vertex is an alias of
// segmentintersector2::Vertex<double>, which is defined at
// namespace scope to make it deducible in partial template
// specializations. In Python, none of this is relevant, so we can more
// simply define it directly as a nested type of SegmentIntersector2d.
// Same for VertexSegment.

template<typename T>
void wrap_Vertex(py::handle scope) {
    using This = typename SegmentIntersector2<T>::Vertex;
    using VertexSegment = typename SegmentIntersector2<T>::VertexSegment;
    std::string name = "Vertex";
    vgc::core::wraps::Class<This>(scope, name.c_str())
        .def(py::init<>())
        .def(py::init<const Vec2<T>&>())
        .def(py::init<const Vec2<T>&, const vgc::core::Array<VertexSegment>&>())
        .def_property("position", &This::position, &This::setPosition)
        .def_property("segments", &This::segments, &This::setSegments)
        .def("addSegment", &This::addSegment);
    vgc::core::wraps::wrapArray<This>(scope, "Vertex");
}

template<typename T>
void wrap_VertexSegment(py::handle scope) {
    using This = typename SegmentIntersector2<T>::VertexSegment;
    using VertexIndex = typename SegmentIntersector2<T>::VertexIndex;
    using SegmentIndex = typename SegmentIntersector2<T>::SegmentIndex;
    std::string name = "VertexSegment";
    vgc::core::wraps::Class<This>(scope, name.c_str())
        .def(py::init<>())
        .def(py::init<VertexIndex, SegmentIndex, T>())
        .def_property("vertexIndex", &This::vertexIndex, &This::setVertexIndex)
        .def_property("segmentIndex", &This::segmentIndex, &This::setSegmentIndex)
        .def_property("parameter", &This::parameter, &This::setParameter);
    vgc::core::wraps::wrapArray<This>(scope, "VertexSegment");
}

template<typename T>
void wrap_Edge(py::handle scope) {
    using This = typename SegmentIntersector2<T>::Edge;
    using EdgeSegment = typename SegmentIntersector2<T>::EdgeSegment;
    std::string name = "Edge";
    vgc::core::wraps::Class<This>(scope, name.c_str())
        .def(py::init<>())
        .def(py::init<const Segment2<T>&>())
        .def(py::init<const Segment2<T>&, const vgc::core::Array<EdgeSegment>&>())
        .def_property("subsegment", &This::subsegment, &This::setSubsegment)
        .def_property("segments", &This::segments, &This::setSegments)
        .def("addSegment", &This::addSegment);
    vgc::core::wraps::wrapArray<This>(scope, "Edge");
}

template<typename T>
void wrap_EdgeSegment(py::handle scope) {
    using This = typename SegmentIntersector2<T>::EdgeSegment;
    using EdgeIndex = typename SegmentIntersector2<T>::EdgeIndex;
    using SegmentIndex = typename SegmentIntersector2<T>::SegmentIndex;
    std::string name = "EdgeSegment";
    vgc::core::wraps::Class<This>(scope, name.c_str())
        .def(py::init<>())
        .def(py::init<EdgeIndex, SegmentIndex, T, T>())
        .def_property("edgeIndex", &This::edgeIndex, &This::setEdgeIndex)
        .def_property("segmentIndex", &This::segmentIndex, &This::setSegmentIndex)
        .def_property("parameter1", &This::parameter1, &This::setParameter1)
        .def_property("parameter2", &This::parameter2, &This::setParameter2);
    vgc::core::wraps::wrapArray<This>(scope, "EdgeSegment");
}

template<typename T>
void wrap_SegmentIntersector2(py::module& m, const std::string& name) {

    using This = SegmentIntersector2<T>;

    using Vec2x = Vec2<T>;

    vgc::core::wraps::Class<This> c(m, name.c_str());
    c.def(py::init<>())
        .def("clear", &This::clear)
        .def("addSegment", &This::addSegment)
        .def(
            "addPolyline",
            [](This& self,
               py::iterable range,
               bool isClosed,
               bool hasDuplicateEndpoints) {
                self.addPolyline(
                    isClosed, hasDuplicateEndpoints, range, [](py::handle h) {
                        return py::cast<Vec2x>(h);
                    });
            },
            "range"_a,
            py::kw_only(),
            "isClosed"_a = false,
            "hasDuplicateEndpoints"_a = false)
        .def("computeIntersections", &This::computeIntersections)
        .def("intersectionPoints", &This::intersectionPoints)
        .def("intersectionSubsegments", &This::intersectionSubsegments);

    wrap_Vertex<T>(c);
    wrap_VertexSegment<T>(c);
    wrap_Edge<T>(c);
    wrap_EdgeSegment<T>(c);
}

} // namespace

void wrap_segmentintersector2(py::module& m) {
    wrap_SegmentIntersector2<double>(m, "SegmentIntersector2d");
    wrap_SegmentIntersector2<float>(m, "SegmentIntersector2f");
}
