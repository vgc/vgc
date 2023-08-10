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

#ifndef VGC_GEOMETRY_STROKESTYLE_H
#define VGC_GEOMETRY_STROKESTYLE_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \enum vgc::geometry::StrokeCap
/// \brief Specifies the style of stroke caps
///
enum class StrokeCap : UInt8 {

    /// The stroke is terminated by a straight line passing through the curve
    /// endpoint.
    ///
    Butt,

    /// The stroke is terminated by a smooth "round" shape. Typically, the
    /// shape is a half circle, but it can be a more general shape (such as a
    /// cubic Bézier) for curves with variable width, otherwise the cap
    /// wouldn't be smooth.
    ///
    Round,

    /// The stroke is terminated by straight line, similar to `Butt` but
    /// extending the length of the curve by half its width.
    ///
    Square
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(StrokeCap)

/// \enum vgc::geometry::StrokeJoin
/// \brief Specifies the style of stroke joins
///
enum class StrokeJoin : UInt8 {

    /// The stroke segments are joined by a straight line between the endpoints
    /// of the offset lines of each segment.
    ///
    Bevel,

    /// The stroke segments are joined by a smooth "round" shape. Typically,
    /// the shape is a circular arc, but it can be a more general shape (such
    /// as a cubic Bézier) for curves with variable width, otherwise the join
    /// wouldn't be smooth.
    ///
    Round,

    /// The stroke offset lines of each segment are extrapolated by a straight
    /// line until they intersect. If the intersection is too far away (as
    /// determined by the "miter limit"), then the join fall backs to the
    /// `Bevel` behavior.
    ///
    Miter
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(StrokeJoin)

/// \class vgc::geometry::StrokeStyle
/// \brief Specifies style parameters to use when stroking a curve
///
class VGC_GEOMETRY_API StrokeStyle {
public:
    /// Creates a default `StrokeStyle`.
    ///
    constexpr StrokeStyle() noexcept {
    }

    /// Creates `StrokeStyle` with the given cap style.
    ///
    explicit constexpr StrokeStyle(StrokeCap cap) noexcept
        : cap_(cap) {
    }

    /// Creates `StrokeStyle` with the given join style.
    ///
    explicit constexpr StrokeStyle(StrokeJoin join, double miterLimit = 4.0) noexcept
        : miterLimit_(miterLimit)
        , join_(join) {
    }

    /// Creates `StrokeStyle` with the given cap and join style.
    ///
    constexpr StrokeStyle(
        StrokeCap cap,
        StrokeJoin join,
        double miterLimit = 4.0) noexcept

        : miterLimit_(miterLimit)
        , cap_(cap)
        , join_(join) {
    }

    /// Returns the cap style of the stroke.
    ///
    /// The default is `StrokeCap::Butt`.
    ///
    /// \sa `setCap()`.
    ///
    constexpr StrokeCap cap() const {
        return cap_;
    }

    /// Sets the cap style of the stroke.
    ///
    /// \sa `cap()`.
    ///
    constexpr void setCap(StrokeCap cap) {
        cap_ = cap;
    }

    /// Returns the join style of the stroke.
    ///
    /// The default is `StrokeJoin::Miter`.
    ///
    /// \sa `setJoin()`, `miterLimit()`.
    ///
    constexpr StrokeJoin join() const {
        return join_;
    }

    /// Sets the join style of the stroke.
    ///
    /// \sa `join()`.
    ///
    constexpr void setJoin(StrokeJoin join) {
        join_ = join;
    }

    /// Sets the join style and miter limit of the stroke.
    ///
    /// \sa `join()`.
    ///
    constexpr void setJoin(StrokeJoin join, double miterLimit) {
        join_ = join;
        miterLimit_ = miterLimit;
    }

    /// Returns the miter limit of the stroke.
    ///
    /// The default is `4.0`.
    ///
    /// This attribute has no effect if the join style is not
    /// `StrokeJoin::Miter`.
    ///
    /// \sa `join()`, `setJoin()`, `setMiterLimit()`.
    ///
    constexpr double miterLimit() const {
        return miterLimit_;
    }

    /// Sets the miter limit of the stroke.
    ///
    /// \sa `miterLimit()`, `setJoin()`.
    ///
    constexpr void setMiterLimit(double miterLimit) {
        miterLimit_ = miterLimit;
    }

private:
    double miterLimit_ = 4.0;
    StrokeCap cap_ = StrokeCap::Butt;
    StrokeJoin join_ = StrokeJoin::Miter;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_STROKESTYLE_H
