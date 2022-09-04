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
#include <vgc/core/format.h>
#include <vgc/core/parse.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/button.h>
#include <vgc/ui/label.h>
#include <vgc/ui/lineedit.h>
#include <vgc/ui/margins.h>
#include <vgc/ui/row.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

namespace {

core::StringId horizontal_group("horizontal-group");
core::StringId first("first");
core::StringId middle("middle");
core::StringId last("last");

core::Color highlightColor = core::Color(0.043, 0.322, 0.714); // VGC Blue

} // namespace

ColorPalette::ColorPalette()
    : Column() {

    selector_ = createChild<ColorPaletteSelector>();

    Row* rgbRow = createChild<Row>();
    rgbRow->createChild<Label>("RGB:");
    rLineEdit_ = rgbRow->createChild<LineEdit>();
    gLineEdit_ = rgbRow->createChild<LineEdit>();
    bLineEdit_ = rgbRow->createChild<LineEdit>();
    rLineEdit_->addStyleClass(horizontal_group);
    gLineEdit_->addStyleClass(horizontal_group);
    bLineEdit_->addStyleClass(horizontal_group);
    rLineEdit_->addStyleClass(first);
    gLineEdit_->addStyleClass(middle);
    bLineEdit_->addStyleClass(last);

    Row* hslRow = createChild<Row>();
    hslRow->createChild<Label>("HSL:");
    hLineEdit_ = hslRow->createChild<LineEdit>();
    sLineEdit_ = hslRow->createChild<LineEdit>();
    lLineEdit_ = hslRow->createChild<LineEdit>();
    hLineEdit_->addStyleClass(horizontal_group);
    sLineEdit_->addStyleClass(horizontal_group);
    lLineEdit_->addStyleClass(horizontal_group);
    hLineEdit_->addStyleClass(first);
    sLineEdit_->addStyleClass(middle);
    lLineEdit_->addStyleClass(last);

    Row* hexRow = createChild<Row>();
    hexRow->createChild<Label>("Hex:");
    hexLineEdit_ = hexRow->createChild<LineEdit>();

    Button* addToPaletteButton = createChild<Button>("Add to Palette");

    colorListView_ = createChild<ColorListView>();

    selector_->colorSelected().connect(onSelectorSelectedColorSlot_());
    rLineEdit_->editingFinished().connect(onRgbEditedSlot_());
    gLineEdit_->editingFinished().connect(onRgbEditedSlot_());
    bLineEdit_->editingFinished().connect(onRgbEditedSlot_());
    hLineEdit_->editingFinished().connect(onHslEditedSlot_());
    sLineEdit_->editingFinished().connect(onHslEditedSlot_());
    lLineEdit_->editingFinished().connect(onHslEditedSlot_());
    hexLineEdit_->editingFinished().connect(onHexEditedSlot_());
    addToPaletteButton->clicked().connect(onAddToPaletteClickedSlot_());
    colorListView_->colorSelected().connect(onColorListViewSelectedColorSlot_());

    setSelectedColorNoCheckNoEmit_(core::colors::black);

    addStyleClass(strings::ColorPalette);
}

ColorPalettePtr ColorPalette::create() {
    return ColorPalettePtr(new ColorPalette());
}

void ColorPalette::setSelectedColor(const core::Color& color) {
    if (selectedColor_ != color) {
        setSelectedColorNoCheckNoEmit_(color);
    }
}

void ColorPalette::onSelectorSelectedColor_() {
    core::Color newColor = selector_->selectedColor();
    if (selectedColor_ != newColor) {
        setSelectedColorNoCheckNoEmit_(newColor);
        colorSelected().emit();
    }
}

void ColorPalette::onColorListViewSelectedColor_() {
    core::Color newColor = colorListView_->selectedColor();
    if (selectedColor_ != newColor) {
        setSelectedColorNoCheckNoEmit_(newColor);
        colorSelected().emit();
    }
}

namespace {

// If the lineEdit is empty, this function sets it to "0", keeps `isValid`
// unchanged, and returns `0`.
//
// If the lineEdit is non-empty and is a valid integer, this function keeps the
// line edit and `isValid` unchanged, and returns the integer.
//
// If the lineEdit is non-empty and is not a valid integer, this function sets `isValid` to false,
// leaves the line edit unchanged, and returns `0`.
//
Int parseInt_(LineEdit* lineEdit, bool& isValid) {
    try {
        const std::string& text = lineEdit->text();
        if (text.empty()) {
            // If a user deletes the whole text, then we snap to zero and place the
            // cursor after the zero, so that doing [select all] [delete] [1] [2]
            // results in `12`, not `120`.
            lineEdit->setText("0");
            lineEdit->moveCursor(graphics::RichTextMoveOperation::EndOfText);
            return 0;
        }
        else {
            return core::parse<Int>(text);
        }
    }
    catch (const core::ParseError&) {
        isValid = false;
        return 0;
    }
}

// If the lineEdit is a valid hex color, this keeps `isValid` unchanged and
// returns the corresponding color.
//
// Otherwise this function sets `isValid` to false and returns `Color()`.
//
core::Color parseHex_(LineEdit* lineEdit, bool& isValid) {
    try {
        const std::string& text = lineEdit->text();
        return core::Color::fromHex(text);
    }
    catch (const core::ParseError&) {
        isValid = false;
        return core::Color();
    }
}

} // namespace

