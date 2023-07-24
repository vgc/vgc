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
#include <vgc/geometry/rect2d.h>
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

namespace detail {

class SvgParser;

} // namespace detail

/// A simplified representation of an SVG path element.
///
/// Group `transform` attributes are already applied to the returned `curves()`
/// data, and `fill`, `stroke`, and `stroke-width` attributes have been
/// resolved to simple color.
///
class VGC_GRAPHICS_API SvgSimplePath {
public:
    /// Returns the geometry of the centerline of the path.
    ///
    /// Note that any `transform` attributes on the path itself or any of its
    /// ancestors are already baked in the returned geometry.
    ///
    const geometry::Curves2d& curves() const {
        return curves_;
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
    /// Note that any `transform` attributes on the path itself or any of its
    /// ancestors are already baked in the returned stroke width.
    ///
    /// In the presence of skewing or non-uniform scaling, it is unfortunately
    /// not possible to correctly bake in the `transform` attributes into the
    /// stroke width: transforming a stroked curve is not the same as stroking
    /// a transformed curve. In this cases, we choose to scale the stroke width
    /// by `sqrt(|det(transform)|)`, which can be interpreted as the geometric
    /// mean of the x-scale and y-scale.
    ///
    double strokeWidth() const {
        return strokeWidth_;
    }

    // XXX Have a "transform()" rather than having baked-in transforms?

private:
    friend detail::SvgParser;

    geometry::Curves2d curves_;
    SvgPaint fill_ = {};
    SvgPaint stroke_ = {};
    double strokeWidth_ = 0;

    SvgSimplePath() = default;
};

/// Parses the given `svg` data and returns all the `<path>` elements as an
/// array of `SvgSimplePath` instances.
///
VGC_GRAPHICS_API
core::Array<SvgSimplePath> getSvgSimplePaths(std::string_view svg);

/// Parses the given `svg` data and returns the size of the view the `<path>` elements as an
/// array of `SvgSimplePath` instances.
///
VGC_GRAPHICS_API
geometry::Rect2d getSvgViewBox(std::string_view svg);

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_SVG_H
