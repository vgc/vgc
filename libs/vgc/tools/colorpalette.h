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

#ifndef VGC_TOOLS_COLORPALETTE_H
#define VGC_TOOLS_COLORPALETTE_H

#include <vgc/geometry/rect2f.h>
#include <vgc/tools/api.h>
#include <vgc/ui/button.h>
#include <vgc/ui/column.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/panel.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(LineEdit);
VGC_DECLARE_OBJECT(NumberEdit);

} // namespace vgc::ui

namespace vgc::tools {

VGC_DECLARE_OBJECT(ColorListView);
VGC_DECLARE_OBJECT(ColorListViewItem);
VGC_DECLARE_OBJECT(ColorPalette);
VGC_DECLARE_OBJECT(ColorPaletteSelector);
VGC_DECLARE_OBJECT(ColorPreview);
VGC_DECLARE_OBJECT(ScreenColorPickerButton);

/// \class vgc::tools::ScreenColorPickerButton
/// \brief Allow users to pick a screen color.
///
// TODO: This should be implemented as a subclass of Action, not a
// subclass of Button.
//
// In order to do this, we need to allow MouseDrag actions to be started with a
// press-then-release, then ended with a press-then-release. For now, it is
// implemented as a subclass of Widget to take advantage of the mouse capture /
// keyboard capture mechanism, but this ought to be achievable purely in terms
// of actions too, since actions may want to capture mouse / keyboard input and
// handle them as sub-actions.
//
class VGC_TOOLS_API ScreenColorPickerButton : public ui::Button {
private:
    VGC_OBJECT(ScreenColorPickerButton, ui::Button)

protected:
    ScreenColorPickerButton(CreateKey);

public:
    /// Creates a `ScreenColorPickerButton`.
    ///
    static ScreenColorPickerButtonPtr create();

    // Reimplementation of `Widget` virtual methods
    bool onMouseMove(ui::MouseMoveEvent* event) override;
    bool onMousePress(ui::MousePressEvent* event) override;
    bool onMouseRelease(ui::MouseReleaseEvent* event) override;
    bool onKeyPress(ui::KeyPressEvent* event) override;
    bool onKeyRelease(ui::KeyReleaseEvent* event) override;

    VGC_SIGNAL(pickingStarted)
    VGC_SIGNAL(pickingStopped)
    VGC_SIGNAL(pickingCancelled)

    VGC_SIGNAL(colorHovered, (const core::Color&, color))
    VGC_SIGNAL(colorClicked, (const core::Color&, color))

private:
    bool isPicking_ = false;

    ui::CursorChanger cursorChanger_;
    core::Color hoveredColor_;
    void startPicking_();
    void stopPicking_();
    VGC_SLOT(startPickingSlot_, startPicking_)
};

/// \class vgc::tools::ColorPalette
/// \brief Allow users to select a color.
///
class VGC_TOOLS_API ColorPalette : public ui::Column {
private:
    VGC_OBJECT(ColorPalette, ui::Column)

protected:
    /// This is an implementation details. Please use
    /// ColorPalette::create() instead.
    ///
    ColorPalette(CreateKey, ui::ActionWeakPtr colorSelectSyncAction);

public:
    /// Creates a ColorPalette.
    ///
    static ColorPalettePtr create(ui::ActionWeakPtr colorSelectSyncAction = nullptr);

    /// Returns the selected color.
    ///
    core::Color selectedColor() const {
        return selectedColor_;
    }

    /// Sets the selected color.
    ///
    void setSelectedColor(const core::Color& color);
    VGC_SLOT(setSelectedColor)

    /// Returns the color list view
    ///
    ColorListView* colorListView() const {
        return colorListView_;
    }

    /// Sets the list of colors.
    ///
    void setColors(const core::Array<core::Color>& colors);
    VGC_SLOT(setColors)

    /// This signal is emitted whenever the selected color changed as a result
    /// of user interaction with the color palette. The signal isn't emitted
    /// when the selected color is set programatically via setSelectedColor().
    ///
    VGC_SIGNAL(colorSelected, (const core::Color&, color))

