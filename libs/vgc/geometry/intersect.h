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

#ifndef VGC_GEOMETRY_INTERSECT_H
#define VGC_GEOMETRY_INTERSECT_H

#include <optional>

#include <vgc/geometry/api.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

/// Returns whether the segments `(a1, b1)` and `(a2, b2)` intersect.
///
/// It is a fast variant that considers collinear overlaps as non
/// intersecting.
///
inline bool fastSegmentIntersects(
    const geometry::Vec2d& a1,
    const geometry::Vec2d& b1,
    const geometry::Vec2d& a2,
    const geometry::Vec2d& b2) {

    geometry::Vec2d d1 = (b1 - a1);
    geometry::Vec2d d2 = (b2 - a2);

    // Solve 2x2 system using Cramer's rule.
    double delta = d1.det(d2);
    if (std::abs(delta) > core::epsilon) {
        geometry::Vec2d a1a2 = a2 - a1;
        double inv_delta = 1 / delta;
        double t1 = a1a2.det(d2) * inv_delta;
        double t2 = a1a2.det(d1) * inv_delta;
        if (t1 >= 0. && t1 <= 1. && t2 >= 0. && t2 <= 1.) {
            return true;
        }
    }

    return false;
}

/// Returns whether the segments `(a1, b1)` and `(a2, b2)` intersect
/// excluding `a2` and `b2`.
///
/// It is a fast variant that considers collinear overlaps as non
/// intersecting.
///
inline bool fastSemiOpenSegmentIntersects(
    const geometry::Vec2d& a1,
    const geometry::Vec2d& b1,
    const geometry::Vec2d& a2,
    const geometry::Vec2d& b2) {

    geometry::Vec2d d1 = (b1 - a1);
    geometry::Vec2d d2 = (b2 - a2);

    // Solve 2x2 system using Cramer's rule.
    double delta = d1.det(d2);
    if (std::abs(delta) > core::epsilon) {
        geometry::Vec2d a1a2 = a2 - a1;
        double inv_delta = 1 / delta;
        double t1 = a1a2.det(d2) * inv_delta;
        double t2 = a1a2.det(d1) * inv_delta;
        if (t1 >= 0. && t1 < 1. && t2 >= 0. && t2 < 1.) {
            return true;
        }
    }

    return false;
}

/// \class vgc::geometry::Segment2dPointIntersection
/// \brief Stores data about the intersection point between two 2D segments
///
class Segment2dPointIntersection {
public:
    /// Constructs an intersection point at the given `position` corresponding to
    /// parameters `t1` along the first segment and parameters `t2` along the
    /// second segment.
    ///
    Segment2dPointIntersection(const geometry::Vec2d& position, double t1, double t2)
        : position_(position)
        , t1_(t1)
        , t2_(t2) {
    }

    /// Returns the position of the intersection point.
    ///
    const geometry::Vec2d& position() const {
        return position_;
    }

    /// Returns the parameter `t1` along the first segment `(a1, b1)`, that is,
    /// `position()` is approximately equal to `lerp(a1, b1, t1())`.
    ///
    double t1() const {
        return t1_;
    }

    /// Returns the parameter `t2` along the second segment `(a2, b2)`, that is,
    /// `position()` is approximately equal to `lerp(a2, b2, t2())`.
    ///
    double t2() const {
        return t2_;
    }

private:
    geometry::Vec2d position_;
    double t1_;
    double t2_;
};

