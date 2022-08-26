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

} // namespace vgc::ui::detail
