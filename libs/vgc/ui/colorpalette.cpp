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
    trianglesId_(-1),
    oldWidth_(0),
    oldHeight_(0),
    reload_(true),
    margin_(15),
    numHueSteps_(12),
    numSaturationSteps_(7),
    numLightnessSteps_(7),
    hoveredHueIndex_(-1),
    hoveredSaturationIndex_(-1),
    hoveredLightnessIndex_(-1)
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
    if (reload_ || std::abs(oldWidth_ - width()) > eps || std::abs(oldHeight_ - height()) > eps) {
        reload_ = false;
        oldWidth_ = width();
        oldHeight_ = height();
        core::FloatArray a;
        float x0 = margin_;
        float y0 = margin_;
        float w = width() - 2*margin_;
        float dx = w / numLightnessSteps_;
        float dy = dx;
        float h = dy * numSaturationSteps_;
        double hue = 330;
        double dl = 1.0 / (numLightnessSteps_ - 1);
        double ds = 1.0 / (numSaturationSteps_ - 1);
        for (Int i = 0; i < numLightnessSteps_; ++i) {
            float x = x0 + i*dx;
            double l = i*dl;
            for (Int j = 0; j < numSaturationSteps_; ++j) {
                float y = y0 + j*dy;
                double s = j*ds;
                auto c = core::Color::hsl(hue, s, l);
                float r = static_cast<float>(c[0]);
                float g = static_cast<float>(c[1]);
                float b = static_cast<float>(c[2]);
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
        if (hoveredLightnessIndex_ != -1) {
            float o = 2; // pixel offset
            float ro = 0.0f;
            float go = 0.0f;
            float bo = 0.0f;
            float x = x0 + hoveredLightnessIndex_*dx;
            float y = x0 + hoveredSaturationIndex_*dy;
            double l = hoveredLightnessIndex_*dl;
            double s = hoveredSaturationIndex_*ds;
            auto c = core::Color::hsl(hue, s, l);
            float r = static_cast<float>(c[0]);
            float g = static_cast<float>(c[1]);
            float b = static_cast<float>(c[2]);
            a.insert(a.end(), {
                 x-o,    y-o,    ro, go, bo,
                 x+dx+o, y-o,    ro, go, bo,
                 x-o,    y+dy+o, ro, go, bo,
                 x+dx+o, y-o,    ro, go, bo,
                 x+dx+o, y+dy+o, ro, go, bo,
                 x-o,    y+dy+o, ro, go, bo,
                 x,    y,    r, g, b,
                 x+dx, y,    r, g, b,
                 x,    y+dy, r, g, b,
                 x+dx, y,    r, g, b,
                 x+dx, y+dy, r, g, b,
                 x,    y+dy, r, g, b});
        }
        //float hueHeight = 0.2 * w;
        double dhue = 360.0 / numHueSteps_;
        Int halfNumHueSteps = numHueSteps_ / 2;
        float y1 = y0 + h + margin_;
        dx = w / halfNumHueSteps;
        dy = dx;
        float y2 = y1 + dy;
        for (Int i = 0; i < numHueSteps_; ++i) {
            float x, y;
            if (i < halfNumHueSteps) {
                x = x0 + i*dx;
                y = y1;
            }
            else {
                x = x0 + (numHueSteps_ - i - 1)*dx;
                y = y2;
            }
            double hue = i*dhue;
            auto c = core::Color::hsl(hue, 1.0, 0.5);
            float r = static_cast<float>(c[0]);
            float g = static_cast<float>(c[1]);
            float b = static_cast<float>(c[2]);
            // TODO: implement a.extend()
            a.insert(a.end(), {
                x,    y,    r, g, b,
                x+dx, y,    r, g, b,
                x,    y+dy, r, g, b,
                x+dx, y,    r, g, b,
                x+dx, y+dy, r, g, b,
                x,    y+dy, r, g, b});
        }
        engine->loadTriangles(trianglesId_, a.data(), a.length());
    }
    engine->clear(core::Color(0.337, 0.345, 0.353));
    engine->drawTriangles(trianglesId_);
}

void ColorPalette::onPaintDestroy(graphics::Engine* engine)
{
    engine->destroyTriangles(trianglesId_);
}

bool ColorPalette::onMouseMove(MouseEvent* event)
{
    Int i = -1;
    Int j = -1;
    float x0 = margin_;
    float y0 = margin_;
    float w = width() - 2*margin_;
    float h = w;
    const core::Vec2f& p = event->pos();
    if (p[0] > x0 && p[0] < x0+w && p[1] > y0 && p[1] < y0+h) {
        float i_ = numLightnessSteps_ * (p[0]-x0) / w;
        float j_ = numSaturationSteps_ * (p[1]-y0) / h;
        i = core::clamp(core::ifloor<Int>(i_), Int(0), numLightnessSteps_ - 1);
        j = core::clamp(core::ifloor<Int>(j_), Int(0), numSaturationSteps_ - 1);
    }
    if (hoveredLightnessIndex_ != i || hoveredSaturationIndex_ != j) {
        hoveredLightnessIndex_ = i;
        hoveredSaturationIndex_ = j;
        reload_ = true;
        repaint();
    }
    return true;
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
