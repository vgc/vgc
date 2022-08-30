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

#ifndef VGC_UI_COLORPALETTE_H
#define VGC_UI_COLORPALETTE_H

#include <vgc/geometry/rect2f.h>
#include <vgc/ui/column.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ColorPalette);
VGC_DECLARE_OBJECT(ColorPaletteSelector);
VGC_DECLARE_OBJECT(LineEdit);

/// \class vgc::ui::ColorPalette
/// \brief Allow users to select a color.
///
class VGC_UI_API ColorPalette : public Column {
private:
    VGC_OBJECT(ColorPalette, Column)

protected:
    /// This is an implementation details. Please use
    /// ColorPalette::create() instead.
    ///
    ColorPalette();

public:
    /// Creates a ColorPalette.
    ///
    static ColorPalettePtr create();

    /// Returns the selected color.
    ///
    core::Color selectedColor() const {
        return selectedColor_;
    }

    /// Sets the selected color.
    ///
    void setSelectedColor(const core::Color& color);

    /// This signal is emitted whenever the selected color changed as a result
    /// of user interaction with the color palette. The signal isn't emitted
    /// when the selected color is set programatically via setSelectedColor().
    ///
    VGC_SIGNAL(colorSelected)

private:
    core::Color selectedColor_;
    ColorPaletteSelector* selector_;
    LineEdit* rLineEdit_;
    LineEdit* gLineEdit_;
    LineEdit* bLineEdit_;
    LineEdit* hLineEdit_;
    LineEdit* sLineEdit_;
    LineEdit* lLineEdit_;
    LineEdit* hexLineEdit_;

    void onSelectorSelectedColor_();
    VGC_SLOT(onSelectorSelectedColorSlot_, onSelectorSelectedColor_)

    void onRgbEdited_();
    VGC_SLOT(onRgbEditedSlot_, onRgbEdited_)

    void onHslEdited_();
    VGC_SLOT(onHslEditedSlot_, onHslEdited_)

    void onHexEdited_();
    VGC_SLOT(onHexEditedSlot_, onHexEdited_)

    void setSelectedColorNoCheckNoEmit_(const core::Color& color);
};

/// \class vgc::ui::ColorPaletteSelector
/// \brief Allow users to select a color.
///
class VGC_UI_API ColorPaletteSelector : public Widget {
private:
    VGC_OBJECT(ColorPaletteSelector, Widget)

protected:
    /// This is an implementation details. Please use
    /// ColorPalette::create() instead.
    ///
    ColorPaletteSelector();

public:
    /// Creates a ColorPalette.
    ///
    static ColorPaletteSelectorPtr create();

    /// Returns the selected color.
    ///
    core::Color selectedColor() const {
        return selectedColor_;
    }

    /// Sets the selected color.
    ///
    void setSelectedColor(const core::Color& color);

    /// This signal is emitted whenever the selected color changed as a result
    /// of user interaction with the color palette. The signal isn't emitted
    /// when the selected color is set programatically via setSelectedColor().
    ///
    VGC_SIGNAL(colorSelected);

    // reimpl
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

protected:
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;
    geometry::Vec2f computePreferredSize() const override;

private:
    enum class SelectorType {
        None,
        Hue,
        SaturationLightness
    };
    core::Color selectedColor_;
    graphics::GeometryViewPtr triangles_;
    float oldWidth_;
    float oldHeight_;
    bool reload_;
    bool isContinuous_;
    core::Color borderColor_;
    Int numHueSteps_;        // >= 2 and even
    Int numSaturationSteps_; // >= 2
    Int numLightnessSteps_;  // >= 2
    Int hoveredHueIndex_;
    Int hoveredSaturationIndex_;
    Int hoveredLightnessIndex_;
    SelectorType scrubbedSelector_;
    bool isSelectedColorExact_; // whether selected color was computed from indices below
    Int selectedHueIndex_;
    Int selectedSaturationIndex_;
    Int selectedLightnessIndex_;
    Int oldSaturationIndex_; // "old" = last chromatic color selected
    Int oldLightnessIndex_;

    struct Metrics {
        float paddingLeft;
        float paddingRight;
        float paddingTop;
        float paddingBottom;
        float rowGap;
        float borderWidth;
        float cellBorderWidth;
        float selectorBorderWidth;
        float cellOffset;
        float startOffset;
        float endOffset;
        float hueDx;
        float hueDy;
        float slDx;
        float slDy;
        float width;
        float height;
        geometry::Rect2f saturationLightnessRect;
        geometry::Rect2f hueRect;
    };
    mutable Metrics metrics_;

    Metrics computeMetricsFromWidth_(float width) const;
    void updateMetrics_() const;
    SelectorType hoveredSelector_(const geometry::Vec2f& p);
    std::pair<Int, Int> hoveredSaturationLightness_(const geometry::Vec2f& p);
    Int hoveredHue_(const geometry::Vec2f& p);
    bool selectColorFromHovered_();
};

} // namespace vgc::ui

#endif // VGC_UI_COLORPALETTE_H
