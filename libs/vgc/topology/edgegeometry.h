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

#ifndef VGC_TOPOLOGY_EDGEGEOMETRY_H
#define VGC_TOPOLOGY_EDGEGEOMETRY_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/flags.h>
#include <vgc/geometry/curve.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/range1d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/topology/api.h>
#include <vgc/topology/dataobject.h>

namespace vgc::topology {

namespace detail {

class Operations;

} // namespace detail

// how to share edge shape correctly ?
// an inbetween edge that doesn't change should have the same shape for all times
// we also need edge shape source/def, which can be different curve types
// -> EdgeParameters

class VGC_TOPOLOGY_API EdgeBezierQuadSampling : DataObject {
public:
    EdgeBezierQuadSampling(core::Id id) noexcept
        : DataObject(id) {
    }

public:
private:
    // to be moved to curve.h.
    // array of Vec2d, points at odd indices are the middle control points.
    // maybe better in curve.h
    //core::Array<geometry::Vec2d> points_;
    //core::Array<double> widths_;
};

// generic parameters for all models
class VGC_TOPOLOGY_API SamplingParameters {
public:
    SamplingParameters() noexcept = default;

    //size_t hash() {
    //    return ...;
    //}

private:
    /*
    UInt8 Lod_ = 0;
    Int16 maxSamples_ = core::tmax<Int16>;
    double maxAngularError_ = 7.0;
    double pixelSize_ = 1.0;
    geometry::Mat3d viewMatrix_ = geometry::Mat3d::identity;
    */
    // mode, uniform s, uniform u -> overload
};

/// \class vgc::topology::KeyEdgeGeometry
/// \brief Authored model of the edge geometry.
///
/// It can be translated from dom or set manually.
///
// Edge geometry is relative to end vertices position.
// We want to snap the source geometry in its own space when:
//    - releasing a dragged end vertex
//    - right before sculpting
//    - right before control point dragging
// We have to snap output geometry (sampling) when the source
// geometry is not already snapped (happens in many cases).
//
// In which space do we sample ?
// inbetweening -> common ancestor for best identification of interest points
//

class VGC_TOPOLOGY_API KeyEdgeGeometry {
private:
    friend detail::Operations;

public:
    KeyEdgeGeometry() noexcept = default;

    virtual ~KeyEdgeGeometry() = default;

public:
    // public api

    // vertices in object space
    virtual void
    snapToVertices(const geometry::Vec2d& start, const geometry::Vec2d& end) = 0;

    // ideally, for inbetweening we would like a sampling that is good in 2 spaces:
    // - the common ancestor group space for best morphing
    // - the canvas space for best rendering

    virtual EdgeBezierQuadSampling
    computeSampling(const SamplingParameters& parameters) = 0;

protected:
    //virtual EdgeSampling computeSampling() = 0;
};

// key edge
//   geometry as pointer or type, but if it's a type it could be integrated to key edge...
//   if pointer then poly or inner pointer again ? poly is more efficient..

enum class KeyEdgeInterpolatedPointsGeometryFlag : UInt16 {
    None = 0x00,
    ReadOnly = 0x01,
};
VGC_DEFINE_FLAGS(
    KeyEdgeInterpolatedPointsGeometryFlags,
    KeyEdgeInterpolatedPointsGeometryFlag)

class VGC_TOPOLOGY_API KeyEdgeInterpolatedPointsGeometry : public KeyEdgeGeometry {
private:
    friend detail::Operations;

public:
    KeyEdgeInterpolatedPointsGeometry() noexcept = default;

    virtual ~KeyEdgeInterpolatedPointsGeometry() = default;

public:
    const geometry::SharedConstVec2dArray& points() const {
        return points_;
    }

    const core::SharedConstDoubleArray& widths() const {
        return widths_;
    }

    void setPoints(const geometry::SharedConstVec2dArray& points) {
        points_ = points;
    }

    void setPoints(geometry::Vec2dArray points) {
        points_ = geometry::SharedConstVec2dArray(std::move(points));
    }

    void setWidths(const core::SharedConstDoubleArray& widths) {
        widths_ = widths;
    }

    // public api impl

    // vertices in object space
    void
    snapToVertices(const geometry::Vec2d& start, const geometry::Vec2d& end) override;

    // ideally, for inbetweening we would like a sampling that is good in 2 spaces:
    // - the common ancestor group space for best morphing
    // - the canvas space for best rendering

    EdgeBezierQuadSampling computeSampling(const SamplingParameters& parameters) override;

protected:
    //virtual EdgeSampling computeSampling() = 0;

private:
    geometry::SharedConstVec2dArray points_;
    core::SharedConstDoubleArray widths_;
    KeyEdgeInterpolatedPointsGeometryFlags flags_ =
        KeyEdgeInterpolatedPointsGeometryFlag::None;
};

} // namespace vgc::topology

#endif // VGC_TOPOLOGY_EDGEGEOMETRY_H
