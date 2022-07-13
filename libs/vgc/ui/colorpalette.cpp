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

#include <vgc/ui/colorpalette.h>

#include <cstdlib> // abs

#include <vgc/core/array.h>
#include <vgc/core/colors.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/internal/paintutil.h>

namespace vgc {
namespace ui {

ColorPalette::ColorPalette() :
    Widget(),
    selectedColor_(core::colors::black),
    oldWidth_(0),
    oldHeight_(0),
    reload_(true),
    margin_(15),
    isContinuous_(false),
    selectorBorderWidth_(2.0),
    cellBorderWidth_(1.0),
    borderColor_(core::colors::black),
    numHueSteps_(12),
    numSaturationSteps_(5),
    numLightnessSteps_(7),
    hoveredHueIndex_(-1),
    hoveredSaturationIndex_(-1),
    hoveredLightnessIndex_(-1),
    scrubbedSelector_(SelectorType::None),
    isSelectedColorExact_(true),
    selectedHueIndex_(0),
    selectedSaturationIndex_(0),
    selectedLightnessIndex_(0),
    oldSaturationIndex_(numSaturationSteps_ - 1),
    oldLightnessIndex_(numLightnessSteps_ / 2)
{
    addStyleClass(strings::ColorPalette);
}

/* static */
ColorPalettePtr ColorPalette::create()
{
    return ColorPalettePtr(new ColorPalette());
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
    triangles_ = engine->createTriangles();
}

namespace {

void insertSmoothRect(
        core::FloatArray& a,
        const core::Color& cTopLeft, const core::Color& cTopRight,
        const core::Color& cBottomLeft, const core::Color& cBottomRight,
        float x1, float y1, float x2, float y2)
{
    float r1 = static_cast<float>(cTopLeft[0]);
    float g1 = static_cast<float>(cTopLeft[1]);
    float b1 = static_cast<float>(cTopLeft[2]);
    float r2 = static_cast<float>(cTopRight[0]);
    float g2 = static_cast<float>(cTopRight[1]);
    float b2 = static_cast<float>(cTopRight[2]);
    float r3 = static_cast<float>(cBottomLeft[0]);
    float g3 = static_cast<float>(cBottomLeft[1]);
    float b3 = static_cast<float>(cBottomLeft[2]);
    float r4 = static_cast<float>(cBottomRight[0]);
    float g4 = static_cast<float>(cBottomRight[1]);
    float b4 = static_cast<float>(cBottomRight[2]);
    a.insert(a.end(), {
        x1, y1, r1, g1, b1,
        x2, y1, r2, g2, b2,
        x1, y2, r3, g3, b3,
        x2, y1, r2, g2, b2,
        x2, y2, r4, g4, b4,
        x1, y2, r3, g3, b3});
}

void insertCellHighlight(
        core::FloatArray& a,
        const core::Color& cellColor,
        float x1, float y1, float x2, float y2)
{
    // TODO: draw 4 lines (= thin rectangles) rather than 2 big rects?
    auto ch = core::Color(0.043, 0.322, 0.714); // VGC Blue
    internal::insertRect(a, ch, x1-2, y1-2, x2+2, y2+2);
    internal::insertRect(a, cellColor, x1+1, y1+1, x2-1, y2-1);
}

} // namespace

void ColorPalette::onPaintDraw(graphics::Engine* engine)
{
    float eps = 1e-6f;
    if (reload_ || std::abs(oldWidth_ - width()) > eps || std::abs(oldHeight_ - height()) > eps) {
        reload_ = false;
        oldWidth_ = width();
        oldHeight_ = height();
        core::FloatArray a;

        // Draw saturation/lightness selector
        // Terminology:
        // - x0: position of selector including border
        // - w: width of selector including border
        // - startOffset: distance between x0 and x of first color cell
        // - endOffset: distance between (x0 + startOffset + N*dx) and (x0 + w)
        // - cellOffset: distance between color cells
        float x0 = margin_;
        float y0 = margin_;
        float w = width() - 2*margin_;
        float startOffset = selectorBorderWidth_;
        float endOffset = selectorBorderWidth_ - cellBorderWidth_;
        float cellOffset = cellBorderWidth_;
        float dx = (w - startOffset - endOffset) / numLightnessSteps_;
        float dy = std::round(dx);
        float h = startOffset + endOffset + dy * numSaturationSteps_;
        if (cellBorderWidth_ > 0 || selectorBorderWidth_ > 0) {
            internal::insertRect(a, borderColor_, x0, y0, x0+w, y0+h);
        }
        double dhue = 360.0 / numHueSteps_;
        double hue = selectedHueIndex_ * dhue;
        double dl = 1.0 / (numLightnessSteps_ - 1);
        double ds = 1.0 / (numSaturationSteps_ - 1);
        for (Int i = 0; i < numLightnessSteps_; ++i) {
            float x1 = std::round(x0 + startOffset + i*dx);
            float x2 = std::round(x0 + startOffset + (i+1)*dx) - cellOffset;
            double l = i*dl;
            for (Int j = 0; j < numSaturationSteps_; ++j) {
                float y1 = y0 + startOffset + j*dy;
                float y2 = y1 + dy - cellOffset;
                double s = j*ds;
                if (isContinuous_) {
                    auto c1 = core::Color::hsl(hue, s, l);
                    auto c2 = core::Color::hsl(hue, s, l+dl);
                    auto c3 = core::Color::hsl(hue, s+ds, l);
                    auto c4 = core::Color::hsl(hue, s+ds, l+dl);
                    insertSmoothRect(a, c1, c2, c3, c4, x1, y1, x2, y2);
                }
                else {
                    auto c = core::Color::hsl(hue, s, l);
                    internal::insertRect(a, c, x1, y1, x2, y2);
                }
            }
        }
        // TODO: draw small disk indicating continuous current
        // color if selected color isn't an exact index.
        if (isSelectedColorExact_) {
            Int i = selectedLightnessIndex_;
            Int j = selectedSaturationIndex_;            
            float x1 = std::round(x0 + startOffset + i*dx);
            float x2 = std::round(x0 + startOffset + (i+1)*dx) - cellOffset;
            float y1 = y0 + startOffset + j*dy;
            float y2 = y1 + dy - cellOffset;
            double l = i*dl;
            double s = j*ds;
            auto c = core::Color::hsl(hue, s, l);
            insertCellHighlight(a, c, x1, y1, x2, y2);
        }
        if (hoveredLightnessIndex_ != -1) {
            Int i = hoveredLightnessIndex_;
            Int j = hoveredSaturationIndex_;
            float x1 = std::round(x0 + startOffset + i*dx);
            float x2 = std::round(x0 + startOffset + (i+1)*dx) - cellOffset;
            float y1 = y0 + startOffset + j*dy;
            float y2 = y1 + dy - cellOffset;
            double l = i*dl;
            double s = j*ds;
            auto c = core::Color::hsl(hue, s, l);
            insertCellHighlight(a, c, x1, y1, x2, y2);
        }

        // Draw hue selector
        Int halfNumHueSteps = numHueSteps_ / 2;
        y0 += h + margin_;
        dx = (w - startOffset - endOffset) / halfNumHueSteps;
        dy = std::round(dx);
        h = startOffset + endOffset + dy * 2;
        if (cellBorderWidth_ > 0 || selectorBorderWidth_ > 0) {
            internal::insertRect(a, borderColor_, x0, y0, x0+w, y0+h);
        }
        double l = oldLightnessIndex_*dl;
        double s = oldSaturationIndex_*ds;
        for (Int i = 0; i < numHueSteps_; ++i) {
            float x1, y1, x2, y2;
            double hue1 = i*dhue;
            double hue2; // for continuous mode
            if (i < halfNumHueSteps) {
                x1 = std::round(x0 + startOffset + i*dx);
                x2 = std::round(x0 + startOffset + (i+1)*dx) - cellOffset;
                y1 = y0 + startOffset;
                y2 = y1 + dy - cellOffset;
                hue2 = hue1 + dhue;
            }
            else {
                x1 = std::round(x0 + startOffset + (numHueSteps_ - i - 1)*dx);
                x2 = std::round(x0 + startOffset + (numHueSteps_ - i)*dx) - cellOffset;
                y1 = y0 + startOffset + dy;
                y2 = y1 + dy - cellOffset;
                hue2 = hue1 - dhue;
            }
            if (isContinuous_) {
                auto c1 = core::Color::hsl(hue1, s, l);
                auto c2 = core::Color::hsl(hue2, s, l);
                insertSmoothRect(a, c1, c2, c1, c2, x1, y1, x2, y2);
            }
            else {
                auto c = core::Color::hsl(hue1, s, l);
                internal::insertRect(a, c, x1, y1, x2, y2);
            }
        }
        if (isSelectedColorExact_) {
            Int i = selectedHueIndex_;
            float x1, y1, x2, y2;
            double shue = i*dhue;
            if (i < halfNumHueSteps) {
                x1 = std::round(x0 + startOffset + i*dx);
                x2 = std::round(x0 + startOffset + (i+1)*dx) - cellOffset;
                y1 = y0 + startOffset;
                y2 = y1 + dy - cellOffset;
            }
            else {
                x1 = std::round(x0 + startOffset + (numHueSteps_ - i - 1)*dx);
                x2 = std::round(x0 + startOffset + (numHueSteps_ - i)*dx) - cellOffset;
                y1 = y0 + startOffset + dy;
                y2 = y1 + dy - cellOffset;
            }
            auto c = core::Color::hsl(shue, s, l);
            insertCellHighlight(a, c, x1, y1, x2, y2);
        }
        if (hoveredHueIndex_ != -1) {
            Int i = hoveredHueIndex_;
            float x1, y1, x2, y2;
            double hhue = i*dhue;
            if (i < halfNumHueSteps) {
                x1 = std::round(x0 + startOffset + i*dx);
                x2 = std::round(x0 + startOffset + (i+1)*dx) - cellOffset;
                y1 = y0 + startOffset;
                y2 = y1 + dy - cellOffset;
            }
            else {
                x1 = std::round(x0 + startOffset + (numHueSteps_ - i - 1)*dx);
                x2 = std::round(x0 + startOffset + (numHueSteps_ - i)*dx) - cellOffset;
                y1 = y0 + startOffset + dy;
                y2 = y1 + dy - cellOffset;
            }
            auto c = core::Color::hsl(hhue, s, l);
            insertCellHighlight(a, c, x1, y1, x2, y2);
        }

        // Load triangles
        triangles_->load(a.data(), a.length());
    }
    engine->clear(core::Color(0.337, 0.345, 0.353));
    triangles_->draw();
}

void ColorPalette::onPaintDestroy(graphics::Engine*)
{
    triangles_.reset();
}

bool ColorPalette::onMouseMove(MouseEvent* event)
{
    // Determine relevant selector

    const geometry::Vec2f& p = event->pos();
    SelectorType selector = scrubbedSelector_;
    if (selector == SelectorType::None) {
        selector = hoveredSelector_(p);
    }

    // Determine hovered cell
    Int i = -1;
    Int j = -1;
    Int k = -1;
    if (selector == SelectorType::SaturationLightness) {
        auto sl = hoveredSaturationLightness_(p);
        i = sl.first;
        j = sl.second;
    }
    else if (selector == SelectorType::Hue) {
        k = hoveredHue_(p);
    }

    // Update
    if (hoveredLightnessIndex_ != i ||
        hoveredSaturationIndex_ != j ||
        hoveredHueIndex_ != k)
    {
        hoveredLightnessIndex_ = i;
        hoveredSaturationIndex_ = j;
        hoveredHueIndex_ = k;
        if (scrubbedSelector_ != SelectorType::None) {
            selectColorFromHovered_(); // -> already emit repaint()
        }
        else {
            reload_ = true;
            repaint();
        }
    }
    return true;
}

bool ColorPalette::onMousePress(MouseEvent* /*event*/)
{
    if (hoveredLightnessIndex_ != -1) {
        scrubbedSelector_ = SelectorType::SaturationLightness;
    }
    else if (hoveredHueIndex_ != -1) {
        scrubbedSelector_ = SelectorType::Hue;
    }
    return selectColorFromHovered_();
}

bool ColorPalette::onMouseRelease(MouseEvent* /*event*/)
{
    scrubbedSelector_ = SelectorType::None;
    return true;
}

bool ColorPalette::onMouseEnter()
{
    return true;
}

bool ColorPalette::onMouseLeave()
{
    Int i = -1;
    Int j = -1;
    Int k = -1;
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

ColorPalette::SelectorType ColorPalette::hoveredSelector_(const geometry::Vec2f& p)
{
    float x0 = margin_;
    float y0 = margin_;
    float w = width() - 2*margin_;
    if (p[0] > x0 && p[0] < x0+w) {
        float dx = (w - 1) / numLightnessSteps_;
        float dy = std::round(dx);
        float h = 1 + dy * numSaturationSteps_;
        if (p[1] > y0 && p[1] < y0+h) {
            return SelectorType::SaturationLightness;
        }
        else {
            y0 += h + margin_;
            Int halfNumHueSteps = numHueSteps_ / 2;
            dx = (w - 1) / halfNumHueSteps;
            dy = std::round(dx);
            h = 1 + dy * 2;
            if (p[1] > y0 && p[1] < y0+h) {
                return SelectorType::Hue;
            }
        }
    }
    return SelectorType::None;
}

std::pair<Int, Int> ColorPalette::hoveredSaturationLightness_(const geometry::Vec2f& p)
{
    float x0 = margin_;
    float y0 = margin_;
    float w = width() - 2*margin_;
    float dx = (w - 1) / numLightnessSteps_;
    float dy = std::round(dx);
    float h = 1 + dy * numSaturationSteps_;
    float x = core::clamp(p[0], x0, x0 + w);
    float y = core::clamp(p[1], y0, y0 + h);
    float i_ = numLightnessSteps_ * (x - x0) / w;
    float j_ = numSaturationSteps_ * (y - y0) / h;
    Int i = core::clamp(core::ifloor<Int>(i_), Int(0), numLightnessSteps_ - 1);
    Int j = core::clamp(core::ifloor<Int>(j_), Int(0), numSaturationSteps_ - 1);
    return {i, j};
}

Int ColorPalette::hoveredHue_(const geometry::Vec2f& p)
{
    float x0 = margin_;
    float y0 = margin_;
    float w = width() - 2*margin_;
    float dx = (w - 1) / numLightnessSteps_;
    float dy = std::round(dx);
    float h = 1 + dy * numSaturationSteps_;
    y0 += h + margin_;
    Int halfNumHueSteps = numHueSteps_ / 2;
    dx = (w - 1) / halfNumHueSteps;
    dy = std::round(dx);
    h = 1 + dy * 2;
    float x = core::clamp(p[0], x0, x0 + w);
    float y = core::clamp(p[1], y0, y0 + h);
    float k_ = halfNumHueSteps * (x - x0) / w;
    Int k = core::clamp(core::ifloor<Int>(k_), Int(0), halfNumHueSteps - 1);
    if (y > y0 + dy) {
        k = numHueSteps_ - k - 1;
    }
    return k;
}

bool ColorPalette::selectColorFromHovered_()
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
        colorSelected().emit();
        repaint();
    }
    return accepted;
}

} // namespace ui
} // namespace vgc
