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

#include <vgc/ui/widget.h>

namespace vgc {
namespace ui {

VGC_DECLARE_OBJECT(ColorPalette);

/// \class vgc::ui::ColorPalette
/// \brief Allow users to select a color.
///
class VGC_UI_API ColorPalette : public Widget {
private:
    VGC_OBJECT(ColorPalette, Widget)

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
    VGC_SIGNAL(colorSelected);

    // reimpl
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintFlags flags) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

private:
    enum class SelectorType {
        None,
        Hue,
        SaturationLightness
    };
    SelectorType hoveredSelector_(const geometry::Vec2f& p);
    std::pair<Int, Int> hoveredSaturationLightness_(const geometry::Vec2f& p);
    Int hoveredHue_(const geometry::Vec2f& p);
    bool selectColorFromHovered_();
    core::Color selectedColor_;
    graphics::GeometryViewPtr triangles_;
    float oldWidth_;
    float oldHeight_;
    bool reload_;
    float margin_;
    bool isContinuous_;
    float selectorBorderWidth_;
    float cellBorderWidth_;
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
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_COLORPALETTE_H