void ColorPalette::onRgbEdited_() {

    // Try to parse the new color from the line edit.
    //
    bool isValid = true;
    Int r_ = parseInt_(rLineEdit_, isValid);
    Int g_ = parseInt_(gLineEdit_, isValid);
    Int b_ = parseInt_(bLineEdit_, isValid);

    // Check if the input was valid.
    //
    core::Color color = selectedColor_;
    if (isValid) {
        double r = core::uint8ToDouble01(r_);
        double g = core::uint8ToDouble01(g_);
        double b = core::uint8ToDouble01(b_);
        color = core::Color(r, g, b);
        color.round8b();
    }

    // Set `color` as the new `selectedColor_` unconditionally, and update
    // child widgets accordingly. This rollbacks the line edits to previous
    // valid values, in case invalid values where entered (letters, leading
    // zeros, etc.).
    //
    core::Color oldColor = selectedColor_;
    setSelectedColorNoCheckNoEmit_(color);

    // Emit the signal only if the color actually changed.
    //
    if (selectedColor_ != oldColor) {
        colorSelected().emit();
    }
}

void ColorPalette::onHslEdited_() {

    // Try to parse the new color from the line edit.
    //
    bool isValid = true;
    Int h_ = parseInt_(hLineEdit_, isValid);
    Int s_ = parseInt_(sLineEdit_, isValid);
    Int l_ = parseInt_(lLineEdit_, isValid);

    // Check if the input was valid.
    //
    core::Color color = selectedColor_;
    if (isValid) {
        // Note: Color::hsl() already does mod-360 hue
        double h = static_cast<double>(h_);
        double s = core::uint8ToDouble01(s_);
        double l = core::uint8ToDouble01(l_);
        color = core::Color::hsl(h, s, l);
        color.round8b();
    }

    // Set `color` as the new `selectedColor_` unconditionally, and update
    // child widgets accordingly. This rollbacks the line edits to previous
    // valid values, in case invalid values where entered (letters, leading
    // zeros, etc.).
    //
    core::Color oldColor = selectedColor_;
    setSelectedColorNoCheckNoEmit_(color);

    // Emit the signal only if the color actually changed.
    //
    if (selectedColor_ != oldColor) {
        colorSelected().emit();
    }
}

void ColorPalette::onHexEdited_() {

    core::Color newColor = selectedColor_;
    core::Color oldColor = selectedColor_;

    bool isValid = true;
    core::Color parsedColor = parseHex_(hexLineEdit_, isValid);
    if (isValid) {
        newColor = parsedColor;
    }

    setSelectedColorNoCheckNoEmit_(newColor);
    if (selectedColor_ != oldColor) {
        colorSelected().emit();
    }
}

void ColorPalette::onAddToPaletteClicked_() {
    colorListView_->appendColor(selectedColor());
    colorListView_->setSelectedColorIndex(colorListView_->numColors() - 1);
}

void ColorPalette::setSelectedColorNoCheckNoEmit_(const core::Color& color) {

    selectedColor_ = color;

    // Update selector
    selector_->setSelectedColor(selectedColor_);

    // Update RGB line edits
    rLineEdit_->setText(core::toString(static_cast<Int>(std::round(color.r() * 255))));
    gLineEdit_->setText(core::toString(static_cast<Int>(std::round(color.g() * 255))));
    bLineEdit_->setText(core::toString(static_cast<Int>(std::round(color.b() * 255))));

    // Update HSL line edits
    // For now, we round to the nearest integer. Later, we may
    // want to show the first digit
    auto [h, s, l] = color.toHsl();
    hLineEdit_->setText(core::toString(static_cast<Int>(std::round(h))));
    sLineEdit_->setText(core::toString(static_cast<Int>(std::round(s * 255))));
    lLineEdit_->setText(core::toString(static_cast<Int>(std::round(l * 255))));
    // hLineEdit_->setText(core::format("{:.1f}", h));
    // sLineEdit_->setText(core::format("{:.1f}", s * 255));
    // lLineEdit_->setText(core::format("{:.1f}", l * 255));

    // Update Hex line edit
    hexLineEdit_->setText(color.toHex());

    // Update color palette list view
    colorListView_->setSelectedColor(selectedColor_);
}

