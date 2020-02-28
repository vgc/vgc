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
    selectedColor_(core::colors::black),
    trianglesId_(-1),
    oldWidth_(0),
    oldHeight_(0),
    reload_(true),
    margin_(15),
    numHueSteps_(12),
    numSaturationSteps_(5),
    numLightnessSteps_(7),
    hoveredHueIndex_(-1),
    hoveredSaturationIndex_(-1),
    hoveredLightnessIndex_(-1),
    isSelectedColorExact_(true),
    selectedHueIndex_(0),
    selectedSaturationIndex_(0),
    selectedLightnessIndex_(0),
    oldSaturationIndex_(numSaturationSteps_ - 1),
    oldLightnessIndex_(numLightnessSteps_ / 2)
{

}

/* static */
ColorPaletteSharedPtr ColorPalette::create()
{
    return std::make_shared<ColorPalette>(ConstructorKey());
}

namespace {

core::Color colorFromHslIndices(
        Int numHueSteps,
        Int numSaturationSteps,
        Int numLightnessSteps,
        Int hueIndex,
        Int saturationIndex,
        Int lightnessIndex)
{
    double dh = 360.0 / numHueSteps;
    double ds = 1.0 / (numSaturationSteps - 1);
    double dl = 1.0 / (numLightnessSteps - 1);
    return core::Color::hsl(
                hueIndex * dh,
                saturationIndex * ds,
                lightnessIndex * dl);
}

} // namespace

void ColorPalette::setSelectedColor(const core::Color& color)
{
    if (selectedColor_ != color) {
        // Note: ColorPalette currently allows selecting, say:
        //
        //   hsl(30deg, 50%, 100%) = pure orange = rgb(100%, 50%, 0%)
        //
        // If we then click on the ColorToolButton, this calls:
        //
        //   QColorDialog::setCurrentColor(toQt(color_))
        //
        // Which converts the color to rgb(255, 128, 0), which isn't the same
        // color, because 50% = 127.5 on a [0, 255] scale:
        //
        //   rgb(255, 128, 0) = rgb(100%, ~50.196%, 0%)
        //
        // So we enter this `if` statement, and the color is de-highlighted in
        // the color palette, despite the user haven't really yet "changed" the
        // color, in his point of view. We may want to fix this either by
        // changing toQt()/fromQt(), or perhaps having this ColorPalette only
        // select colors which are an exact integer in [0, 255] scale.
        //
        // More info: https://github.com/vgc/vgc/issues/270#issuecomment-592041186
        //
        selectedColor_ = color;
        isSelectedColorExact_ = (selectedColor_ == colorFromHslIndices(
            numHueSteps_, numSaturationSteps_, numLightnessSteps_,
            selectedHueIndex_, selectedSaturationIndex_, selectedLightnessIndex_));
        reload_ = true;
        repaint();
        // TODO: find other indices if an exact match exist
        // TODO: emit selectedColorChanged (but not colorSelected)
    }
}

void ColorPalette::onPaintCreate(graphics::Engine* engine)
{
    trianglesId_ = engine->createTriangles();
}

namespace {

void highlightCell(
        core::FloatArray& a,
        const core::Color& cellColor,
        float x1, float y1, float x2, float y2)
{
    float r = static_cast<float>(cellColor[0]);
    float g = static_cast<float>(cellColor[1]);
    float b = static_cast<float>(cellColor[2]);
    float x1h = x1 - 2;
    float y1h = y1 - 2;
    float x2h = x2 + 2;
    float y2h = y2 + 2;
    x1 += 1;
    y1 += 1;
    x2 -= 1;
    y2 -= 1;
    float rh = 0.043; //
    float gh = 0.322; // VGC Blue
    float bh = 0.714; //
    a.insert(a.end(), {
        x1h, y1h, rh, gh, bh,
        x2h, y1h, rh, gh, bh,
        x1h, y2h, rh, gh, bh,
        x2h, y1h, rh, gh, bh,
        x2h, y2h, rh, gh, bh,
        x1h, y2h, rh, gh, bh,
        x1, y1, r, g, b,
        x2, y1, r, g, b,
        x1, y2, r, g, b,
        x2, y1, r, g, b,
        x2, y2, r, g, b,
        x1, y2, r, g, b});
}

} // namespace