    /// This signal is emitted whenever the palette colors changed as a result
    /// of user interaction or if they were set programatically via colorListView->setColors().
    ///
    VGC_SIGNAL(colorsChanged, (const core::Array<core::Color>&, colors))

private:
    core::Color selectedColor_;
    core::Color selectedColorOnPickScreenStarted_;
    ColorPreviewPtr colorPreview_;
    ui::ActionGroupPtr stepsActionGroup_;
    ui::Button* stepsButton_;
    ui::Button* continuousButton_;
    ColorPaletteSelector* selector_;
    ui::NumberEdit* hStepsEdit_;
    ui::NumberEdit* sStepsEdit_;
    ui::NumberEdit* lStepsEdit_;
    ui::NumberEdit* rEdit_;
    ui::NumberEdit* gEdit_;
    ui::NumberEdit* bEdit_;
    ui::NumberEdit* hEdit_;
    ui::NumberEdit* sEdit_;
    ui::NumberEdit* lEdit_;
    ui::LineEdit* hexEdit_;
    ColorListView* colorListView_;

    // Prevent infinite loops: RGB changed -> color changed -> HSL changed -> ...
    //
    // Another design would be to add the NumberEdit::valueEdited() signal,
    // which would be similar to NumberEdit::valueChanged(), except that it
    // would not be edited when the value is edited programmatically (similarly
    // to LineEdit textChanged() vs textEdited()).
    //
    enum class ColorChangeOrigin {
        None,
        RgbNumberEdit,
        HslNumberEdit,
        HexLineEdit,
        PublicSetter,
        PrivateSetter
    };
    ColorChangeOrigin colorChangeOrigin_ = ColorChangeOrigin::None;

    void onSelectorColorSelected_();
    VGC_SLOT(onSelectorColorSelectedSlot_, onSelectorColorSelected_)

    void onColorListViewColorSelected_();
    VGC_SLOT(onColorListViewColorSelectedSlot_, onColorListViewColorSelected_)

    void onColorListViewColorsChanged_();
    VGC_SLOT(onColorListViewColorsChangedSlot_, onColorListViewColorsChanged_)

    void onContinuousChanged_();
    VGC_SLOT(onContinuousChangedSlot_, onContinuousChanged_)

    void onStepsValueChanged_();
    VGC_SLOT(onStepsValueChangedSlot_, onStepsValueChanged_)

    void onRgbValueChanged_();
    VGC_SLOT(onRgbValueChangedSlot_, onRgbValueChanged_)

    void onHslValueChanged_();
    VGC_SLOT(onHslValueChangedSlot_, onHslValueChanged_)

    void onHexEdited_();
    VGC_SLOT(onHexEditedSlot_, onHexEdited_)

    void onPickScreenStarted_();
    VGC_SLOT(onPickScreenStartedSlot_, onPickScreenStarted_)

    void onPickScreenCancelled_();
    VGC_SLOT(onPickScreenCancelledSlot_, onPickScreenCancelled_)

    void onPickScreenColorHovered_(const core::Color& color);
    VGC_SLOT(onPickScreenColorHoveredSlot_, onPickScreenColorHovered_)

    void onAddToPalette_();
    VGC_SLOT(onAddToPaletteSlot_, onAddToPalette_)

    void onRemoveFromPalette_();
    VGC_SLOT(onRemoveFromPaletteSlot_, onRemoveFromPalette_)

    void updateStepsLineEdits_();
    void selectColor_(const core::Color& color);
    void setSelectedColorNoCheckNoEmit_(const core::Color& color);
};

/// \class vgc::tools::ColorPaletteSelector
/// \brief Allow users to select a color.
///
class VGC_TOOLS_API ColorPaletteSelector : public ui::Widget {
private:
    VGC_OBJECT(ColorPaletteSelector, ui::Widget)

protected:
    /// This is an implementation details. Please use
    /// ColorPalette::create() instead.
    ///
    ColorPaletteSelector(CreateKey);

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

    /// Returns the number of hue steps.
    ///
    Int numHueSteps() const {
        return numHueSteps_;
    }

    /// Returns the number of saturation steps.
    ///
    Int numSaturationSteps() const {
        return numSaturationSteps_;
    }

