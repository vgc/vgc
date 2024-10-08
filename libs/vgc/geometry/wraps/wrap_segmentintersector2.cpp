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

// Note: In C++, SegmentIntersector2d::PointIntersection is an alias of
// segmentintersector2::PointIntersection<double>, which is defined at
// namespace scope to make it deducible in partial template
// specializations. In Python, none of this is relevant, so we can more
// simply define it directly as a nested type of SegmentIntersector2d.
// Same for PointIntersectionInfo.

template<typename T>
void wrap_PointIntersection(py::handle scope) {
    using This = typename SegmentIntersector2<T>::PointIntersection;
    using PointIntersectionInfo = typename SegmentIntersector2<T>::PointIntersectionInfo;
    std::string name = "PointIntersection";
    vgc::core::wraps::Class<This>(scope, name.c_str())
        .def(py::init<>())
        .def(py::init<const Vec2<T>&>())
        .def(py::init<const Vec2<T>&, const vgc::core::Array<PointIntersectionInfo>&>())
        .def_property("position", &This::position, &This::setPosition)
        .def_property("infos", &This::infos, &This::setInfos)
        .def("addInfo", &This::addInfo);
    vgc::core::wraps::wrapArray<This>(scope, "PointIntersection");
}

template<typename T>
void wrap_PointIntersectionInfo(py::handle scope) {
    using This = typename SegmentIntersector2<T>::PointIntersectionInfo;
    using PointIntersectionIndex =
        typename SegmentIntersector2<T>::PointIntersectionIndex;
    using SegmentIndex = typename SegmentIntersector2<T>::SegmentIndex;
    std::string name = "PointIntersectionInfo";
    vgc::core::wraps::Class<This>(scope, name.c_str())
        .def(py::init<>())
        .def(py::init<PointIntersectionIndex, SegmentIndex, T>())
        .def_property(
            "pointIntersectionIndex",
            &This::pointIntersectionIndex,
            &This::setPointIntersectionIndex)
        .def_property("segmentIndex", &This::segmentIndex, &This::setSegmentIndex)
        .def_property("parameter", &This::parameter, &This::setParameter);
    vgc::core::wraps::wrapArray<This>(scope, "PointIntersectionInfo");
}

template<typename T>
void wrap_SegmentIntersector2(py::module& m, const std::string& name) {

    using This = SegmentIntersector2<T>;

    using Vec2x = Vec2<T>;
    using Vec2xArray = vgc::core::Array<Vec2x>;

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
                // Currently, py::iterable is broken, see:
                // https://github.com/pybind/pybind11/issues/5399
                // So drop(range, 1) in addPolyline() fails and we cannot do:
                // self.addPolyline(
                //     isClosed, hasDuplicateEndpoints, range, [](py::handle h) {
                //         return py::cast<Vec2x>(h);
                //     });
                // So we instead first convert to a Vec2d array
                Vec2xArray array(range, [](py::handle h) { return py::cast<Vec2x>(h); });
                self.addPolyline(isClosed, hasDuplicateEndpoints, array);
            },
            "range"_a,
            py::kw_only(),
            "isClosed"_a = false,
            "hasDuplicateEndpoints"_a = false)
        .def("computeIntersections", &This::computeIntersections)
        .def("pointIntersections", &This::pointIntersections);

    wrap_PointIntersection<T>(c);
    wrap_PointIntersectionInfo<T>(c);
}

} // namespace

void wrap_segmentintersector2(py::module& m) {
    wrap_SegmentIntersector2<double>(m, "SegmentIntersector2d");
    wrap_SegmentIntersector2<float>(m, "SegmentIntersector2f");
}
