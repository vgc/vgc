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

#include <vgc/ui/detail/paintutil.h>

#include <vgc/graphics/font.h>
#include <vgc/graphics/strings.h>

namespace vgc::ui::detail {

// clang-format off

void insertTriangle(
    core::FloatArray& a,
    float r, float g, float b,
    float x1, float y1, float x2, float y2, float x3, float y3) {

    a.extend({
        x1, y1, r, g, b,
        x2, y2, r, g, b,
        x3, y3, r, g, b});
}

void writeTriangleAt(
    core::FloatArray& a, Int i,
    float r, float g, float b,
    float x1, float y1, float x2, float y2, float x3, float y3) {

    constexpr Int numFloatsPerTriangle = 15;
    if (i < 0 || i + numFloatsPerTriangle > a.length()) {
        throw core::IndexError(
            core::format(
                "Cannot write triangle at index {}: "
                "array length is {} and {} floats must be written",
                i, a.length(), numFloatsPerTriangle));
    }
    float* p = &a.getUnchecked(i);
    *p     = x1; *(++p) = y1; *(++p) = r; *(++p) = g; *(++p) = b;
    *(++p) = x2; *(++p) = y2; *(++p) = r; *(++p) = g; *(++p) = b;
    *(++p) = x3; *(++p) = y3; *(++p) = r; *(++p) = g; *(++p) = b;
}

void insertTriangle(
    core::FloatArray& a,
    const core::Colorf& color,
    const geometry::Vec2f& v1,
    const geometry::Vec2f& v2,
    const geometry::Vec2f& v3) {

    insertTriangle(
        a,
        color.r(), color.g(), color.b(),
        v1.x(), v1.y(), v2.x(),v2.y(), v3.x(), v3.y());
}

void insertRect(
    core::FloatArray& a,
    float r, float g, float b,
    float x1, float y1, float x2, float y2) {
    
    a.extend({
        x1, y1, r, g, b,
        x2, y1, r, g, b,
        x1, y2, r, g, b,
        x2, y1, r, g, b,
        x2, y2, r, g, b,
        x1, y2, r, g, b});
}

void insertRect(
    core::FloatArray& a,
    const core::Colorf& color,
    const geometry::Rect2f& rect) {

    insertRect(
        a,
        color.r(), color.g(), color.b(),
        rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
}

void insertRect(
    core::FloatArray& a,
    const core::Color& c,
    float x1, float y1, float x2, float y2,
    float borderRadius) {

    float r = static_cast<float>(c[0]);
    float g = static_cast<float>(c[1]);
    float b = static_cast<float>(c[2]);
    float maxBorderRadius = 0.5f * (std::min)(std::abs(x2 - x1), std::abs(y2 - y1));
    borderRadius = core::clamp(borderRadius, 0.0f, maxBorderRadius);
    Int32 numCornerTriangles = core::ifloor<Int32>(borderRadius);
    if (numCornerTriangles < 1) {
        a.extend({
            x1, y1, r, g, b,
            x2, y1, r, g, b,
            x1, y2, r, g, b,
            x2, y1, r, g, b,
            x2, y2, r, g, b,
            x1, y2, r, g, b});
    }
    else {
        float x1_ = x1 + borderRadius;
        float x2_ = x2 - borderRadius;
        float y1_ = y1 + borderRadius;
        float y2_ = y2 - borderRadius;
        // center rectangle
        insertRect(a, r, g, b, x1_, y1_, x2_, y2_);
        // side rectangles
        insertRect(a, r, g, b, x1_, y1, x2_, y1_);
        insertRect(a, r, g, b, x2_, y1_, x2, y2_);
        insertRect(a, r, g, b, x1_, y2_, x2_, y2);
        insertRect(a, r, g, b, x1, y1_, x1_, y2_);
        // rounded corners
        float rcost_ = borderRadius;
        float rsint_ = 0;
        float pi = static_cast<float>(core::pi);
        float dt = (0.5f * pi) / numCornerTriangles;
        for (Int32 i = 1; i <= numCornerTriangles; ++i) {
            float t = i * dt;
            float rcost = borderRadius * std::cos(t);
            float rsint = borderRadius * std::sin(t);
            insertTriangle(a, r, g, b, x1_, y1_, x1_ - rcost_, y1_ - rsint_, x1_ - rcost, y1_ - rsint);
            insertTriangle(a, r, g, b, x2_, y1_, x2_ + rsint_, y1_ - rcost_, x2_ + rsint, y1_ - rcost);
            insertTriangle(a, r, g, b, x2_, y2_, x2_ + rcost_, y2_ + rsint_, x2_ + rcost, y2_ + rsint);
            insertTriangle(a, r, g, b, x1_, y2_, x1_ - rsint_, y2_ + rcost_, x1_ - rsint, y2_ + rcost);
            rcost_ = rcost;
            rsint_ = rsint;
        }
    }
}

void insertRect(
    core::FloatArray& a,
    const core::Color& color,
    const geometry::Rect2f& rect,
    float borderRadius) {

    insertRect(
        a,
        color,
        rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax(),
        borderRadius);
}

void insertRect(
    core::FloatArray& a,
    const core::Color& c,
    float x1, float y1, float x2, float y2) {

    float r = static_cast<float>(c[0]);
    float g = static_cast<float>(c[1]);
    float b = static_cast<float>(c[2]);
    insertRect(a, r, g, b, x1, y1, x2, y2);
}

void insertRect(
    core::FloatArray& a,
    const core::Color& color,
    const geometry::Rect2f& rect) {

    insertRect(
        a,
        color,
        rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
}

// clang-format on

namespace {

enum CornerType {
    TopLeft,
    TopRight,
    BottomRight,
    BottomLeft
};

template<CornerType cornerType>
geometry::Vec2f getCorner(const geometry::Rect2f& rect) {
    if constexpr (cornerType == TopLeft) {
        return geometry::Vec2f(rect.xMin(), rect.yMin());
    }
    else if constexpr (cornerType == TopRight) {
        return geometry::Vec2f(rect.xMax(), rect.yMin());
    }
    else if constexpr (cornerType == BottomRight) {
        return geometry::Vec2f(rect.xMax(), rect.yMax());
    }
    else if constexpr (cornerType == BottomLeft) {
        return geometry::Vec2f(rect.xMin(), rect.yMax());
    }
}

float getRadiusInPx(const style::LengthOrPercentage radius, float side) {
    float res;
    if (radius.isPercentage()) {
        res = side * radius.valuef() / 100.0f;
    }
    else {
        // TODO: convert to px
        res = radius.valuef();
    }
    return core::clamp(res, 0.0f, 0.5 * side);
}

geometry::Vec2f
getRadiusInPx(const style::BorderRadius& radius, const geometry::Rect2f& rect) {

    return geometry::Vec2f(
        getRadiusInPx(radius.horizontalRadius(), rect.width()),
        getRadiusInPx(radius.verticalRadius(), rect.height()));
}

template<CornerType cornerType>
geometry::Vec2f
getRadiusInPx(const style::BorderRadiuses& radiuses, const geometry::Rect2f& rect) {
    if constexpr (cornerType == TopLeft) {
        return getRadiusInPx(radiuses.topLeft(), rect);
    }
    else if constexpr (cornerType == TopRight) {
        return getRadiusInPx(radiuses.topRight(), rect);
    }
    else if constexpr (cornerType == BottomRight) {
        return getRadiusInPx(radiuses.bottomRight(), rect);
    }
    else if constexpr (cornerType == BottomLeft) {
        return getRadiusInPx(radiuses.bottomLeft(), rect);
    }
}

inline constexpr float halfPi = static_cast<float>(core::pi);

struct QuarterEllipseParams {
    float eps;
    float invPixelSize;
};

// For now, we insert both the first and last point.
// In the future, we want to omit them if equal to already inserted point.
template<CornerType cornerType>
void insertQuarterEllipse(
    core::FloatArray& a,
    const geometry::Rect2f& rect,
    const style::BorderRadiuses& borderRadiuses,
    const QuarterEllipseParams& params) {

    geometry::Vec2f corner = getCorner<cornerType>(rect);
    geometry::Vec2f radius = getRadiusInPx<cornerType>(borderRadiuses, rect);

    if (radius.x() < params.eps || radius.y() < params.eps) {
        a.extend({corner.x(), corner.y()});
    }
    else {
        // Note: the compiler should be able to optimize out multiplications by zero.
        // Even if it doesn't, it should be negligible compared to cos/sin computation.
        geometry::Vec2f sourceAxis;
        geometry::Vec2f targetAxis;
        if constexpr (cornerType == TopLeft) {
            sourceAxis = {-radius.x(), 0};
            targetAxis = {0, -radius.y()};
        }
        else if constexpr (cornerType == TopRight) {
            sourceAxis = {0, -radius.y()};
            targetAxis = {radius.x(), 0};
        }
        else if constexpr (cornerType == BottomRight) {
            sourceAxis = {radius.x(), 0};
            targetAxis = {0, radius.y()};
        }
        else if constexpr (cornerType == BottomLeft) {
            sourceAxis = {0, radius.y()};
            targetAxis = {-radius.x(), 0};
        }
        geometry::Vec2f center = corner - sourceAxis - targetAxis;

        float minRadius = (std::min)(radius.x(), radius.y());
        Int numSegments =
            core::clamp(core::ifloor<Int>(minRadius * params.invPixelSize), 1, 64);

        float dt = halfPi / numSegments;
        geometry::Vec2f p = center + sourceAxis;
        a.extend({p.x(), p.y()});
        for (Int i = 1; i < numSegments; ++i) {
            float t = i * dt;
            p = center + std::cos(t) * sourceAxis + std::sin(t) * targetAxis;
            a.extend({p.x(), p.y()});
        }
        p = center + targetAxis;
        a.extend({p.x(), p.y()});
    }
}

} // namespace

void insertRect(
    core::FloatArray& a,
    const core::Color& color,
    const geometry::Rect2f& rect,
    const style::BorderRadiuses& radiuses,
    float pixelSize) {

    if (rect.isEmpty()) {
        return;
    }

    QuarterEllipseParams params;
    params.eps = 1e-3f * pixelSize;
    params.invPixelSize = 1.0f / pixelSize;

    float r = static_cast<float>(color[0]);
    float g = static_cast<float>(color[1]);
    float b = static_cast<float>(color[2]);

    // Compute polygon as (x, y) data points, appending them to `a`
    Int oldLength = a.length();
    insertQuarterEllipse<TopLeft>(a, rect, radiuses, params);
    insertQuarterEllipse<TopRight>(a, rect, radiuses, params);
    insertQuarterEllipse<BottomRight>(a, rect, radiuses, params);
    insertQuarterEllipse<BottomLeft>(a, rect, radiuses, params);
    Int newLength = a.length();

    // Convert to triangle fan
    Int numPoints = (newLength - oldLength) / 2;
    Int numTriangles = numPoints - 2;
    VGC_ASSERT(numTriangles > 1);
    a.resize(oldLength + numTriangles * 15);
    float* p = &a.getUnchecked(oldLength);
    float x0 = *p;
    float y0 = *(p + 1);
    p = &a.getUnchecked(newLength) - 2;
    for (Int i = numTriangles - 1; i >= 0; --i) {
        p -= 2;
        writeTriangleAt(
            a, oldLength + 15 * i, r, g, b, x0, y0, *p, *(p + 1), *(p + 2), *(p + 3));
    }
}

core::Color getColor(const Widget* widget, core::StringId property) {
    core::Color res;
    style::StyleValue value = widget->style(property);
    if (value.has<core::Color>()) {
        res = value.to<core::Color>();
    }
    return res;
}

float getLength(const Widget* widget, core::StringId property) {
    float res = 0;
    style::StyleValue value = widget->style(property);
    if (value.type() == style::StyleValueType::Number) {
        res = value.toFloat();
    }
    return res;
}

style::BorderRadiuses getBorderRadiuses(Widget* widget) {

    namespace gs = graphics::strings;

    style::BorderRadius topLeft =
        widget->style(gs::border_top_left_radius).to<style::BorderRadius>();

    style::BorderRadius topRight =
        widget->style(gs::border_top_right_radius).to<style::BorderRadius>();

    style::BorderRadius bottomRight =
        widget->style(gs::border_bottom_right_radius).to<style::BorderRadius>();

    style::BorderRadius bottomLeft =
        widget->style(gs::border_bottom_left_radius).to<style::BorderRadius>();

    return style::BorderRadiuses(topLeft, topRight, bottomRight, bottomLeft);
}

} // namespace vgc::ui::detail
