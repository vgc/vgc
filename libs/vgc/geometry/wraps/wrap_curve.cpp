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

#include <vgc/geometry/curve.h>
#include <vgc/geometry/vec2d.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

void wrap_curve(py::module& m) {

    using This = vgc::geometry::Curve;
    //using Vec2d = vgc::geometry::Vec2d;

    vgc::core::wraps::Class<This>(m, "Curve")
        .def(py::init<>())

        // Note: there's a more readable syntax to disambiguate function
        // overloads if we use C++14. See:
        // http://pybind11.readthedocs.io/en/latest/classes.html#overloaded-methods

        /*.def(
            "addControlPoint",
            py::overload_cast<double, double, double>(&This::addControlPoint))
        .def(
            "addControlPoint",
            py::overload_cast<const Vec2d&, double>(&This::addControlPoint))*/

        .def("__repr__", [](const This& c) {
            return "<Curve containing " //
                   + std::to_string(
                       c.positionData() ? (c.positionData()->size() / 2) : 0) //
                   + " control points>";
        });
}