    /// Returns the number of lightness steps.
    ///
    Int numLightnessSteps() const {
        return numLightnessSteps_;
    }

    /// Returns the number of lightness steps.
    ///
    void setHslSteps(Int hue, Int saturation, Int lightness);

    /// Returns whether the selector is in continuous mode.
    ///
    bool isContinuous() const {
        return isContinuous_;
    }

    /// Sets whether the selector is in continuous mode.
    ///
    void setContinuous(bool isContinuous);

    // reimpl
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(ui::MouseMoveEvent* event) override;
    bool onMousePress(ui::MousePressEvent* event) override;
    bool onMouseRelease(ui::MouseReleaseEvent* event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;

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
    Int numHueSteps_;        // >= 2
    Int numSaturationSteps_; // >= 2
    Int numLightnessSteps_;  // >= 3
    Int hoveredHueIndex_;
    Int hoveredSaturationIndex_;
    Int hoveredLightnessIndex_;
    SelectorType scrubbedSelector_;
    bool isSelectedColorExact_; // whether selected color was computed from indices below
    Int selectedHueIndex_;
    Int selectedSaturationIndex_;
    Int selectedLightnessIndex_;
    // Continuous mode. Note that these values can be different
    // from selectedColor_.toHsl() in case of non-chromatic colors.
    float selectedHue_ = 0;
    float selectedSaturation_ = 0;
    float selectedLightness_ = 0;
    float hoveredHue_ = -1;
    float hoveredSaturation_ = -1;
    float hoveredLightness_ = -1;

    struct Metrics {
        float paddingLeft;
        float paddingRight;
        float paddingTop;
        float paddingBottom;
        float rowGap;
        float borderWidth;
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

    enum class SelectionOrigin {
        External,
        Steps,
        Continuous
    };
    SelectionOrigin selectionOrigin_ = SelectionOrigin::External;

    // Cache hue values for hue selector
    core::FloatArray hues_;

    void drawHueSelector_(core::FloatArray& a);
    void computeSlSubMetrics_(float width, Metrics& m) const;
    void computeHueSubMetrics_(float width, Metrics& m) const;
    Metrics computeMetricsFromWidth_(float width) const;
    void updateMetrics_() const;
    SelectorType hoveredSelector_(const geometry::Vec2f& p);
    std::pair<Int, Int> getHoveredSaturationLightness_(const geometry::Vec2f& p);
    Int getHoveredHueIndex_(const geometry::Vec2f& p);
    void setSelectedColor_(const core::Color& color);
    void updateStepsFromSelectedColor_();
    void updateContinuousFromSelectedColor_();
    bool selectColorFromHovered_();
    float getContinuousHoveredHue_(const geometry::Vec2f& position);
    std::pair<float, float>
    getContinuousHoveredSaturationLightness_(const geometry::Vec2f& position);
    bool selectContinuousColor_(const geometry::Vec2f& position);
};

/// \class vgc::tools::ColorPreview
/// \brief Display a color.
///
class VGC_TOOLS_API ColorPreview : public ui::Widget {
private:
    VGC_OBJECT(ColorPreview, ui::Widget)

protected:
    ColorPreview(CreateKey);

public:
    /// Creates a ColorPreview.
    ///
    static ColorPreviewPtr create();

    /// Returns the color.
    ///
    const core::Color& color() const {
        return color_;
    }

    /// Sets the color.
    ///
    void setColor(const core::Color& color);

    /// This signal is emitted when the color changed.
    ///
    VGC_SIGNAL(colorChanged)

    // Implement Widget interface
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

private:
    core::Color color_;
    graphics::GeometryViewPtr triangles_;
    bool reload_ = true;
};

/// \class vgc::tools::ColorListView
/// \brief Shows a list of colors.
///
class VGC_TOOLS_API ColorListView : public ui::Widget {
private:
    VGC_OBJECT(ColorListView, ui::Widget)

protected:
    ColorListView(CreateKey);

public:
    /// Creates a ColorListView.
    ///
    static ColorListViewPtr create();

    /// Returns whether there is a selected color.
    ///
    bool hasSelectedColor() const {
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

    /// Returns the color index that matches the given `color`.
    ///
    /// Returns -1 if no such color is found.
    ///
    Int findColor(const core::Color& color);

    /// Attempts to find an index that matches the current color, and set it as selected
    /// index if found. Otherwise, unselects the current selected color index if any.
    ///
    void setSelectedColor(const core::Color& color);

    /// Returns the number of colors in this ColorListView.
    ///
    Int numColors() const {
        return colors_.length();
    }

    /// Returns the color at the given index.
    ///
    /// Throws `IndexError` if the given index is not between `0` and
    /// `numColors() - 1`.
    ///
    const core::Color& colorAt(Int index) const {
        return colors_[index];
    }

    /// Sets the color at the given index.
    ///
    /// If `index == selectedColorIndex()` and `color != selectedColor()`, then
    /// `selectedColorIndex()` becomes `-1`, unless another color matches the
    /// old `selectedColor()`, in which case `selectedColorIndex()` becomes the
    /// index of that color.
    ///
    /// Throws `IndexError` if the given index is not between `0` and
    /// `numColors() - 1`.
    ///
    void setColorAt(Int index, const core::Color& color);

    /// Appends a new color to the list of colors.
    ///
    void appendColor(const core::Color& color);

    /// Removes the color at the given index.
    ///
    /// If `index == selectedColorIndex()`, then `selectedColorIndex()` becomes
    /// `-1`, unless another color matches the old `selectedColor()`, in which
    /// case `selectedColorIndex()` becomes the index of that color.
    ///
    /// If `index < selectedColorIndex()`, then the `selectedColorIndex()`
    /// is decreased by `1`.
    ///
    /// Throws `IndexError` if the given index is not between `0` and
    /// `numColors() - 1`.
    ///
    void removeColorAt(Int index);

    /// Changes all the colors in this `ColorListView`.
    ///
    /// This attempts to preserve `selectedColor()` by updating the
    /// `selectedColorIndex()`. If none of the color in `colors` is equal to
    /// the old `selectedColor()`, then the `selectedColorIndex()` becomes
    /// `-1`.
    ///
    void setColors(const core::Array<core::Color>& colors);

    /// This signal is emitted when the selected color index changed, either as
    /// a result of user interaction or because `setSelectedColorIndex()` was
    /// called.
    ///
    VGC_SIGNAL(selectedColorIndexChanged)

    /// This signal is emitted when the selected color changed. This can happen
    /// in the following situations:
    ///
    /// - The `selectedColorIndex()` changed, either due to user interaction or
    /// calling `setSelectedColorIndex()`.
    ///
    /// - The `colors()` changed.
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
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(ui::MouseMoveEvent* event) override;
    bool onMousePress(ui::MousePressEvent* event) override;
    bool onMouseRelease(ui::MouseReleaseEvent* event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;

protected:
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;
    geometry::Vec2f computePreferredSize() const override;

private:
    struct Metrics {
        float itemPreferredWidth;
        float itemPreferredHeight;
        Int numColumns;
        Int numRows;
        float rowGap;
        float columnGap;
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
    style::StylableObjectPtr item_;
    bool isScrubbing_ = false;
    bool reload_ = true;

    Metrics computeMetricsFromWidth_(float width) const;
    void updateMetrics_() const;
    bool selectColorFromHovered_();
};

VGC_DECLARE_OBJECT(ColorsPanel);

/// \class vgc::canvas::ColorsPanel
/// \brief A `Panel` for selecting the current color and document colors.
///
class VGC_TOOLS_API ColorsPanel : public ui::Panel {
private:
    VGC_OBJECT(ColorsPanel, ui::Panel)

protected:
    ColorsPanel(CreateKey key, const ui::PanelContext& context);

public:
    // TODO: A cleaner way to do this, also supporting translations.
    static constexpr std::string_view label = "Colors";
    static constexpr std::string_view id = "vgc.common.colors";
    static constexpr ui::PanelDefaultArea defaultArea = ui::PanelDefaultArea::Left;

    /// Creates a `ToolsPanel`.
    ///
    static ColorsPanelPtr create(const ui::PanelContext& context);
};

} // namespace vgc::tools

#endif // VGC_TOOLS_COLORPALETTE_H