void ColorPalette::onPaintDraw(graphics::Engine* engine)
{
    float eps = 1e-6;
    if (reload_ || std::abs(oldWidth_ - width()) > eps || std::abs(oldHeight_ - height()) > eps) {
        reload_ = false;
        oldWidth_ = width();
        oldHeight_ = height();
        core::FloatArray a;

        // Draw saturation/lightness selector
        float x0 = margin_;
        float y0 = margin_;
        float w = width() - 2*margin_;
        float dx = (w - 1) / numLightnessSteps_;
        float dy = std::round(dx);
        float h = 1 + dy * numSaturationSteps_;
        float x, y, r, g, b;
        x = x0; y = y0;
        r = 0.0; g = 0.0; b = 0.0;
        a.insert(a.end(), {
            x,   y,   r, g, b,
            x+w, y,   r, g, b,
            x,   y+h, r, g, b,
            x+w, y,   r, g, b,
            x+w, y+h, r, g, b,
            x,   y+h, r, g, b});
        double dhue = 360.0 / numHueSteps_;
        double hue = selectedHueIndex_ * dhue;
        double dl = 1.0 / (numLightnessSteps_ - 1);
        double ds = 1.0 / (numSaturationSteps_ - 1);
        for (Int i = 0; i < numLightnessSteps_; ++i) {
            float x1 = std::round(x0 + 1 + i*dx);
            float x2 = std::round(x0 + (i+1)*dx);
            double l = i*dl;
            for (Int j = 0; j < numSaturationSteps_; ++j) {
                float y1 = y0 + 1 + j*dy;
                float y2 = y1 + dy - 1;
                double s = j*ds;
                auto c = core::Color::hsl(hue, s, l);
                float r = static_cast<float>(c[0]);
                float g = static_cast<float>(c[1]);
                float b = static_cast<float>(c[2]);
                // TODO: implement a.extend()
                a.insert(a.end(), {
                    x1, y1, r, g, b,
                    x2, y1, r, g, b,
                    x1, y2, r, g, b,
                    x2, y1, r, g, b,
                    x2, y2, r, g, b,
                    x1, y2, r, g, b});
            }
        }
        // TODO: draw small disk indicating continuous current
        // color if selected color isn't an exact index.
        if (isSelectedColorExact_) {
            Int i = selectedLightnessIndex_;
            Int j = selectedSaturationIndex_;
            float x1 = std::round(x0 + 1 + i*dx);
            float x2 = std::round(x0 + (i+1)*dx);
            float y1 = y0 + 1 + j*dy;
            float y2 = y1 + dy - 1;
            double l = i*dl;
            double s = j*ds;
            auto c = core::Color::hsl(hue, s, l);
            highlightCell(a, c, x1, y1, x2, y2);
        }
        if (hoveredLightnessIndex_ != -1) {
            Int i = hoveredLightnessIndex_;
            Int j = hoveredSaturationIndex_;
            float x1 = std::round(x0 + 1 + i*dx);
            float x2 = std::round(x0 + (i+1)*dx);
            float y1 = y0 + 1 + j*dy;
            float y2 = y1 + dy - 1;
            double l = i*dl;
            double s = j*ds;
            auto c = core::Color::hsl(hue, s, l);
            highlightCell(a, c, x1, y1, x2, y2);
        }

        // Draw hue selector
        y0 += h + margin_;
        Int halfNumHueSteps = numHueSteps_ / 2;
        dx = (w - 1) / halfNumHueSteps;
        dy = std::round(dx);
        h = 1 + dy * 2;
        x = x0; y = y0;
        r = 0.0; g = 0.0; b = 0.0;
        a.insert(a.end(), {
            x,   y,   r, g, b,
            x+w, y,   r, g, b,
            x,   y+h, r, g, b,
            x+w, y,   r, g, b,
            x+w, y+h, r, g, b,
            x,   y+h, r, g, b});
        float y01 = y0 + 1;
        float y02 = y01 + dy;
        float y03 = y02 + dy;
        double l = oldLightnessIndex_*dl;
        double s = oldSaturationIndex_*ds;
        for (Int i = 0; i < numHueSteps_; ++i) {
            float x1, y1, x2, y2;
            if (i < halfNumHueSteps) {
                x1 = std::round(x0 + 1 + i*dx);
                x2 = std::round(x0 + (i+1)*dx);
                y1 = y01;
                y2 = y02 - 1;
            }
            else {
                x1 = std::round(x0 + 1 + (numHueSteps_ - i - 1)*dx);
                x2 = std::round(x0 + (numHueSteps_ - i)*dx);
                y1 = y02;
                y2 = y03 - 1;
            }
            double hue = i*dhue;
            auto c = core::Color::hsl(hue, s, l);
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
        if (isSelectedColorExact_) {
            Int i = selectedHueIndex_;
            float x1, y1, x2, y2;
            if (i < halfNumHueSteps) {
                x1 = std::round(x0 + 1 + i*dx);
                x2 = std::round(x0 + (i+1)*dx);
                y1 = y01;
                y2 = y02 - 1;
            }
            else {
                x1 = std::round(x0 + 1 + (numHueSteps_ - i - 1)*dx);
                x2 = std::round(x0 + (numHueSteps_ - i)*dx);
                y1 = y02;
                y2 = y03 - 1;
            }
            double hue = i*dhue;
            auto c = core::Color::hsl(hue, s, l);
            highlightCell(a, c, x1, y1, x2, y2);
        }
        if (hoveredHueIndex_ != -1) {
            Int i = hoveredHueIndex_;
            float x1, y1, x2, y2;
            if (i < halfNumHueSteps) {
                x1 = std::round(x0 + 1 + i*dx);
                x2 = std::round(x0 + (i+1)*dx);
                y1 = y01;
                y2 = y02 - 1;
            }
            else {
                x1 = std::round(x0 + 1 + (numHueSteps_ - i - 1)*dx);
                x2 = std::round(x0 + (numHueSteps_ - i)*dx);
                y1 = y02;
                y2 = y03 - 1;
            }
            double hue = i*dhue;
            auto c = core::Color::hsl(hue, s, l);
            highlightCell(a, c, x1, y1, x2, y2);
        }

        // Load triangles
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
    Int k = -1;
    float x0 = margin_;
    float y0 = margin_;
    float w = width() - 2*margin_;
    const core::Vec2f& p = event->pos();
    if (p[0] > x0 && p[0] < x0+w) {
        float dx = (w - 1) / numLightnessSteps_;
        float dy = std::round(dx);
        float h = 1 + dy * numSaturationSteps_;
        if (p[1] > y0 && p[1] < y0+h) {
            // Mouse is in Saturation/Lightness selector
            float i_ = numLightnessSteps_ * (p[0]-x0) / w;
            float j_ = numSaturationSteps_ * (p[1]-y0) / h;
            i = core::clamp(core::ifloor<Int>(i_), Int(0), numLightnessSteps_ - 1);
            j = core::clamp(core::ifloor<Int>(j_), Int(0), numSaturationSteps_ - 1);
        }
        else {
            y0 += h + margin_;
            Int halfNumHueSteps = numHueSteps_ / 2;
            dx = (w - 1) / halfNumHueSteps;
            dy = std::round(dx);
            h = 1 + dy * 2;
            if (p[1] > y0 && p[1] < y0+h) {
                // Mouse is in Hue selector
                float k_ = halfNumHueSteps * (p[0]-x0) / w;
                k = core::clamp(core::ifloor<Int>(k_), Int(0), halfNumHueSteps - 1);
                if (p[1] > y0+dy) {
                    k = numHueSteps_ - k - 1;
                }
            }
        }
    }
    if (hoveredLightnessIndex_ != i ||
        hoveredSaturationIndex_ != j ||
        hoveredHueIndex_ != k)
    {
        hoveredLightnessIndex_ = i;
        hoveredSaturationIndex_ = j;
        hoveredHueIndex_ = k;
        reload_ = true;
        repaint();
    }
    return true;
}

bool ColorPalette::onMousePress(MouseEvent* /*event*/)
{
    bool accepted = false;
    if (hoveredLightnessIndex_ != -1) {
        selectedLightnessIndex_ = hoveredLightnessIndex_;
        selectedSaturationIndex_ = hoveredSaturationIndex_;
        if (selectedSaturationIndex_ != 0 &&
            selectedLightnessIndex_ != 0 &&
            selectedLightnessIndex_ != numLightnessSteps_ -1)
        {
            // Remember L/S of last chromatic color selected
            oldSaturationIndex_ = selectedSaturationIndex_;
            oldLightnessIndex_ = selectedLightnessIndex_;
        }
        accepted = true;
    }
    else if (hoveredHueIndex_ != -1) {
        selectedHueIndex_ = hoveredHueIndex_;
        if (selectedSaturationIndex_ == 0 ||
            selectedLightnessIndex_ == 0 ||
            selectedLightnessIndex_ == numLightnessSteps_ -1)
        {
            // When users click on the hue selector while the current color is
            // non-chromatic (black, white, greys), it's obviously to change to
            // a chromatic color. So we restore the saturation/lightness of the
            // last chromatic color selected, which gives the color currently
            // displayed by the hue selector
            selectedSaturationIndex_ = oldSaturationIndex_;
            selectedLightnessIndex_ = oldLightnessIndex_;
        }
        accepted = true;
    }

    if (accepted) {
        reload_ = true;
        isSelectedColorExact_ = true;
        selectedColor_ = colorFromHslIndices(
            numHueSteps_, numSaturationSteps_, numLightnessSteps_,
            selectedHueIndex_, selectedSaturationIndex_, selectedLightnessIndex_);
        colorSelected();
        repaint();
    }
    return accepted;
}

} // namespace ui
} // namespace vgc
