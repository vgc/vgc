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

#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/vec4f.h>
#include <vgc/graphics/engine.h>

namespace vgc::graphics::detail {

void updateScreenSpaceInstance(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Vec2f& position,
    float displacementScale,
    const core::Color& color) {

    if (geometry
        && geometry->builtinGeometryLayout()
               == BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA) {

        engine->updateBufferData(
            geometry->vertexBuffer(1),
            core::FloatArray(
                {position.x(),
                 position.y(),
                 1.f,
                 displacementScale,
                 color.r(),
                 color.g(),
                 color.b(),
                 color.a()}));
    }
}

// numSides is always rounded to the next even number.
GeometryViewPtr createScreenSpaceDisk(
    Engine* engine,
    const geometry::Vec2f& position,
    float radius,
    const core::Color& color,
    Int numSides = 128) {

    // TODO: cache vertex buffer per numSides in Engine

    Int numSteps = (numSides + 1) / 2;

    // XYDxDy
    geometry::Vec4fArray vertices;
    vertices.emplaceLast(0.f, 0.f, -1.f, 0.f);
    for (Int i = 1; i < numSteps; ++i) {
        float a = static_cast<float>(core::pi) * (static_cast<float>(i) / numSteps);
        float dx = -std::cos(a);
        float dy = std::sin(a);
        vertices.emplaceLast(0.f, 0.f, dx, dy);
        vertices.emplaceLast(0.f, 0.f, dx, -dy);
    }
    vertices.emplaceLast(0.f, 0.f, 1.f, 0.f);
    BufferPtr vertexBuffer = engine->createVertexBuffer(std::move(vertices), false);

    // XYRotWRGBA
    BufferPtr instanceBuffer = engine->createVertexBuffer(
        core::FloatArray(
            {position.x(),
             position.y(),
             1.f,
             radius,
             color.r(),
             color.g(),
             color.b(),
             color.a()}),
        true);

    graphics::GeometryViewCreateInfo createInfo = {};
    createInfo.setBuiltinGeometryLayout(
        graphics::BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    createInfo.setPrimitiveType(graphics::PrimitiveType::TriangleStrip);
    createInfo.setVertexBuffer(0, vertexBuffer);
    createInfo.setVertexBuffer(1, instanceBuffer);
    return engine->createGeometryView(createInfo);
}

void updateScreenSpaceDisk(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Vec2f& position,
    float radius,
    const core::Color& color) {

    updateScreenSpaceInstance(engine, geometry, position, radius, color);
}

GeometryViewPtr createCircleWithScreenSpaceThickness(
    Engine* engine,
    float thickness,
    const core::Color& color,
    Int numSides = 127) {

    // TODO: cache vertex buffer per numSides in Engine

    // XYDxDy
    Int n = numSides + 1;
    geometry::Vec4fArray vertices;
    vertices.resizeNoInit(2 * n);
    for (Int i = 0; i < n; ++i) {
        float a = (static_cast<float>(i) / (n - 1)) * 2 * static_cast<float>(core::pi);
        float x = std::cos(a);
        float y = std::sin(a);
        //
        //  0┄┄┄┄┄┄2
        //   \    /
        //    1┄┄3
        //
        Int j = 2 * i;
        vertices[j] = geometry::Vec4f(x, y, x, y);
        vertices[j + 1] = geometry::Vec4f(x, y, 0, 0);
    }
    BufferPtr vertexBuffer = engine->createVertexBuffer(std::move(vertices), false);

    // XYRotWRGBA
    BufferPtr instanceBuffer = engine->createVertexBuffer(
        core::FloatArray(
            {0, 0, 1.f, thickness, color.r(), color.g(), color.b(), color.a()}),
        true);

    GeometryViewCreateInfo createInfo = {};
    createInfo.setBuiltinGeometryLayout(BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    createInfo.setPrimitiveType(PrimitiveType::TriangleStrip);
    createInfo.setVertexBuffer(0, vertexBuffer);
    createInfo.setVertexBuffer(1, instanceBuffer);
    return engine->createGeometryView(createInfo);
}

void updateCircleWithScreenSpaceThickness(
    Engine* engine,
    const GeometryViewPtr& geometry,
    float thickness,
    const core::Color& color) {

    updateScreenSpaceInstance(engine, geometry, geometry::Vec2f(), thickness, color);
}

} // namespace vgc::graphics::detail

#endif // VGC_GRAPHICS_DETAIL_SHAPEUTIL_H
