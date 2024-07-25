// Copyright 2023 The VGC Developers
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

#ifndef VGC_GEOMETRY_TESSELATOR_H
#define VGC_GEOMETRY_TESSELATOR_H

#include <vgc/core/array.h>
#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/windingrule.h>

namespace vgc::geometry {

/// \class Tesselator
/// \brief A class to tesselate a list of contours.
///
class VGC_GEOMETRY_API Tesselator {
public:
    VGC_DISABLE_COPY_AND_MOVE(Tesselator);

    /// Creates a `Tesselator`.
    ///
    Tesselator();

    /// Destroys the `Tesselator`.
    ///
    ~Tesselator();

    /// Adds a contour to the polygon being tesselated.
    ///
    void addContour(core::ConstSpan<float> coords);

    /// Adds a contour to the polygon being tesselated.
    ///
    void addContour(core::ConstSpan<double> coords);

    /// Adds a contour to the polygon being tesselated.
    ///
    void addContour(core::ConstSpan<Vec2f> vertices);

    /// Adds a contour to the polygon being tesselated.
    ///
    void addContour(core::ConstSpan<Vec2d> vertices);

    /// Computes the tesselation, filling the given array with the (X, Y)
    /// values as a list of triangles.
    ///
    void tesselate(core::Array<float>& data, WindingRule windingRule);

    /// Computes the tesselation, filling the given array with the (X, Y)
    /// values as a list of triangles.
    ///
    void tesselate(core::Array<double>& data, WindingRule windingRule);

private:
    void* tess_;
    core::Array<float> buffer_;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_TESSELATOR_H
