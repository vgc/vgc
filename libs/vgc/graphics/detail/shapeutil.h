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

#ifndef VGC_GRAPHICS_DETAIL_SHAPEUTIL_H
#define VGC_GRAPHICS_DETAIL_SHAPEUTIL_H

#include <vgc/core/color.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/engine.h>

namespace vgc::graphics::detail {

struct ScreenSpaceInstanceData {
    // XYRotWRGBA
    geometry::Vec2f position = {};
    float isRotationEnabled = 0.f;
    float displacementScale = 1.f;
    core::Color color = {0, 0, 0, 1};
};
static_assert(sizeof(ScreenSpaceInstanceData) == 8 * sizeof(float));

VGC_GRAPHICS_API void updateScreenSpaceInstance(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Vec2f& position,
    float displacementScale,
    const core::Color& color,
    bool isRotationEnabled = true);

// numSides is always rounded to the next even number.
VGC_GRAPHICS_API GeometryViewPtr createScreenSpaceDisk(
    Engine* engine,
    const geometry::Vec2f& position,
    float radius,
    const core::Color& color,
    Int numSides = 128);

VGC_GRAPHICS_API GeometryViewPtr createScreenSpaceDisk(Engine* engine, Int numSides);

VGC_GRAPHICS_API void updateScreenSpaceDisk(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Vec2f& position,
    float radius,
    const core::Color& color);

VGC_GRAPHICS_API GeometryViewPtr createScreenSpaceSquare(
    Engine* engine,
    const geometry::Vec2f& position,
    float width,
    const core::Color& color);

VGC_GRAPHICS_API void updateScreenSpaceSquare(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Vec2f& position,
    float width,
    const core::Color& color);

VGC_GRAPHICS_API GeometryViewPtr createCircleWithScreenSpaceThickness(
    Engine* engine,
    float thickness,
    const core::Color& color,
    Int numSides = 127);

VGC_GRAPHICS_API void updateCircleWithScreenSpaceThickness(
    Engine* engine,
    const GeometryViewPtr& geometry,
    float thickness,
    const core::Color& color);

VGC_GRAPHICS_API void updateRectangleWithScreenSpaceThickness(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Rect2f& rect);

VGC_GRAPHICS_API void updateRectangleWithScreenSpaceThickness(
    Engine* engine,
    const GeometryViewPtr& geometry,
    float thickness,
    const core::Color& color);

VGC_GRAPHICS_API GeometryViewPtr createRectangleWithScreenSpaceThickness(
    Engine* engine,
    const geometry::Rect2f& rect,
    float thickness,
    const core::Color& color);

VGC_GRAPHICS_API void updateRectangleWithScreenSpaceThickness(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Rect2f& rect);

VGC_GRAPHICS_API void updateRectangleWithScreenSpaceThickness(
    Engine* engine,
    const GeometryViewPtr& geometry,
    float thickness,
    const core::Color& color);

} // namespace vgc::graphics::detail

#endif // VGC_GRAPHICS_DETAIL_SHAPEUTIL_H
