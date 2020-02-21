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

#include <cstdlib> // abs

#include <vgc/core/colors.h>
#include <vgc/core/floatarray.h>

namespace vgc {
namespace ui {

ColorPalette::ColorPalette(const ConstructorKey&) :
    Widget(Widget::ConstructorKey()),
    currentColor_(core::colors::blue), // tmp color for testing. TODO: black
    oldWidth_(0),
    oldHeight_(0)
{

}

/* static */
ColorPaletteSharedPtr ColorPalette::create()
{
    return std::make_shared<ColorPalette>(ConstructorKey());
}

void ColorPalette::onPaintCreate(graphics::Engine* engine)
{
    trianglesId_ = engine->createTriangles();
}

void ColorPalette::onPaintDraw(graphics::Engine* engine)
{
    float eps = 1e-6;
    if (std::abs(oldWidth_ - width()) > eps || std::abs(oldHeight_ - height()) > eps) {
        oldWidth_ = width();
        oldHeight_ = height();
        core::FloatArray a;
        float margin = 10;
        float x0 = margin;
        float y0 = margin;
        float w = width() - 2*margin;
        float h = w;
        Int k = 8;
        float dx = w / k;
        float dy = h / k;
        float dr = 1.0f / k;
        float dg = 1.0f / k;
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
        engine->loadTriangles(trianglesId_, a.data(), a.length());
    }
    engine->clear(currentColor());
    engine->drawTriangles(trianglesId_);
}

void ColorPalette::onPaintDestroy(graphics::Engine* engine)
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