ColorPaletteSelector::ColorPaletteSelector()
    : Widget()
    , selectedColor_(core::colors::black)
    , oldWidth_(0)
    , oldHeight_(0)
    , reload_(true)
    , isContinuous_(false)
    , numHueSteps_(12)
    , numSaturationSteps_(5)
    , numLightnessSteps_(7)
    , hoveredHueIndex_(-1)
    , hoveredSaturationIndex_(-1)
    , hoveredLightnessIndex_(-1)
    , scrubbedSelector_(SelectorType::None)
    , isSelectedColorExact_(true)
    , selectedHueIndex_(0)
    , selectedSaturationIndex_(0)
    , selectedLightnessIndex_(0)
    , oldSaturationIndex_(numSaturationSteps_ - 1)
    , oldLightnessIndex_(numLightnessSteps_ / 2) {

    addStyleClass(strings::ColorPaletteSelector);
}

/* static */
ColorPaletteSelectorPtr ColorPaletteSelector::create() {
    return ColorPaletteSelectorPtr(new ColorPaletteSelector());
}

namespace {

core::Color colorFromHslIndices(
    Int numHueSteps,
    Int numSaturationSteps,
    Int numLightnessSteps,
    Int hueIndex,
    Int saturationIndex,
    Int lightnessIndex) {

    double dh = 360.0 / numHueSteps;
    double ds = 1.0 / (numSaturationSteps - 1);
    double dl = 1.0 / (numLightnessSteps - 1);

    core::Color color = core::Color::hsl( //
        hueIndex * dh,
        saturationIndex * ds,
        lightnessIndex * dl);

    return color.rounded8b();
}

} // namespace

void ColorPaletteSelector::setSelectedColor(const core::Color& color) {

    if (selectedColor_ != color) {

        selectedColor_ = color;

        // Find closest user-selectable color
        auto [h, s, l] = color.toHsl();
        Int hueIndex = static_cast<Int>(std::round(h * numHueSteps_ / 360.0));
        Int saturationIndex = static_cast<Int>(std::round(s * (numSaturationSteps_ - 1)));
        Int lightnessIndex = static_cast<Int>(std::round(l * (numLightnessSteps_ - 1)));
        core::Color closestSelectableColor = colorFromHslIndices(
            numHueSteps_,
            numSaturationSteps_,
            numLightnessSteps_,
            hueIndex,
            saturationIndex,
            lightnessIndex);

        // Detect whether there is an exact match
        isSelectedColorExact_ = (closestSelectableColor == color);

        // Set indices based on closest user-selectable color, regardless
        // of whether there is an exact match or not
        selectedHueIndex_ = hueIndex % numHueSteps_;
        selectedSaturationIndex_ =
            core::clamp(saturationIndex, 0, numSaturationSteps_ - 1);
        selectedLightnessIndex_ = core::clamp(lightnessIndex, 0, numLightnessSteps_ - 1);

        // Remember L/S of last chromatic color selected
        if (selectedSaturationIndex_ != 0   //
            && selectedLightnessIndex_ != 0 //
            && selectedLightnessIndex_ != numLightnessSteps_ - 1) {

            oldSaturationIndex_ = selectedSaturationIndex_;
            oldLightnessIndex_ = selectedLightnessIndex_;
        }

        // Emit signals
        reload_ = true;
        selectedColorChanged().emit();
        requestRepaint();
    }
}

