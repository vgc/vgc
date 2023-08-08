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

#ifndef VGC_GRAPHICS_SVG_H
#define VGC_GRAPHICS_SVG_H

#include <vgc/core/object.h>

#include <vgc/core/color.h>
#include <vgc/geometry/curves2d.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/strokestyle.h>
#include <vgc/graphics/api.h>

namespace vgc::graphics {

/// \enum vgc::graphics::CssValueType
/// \brief Represents the type of an `SvgPaint` value.
///
// https://www.w3.org/TR/SVG11/painting.html#InterfaceSVGPaint
//
enum class SvgPaintType : Int8 {
    None,
    Color,
    Url,

    /*
    Unknown,
    CurrentColor,
    RgbColor,
    RgbColorIccColor,
    UriNone,
    UriCurrentColor,
    UriRgbColor,
    UriRgbColorIccColor
    (Inherit?) -> Given by CssValueType
    */
};

/// \class vgc::graphics::SvgPaint
/// \brief Represents the value of `fill` and `stroke` SVG attributes
///
// SVG 1.1
// https://www.w3.org/TR/SVG11/painting.html#SpecifyingPaint
// <paint>: none |
//          currentColor |
//          <color> [<icccolor>] |
//          <funciri> [ none | currentColor | <color> [<icccolor>] ] |
//          inherit
//
// The SvgPaint and SvgColor DOM interfaces are marked as "deprecated"
//
// SVG 2
// https://svgwg.org/svg2-draft/painting.html#SpecifyingPaint
// <paint> = none |
//           <color> |
//           <url> [none | <color>]? |
//           context-fill |
//           context-stroke
//
// Thus, it is unclear where all of this is going. For now, we stay close to
// SVG 1.1, but without the inheritance hierarchy CssValue > SvgColor >
// SvgPaint.
//
class VGC_GRAPHICS_API SvgPaint {
public:
    /// Creates a `SvgPaint` of type `None`.
    ///
    SvgPaint() {
    }

    /// Creates a `SvgPaint` of type `Color` with the given `color`.
    ///
    SvgPaint(const core::Color& color)
        : paintType_(SvgPaintType::Color)
        , color_(color) {
    }

    /// Returns the `SvgPaintType` of this `SvgPaint`.
    ///
    // Note: we do not call this simply "type()" to prevent future potential
    // collisions with CssValue::valueType() or SvgColor::colorType().
    //
    SvgPaintType paintType() const {
        return paintType_;
    }

    /// Returns the color of this `SvgPaint`.
    ///
    /// Returns a black transparent color if `paintType` is `None`.
    ///
    core::Color color() const {
        return color_;
    }

    /// Sets this color or this `SvgPaint` to be the given color.
    ///
    /// The type of this `SvgPaint` becomes `color` if this was not
    /// already the case.
    ///
    void setColor(const core::Color& color) {
        paintType_ = SvgPaintType::Color;
        color_ = color;
    }

private:
    SvgPaintType paintType_ = SvgPaintType::None;
    core::Color color_ = {0, 0, 0, 0}; // transparent
};

/// \enum vgc::graphics::SvgStrokeCap
/// \brief Specifies the style of SVG stroke caps
///
enum class SvgStrokeLineCap : UInt8 {

    /// The stroke is terminated by a straight line passing through the curve
    /// endpoint.
    ///
    Butt,

    /// The stroke is terminated by a half disk.
    ///
    Round,

    /// The stroke is terminated by straight line, similar to `Butt` but
    /// extending the length of the curve by half its width.
    ///
    Square
};

VGC_GRAPHICS_API
VGC_DECLARE_ENUM(SvgStrokeLineCap)

namespace detail {

class SvgParser;

} // namespace detail

/// \class vgc::graphics::SvgSimplePath
/// \brief A simplified "flattened" representation of an SVG path element.
///
/// The `transform()` method returns the cumulated transform of this path and
/// its ancestors.
///
/// The `fill()`, `stroke()`, and `strokeWidth` methods returns the resolved
/// style taking into account ancestor's style if any.
///
class VGC_GRAPHICS_API SvgSimplePath {
public:
    /// Returns the geometry of the centerline of the path, in local coordinates.
    ///
    const geometry::Curves2d& curves() const {
        return curves_;
    }

    /// Returns the cumulated `transform` attribute of the path.
    ///
    const geometry::Mat3d& transform() const {
        return transform_;
    }

    /// Returns the resolved `fill` attribute of the path.
    ///
    const SvgPaint& fill() const {
        return fill_;
    }

    /// Returns the resolved `stroke` attribute of the path.
    ///
    const SvgPaint& stroke() const {
        return stroke_;
    }

    /// Returns the resolved `stroke-width` attribute of the path.
    ///
    double strokeWidth() const {
        return strokeWidth_;
    }

    /// Returns the resolved `StrokeStyle` of the path.
    ///
    const geometry::StrokeStyle& strokeStyle() const {
        return strokeStyle_;
    }

    /// Returns the value of the style "class" attribute.
    ///
    std::string_view styleClass() const {
        return styleClass_;
    }

    /// Returns the classes value of the style "class" attribute.
    ///
    const core::Array<std::string>& styleClasses() const {
        return styleClasses_;
    }

private:
    friend detail::SvgParser;

    geometry::Curves2d curves_;
    geometry::Mat3d transform_;
    SvgPaint fill_ = {};
    SvgPaint stroke_ = {};
    double strokeWidth_ = 0;
    geometry::StrokeStyle strokeStyle_;

    std::string styleClass_;
    core::Array<std::string> styleClasses_;

    SvgSimplePath() = default;
};

/// Parses the given `svg` data and returns all the `<path>` elements as an
/// array of `SvgSimplePath` instances.
///
VGC_GRAPHICS_API
core::Array<SvgSimplePath> getSvgSimplePaths(std::string_view svg);

/// Parses the given `svg` data and returns the SVG's viewbox.
///
/// If the `viewBox` attribute is not provided, it is determined
/// from the `width` and `height` attribute.
///
VGC_GRAPHICS_API
geometry::Rect2d getSvgViewBox(std::string_view svg);

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_SVG_H
