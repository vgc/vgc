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

VGC_DECLARE_OBJECT(ColorListView);
VGC_DECLARE_OBJECT(ColorListViewItem);
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
    ColorListView* colorListView_;

    void onSelectorSelectedColor_();
    VGC_SLOT(onSelectorSelectedColorSlot_, onSelectorSelectedColor_)

    void onColorListViewSelectedColor_();
    VGC_SLOT(onColorListViewSelectedColorSlot_, onColorListViewSelectedColor_)

    void onRgbEdited_();
    VGC_SLOT(onRgbEditedSlot_, onRgbEdited_)

    void onHslEdited_();
    VGC_SLOT(onHslEditedSlot_, onHslEdited_)

    void onHexEdited_();
    VGC_SLOT(onHexEditedSlot_, onHexEdited_)

    void onAddToPaletteClicked_();
    VGC_SLOT(onAddToPaletteClickedSlot_, onAddToPaletteClicked_)

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

    /// This signal is emitted when the selected color changed, either as a
    /// result of user interaction, or because `setSelectedColor()` was
    /// called.
    ///
    VGC_SIGNAL(selectedColorChanged)

    /// This signal is emitted whenever the selected color changed as a result
    /// of user interaction with the color palette. The signal isn't emitted
    /// when the selected color is set programatically via setSelectedColor().
    ///
    VGC_SIGNAL(colorSelected)

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
        bool hinting;
    };
    mutable Metrics metrics_;

    Metrics computeMetricsFromWidth_(float width) const;
    void updateMetrics_() const;
    SelectorType hoveredSelector_(const geometry::Vec2f& p);
    std::pair<Int, Int> hoveredSaturationLightness_(const geometry::Vec2f& p);
    Int hoveredHue_(const geometry::Vec2f& p);
    bool selectColorFromHovered_();
};

/// \class vgc::ui::ColorListViewItem
/// \brief A StylableObject used for styling items in a ColorListView.
///
class VGC_UI_API ColorListViewItem : public style::StylableObject {
private:
    VGC_OBJECT(ColorListViewItem, style::StylableObject)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

    friend class ColorListView;
    ColorListViewItem(ColorListView* view);
    static ColorListViewItemPtr create(ColorListView* view);

public:
    // implements StylableObject interface
    style::StylableObject* parentStylableObject() const override;
    style::StylableObject* firstChildStylableObject() const override;
    style::StylableObject* lastChildStylableObject() const override;
    style::StylableObject* previousSiblingStylableObject() const override;
    style::StylableObject* nextSiblingStylableObject() const override;
    const style::StyleSheet* defaultStyleSheet() const override;

private:
    ColorListView* view_;
};

/// \class vgc::ui::ColorListView
/// \brief Shows a list of colors.
///
class VGC_UI_API ColorListView : public Widget {
private:
    VGC_OBJECT(ColorListView, Widget)

protected:
    ColorListView();

public:
    /// Creates a ColorListView.
    ///
    static ColorListViewPtr create();

    /// Returns whether there is a selected color.
    ///
    Int hasSelectedColor() const {
        return selectedColorIndex_ != -1;
    }

    /// Returns the index of the selected color. Returns -1 if there is no
    /// selected color.
    ///
    Int selectedColorIndex() const {
        return selectedColorIndex_;
    }

    /// Sets the selected color index.
    ///
    /// If the given integer is not in the range [0, `numColors()` - 1], then
    /// the current selected color (if any) will be deselected, and the
    /// `selectedColorIndex()` will be set to `-1`.
    ///
    void setSelectedColorIndex(Int index);

    /// Returns the selected color. Returns a fully black color if there is no
    /// selected color.
    ///
    const core::Color& selectedColor() const;

    /// Attempts to find an index that matches the current color, and set it as selected
    /// index if found. Otherwise, unselects the current selected color index if any.
    ///
    void setSelectedColor(const core::Color& color);

    /// Returns the number of colors in this ColorListView.
    ///
    Int numColors() const {
        return colors_.length();
    }

    /// Returns the color at the given index. Throws `IndexError` if the given index is not
    /// between `0` and `numColors() - 1`.
    ///
    const core::Color& colorAt(Int index) const {
        return colors_[index];
    }

    /// Sets the color at the given index. Throws `IndexError` if the given
    /// index is not between `0` and `numColors() - 1`.
    ///
    void setColorAt(Int index, const core::Color& color);

    /// Appends a new color to the list of colors.
    ///
    void appendColor(const core::Color& color);

    /// This signal is emitted when the selected color index changed, either as
    /// a result of user interaction or because `setSelectedColorIndex()` was
    /// called.
    ///
    VGC_SIGNAL(selectedColorIndexChanged)

    /// This signal is emitted when the selected color changed, either as a
    /// result of user interaction, or because `setSelectedColorIndex()` was
    /// called, or because `setColorAt()` changed the color value of the
    /// selected color index.
    ///
    VGC_SIGNAL(selectedColorChanged)

    /// This signal is emitted when the selected color changed as a result of
    /// user interaction, that is, the user clicked on a color.
    ///
    VGC_SIGNAL(colorSelected)

    /// This signal is emitted when the list of colors changed, for example,
    /// `setColorAt()` or `appendColor()` was called.
    ///
    VGC_SIGNAL(colorsChanged)

    // Implement Widget interface
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

    // Implement StylableObject interface
    style::StylableObject* firstChildStylableObject() const override;
    style::StylableObject* lastChildStylableObject() const override;

protected:
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;
    geometry::Vec2f computePreferredSize() const override;

private:
    struct Metrics {
        float itemPreferredWidth;
        Int numColumns;
        Int numRows;
        float gap;
        float itemWidth;
        float itemHeight;
        float width;
        float height;
        bool hinting;
    };
    mutable Metrics metrics_;

    Int hoveredColorIndex_ = -1;
    Int selectedColorIndex_ = -1;
    core::Array<core::Color> colors_;
    graphics::GeometryViewPtr triangles_;
    ColorListViewItemPtr item_;
    bool isScrubbing_ = false;
    bool reload_ = true;

    Metrics computeMetricsFromWidth_(float width) const;
    void updateMetrics_() const;
    bool selectColorFromHovered_();
};

} // namespace vgc::ui

#endif // VGC_UI_COLORPALETTE_H
