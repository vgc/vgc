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
#include <vgc/ui/margins.h>

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

void writeTriangleAt(
    core::FloatArray& a, Int i,
    float r, float g, float b,
    const geometry::Triangle2f& t) {

    constexpr Int numFloatsPerTriangle = 15;
    if (i < 0 || i + numFloatsPerTriangle > a.length()) {
        throw core::IndexError(
            core::format(
                "Cannot write triangle at index {}: "
                "array length is {} and {} floats must be written",
                i, a.length(), numFloatsPerTriangle));
    }
    float* p = &a.getUnchecked(i);
    *p     = t[0][0]; *(++p) = t[0][1]; *(++p) = r; *(++p) = g; *(++p) = b;
    *(++p) = t[1][0]; *(++p) = t[1][1]; *(++p) = r; *(++p) = g; *(++p) = b;
    *(++p) = t[2][0]; *(++p) = t[2][1]; *(++p) = r; *(++p) = g; *(++p) = b;
}

void insertTriangle(
    core::FloatArray& a,
    const core::Color& color,
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
    const core::Color& color,
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

// clang-format on

namespace {

enum CornerType {
    TopLeft = 0,
    TopRight,
    BottomRight,
    BottomLeft
};

enum EllipseAxis {
    HAxis = 0,
    VAxis
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

inline constexpr float halfPi = 0.5f * static_cast<float>(core::pi);

struct QuarterEllipseParams {
    float eps;
    float invPixelSize;
};

// Note: for now, in insertQuarterEllipse, we insert both the first and last
// point. In the future, we want to omit them if equal to already inserted
// point.

template<CornerType cornerType>
void insertQuarterEllipse(
    core::FloatArray& a,
    const geometry::Rect2f& rect,
    const style::BorderRadiusesInPx<float>& radiuses,
    const QuarterEllipseParams& params) {

    geometry::Vec2f corner = rect.corner(cornerType);
    style::BorderRadiusInPx<float> radius = radiuses[cornerType];

    if (radius.horizontalRadius() < params.eps || radius.verticalRadius() < params.eps) {
        a.extend({corner.x(), corner.y()});
    }
    else {
        // Note: the compiler should be able to optimize out multiplications by zero.
        // Even if it doesn't, it should be negligible compared to cos/sin computation.
        geometry::Vec2f sourceAxis;
        geometry::Vec2f targetAxis;
        if constexpr (cornerType == TopLeft) {
            sourceAxis = {-radius.horizontalRadius(), 0};
            targetAxis = {0, -radius.verticalRadius()};
        }
        else if constexpr (cornerType == TopRight) {
            sourceAxis = {0, -radius.verticalRadius()};
            targetAxis = {radius.horizontalRadius(), 0};
        }
        else if constexpr (cornerType == BottomRight) {
            sourceAxis = {radius.horizontalRadius(), 0};
            targetAxis = {0, radius.verticalRadius()};
        }
        else if constexpr (cornerType == BottomLeft) {
            sourceAxis = {0, radius.verticalRadius()};
            targetAxis = {-radius.horizontalRadius(), 0};
        }
        geometry::Vec2f center = corner - sourceAxis - targetAxis;

        float minRadius = (std::min)(radius.horizontalRadius(), radius.verticalRadius());
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

template<CornerType cornerType>
void insertQuarterEllipseWithBorder(
    core::FloatArray& a,
    const geometry::Rect2f& innerRect,
    const geometry::Rect2f& outerRect,
    const style::BorderRadiusesInPx<float>& innerRadiuses,
    const style::BorderRadiusesInPx<float>& outerRadiuses,
    const style::BorderRadiusesInPx<float>& refRadiuses,
    const QuarterEllipseParams& params) {

    style::BorderRadiusInPx<float> radius = refRadiuses[cornerType];

    float minRadius = (std::min)(radius.horizontalRadius(), radius.verticalRadius());
    Int numSegments =
        core::clamp(core::ifloor<Int>(minRadius * params.invPixelSize), 1, 64);

    insertQuarterEllipseWithBorder<cornerType>(
        a, innerRect, outerRect, innerRadiuses, outerRadiuses, numSegments, params);
}

template<CornerType cornerType>
void insertQuarterEllipseWithBorder(
    core::FloatArray& a,
    const geometry::Rect2f& innerRect,
    const geometry::Rect2f& outerRect,
    const style::BorderRadiusesInPx<float>& innerRadiuses,
    const style::BorderRadiusesInPx<float>& outerRadiuses,
    const QuarterEllipseParams& params) {

    style::BorderRadiusInPx<float> outerRadius = outerRadiuses[cornerType];

    float minOuterRadius =
        (std::min)(outerRadius.horizontalRadius(), outerRadius.verticalRadius());
    Int numSegments =
        core::clamp(core::ifloor<Int>(minOuterRadius * params.invPixelSize), 1, 64);

    insertQuarterEllipseWithBorder<cornerType>(
        a, innerRect, outerRect, innerRadiuses, outerRadiuses, numSegments, params);
}

template<CornerType cornerType>
void insertQuarterEllipseWithBorder(
    core::FloatArray& a,
    const geometry::Rect2f& innerRect,
    const geometry::Rect2f& outerRect,
    const style::BorderRadiusesInPx<float>& innerRadiuses,
    const style::BorderRadiusesInPx<float>& outerRadiuses,
    const Int numSegments,
    const QuarterEllipseParams& params) {

    geometry::Vec2f innerCorner = getCorner<cornerType>(innerRect);
    geometry::Vec2f outerCorner = getCorner<cornerType>(outerRect);
    style::BorderRadiusInPx<float> innerRadius = innerRadiuses[cornerType];
    style::BorderRadiusInPx<float> outerRadius = outerRadiuses[cornerType];

    if (outerRadius.horizontalRadius() < params.eps
        || outerRadius.verticalRadius() < params.eps) {

        a.extend({innerCorner.x(), innerCorner.y(), outerCorner.x(), outerCorner.y()});
    }
    else {
        // Note: the compiler should be able to optimize out multiplications by zero.
        // Even if it doesn't, it should be negligible compared to cos/sin computation.
        geometry::Vec2f innerSourceAxis;
        geometry::Vec2f innerTargetAxis;
        geometry::Vec2f outerSourceAxis;
        geometry::Vec2f outerTargetAxis;
        if constexpr (cornerType == TopLeft) {
            innerSourceAxis = {-innerRadius.horizontalRadius(), 0};
            innerTargetAxis = {0, -innerRadius.verticalRadius()};
            outerSourceAxis = {-outerRadius.horizontalRadius(), 0};
            outerTargetAxis = {0, -outerRadius.verticalRadius()};
        }
        else if constexpr (cornerType == TopRight) {
            innerSourceAxis = {0, -innerRadius.verticalRadius()};
            innerTargetAxis = {innerRadius.horizontalRadius(), 0};
            outerSourceAxis = {0, -outerRadius.verticalRadius()};
            outerTargetAxis = {outerRadius.horizontalRadius(), 0};
        }
        else if constexpr (cornerType == BottomRight) {
            innerSourceAxis = {innerRadius.horizontalRadius(), 0};
            innerTargetAxis = {0, innerRadius.verticalRadius()};
            outerSourceAxis = {outerRadius.horizontalRadius(), 0};
            outerTargetAxis = {0, outerRadius.verticalRadius()};
        }
        else if constexpr (cornerType == BottomLeft) {
            innerSourceAxis = {0, innerRadius.verticalRadius()};
            innerTargetAxis = {-innerRadius.horizontalRadius(), 0};
            outerSourceAxis = {0, outerRadius.verticalRadius()};
            outerTargetAxis = {-outerRadius.horizontalRadius(), 0};
        }
        geometry::Vec2f innerCenter = innerCorner - innerSourceAxis - innerTargetAxis;
        geometry::Vec2f outerCenter = outerCorner - outerSourceAxis - outerTargetAxis;

        float dt = halfPi / numSegments;
        geometry::Vec2f innerPoint = innerCenter + innerSourceAxis;
        geometry::Vec2f outerPoint = outerCenter + outerSourceAxis;
        a.extend({innerPoint.x(), innerPoint.y(), outerPoint.x(), outerPoint.y()});
        for (Int i = 1; i < numSegments; ++i) {
            float t = i * dt;
            float cost = std::cos(t);
            float sint = std::sin(t);
            innerPoint = innerCenter + cost * innerSourceAxis + sint * innerTargetAxis;
            outerPoint = outerCenter + cost * outerSourceAxis + sint * outerTargetAxis;
            a.extend({innerPoint.x(), innerPoint.y(), outerPoint.x(), outerPoint.y()});
        }
        innerPoint = innerCenter + innerTargetAxis;
        outerPoint = outerCenter + outerTargetAxis;
        a.extend({innerPoint.x(), innerPoint.y(), outerPoint.x(), outerPoint.y()});
    }
}

} // namespace

void insertRect(
    core::FloatArray& a,
    const core::Color& color,
    const geometry::Rect2f& rect,
    const style::BorderRadiuses& radiuses_,
    float pixelSize) {

    if (rect.isEmpty()) {
        return;
    }

    QuarterEllipseParams params;
    params.eps = 1e-3f * pixelSize;
    params.invPixelSize = 1.0f / pixelSize;

    float scaleFactor = 1;
    style::BorderRadiusesInPx radiuses =
        radiuses_.toPx(scaleFactor, rect.width(), rect.height());

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
    geometry::Triangle2f triangle(core::NoInit{});
    triangle.setA(x0, y0);
    p = &a.getUnchecked(newLength) - 2;
    for (Int i = numTriangles - 1; i >= 0; --i) {
        p -= 2;
        triangle.setB(*p, *(p + 1));
        triangle.setC(*(p + 2), *(p + 3));
        writeTriangleAt(a, oldLength + 15 * i, r, g, b, triangle);
    }
}

void insertRect(
    core::FloatArray& a,
    const core::Color& fillColor,
    const core::Color& borderColor,
    const geometry::Rect2f& outerRect,
    const style::BorderRadiuses& outerRadiuses_,
    float borderWidth,
    float pixelSize) {

    float scaleFactor = 1;
    style::BorderRadiusesInPx<float> outerRadiuses =
        outerRadiuses_.toPx(scaleFactor, outerRect.width(), outerRect.height());
    insertRect(
        a,
        fillColor,
        borderColor,
        outerRect,
        outerRadiuses,
        outerRadiuses,
        borderWidth,
        pixelSize);
}

void insertRect(
    core::FloatArray& a,
    const core::Color& fillColor,
    const core::Color& borderColor,
    const geometry::Rect2f& outerRect,
    const style::BorderRadiusesInPx<float>& outerRadiuses,
    const style::BorderRadiusesInPx<float>& refRadiuses,
    float borderWidth,
    float pixelSize) {

    if (outerRect.isEmpty()) {
        return;
    }
    bool hasFill = fillColor.a() > 0;
    bool hasBorder = borderColor.a() > 0;
    if (!hasFill && !hasBorder) {
        return;
    }

    // Compute inner rect, clamping border-[top|right|bottom|left]-width
    ui::Margins inputBorderWidths = ui::Margins(borderWidth);
    geometry::Rect2f innerRect = outerRect - inputBorderWidths;
    if (innerRect.xMin() > innerRect.xMax()) {
        float middle = 0.5f * (innerRect.xMin() + innerRect.xMax());
        innerRect.setXMin(middle);
        innerRect.setXMax(middle);
    }
    if (innerRect.yMin() > innerRect.yMax()) {
        float middle = 0.5f * (innerRect.yMin() + innerRect.yMax());
        innerRect.setYMin(middle);
        innerRect.setYMax(middle);
    }
    ui::Margins borderWidths(outerRect, innerRect);

    // Compute inner radiuses in px
    style::BorderRadiusesInPx<float> innerRadiuses = outerRadiuses.offsetted(
        -borderWidths.top(),
        -borderWidths.right(),
        -borderWidths.bottom(),
        -borderWidths.left());

    // Compute point-pair samples (innerX, innerY, outerX, outerY), appending them to `a`
    QuarterEllipseParams params;
    params.eps = 1e-3f * pixelSize;
    params.invPixelSize = 1.0f / pixelSize;
    Int samplesBeginIndex = a.length();
    insertQuarterEllipseWithBorder<TopLeft>(
        a, innerRect, outerRect, innerRadiuses, outerRadiuses, refRadiuses, params);
    insertQuarterEllipseWithBorder<TopRight>(
        a, innerRect, outerRect, innerRadiuses, outerRadiuses, refRadiuses, params);
    insertQuarterEllipseWithBorder<BottomRight>(
        a, innerRect, outerRect, innerRadiuses, outerRadiuses, refRadiuses, params);
    insertQuarterEllipseWithBorder<BottomLeft>(
        a, innerRect, outerRect, innerRadiuses, outerRadiuses, refRadiuses, params);
    Int samplesEndIndex = a.length();

    // Compute how many triangles in total we will need:
    // - the border is represented as a quad strip (quad = 2 triangles) between
    //   the outer and the inner rectangle
    // - the interior is represented as a triangle fan.
    //
    // A rectangle is at least 4 samples (the four corners), so:
    // - numTrianglesInFan >= 2
    // - numTrianglesInQuadStrip >= 8
    //
    // Note that each triangle takes 15 floats, while each sample (inX, inY,
    // outX, outY) only takes 4 floats. So by filling the memory back to front,
    // we're never are risk of overwriting sample data we still need.
    //
    Int numSamples = (samplesEndIndex - samplesBeginIndex) / 4;
    VGC_ASSERT(numSamples >= 4);
    Int numTrianglesInFan = hasFill ? numSamples - 2 : 0;
    Int numTrianglesInQuadStrip = hasBorder ? 2 * numSamples : 0;
    Int numTriangles = numTrianglesInFan + numTrianglesInQuadStrip;
    a.resize(samplesBeginIndex + numTriangles * 15);
    float* p = &a.getUnchecked(samplesBeginIndex);
    float inX0 = *p;
    float inY0 = *(p + 1);
    float outX0 = *(p + 2);
    float outY0 = *(p + 3);

    // Fill quad strip
    if (hasBorder) {
        float rb = static_cast<float>(borderColor[0]);
        float gb = static_cast<float>(borderColor[1]);
        float bb = static_cast<float>(borderColor[2]);
        Int quadStripBeginIndex = samplesBeginIndex + numTrianglesInFan * 15;
        geometry::Vec2f inP_(inX0, inY0);
        geometry::Vec2f outP_(outX0, outY0);
        p = &a.getUnchecked(samplesEndIndex);
        for (Int i = numSamples - 1; i >= 0; --i) {
            p -= 4;
            geometry::Vec2f inP(*p, *(p + 1));
            geometry::Vec2f outP(*(p + 2), *(p + 3));
            geometry::Triangle2f t1(outP_, inP_, inP);
            geometry::Triangle2f t2(outP_, inP, outP);
            writeTriangleAt(a, quadStripBeginIndex + 15 * (2 * i), rb, gb, bb, t1);
            writeTriangleAt(a, quadStripBeginIndex + 15 * (2 * i + 1), rb, gb, bb, t2);
            inP_ = inP;
            outP_ = outP;
        }
    }

    // Fill triangle fan
    if (hasFill) {
        float rf = static_cast<float>(fillColor[0]);
        float gf = static_cast<float>(fillColor[1]);
        float bf = static_cast<float>(fillColor[2]);
        Int fanBeginIndex = samplesBeginIndex;
        geometry::Triangle2f triangle(core::NoInit{});
        triangle.setA(inX0, inY0);
        p = &a.getUnchecked(samplesEndIndex) - 4;
        for (Int i = numTrianglesInFan - 1; i >= 0; --i) {
            p -= 4;
            triangle.setB(*p, *(p + 1));
            triangle.setC(*(p + 4), *(p + 5));
            writeTriangleAt(a, fanBeginIndex + 15 * i, rf, gf, bf, triangle);
        }
    }
}

core::Color getColor(const style::StylableObject* obj, core::StringId property) {
    core::Color res;
    style::StyleValue value = obj->style(property);
    if (value.has<core::Color>()) {
        res = value.to<core::Color>();
    }
    return res;
}

float getLength(const style::StylableObject* obj, core::StringId property) {
    float res = 0;
    style::StyleValue value = obj->style(property);
    if (value.type() == style::StyleValueType::Number) {
        res = value.toFloat();
    }
    return res;
}

style::BorderRadiuses getBorderRadiuses(const style::StylableObject* obj) {

    namespace gs = graphics::strings;

    style::BorderRadius topLeft =
        obj->style(gs::border_top_left_radius).to<style::BorderRadius>();

    style::BorderRadius topRight =
        obj->style(gs::border_top_right_radius).to<style::BorderRadius>();

    style::BorderRadius bottomRight =
        obj->style(gs::border_bottom_right_radius).to<style::BorderRadius>();

    style::BorderRadius bottomLeft =
        obj->style(gs::border_bottom_left_radius).to<style::BorderRadius>();

    return style::BorderRadiuses(topLeft, topRight, bottomRight, bottomLeft);
}

} // namespace vgc::ui::detail
