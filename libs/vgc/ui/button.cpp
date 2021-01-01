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

#include <vgc/ui/button.h>

#include <vgc/core/floatarray.h>
#include <vgc/core/random.h>

namespace vgc {
namespace ui {

Button::Button() :
    Widget(),
    trianglesId_(-1),
    reload_(true),
    isHovered_(false)
{
    core::UniformDistribution d(0, 360);
    double h = d();
    backgroundColor_ = core::Color::hsl(h, 1, 0.5);
    backgroundColorOnHover_ = core::Color::hsl(h, 1, 0.7);
}

ButtonPtr Button::create()
{
    return ButtonPtr(new Button());
}

void Button::onPaintCreate(graphics::Engine* engine)
{
    trianglesId_ = engine->createTriangles();
}

namespace {

void insertRect(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2)
{
    float r = static_cast<float>(c[0]);
    float g = static_cast<float>(c[1]);
    float b = static_cast<float>(c[2]);
    a.insert(a.end(), {
        x1, y1, r, g, b,
        x2, y1, r, g, b,
        x1, y2, r, g, b,
        x2, y1, r, g, b,
        x2, y2, r, g, b,
        x1, y2, r, g, b});
}

} // namespace

void Button::onPaintDraw(graphics::Engine* engine)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a;
        core::Color color = isHovered_ ?
            backgroundColorOnHover_ :
            backgroundColor_;
        insertRect(a, color, 0, 0, width(), height());
        engine->loadTriangles(trianglesId_, a.data(), a.length());
    }
    engine->drawTriangles(trianglesId_);
}

void Button::onPaintDestroy(graphics::Engine* engine)
{
    engine->destroyTriangles(trianglesId_);
}

bool Button::onMouseMove(MouseEvent* /*event*/)
{
    return true;
}

bool Button::onMousePress(MouseEvent* /*event*/)
{
    return true;
}

bool Button::onMouseRelease(MouseEvent* /*event*/)
{
    return true;
}

bool Button::onMouseEnter()
{
    isHovered_ = true;
    reload_ = true;
    repaint();
    return true;
}

bool Button::onMouseLeave()
{
    isHovered_ = false;
    reload_ = true;
    repaint();
    return true;
}

core::Vec2f Button::computePreferredSize() const
{
    float width = 0;
    float height = 0;
    if (widthPolicy().type() == LengthType::Auto) {
        width = 60;
        // TODO: compute appropriate width based on text length
    }
    else {
        width = widthPolicy().value();
    }
    if (heightPolicy().type() == LengthType::Auto) {
        height = 30;
        // TODO: compute appropriate height based on font size?
    }
    else {
        height = heightPolicy().value();
    }
    return core::Vec2f(width, height);
}

} // namespace ui
} // namespace vgc
