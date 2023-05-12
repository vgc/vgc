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

#ifndef VGC_UI_INTERNAL_PAINTUTIL_H
#define VGC_UI_INTERNAL_PAINTUTIL_H

#include <string>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/stringid.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/geometry/triangle2f.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/text.h>
#include <vgc/style/types.h>
#include <vgc/ui/api.h>
#include <vgc/ui/widget.h>

namespace vgc::ui::detail {

// clang-format off

VGC_UI_API
void insertTriangle(
    core::FloatArray& a,
    float r, float g, float b,
    float x1, float y1, float x2, float y2, float x3, float y3);

// replaces values from a[i] to a[i + 14] with the given triangle
//
VGC_UI_API
void writeTriangleAt(
    core::FloatArray& a, Int i,
    float r, float g, float b,
    float x1, float y1, float x2, float y2, float x3, float y3);

// replaces values from a[i] to a[i + 14] with the given triangle
//
VGC_UI_API
void writeTriangleAt(
    core::FloatArray& a, Int i,
    float r, float g, float b,
    const geometry::Triangle2f& t);

VGC_UI_API
void insertTriangle(
    core::FloatArray& a,
    const core::Color& color,
    const geometry::Vec2f& v1,
    const geometry::Vec2f& v2,
    const geometry::Vec2f& v3);

VGC_UI_API
void insertRect(
    core::FloatArray& a,
    float r, float g, float b,
    float x1, float y1, float x2, float y2);

VGC_UI_API
void insertRect(
    core::FloatArray& a,
    const core::Color& color,
    const geometry::Rect2f& rect);

VGC_UI_API
void insertRect(
    core::FloatArray& a,
    const core::Color& color,
    float x1, float y1, float x2, float y2);

// clang-format on

VGC_UI_API
void insertRect(
    core::FloatArray& a,
    const style::Metrics& styleMetrics,
    const core::Color& color,
    const geometry::Rect2f& rect,
    const style::BorderRadii& borderRadii,
    float pixelSize = 1.0f);

VGC_UI_API
void insertRect(
    core::FloatArray& a,
    const style::Metrics& styleMetrics,
    const core::Color& fillColor,
    const core::Color& borderColor,
    const geometry::Rect2f& outerRect,
    const style::BorderRadii& outerRadii_,
    float borderWidth,
    float pixelSize = 1.0f);

// refRadii is used to determine the number of samples.
// this is useful if you want to add a border to an existing
// rounded rectangle: you want to use the same number of samples
// so that the quad strips match perfectly.
VGC_UI_API
void insertRect(
    core::FloatArray& a,
    const core::Color& fillColor,
    const core::Color& borderColor,
    const geometry::Rect2f& outerRect,
    const style::BorderRadiiInPx& outerRadii_,
    const style::BorderRadiiInPx& refRadii_,
    float borderWidth,
    float pixelSize = 1.0f);

VGC_UI_API
core::Color getColor(const style::StylableObject* obj, core::StringId property);

VGC_UI_API
style::Length getLength(const style::StylableObject* obj, core::StringId property);

VGC_UI_API
style::LengthOrPercentage
getLengthOrPercentage(const style::StylableObject* obj, core::StringId property);

VGC_UI_API
float getLengthInPx(
    const style::StylableObject* obj,
    core::StringId property,
    bool hinted = false);

VGC_UI_API
float getLengthOrPercentageInPx(
    const style::StylableObject* obj,
    core::StringId property,
    float refLength,
    bool hinted = false);

} // namespace vgc::ui::detail

#endif // VGC_UI_INTERNAL_PAINTUTIL_H
