// Copyright 2020 The VGC Developers
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

#include <vgc/ui/colorpalette.h>

#include <vgc/core/colors.h>
#include <vgc/core/floatarray.h>

namespace vgc {
namespace ui {

ColorPalette::ColorPalette(const ConstructorKey&) :
    Widget(Widget::ConstructorKey()),
    currentColor_(core::colors::blue) // tmp color for testing. TODO: black
{

}

/* static */
ColorPaletteSharedPtr ColorPalette::create()
{
    return std::make_shared<ColorPalette>(ConstructorKey());
}

void ColorPalette::initialize(graphics::Engine* engine)
{
    core::FloatArray a;
    float x0 = 0.0;
    float y0 = 0.0;
    float w = 50;
    float h = 50;
    Int k = 8;
    float dx = w / k;
    float dy = h / k;
    float dr = 1.0 / k;
    float dg = 1.0 / k;
    float b = 0.0;
    for (Int i = 0; i < k; ++i) {
        float x = x0 + i*dx;
        float r = i*dr;
        for (Int j = 0; j < k; ++j) {
            float y = y0 + j*dy;
            float g = j*dg;
            // TODO: implement a.extend()
            a.insert(a.end(), {
                x,    y,    r, g, b,
                x+dx, y,    r, g, b,
                x,    y+dy, r, g, b,
                x+dx, y,    r, g, b,
                x+dx, y+dy, r, g, b,
                x,    y+dy, r, g, b});
        }
    }
    trianglesId_ = engine->createTriangles();
    engine->loadTriangles(trianglesId_, a.data(), a.length());
}

void ColorPalette::resize(graphics::Engine* /*engine*/, Int /*w*/, Int /*h*/)
{

}

void ColorPalette::paint(graphics::Engine* engine)
{
    engine->clear(currentColor());
    engine->drawTriangles(trianglesId_);
}

void ColorPalette::cleanup(graphics::Engine* engine)
{
    engine->destroyTriangles(trianglesId_);
}

bool ColorPalette::onMousePress(MouseEvent* /*event*/)
{
    setCurrentColor(
        currentColor() == core::colors::blue ?
        core::colors::red :
        core::colors::blue);
    repaint();
    return true;
}

} // namespace ui
} // namespace vgc