void ColorPaletteSelector::onPaintCreate(graphics::Engine* engine) {
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

namespace {

// clang-format off


void insertSmoothRect(
    core::FloatArray& a,
    const core::Color& cTopLeft, const core::Color& cTopRight,
    const core::Color& cBottomLeft, const core::Color& cBottomRight,
    float x1, float y1, float x2, float y2) {

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
    a.extend({
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
    float x1, float y1, float x2, float y2) {

    // TODO: draw 4 lines (= thin rectangles) rather than 2 big rects?
    detail::insertRect(a, highlightColor, x1 - 2, y1 - 2, x2 + 2, y2 + 2);
    detail::insertRect(a, cellColor, x1 + 1, y1 + 1, x2 - 1, y2 - 1);
}

// clang-format on

float hint(float value, bool hinting) {
    if (hinting) {
        return std::round(value);
    }
    else {
        return value;
    }
}

} // namespace

void ColorPaletteSelector::onPaintDraw(
    graphics::Engine* engine,
    PaintOptions /*options*/) {

    float eps = 1e-6f;
    bool hasWidthChanged = std::abs(oldWidth_ - width()) > eps;
    bool hasHeightChanged = std::abs(oldHeight_ - height()) > eps;
    if (reload_ || hasWidthChanged || hasHeightChanged) {

        reload_ = false;
        oldWidth_ = width();
        oldHeight_ = height();
        core::FloatArray a = {};

        core::Color borderColor = detail::getColor(this, graphics::strings::border_color);
        updateMetrics_(); // TODO: only update if we know that they have changed
        const Metrics& m = metrics_;

        // draw saturation/lightness selector
        if (m.cellBorderWidth > 0 || m.selectorBorderWidth > 0) {
            detail::insertRect(a, borderColor, m.saturationLightnessRect);
        }
        float x0 = m.saturationLightnessRect.xMin();
        float y0 = m.saturationLightnessRect.yMin();
        double dhue = 360.0 / numHueSteps_;
        double hue = selectedHueIndex_ * dhue;
        double dl = 1.0 / (numLightnessSteps_ - 1);
        double ds = 1.0 / (numSaturationSteps_ - 1);
        for (Int i = 0; i < numLightnessSteps_; ++i) {
            float x1 = hint(x0 + m.startOffset + i * m.slDx, m.hinting);
            float x2 =
                hint(x0 + m.startOffset + (i + 1) * m.slDx, m.hinting) - m.cellOffset;
            double l = i * dl;
            for (Int j = 0; j < numSaturationSteps_; ++j) {
                float y1 = y0 + m.startOffset + j * m.slDy;
                float y2 = y1 + m.slDy - m.cellOffset;
                double s = j * ds;
                if (isContinuous_) {
                    auto c1 = core::Color::hsl(hue, s, l);
                    auto c2 = core::Color::hsl(hue, s, l + dl);
                    auto c3 = core::Color::hsl(hue, s + ds, l);
                    auto c4 = core::Color::hsl(hue, s + ds, l + dl);
                    insertSmoothRect(a, c1, c2, c3, c4, x1, y1, x2, y2);
                }
                else {
                    auto c = core::Color::hsl(hue, s, l).round8b();
                    detail::insertRect(a, c, x1, y1, x2, y2);
                }
            }
        }
        // TODO: draw small disk indicating continuous current
        // color if selected color isn't an exact index.
        if (isSelectedColorExact_) {
            Int i = selectedLightnessIndex_;
            Int j = selectedSaturationIndex_;
            float x1 = hint(x0 + m.startOffset + i * m.slDx, m.hinting);
            float x2 =
                hint(x0 + m.startOffset + (i + 1) * m.slDx, m.hinting) - m.cellOffset;
            float y1 = y0 + m.startOffset + j * m.slDy;
            float y2 = y1 + m.slDy - m.cellOffset;
            double l = i * dl;
            double s = j * ds;
            auto c = core::Color::hsl(hue, s, l).round8b();
            insertCellHighlight(a, c, x1, y1, x2, y2);
        }
        if (hoveredLightnessIndex_ != -1) {
            Int i = hoveredLightnessIndex_;
            Int j = hoveredSaturationIndex_;
            float x1 = hint(x0 + m.startOffset + i * m.slDx, m.hinting);
            float x2 =
                hint(x0 + m.startOffset + (i + 1) * m.slDx, m.hinting) - m.cellOffset;
            float y1 = y0 + m.startOffset + j * m.slDy;
            float y2 = y1 + m.slDy - m.cellOffset;
            double l = i * dl;
            double s = j * ds;
            auto c = core::Color::hsl(hue, s, l).round8b();
            insertCellHighlight(a, c, x1, y1, x2, y2);
        }

        // Draw hue selector
        Int halfNumHueSteps = numHueSteps_ / 2;
        y0 = m.hueRect.yMin();
        if (m.cellBorderWidth > 0 || m.selectorBorderWidth > 0) {
            detail::insertRect(a, borderColor, m.hueRect);
        }
        double l = oldLightnessIndex_ * dl;
        double s = oldSaturationIndex_ * ds;
        for (Int i = 0; i < numHueSteps_; ++i) {
            float x1, y1, x2, y2;
            double hue1 = i * dhue;
            double hue2; // for continuous mode
            if (i < halfNumHueSteps) {
                x1 = hint(x0 + m.startOffset + i * m.hueDx, m.hinting);
                x2 = hint(x0 + m.startOffset + (i + 1) * m.hueDx, m.hinting)
                     - m.cellOffset;
                y1 = y0 + m.startOffset;
                y2 = y1 + m.hueDy - m.cellOffset;
                hue2 = hue1 + dhue;
            }
            else {
                x1 = hint(
                    x0 + m.startOffset + (numHueSteps_ - i - 1) * m.hueDx, m.hinting);
                x2 = hint(x0 + m.startOffset + (numHueSteps_ - i) * m.hueDx, m.hinting)
                     - m.cellOffset;
                y1 = y0 + m.startOffset + m.hueDy;
                y2 = y1 + m.hueDy - m.cellOffset;
                hue2 = hue1 - dhue;
            }
            if (isContinuous_) {
                auto c1 = core::Color::hsl(hue1, s, l);
                auto c2 = core::Color::hsl(hue2, s, l);
                insertSmoothRect(a, c1, c2, c1, c2, x1, y1, x2, y2);
            }
            else {
                auto c = core::Color::hsl(hue1, s, l).round8b();
                detail::insertRect(a, c, x1, y1, x2, y2);
            }
        }
        if (isSelectedColorExact_) {
            Int i = selectedHueIndex_;
            float x1, y1, x2, y2;
            double shue = i * dhue;
            if (i < halfNumHueSteps) {
                x1 = hint(x0 + m.startOffset + i * m.hueDx, m.hinting);
                x2 = hint(x0 + m.startOffset + (i + 1) * m.hueDx, m.hinting)
                     - m.cellOffset;
                y1 = y0 + m.startOffset;
                y2 = y1 + m.hueDy - m.cellOffset;
            }
            else {
                x1 = hint(
                    x0 + m.startOffset + (numHueSteps_ - i - 1) * m.hueDx, m.hinting);
                x2 = hint(x0 + m.startOffset + (numHueSteps_ - i) * m.hueDx, m.hinting)
                     - m.cellOffset;
                y1 = y0 + m.startOffset + m.hueDy;
                y2 = y1 + m.hueDy - m.cellOffset;
            }
            auto c = core::Color::hsl(shue, s, l).round8b();
            insertCellHighlight(a, c, x1, y1, x2, y2);
        }
        if (hoveredHueIndex_ != -1) {
            Int i = hoveredHueIndex_;
            float x1, y1, x2, y2;
            double hhue = i * dhue;
            if (i < halfNumHueSteps) {
                x1 = hint(x0 + m.startOffset + i * m.hueDx, m.hinting);
                x2 = hint(x0 + m.startOffset + (i + 1) * m.hueDx, m.hinting)
                     - m.cellOffset;
                y1 = y0 + m.startOffset;
                y2 = y1 + m.hueDy - m.cellOffset;
            }
            else {
                x1 = hint(
                    x0 + m.startOffset + (numHueSteps_ - i - 1) * m.hueDx, m.hinting);
                x2 = hint(x0 + m.startOffset + (numHueSteps_ - i) * m.hueDx, m.hinting)
                     - m.cellOffset;
                y1 = y0 + m.startOffset + m.hueDy;
                y2 = y1 + m.hueDy - m.cellOffset;
            }
            auto c = core::Color::hsl(hhue, s, l).round8b();
            insertCellHighlight(a, c, x1, y1, x2, y2);
        }

        // Load triangles
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    //engine->clear(core::Color(0.337, 0.345, 0.353));
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_, -1, 0);
}

ColorPaletteSelector::Metrics
ColorPaletteSelector::computeMetricsFromWidth_(float width) const {

    namespace gs = graphics::strings;

    // Terminology:
    // - x0: position of selector including border
    // - w: width of selector including border
    // - startOffset: distance between x0 and x of first color cell
    // - endOffset: distance between (x0 + startOffset + N*dx) and (x0 + w)
    // - cellOffset: distance between color cells

    Metrics m;

    // TODO: pass numHueSteps / etc. as a `Params` struct

    // Get metrics from style

    m.hinting = (style(gs::pixel_hinting) == gs::normal);
    m.borderWidth = detail::getLength(this, gs::border_width);
    m.paddingTop = detail::getLength(this, gs::padding_top);
    m.paddingRight = detail::getLength(this, gs::padding_right);
    m.paddingBottom = detail::getLength(this, gs::padding_bottom);
    m.paddingLeft = detail::getLength(this, gs::padding_left);
    m.rowGap = detail::getLength(this, ui::strings::row_gap);
    m.cellBorderWidth = m.borderWidth;
    m.selectorBorderWidth = m.borderWidth;

    // Saturation/lightness selector
    float x0 = m.paddingLeft;
    float y0 = m.paddingTop;
    float w = width - (m.paddingLeft + m.paddingRight);
    m.startOffset = m.selectorBorderWidth;
    m.endOffset = m.selectorBorderWidth - m.cellBorderWidth;
    m.cellOffset = m.cellBorderWidth;
    m.slDx = (w - m.startOffset - m.endOffset) / numLightnessSteps_;
    m.slDy = hint(m.slDx, m.hinting);
    float h = m.startOffset + m.endOffset + m.slDy * numSaturationSteps_;
    m.saturationLightnessRect = {x0, y0, x0 + w, y0 + h};

    // Hue selector
    Int halfNumHueSteps = numHueSteps_ / 2;
    y0 += h + m.rowGap;
    m.hueDx = (w - m.startOffset - m.endOffset) / halfNumHueSteps;
    m.hueDy = m.hueDx;
    m.hueDy = hint(m.hueDy, m.hinting);
    h = m.startOffset + m.endOffset + m.hueDy * 2;
    m.hueRect = {x0, y0, x0 + w, y0 + h};

    // Full height and width
    m.width = width;
    m.height = y0 + h + m.paddingBottom;

    return m;
}

void ColorPaletteSelector::updateMetrics_() const {
    metrics_ = computeMetricsFromWidth_(width());
}

void ColorPaletteSelector::onPaintDestroy(graphics::Engine*) {
    triangles_.reset();
}

bool ColorPaletteSelector::onMouseMove(MouseEvent* event) {

    // Determine relevant selector
    const geometry::Vec2f& p = event->position();
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
    if (hoveredLightnessIndex_ != i     //
        || hoveredSaturationIndex_ != j //
        || hoveredHueIndex_ != k) {

        hoveredLightnessIndex_ = i;
        hoveredSaturationIndex_ = j;
        hoveredHueIndex_ = k;
        if (scrubbedSelector_ != SelectorType::None) {
            selectColorFromHovered_(); // -> includes requestRepaint()
        }
        else {
            reload_ = true;
            requestRepaint();
        }
    }
    return true;
}

bool ColorPaletteSelector::onMousePress(MouseEvent* /*event*/) {
    if (hoveredLightnessIndex_ != -1) {
        scrubbedSelector_ = SelectorType::SaturationLightness;
    }
    else if (hoveredHueIndex_ != -1) {
        scrubbedSelector_ = SelectorType::Hue;
    }
    return selectColorFromHovered_();
}

bool ColorPaletteSelector::onMouseRelease(MouseEvent* /*event*/) {
    scrubbedSelector_ = SelectorType::None;
    return true;
}

bool ColorPaletteSelector::onMouseEnter() {
    return true;
}

bool ColorPaletteSelector::onMouseLeave() {
    Int i = -1;
    Int j = -1;
    Int k = -1;
    if (hoveredLightnessIndex_ != i     //
        || hoveredSaturationIndex_ != j //
        || hoveredHueIndex_ != k) {

        hoveredLightnessIndex_ = i;
        hoveredSaturationIndex_ = j;
        hoveredHueIndex_ = k;
        reload_ = true;
        requestRepaint();
    }
    return true;
}

float ColorPaletteSelector::preferredWidthForHeight(float) const {
    // TODO
    return preferredSize()[0];
}

float ColorPaletteSelector::preferredHeightForWidth(float width) const {
    Metrics m = computeMetricsFromWidth_(width);
    return m.height;
}

geometry::Vec2f ColorPaletteSelector::computePreferredSize() const {
    geometry::Vec2f res(0, 0);
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    if (w.type() != auto_) {
        res[0] = w.value();
    }
    else {
        // TODO: something better , e.g., based on the number of
        // hue/saturation/lightness steps?
        res[0] = 100.0f;
    }
    if (h.type() != auto_) {
        res[1] = h.value();
    }
    else {
        Metrics m = computeMetricsFromWidth_(res[0]);
        res[1] = m.height;
    }
    return res;
}

ColorPaletteSelector::SelectorType
ColorPaletteSelector::hoveredSelector_(const geometry::Vec2f& p) {
    const Metrics& m = metrics_;
    if (m.saturationLightnessRect.contains(p)) {
        return SelectorType::SaturationLightness;
    }
    else if (m.hueRect.contains(p)) {
        return SelectorType::Hue;
    }
    else {
        return SelectorType::None;
    }
}

std::pair<Int, Int>
ColorPaletteSelector::hoveredSaturationLightness_(const geometry::Vec2f& p) {
    const geometry::Rect2f& r = metrics_.saturationLightnessRect;
    float i_ = numLightnessSteps_ * (p.x() - r.xMin()) / r.width();
    float j_ = numSaturationSteps_ * (p.y() - r.yMin()) / r.height();
    Int i = core::clamp(core::ifloor<Int>(i_), Int(0), numLightnessSteps_ - 1);
    Int j = core::clamp(core::ifloor<Int>(j_), Int(0), numSaturationSteps_ - 1);
    return {i, j};
}

Int ColorPaletteSelector::hoveredHue_(const geometry::Vec2f& p) {
    const geometry::Rect2f& r = metrics_.hueRect;
    Int halfNumHueSteps = numHueSteps_ / 2;
    float k_ = halfNumHueSteps * (p.x() - r.xMin()) / r.width();
    Int k = core::clamp(core::ifloor<Int>(k_), Int(0), halfNumHueSteps - 1);
    if (p.y() > r.yMin() + 0.5 * r.height()) {
        k = numHueSteps_ - k - 1;
    }
    return k;
}

bool ColorPaletteSelector::selectColorFromHovered_() {
    bool accepted = false;
    if (hoveredLightnessIndex_ != -1) {
        selectedLightnessIndex_ = hoveredLightnessIndex_;
        selectedSaturationIndex_ = hoveredSaturationIndex_;
        if (selectedSaturationIndex_ != 0   //
            && selectedLightnessIndex_ != 0 //
            && selectedLightnessIndex_ != numLightnessSteps_ - 1) {

            // Remember L/S of last chromatic color selected
            oldSaturationIndex_ = selectedSaturationIndex_;
            oldLightnessIndex_ = selectedLightnessIndex_;
        }
        accepted = true;
    }
    else if (hoveredHueIndex_ != -1) {
        selectedHueIndex_ = hoveredHueIndex_;
        if (selectedSaturationIndex_ == 0   //
            || selectedLightnessIndex_ == 0 //
            || selectedLightnessIndex_ == numLightnessSteps_ - 1) {
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
            numHueSteps_,
            numSaturationSteps_,
            numLightnessSteps_,
            selectedHueIndex_,
            selectedSaturationIndex_,
            selectedLightnessIndex_);
        colorSelected().emit();
        requestRepaint();
    }
    return accepted;
}

ColorListViewItem::ColorListViewItem(ColorListView* view)
    : view_(view) {

    addStyleClass(strings::ColorListViewItem);
}

ColorListViewItemPtr ColorListViewItem::create(ColorListView* view) {
    return ColorListViewItemPtr(new ColorListViewItem(view));
}

style::StylableObject* ColorListViewItem::parentStylableObject() const {
    return view_;
}

style::StylableObject* ColorListViewItem::firstChildStylableObject() const {
    return nullptr;
}

style::StylableObject* ColorListViewItem::lastChildStylableObject() const {
    return nullptr;
}

style::StylableObject* ColorListViewItem::previousSiblingStylableObject() const {
    return nullptr;
}

style::StylableObject* ColorListViewItem::nextSiblingStylableObject() const {
    return nullptr;
}

const style::StyleSheet* ColorListViewItem::defaultStyleSheet() const {
    return nullptr;
}

ColorListView::ColorListView()
    : item_(ColorListViewItem::create(this)) {

    addStyleClass(strings::ColorListView);
}

ColorListViewPtr ColorListView::create() {
    return ColorListViewPtr(new ColorListView());
}

void ColorListView::setSelectedColorIndex(Int index) {
    if (index < 0 || index >= numColors()) {
        index = -1;
    }
    if (selectedColorIndex_ != index) {
        selectedColorIndex_ = index;
        reload_ = true;
        selectedColorIndexChanged().emit();
        selectedColorChanged().emit();
        requestRepaint();
    }
}

const core::Color& ColorListView::selectedColor() const {
    if (0 <= selectedColorIndex_ && selectedColorIndex_ < colors_.length()) {
        return colors_.getUnchecked(selectedColorIndex_);
    }
    else {
        return core::colors::black;
    }
}

void ColorListView::setSelectedColor(const core::Color& color) {
    if (hasSelectedColor() && selectedColor() == color) {
        return;
    }
    for (Int i = 0; i < numColors(); ++i) {
        const core::Color& color_ = colors_.getUnchecked(i);
        if (color_ == color) {
            setSelectedColorIndex(i);
            return;
        }
    }
    setSelectedColorIndex(-1);
}

void ColorListView::setColorAt(Int index, const core::Color& color) {
    core::Color& colorAtIndex_ = colors_[index];
    if (colorAtIndex_ != color) {
        colorAtIndex_ = color;
        reload_ = true;
        colorsChanged().emit();
        if (selectedColorIndex_ == index) {
            selectedColorChanged().emit();
        }
        requestRepaint();
    }
}

void ColorListView::appendColor(const core::Color& color) {
    colors_.append(color);
    reload_ = true;
    colorsChanged().emit();
    requestGeometryUpdate();
}

void ColorListView::onResize() {
    reload_ = true;
}

void ColorListView::onPaintCreate(graphics::Engine* engine) {
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

namespace {

float getItemLengthInPx(style::StylableObject* item, core::StringId property) {
    PreferredSize p = item->style(property).to<PreferredSize>();
    if (p.isAuto()) {
        return 10.0f;
    }
    else {
        // TODO: convert units
        return p.value();
    }
}

} // namespace

void ColorListView::onPaintDraw(graphics::Engine* engine, PaintOptions) {

    namespace gs = graphics::strings;

    if (reload_) {
        reload_ = false;
        core::FloatArray a = {};
        if (numColors() > 0) {
            updateMetrics_();
            const Metrics& m = metrics_;

            float borderWidth = detail::getLength(item_.get(), gs::border_width);
            core::Color borderColor = detail::getColor(item_.get(), gs::border_color);
            style::BorderRadiuses radiuses = detail::getBorderRadiuses(item_.get());

            geometry::Vec2f itemSize(m.itemWidth, m.itemHeight);

            for (Int i = 0; i < numColors(); ++i) {
                core::Color borderColor_ = borderColor;
                float borderWidth_ = borderWidth;
                const core::Color& color = colorAt(i);
                Int row = i / m.numColumns;
                Int column = i - m.numColumns * row;
                float x1 = column * (m.itemWidth + m.gap);
                float y1 = row * (m.itemHeight + m.gap);
                float x2 = hint(x1 + m.itemWidth, m.hinting);
                float y2 = hint(y1 + m.itemHeight, m.hinting);
                x1 = hint(x1, m.hinting);
                x2 = hint(x2, m.hinting);
                geometry::Rect2f itemRect(x1, y1, x2, y2);
                if (i == hoveredColorIndex_ || i == selectedColorIndex_) {
                    borderColor_ = highlightColor;
                    borderWidth_ += 2;
                    itemRect = itemRect + ui::Margins(2);
                }
                detail::insertRect(
                    a, color, borderColor_, itemRect, radiuses, borderWidth_);
            }
        }
        engine->updateVertexBufferData(triangles_, std::move(a));
    }

    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_, -1, 0);
}

void ColorListView::onPaintDestroy(graphics::Engine*) {
    triangles_.reset();
}

namespace {

Int computeTrackIndex(float position, float itemSize, float gap, float numTracks) {
    float trackWidth = gap + itemSize;
    float trackIndex_ = std::floor(position / trackWidth);
    Int trackIndex = static_cast<Int>(trackIndex_);
    if (trackIndex >= 0 && trackIndex < numTracks) {
        float trackRelativePosition = position - trackWidth * trackIndex_;
        if (trackRelativePosition > itemSize) {
            trackIndex = -1;
        }
    }
    else {
        trackIndex = -1;
    }
    return trackIndex;
}

} // namespace

bool ColorListView::onMouseMove(MouseEvent* event) {

    const Metrics& m = metrics_;

    // Find color slot under mouse
    Int column = computeTrackIndex(event->x(), m.itemWidth, m.gap, m.numColumns);
    Int row = computeTrackIndex(event->y(), m.itemHeight, m.gap, m.numRows);

    Int newHoveredColorIndex = -1;
    if (column >= 0 && row >= 0) {
        Int index = row * m.numColumns + column;
        if (index >= 0 && index < numColors()) {
            newHoveredColorIndex = index;
        }
    }
    if (newHoveredColorIndex != hoveredColorIndex_) {
        hoveredColorIndex_ = newHoveredColorIndex;
        reload_ = true;
        requestRepaint();
    }

    if (isScrubbing_) {
        selectColorFromHovered_();
    }

    return true;
}

bool ColorListView::onMousePress(MouseEvent* event) {
    if (event->button() == MouseButton::Left) {
        isScrubbing_ = true;
        selectColorFromHovered_();
        return true;
    }
    else {
        return false;
    }
}

bool ColorListView::onMouseRelease(MouseEvent* event) {
    if (event->button() == MouseButton::Left) {
        isScrubbing_ = false;
        selectColorFromHovered_();
        return true;
    }
    else {
        return false;
    }
}

bool ColorListView::onMouseEnter() {
    return true;
}

bool ColorListView::onMouseLeave() {
    if (hoveredColorIndex_ != -1) {
        hoveredColorIndex_ = -1;
        reload_ = true;
        requestRepaint();
    }
    return true;
}

style::StylableObject* ColorListView::firstChildStylableObject() const {
    return nullptr;
}

style::StylableObject* ColorListView::lastChildStylableObject() const {
    return nullptr;
}

float ColorListView::preferredWidthForHeight(float /* height */) const {
    // TODO
    return preferredSize()[0];
}

float ColorListView::preferredHeightForWidth(float width) const {
    Metrics m = computeMetricsFromWidth_(width);
    return m.height;
}

geometry::Vec2f ColorListView::computePreferredSize() const {
    geometry::Vec2f res(0, 0);
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    if (w.type() != auto_) {
        res[0] = w.value();
    }
    else {
        // TODO: something better?
        res[0] = 100.0f;
    }
    if (h.type() != auto_) {
        res[1] = h.value();
    }
    else {
        Metrics m = computeMetricsFromWidth_(res[0]);
        res[1] = m.height;
    }
    return res;
}

ColorListView::Metrics ColorListView::computeMetricsFromWidth_(float width) const {

    namespace gs = graphics::strings;

    // Note: in order to fill the available width while being "justified", we
    // need to stretch either the gap between the items, or the items
    // themselves. For now, we decide to stretch the items. In the
    // future, we may want to make it configurable in the stylesheet.

    Metrics m;
    m.hinting = (style(gs::pixel_hinting) == gs::normal);
    m.itemPreferredWidth = getItemLengthInPx(item_.get(), strings::preferred_width);
    m.numColumns = static_cast<Int>(std::round(width / m.itemPreferredWidth));
    m.numColumns = (std::max)(Int(1), m.numColumns);
    m.gap = 3;
    m.itemWidth = (width - (m.numColumns - 1) * m.gap) / m.numColumns;
    m.itemHeight = hint(m.itemWidth, m.hinting);
    m.numRows = (numColors() + m.numColumns - 1) / m.numColumns;
    m.width = width;
    m.height = (m.numRows - 1) * (m.itemHeight + m.gap) + m.itemHeight;
    return m;
}

void ColorListView::updateMetrics_() const {
    metrics_ = computeMetricsFromWidth_(width());
}

bool ColorListView::selectColorFromHovered_() {
    if (hoveredColorIndex_ != -1 && hoveredColorIndex_ != selectedColorIndex_) {
        selectedColorIndex_ = hoveredColorIndex_;
        reload_ = true;
        requestRepaint();
        selectedColorIndexChanged().emit();
        selectedColorChanged().emit();
        colorSelected().emit();
        return true;
    }
    else {
        return false;
    }
}

} // namespace vgc::ui