/// Returns the intersection point, if any, between the segments `(a1, b1)` and `(a2, b2)`.
///
/// It is a fast variant that considers collinear overlaps as non
/// intersecting.
///
inline std::optional<Segment2dPointIntersection> fastSegmentIntersection(
    const geometry::Vec2d& a1,
    const geometry::Vec2d& b1,
    const geometry::Vec2d& a2,
    const geometry::Vec2d& b2) {

    geometry::Vec2d d1 = (b1 - a1);
    geometry::Vec2d d2 = (b2 - a2);

    // Solve 2x2 system using Cramer's rule.
    double delta = d1.det(d2);
    if (std::abs(delta) > core::epsilon) {
        geometry::Vec2d a1a2 = a2 - a1;
        double inv_delta = 1 / delta;
        double t1 = a1a2.det(d2) * inv_delta;
        double t2 = a1a2.det(d1) * inv_delta;
        if (t1 >= 0. && t1 <= 1. && t2 >= 0. && t2 <= 1.) {
            geometry::Vec2d p = core::fastLerp(a1, b1, t1);
            return Segment2dPointIntersection(p, t1, t2);
        }
    }

    return std::nullopt;
}

enum class SegmentIntersectionType : UInt8 {
    Empty,
    Point,
    Segment
};

class Segment2dIntersection {
public:
    Segment2dIntersection()
        : type_(SegmentIntersectionType::Empty) {
    }

    Segment2dIntersection(const geometry::Vec2d& p, double t1, double t2)
        : p1_(p)
        , p2_(p)
        , s1_(t1)
        , t1_(t1)
        , s2_(t2)
        , t2_(t2)
        , type_(SegmentIntersectionType::Point) {
    }

    Segment2dIntersection(
        const geometry::Vec2d& p1,
        const geometry::Vec2d& p2,
        double s1,
        double t1,
        double s2,
        double t2)

        : p1_(p1)
        , p2_(p2)
        , s1_(s1)
        , t1_(t1)
        , s2_(s2)
        , t2_(t2)
        , type_(SegmentIntersectionType::Segment) {
    }

    SegmentIntersectionType type() const {
        return type_;
    }

    const geometry::Vec2d& p1() const {
        return p1_;
    }

    const geometry::Vec2d& p2() const {
        return p2_;
    }

    double s1() const {
        return s1_;
    }

    double t1() const {
        return t1_;
    }

    double s2() const {
        return s2_;
    }

    double t2() const {
        return t2_;
    }

private:
    geometry::Vec2d p1_ = core::noInit;
    geometry::Vec2d p2_ = core::noInit;
    double s1_; // no-init
    double t1_; // no-init
    double s2_; // no-init
    double t2_; // no-init
    SegmentIntersectionType type_;
};

/// Returns the intersection between the segments `(a1, b1)` and `(a2, b2)`.
///
/// TODO: Actually handle Segment case
///
inline Segment2dIntersection segmentIntersection(
    const geometry::Vec2d& a1,
    const geometry::Vec2d& b1,
    const geometry::Vec2d& a2,
    const geometry::Vec2d& b2) {

    geometry::Vec2d d1 = (b1 - a1);
    geometry::Vec2d d2 = (b2 - a2);

    // Solve 2x2 system using Cramer's rule.
    double delta = d1.det(d2);
    if (std::abs(delta) > 0) {
        if (a1 == a2) {
            return Segment2dIntersection(a1, 0, 0);
        }
        else if (b1 == b2) {
            return Segment2dIntersection(b1, 1, 1);
        }
        else if (a1 == b2) {
            return Segment2dIntersection(a1, 0, 1);
        }
        else if (a2 == b1) {
            return Segment2dIntersection(a1, 1, 0);
        }
        geometry::Vec2d a1a2 = a2 - a1;
        double inv_delta = 1 / delta;
        double t1 = a1a2.det(d2) * inv_delta;
        double t2 = a1a2.det(d1) * inv_delta;
        if (t1 >= 0. && t1 <= 1. && t2 >= 0. && t2 <= 1.) {
            geometry::Vec2d p = core::fastLerp(a1, b1, t1);
            return Segment2dIntersection(p, t1, t2);
        }
        else {
            return Segment2dIntersection();
        }
    }
    else {
        // Collinear => either empty intersection of segment intersection
        // For now we return a dummy segment intersection so that we can
        // at least detect this.
        // TODO: actually compute the segment or empty intersection.
        return Segment2dIntersection(a1, a1, 0, 0, 0, 0);
    }
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_INTERSECT_H
