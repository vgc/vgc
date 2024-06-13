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

#include <vgc/graphics/detail/shapeutil.h>

#include <vgc/geometry/vec4f.h>

namespace vgc::graphics::detail {

void updateScreenSpaceInstance(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Vec2f& position,
    float displacementScale,
    const core::Color& color,
    bool isRotationEnabled) {

    if (geometry
        && geometry->builtinGeometryLayout()
               == BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA) {

        // XYRotWRGBA
        const core::Color& c = color;
        const float dispS = displacementScale;
        const float rot = isRotationEnabled ? 1.f : 0.f;
        core::FloatArray instanceData(
            {position.x(), position.y(), rot, dispS, c.r(), c.g(), c.b(), c.a()});
        engine->updateInstanceBufferData(geometry, std::move(instanceData));
    }
}

// numSides is always rounded to the next even number.
GeometryViewPtr createScreenSpaceDisk(
    Engine* engine,
    const geometry::Vec2f& position,
    float radius,
    const core::Color& color,
    Int numSides) {

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
    const core::Color& c = color;
    core::FloatArray instanceData(
        {position.x(), position.y(), 1.f, radius, c.r(), c.g(), c.b(), c.a()});
    BufferPtr instanceBuffer = engine->createVertexBuffer(std::move(instanceData), true);

    graphics::GeometryViewCreateInfo createInfo = {};
    createInfo.setBuiltinGeometryLayout(
        graphics::BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    createInfo.setPrimitiveType(graphics::PrimitiveType::TriangleStrip);
    createInfo.setVertexBuffer(0, vertexBuffer);
    createInfo.setVertexBuffer(1, instanceBuffer);
    return engine->createGeometryView(createInfo);
}

// numSides is always rounded to the next even number.
GeometryViewPtr createScreenSpaceDisk(Engine* engine, Int numSides) {
    return createScreenSpaceDisk(
        engine, geometry::Vec2f(), 1, core::Color(0, 0, 0), numSides);
}

void updateScreenSpaceDisk(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Vec2f& position,
    float radius,
    const core::Color& color) {

    updateScreenSpaceInstance(engine, geometry, position, radius, color, false);
}

GeometryViewPtr createScreenSpaceSquare(
    Engine* engine,
    const geometry::Vec2f& position,
    float width,
    const core::Color& color) {

    // TODO: cache vertex buffer in Engine

    // XYDxDy
    // ┌─── x
    // │  0┌┄┄┄┄┐2
    // y   ┆    ┆
    //    1└┄┄┄┄┘3
    //
    geometry::Vec4fArray vertices{
        {0, 0, -1, -1}, {0, 0, -1, 1}, {0, 0, 1, -1}, {0, 0, 1, 1}};
    BufferPtr vertexBuffer = engine->createVertexBuffer(std::move(vertices), false);

    // XYRotWRGBA
    const core::Color& c = color;
    core::FloatArray instanceData(
        {position.x(), position.y(), 1.f, width * 0.5f, c.r(), c.g(), c.b(), c.a()});
    BufferPtr instanceBuffer = engine->createVertexBuffer(std::move(instanceData), true);

    graphics::GeometryViewCreateInfo createInfo = {};
    createInfo.setBuiltinGeometryLayout(
        graphics::BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    createInfo.setPrimitiveType(graphics::PrimitiveType::TriangleStrip);
    createInfo.setVertexBuffer(0, vertexBuffer);
    createInfo.setVertexBuffer(1, instanceBuffer);
    return engine->createGeometryView(createInfo);
}

void updateScreenSpaceSquare(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Vec2f& position,
    float width,
    const core::Color& color) {

    updateScreenSpaceInstance(engine, geometry, position, width * 0.5f, color, true);
}

GeometryViewPtr createCircleWithScreenSpaceThickness(
    Engine* engine,
    float thickness,
    const core::Color& color,
    Int numSides) {

    // TODO: cache vertex buffer per numSides in Engine

    // XYDxDy
    Int n = numSides + 1;
    geometry::Vec4fArray vertices;
    vertices.resizeNoInit(2 * n);
    for (Int i = 0; i < n; ++i) {
        float a = (static_cast<float>(i) / (n - 1)) * 2 * static_cast<float>(core::pi);
        float x = std::cos(a);
        float y = std::sin(a);
        // ┌─── x
        // │ 0┄┄┄┄┄┄2
        // y  \    /
        //     1┄┄3
        //
        Int j = 2 * i;
        vertices[j] = geometry::Vec4f(x, y, x, y);
        vertices[j + 1] = geometry::Vec4f(x, y, 0, 0);
    }
    BufferPtr vertexBuffer = engine->createVertexBuffer(std::move(vertices), false);

    // XYRotWRGBA
    const core::Color& c = color;
    core::FloatArray instanceData({0, 0, 1.f, thickness, c.r(), c.g(), c.b(), c.a()});
    BufferPtr instanceBuffer = engine->createVertexBuffer(std::move(instanceData), true);

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
    const core::Color& c) {

    updateScreenSpaceInstance(engine, geometry, geometry::Vec2f(), thickness, c, false);
}

namespace {

geometry::Vec4fArray
createRectangleWithScreenSpaceThicknessVertexData(const geometry::Rect2f& rect) {
    // XYDxDy
    // ┌─── x
    // │  1┌┄┄┄┄┄┄┄┄┄┄┄┄┄┄┐3
    // y  9┆ \          / ┆
    //     ┆  0┌┄┄┄┄┄┄┐2  ┆
    //     ┆  8┆      ┆   ┆
    //     ┆   ┆      ┆   ┆
    //     ┆  6└┄┄┄┄┄┄┘4  ┆
    //     ┆ /          \ ┆
    //    7└┄┄┄┄┄┄┄┄┄┄┄┄┄┄┘5
    //
    float x0 = rect.xMin();
    float x1 = rect.xMax();
    float y0 = rect.yMin();
    float y1 = rect.yMax();
    return geometry::Vec4fArray{
        {x0, y0, 0, 0},
        {x0, y0, -1, -1},
        {x1, y0, 0, 0},
        {x1, y0, 1, -1},
        {x1, y1, 0, 0},
        {x1, y1, 1, 1},
        {x0, y1, 0, 0},
        {x0, y1, -1, 1},
        {x0, y0, 0, 0},
        {x0, y0, -1, -1}};
}

} // namespace

GeometryViewPtr createRectangleWithScreenSpaceThickness(
    Engine* engine,
    const geometry::Rect2f& rect,
    float thickness,
    const core::Color& color) {

    // XYDxDy
    geometry::Vec4fArray vertices =
        createRectangleWithScreenSpaceThicknessVertexData(rect);
    BufferPtr vertexBuffer = engine->createVertexBuffer(std::move(vertices), true);

    // XYRotWRGBA
    const core::Color& c = color;
    core::FloatArray instanceData({0, 0, 1.f, thickness, c.r(), c.g(), c.b(), c.a()});
    BufferPtr instanceBuffer = engine->createVertexBuffer(std::move(instanceData), true);

    graphics::GeometryViewCreateInfo createInfo = {};
    createInfo.setBuiltinGeometryLayout(
        graphics::BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    createInfo.setPrimitiveType(graphics::PrimitiveType::TriangleStrip);
    createInfo.setVertexBuffer(0, vertexBuffer);
    createInfo.setVertexBuffer(1, instanceBuffer);
    return engine->createGeometryView(createInfo);
}

void updateRectangleWithScreenSpaceThickness(
    Engine* engine,
    const GeometryViewPtr& geometry,
    const geometry::Rect2f& rect) {

    if (geometry
        && geometry->builtinGeometryLayout()
               == BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA) {

        // XYDxDy
        geometry::Vec4fArray vertices =
            createRectangleWithScreenSpaceThicknessVertexData(rect);
        engine->updateVertexBufferData(geometry, std::move(vertices));
    }
}

void updateRectangleWithScreenSpaceThickness(
    Engine* engine,
    const GeometryViewPtr& geometry,
    float thickness,
    const core::Color& color) {

    updateScreenSpaceInstance(
        engine, geometry, geometry::Vec2f(), thickness * std::sqrt(2.f), color);
}

} // namespace vgc::graphics::detail
