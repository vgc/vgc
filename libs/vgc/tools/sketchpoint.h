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

#ifndef VGC_TOOLS_SKETCHPOINT_H
#define VGC_TOOLS_SKETCHPOINT_H

#include <vgc/core/array.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/tools/api.h>

namespace vgc::tools {

/// \class vgc::tools::SketchPoint
/// \brief Stores data about one sample when sketching
///
class VGC_TOOLS_API SketchPoint {
public:
    /// Creates a zero-initialized `SketchPoint`.
    ///
    constexpr SketchPoint() noexcept
        : position_()
        , pressure_(0)
        , timestamp_(0)
        , width_(0)
        , s_(0) {
    }

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized

    /// Creates an uninitialized `SketchPoint`.
    ///
    SketchPoint(core::NoInit) noexcept
        : position_(core::noInit) {
    }

    VGC_WARNING_POP

    /// Creates a `SketchPoint` initialized with the given values.
    ///
    SketchPoint(
        const geometry::Vec2d& position,
        double pressure,
        double timestamp,
        double width,
        double s = 0) noexcept

        : position_(position)
        , pressure_(pressure)
        , timestamp_(timestamp)
        , width_(width)
        , s_(s) {
    }

    /// Returns the position of this `SketchPoint`.
    ///
    const geometry::Vec2d& position() const {
        return position_;
    }

    /// Sets the position of this `SketchPoint`.
    ///
    void setPosition(const geometry::Vec2d& position) {
        position_ = position;
    }

    /// Returns the pressure of this `SketchPoint`.
    ///
    double pressure() const {
        return pressure_;
    }

    /// Sets the pressure of this `SketchPoint`.
    ///
    void setPressure(double pressure) {
        pressure_ = pressure;
    }

    /// Returns the timestamp of this `SketchPoint`.
    ///
    double timestamp() const {
        return timestamp_;
    }

    /// Sets the timestamp of this `SketchPoint`.
    ///
    void setTimestamp(double timestamp) {
        timestamp_ = timestamp;
    }

    /// Returns the width of this `SketchPoint`.
    ///
    double width() const {
        return width_;
    }

    /// Sets the width of this `SketchPoint`.
    ///
    void setWidth(double width) {
        width_ = width;
    }

    /// Returns the cumulative chordal distance from the first point to this
    /// point.
    ///
    double s() const {
        return s_;
    }

    /// Sets the cumulative chordal distance from the first point to this
    /// point.
    ///
    void setS(double s) {
        s_ = s;
    }

private:
    geometry::Vec2d position_;
    double pressure_;
    double timestamp_;
    double width_;
    double s_;
};

using SketchPointArray = core::Array<SketchPoint>;

} // namespace vgc::tools

#endif // VGC_TOOLS_SKETCHPOINT_H
