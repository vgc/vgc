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
#include <vgc/geometry/mat3f.h>
#include <vgc/graphics/strings.h>
#include <vgc/style/strings.h>
#include <vgc/ui/button.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/label.h>
#include <vgc/ui/lineedit.h>
#include <vgc/ui/margins.h>
#include <vgc/ui/row.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

namespace {

core::Color initialColor(0.416f, 0.416f, 0.918f);
core::Color cursorOuterColor(0.15f, 0.2f, 0.3f);
core::Color cursorInnerColor(1.0f, 1.0f, 1.0f);

namespace strings_ {

core::StringId horizontal("horizontal");
core::StringId first("first");
core::StringId last("last");
core::StringId not_first("not-first");
core::StringId not_last("not-last");
core::StringId steps("steps");
core::StringId rgb("rgb");
core::StringId hsl("hsl");
core::StringId hex("hex");
core::StringId button_group("button-group");
core::StringId field_group("field-group");
core::StringId field_label("field-label");
core::StringId field_row("field-row");

} // namespace strings_

// Converts a gamma-corrected sRGB color channel to its linear RGB value.
//
float srgbGammaToLinear(float v) {
    if (v <= 0.04045f) {
        return v / 12.92f;
    }
    else {
        return std::pow(((v + 0.055f) / 1.055f), 2.4f);
    }
}

// Converts a gamma-corrected sRGB color to its linear RGB value.
//
geometry::Vec3f srgbGammaToLinear(const core::Color& c) {
    return {srgbGammaToLinear(c.r()), srgbGammaToLinear(c.g()), srgbGammaToLinear(c.b())};
}

// Converts a linear RGB color channel to its gamma-corrected sRGB value.
//
float srgbLinearToGamma(float v) {
    if (v <= 0.0031308f) {
        return v * 12.92f;
    }
    else {
        return std::pow(v, 1.0f / 2.4f) * 1.055f - 0.055f;
    }
}

// Converts a gamma-corrected sRGB color to its linear RGB value.
//
core::Color srgbLinearToGamma(const geometry::Vec3f& c) {
    return {srgbLinearToGamma(c.x()), srgbLinearToGamma(c.y()), srgbLinearToGamma(c.z())};
}

// Converts an sRGB color to XYZ.
//
geometry::Vec3f srgbToXyz(const core::Color& c) {
    // clang-format off
    static geometry::Mat3f m = {
        0.4124f, 0.3576f, 0.1805f,
        0.2126f, 0.7152f, 0.0722f,
        0.0193f, 0.1192f, 0.9505f
    };
    // clang-format on
    return m * srgbGammaToLinear(c);
}

// Converts an XYZ color to sRGB.
//
core::Color xyzToSrgb(const geometry::Vec3f& c) {
    // clang-format off
    static geometry::Mat3f m = {
        0.4124f, 0.3576f, 0.1805f,
        0.2126f, 0.7152f, 0.0722f,
        0.0193f, 0.1192f, 0.9505f
    };
    // clang-format on
    static geometry::Mat3f invM = m.inverted();
    return srgbLinearToGamma(invM * c);
}

// Helper function for xyz to lab
float labFn(float t) {
    constexpr float d = 6.0f / 29;
    constexpr float e = 4.0f / 29;
    constexpr float d2 = d * d;
    constexpr float d3 = d * d * d;
    constexpr float inv3d2 = 1.0f / (3 * d2);
    constexpr float inv3 = 1.0f / 3.0f;
    if (t > d3) {
        return std::pow(t, inv3);
    }
    else {
        return t * inv3d2 + e;
    }
}

// Helper function for lab to xyz
float invLabFn(float t) {
    constexpr float d = 6.0f / 29;
    constexpr float e = 4.0f / 29;
    constexpr float _3d2 = 3 * d * d;
    if (t > d) {
        return t * t * t;
    }
    else {
        return _3d2 * (t - e);
    }
}

namespace lab {

// Achromatic reference for standard illuminant D65
constexpr float xn = 0.950489f;
constexpr float yn = 1.000000f;
constexpr float zn = 1.088840f;

} // namespace lab

// Converts an XYZ color to Lab D65.
//
geometry::Vec3f xyzToLabD65(const geometry::Vec3f& c) {
    float fx = labFn(c.x() / lab::xn);
    float fy = labFn(c.y() / lab::yn);
    float fz = labFn(c.z() / lab::zn);
    return {116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)};
}

// Converts a Lab D65 color to XYZ.
//
geometry::Vec3f labD65ToXyz(const geometry::Vec3f& c) {
    constexpr float inv116 = 1.0f / 116;
    constexpr float inv200 = 1.0f / 200;
    constexpr float inv500 = 1.0f / 500;
    float l = c.x();
    float a = c.y();
    float b = c.z();
    float l_ = inv116 * (l + 16);
    return {
        lab::xn * invLabFn(l_ + inv500 * a),
        lab::yn * invLabFn(l_),
        lab::zn * invLabFn(l_ - inv200 * b)};
}

// Converts an sRGB color to Lab D65.
//
geometry::Vec3f srgbToLabD65(const core::Color& c) {
    return xyzToLabD65(srgbToXyz(c));
}

// Converts a Lab D65 color to sRGB.
//
core::Color labD65ToSrgb(const geometry::Vec3f& c) {
    return xyzToSrgb(labD65ToXyz(c));
}

// Whether to darken or lighten (or let the algorithm choose) a given color
// when asked to compute its corresponding highlight color.
//
enum class HighlightStyle {
    DarkenOnly,
    LightenOnly,
    Auto
};

// Returns a color (H, S', L') with the same hue as the given color (H, S, L),
// but a slightly different saturation and lightness so that users can perceive
// the difference.
//
core::Color
computeHighlightColor(const core::Color& c, HighlightStyle style = HighlightStyle::Auto) {

    // Convert to HSL
    auto [h, s, lightness] = c.toHsl();
    std::ignore = s;

    // Convert to Lab space, which is a perceptual color space. This means that
    // increasing the luminance by a fixed amount in this space looks like an
    // increase by a fixed amount to the human eye (at least, approximately).
    //
    geometry::Vec3f lab = srgbToLabD65(c);

    // Slightly increase or decrease the luminance in Lab space, based on a
    // lightness threshold.
    //
    // Note that in theory, it would make more sense to also use luminance
    // (instead of lightness) for the threshold. In practice, switching modes
    // based on luminance makes the discontinuity quite surprising in some
    // typical scenarios and does not look as good.
    //
    // Also, in theory, applying a fixed offset should be enough, but in
    // practice, this does not lighten enough very dark colors or darken enough
    // very light colors. So we give a bigger offset to colors whose luminance
    // are close to 0 or 100.
    //
    float luminance = lab[0];
    if (style == HighlightStyle::LightenOnly
        || (style == HighlightStyle::Auto && lightness < 0.4)) {

        luminance = 25 + 0.8f * luminance;
    }
    else {
        luminance = 75 - (0.9f * (100 - luminance));
    }
    lab[0] = luminance;

    // Convert back to sRGB
    core::Color labSpaceContrasted = labD65ToSrgb(lab);

    // Apply back the original hue. Indeed, modifying the luminance in Lab
    // space alters the hue, which sometimes look weird for an highlight.
    auto [newH, newS, newL] = labSpaceContrasted.toHsl();
    std::ignore = newH;
    core::Color res = core::Color::hsl(h, newS, newL);

    return res;
}

} // namespace

void ScreenColorPickerButton::startPicking_() {
    setHovered(false);
    isPicking_ = true;
    hoveredColor_ = colorUnderCursor();
    cursorChanger_.set(Qt::CrossCursor); // TODO: custom picker-shaped cursor
    startMouseCapture();
    startKeyboardCapture();
    pickingStarted().emit();
}

void ScreenColorPickerButton::stopPicking_() {
    isPicking_ = false;
    stopMouseCapture();
    stopKeyboardCapture();
    cursorChanger_.clear();
    pickingStopped().emit();
}

ScreenColorPickerButtonPtr ScreenColorPickerButton::create(std::string_view name) {
    Action* action = nullptr;
    ScreenColorPickerButtonPtr button = new ScreenColorPickerButton(action);
    action = button->createAction(name);
    button->setAction(action);
    action->triggered().connect(button->startPickingSlot_());
    return button;
}

ScreenColorPickerButton::ScreenColorPickerButton(Action* action)
    : Button(action, FlexDirection::Row) {
}

bool ScreenColorPickerButton::onMousePress(MouseEvent* event) {
    if (isPicking_) {
        return true;
    }
    else {
        return Button::onMousePress(event);
    }
}

bool ScreenColorPickerButton::onMouseMove(MouseEvent* event) {
    if (isPicking_) {
        hoveredColor_ = colorUnderCursor();
        colorHovered().emit(hoveredColor_);
        return true;
    }
    else {
        return Button::onMouseMove(event);
    }
}

bool ScreenColorPickerButton::onMouseRelease(MouseEvent* event) {
    if (isPicking_) {
        colorClicked().emit(hoveredColor_);
        stopPicking_();
        return true;
    }
    else {
        return Button::onMouseRelease(event);
    }
}

bool ScreenColorPickerButton::onKeyPress(KeyEvent* event) {
    if (isPicking_) {
        if (event->key() == Key::Escape) {
            pickingCancelled().emit();
            stopPicking_();
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return Button::onKeyPress(event);
    }
}

bool ScreenColorPickerButton::onKeyRelease(KeyEvent* event) {
    if (isPicking_) {
        return false;
    }
    else {
        return Button::onKeyRelease(event);
    }
}

namespace {

Widget* createFieldRow(Widget* parent, core::StringId styleClass) {
    Row* row = parent->createChild<Row>();
    row->addStyleClass(strings_::field_row);
    row->addStyleClass(styleClass);
    return row;
}

Widget* createFieldRowAndLabel(
    Widget* parent,
    core::StringId styleClass,
    std::string_view labelText) {

    Widget* row = createFieldRow(parent, styleClass);
    Label* label = row->createChild<Label>(labelText);
    label->addStyleClass(strings_::field_label);
    return row;
}

Widget* createFieldRowLabelAndGroup(
    Widget* parent,
    core::StringId styleClass,
    std::string_view labelText) {

    Widget* row = createFieldRowAndLabel(parent, styleClass, labelText);
    Row* group = row->createChild<Row>();
    group->addStyleClass(strings_::field_group);
    group->addStyleClass(strings_::horizontal);
    return group;
}

void addFirstLastStyleClasses_(std::initializer_list<Widget*> widgets) {
    const size_t n = widgets.size();
    size_t i = 0;
    for (Widget* widget : widgets) {
        bool isFirst = (i == 0);
        bool isLast = (i == n - 1);
        if (isFirst) {
            widget->addStyleClass(strings_::first);
        }
        else {
            widget->addStyleClass(strings_::not_first);
        }
        if (isLast) {
            widget->addStyleClass(strings_::last);
        }
        else {
            widget->addStyleClass(strings_::not_last);
        }
        ++i;
    }
}

void createThreeLineEdits_(
    Widget* parent,
    core::StringId styleClass,
    std::string_view labelText,
    LineEdit*& a,
    LineEdit*& b,
    LineEdit*& c) {

    Widget* group = createFieldRowLabelAndGroup(parent, styleClass, labelText);
    a = group->createChild<LineEdit>();
    b = group->createChild<LineEdit>();
    c = group->createChild<LineEdit>();
    addFirstLastStyleClasses_({a, b, c});
}

void createOneLineEdit_(
    Widget* parent,
    core::StringId styleClass,
    std::string_view labelText,
    LineEdit*& a) {

    Widget* group = createFieldRowLabelAndGroup(parent, styleClass, labelText);
    a = group->createChild<LineEdit>();
    addFirstLastStyleClasses_({a});
}

Button* createCheckableButton_(Widget* parent, std::string_view text) {
    Action* action = parent->createAction(text);
    action->setCheckable(true);
    Button* res = parent->createChild<Button>(action);
    return res;
}

} // namespace

ColorPalette::ColorPalette()
    : Column() {

    // Color preview
    colorPreview_ = createChild<ColorPreview>();

    // Continuous vs. Steps
    Row* stepsModeRow = createChild<Row>();
    stepsModeRow->addStyleClass(strings_::field_group);
    stepsButton_ = createCheckableButton_(stepsModeRow, "Steps");
    continuousButton_ = createCheckableButton_(stepsModeRow, "Continuous");
    addFirstLastStyleClasses_({stepsButton_, continuousButton_});
    stepsActionGroup_ = ActionGroup::create(CheckPolicy::ExactlyOne);
    stepsActionGroup_->addAction(stepsButton_->action());
    stepsActionGroup_->addAction(continuousButton_->action());
    createThreeLineEdits_(
        this, strings_::steps, "Steps:", hStepsEdit_, sStepsEdit_, lStepsEdit_);

    // Main color selector
    selector_ = createChild<ColorPaletteSelector>();

    // Color line edits
    createThreeLineEdits_(this, strings_::rgb, "RGB:", rEdit_, gEdit_, bEdit_);
    createThreeLineEdits_(this, strings_::hsl, "HSL:", hEdit_, sEdit_, lEdit_);
    createOneLineEdit_(this, strings_::hex, "Hex:", hexEdit_);

    // Pick screen
    ScreenColorPickerButton* pickScreenButton =
        createChild<ScreenColorPickerButton>("Pick Screen Color");

    // Palette
    Row* paletteButtons = createChild<Row>();
    paletteButtons->addStyleClass(strings_::field_row);
    Action* addToPaletteAction = createAction("Add to Palette");
    paletteButtons->createChild<Button>(addToPaletteAction);
    Action* removeFromPaletteAction = createAction("-");
    paletteButtons->createChild<Button>(removeFromPaletteAction);
    colorListView_ = createChild<ColorListView>();

    // Connections
    continuousButton_->action()->checkStateChanged().connect(onContinuousChangedSlot_());
    hStepsEdit_->editingFinished().connect(onStepsEditedSlot_());
    sStepsEdit_->editingFinished().connect(onStepsEditedSlot_());
    lStepsEdit_->editingFinished().connect(onStepsEditedSlot_());
    selector_->colorSelected().connect(onSelectorSelectedColorSlot_());
    rEdit_->editingFinished().connect(onRgbEditedSlot_());
    gEdit_->editingFinished().connect(onRgbEditedSlot_());
    bEdit_->editingFinished().connect(onRgbEditedSlot_());
    hEdit_->editingFinished().connect(onHslEditedSlot_());
    sEdit_->editingFinished().connect(onHslEditedSlot_());
    lEdit_->editingFinished().connect(onHslEditedSlot_());
    hexEdit_->editingFinished().connect(onHexEditedSlot_());
    addToPaletteAction->triggered().connect(onAddToPaletteSlot_());
    removeFromPaletteAction->triggered().connect(onRemoveFromPaletteSlot_());
    pickScreenButton->pickingStarted().connect(onPickScreenStartedSlot_());
    pickScreenButton->pickingCancelled().connect(onPickScreenCancelledSlot_());
    pickScreenButton->colorHovered().connect(onPickScreenColorHoveredSlot_());
    colorListView_->selectedColorChanged().connect(onColorListViewSelectedColorSlot_());

    // Init
    onContinuousChanged_();
    updateStepsLineEdits_();
    setSelectedColorNoCheckNoEmit_(initialColor.rounded8b());

    // Style class
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
    selectColor_(selector_->selectedColor());
}

void ColorPalette::onColorListViewSelectedColor_() {
    if (colorListView_->hasSelectedColor()) {
        selectColor_(colorListView_->selectedColor());
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

void ColorPalette::onContinuousChanged_() {
    if (hasReachedStage(core::ObjectStage::AboutToBeDestroyed)) {
        return;
    }
    bool isContinuous = continuousButton_->isChecked();
    selector_->setContinuous(isContinuous);
}

void ColorPalette::onStepsEdited_() {

    // Try to parse the new steps from the line edits.
    //
    bool isValid = true;
    Int numHueSteps = parseInt_(hStepsEdit_, isValid);
    Int numSaturationSteps = parseInt_(sStepsEdit_, isValid);
    Int numLightnessSteps = parseInt_(lStepsEdit_, isValid);

    // Check if the input was valid.
    //
    if (isValid) {
        selector_->setHslSteps(numHueSteps, numSaturationSteps, numLightnessSteps);
    }
    updateStepsLineEdits_();
}

void ColorPalette::onRgbEdited_() {

    // Try to parse the new color from the line edit.
    //
    bool isValid = true;
    Int r_ = parseInt_(rEdit_, isValid);
    Int g_ = parseInt_(gEdit_, isValid);
    Int b_ = parseInt_(bEdit_, isValid);

    // Check if the input was valid.
    //
    core::Color color = selectedColor_;
    if (isValid) {
        float r = core::Color::mapFrom255(r_);
        float g = core::Color::mapFrom255(g_);
        float b = core::Color::mapFrom255(b_);
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
    Int h_ = parseInt_(hEdit_, isValid);
    Int s_ = parseInt_(sEdit_, isValid);
    Int l_ = parseInt_(lEdit_, isValid);

    // Check if the input was valid.
    //
    core::Color color = selectedColor_;
    if (isValid) {
        // Note: Color::hsl() already does mod-360 hue
        float h = static_cast<float>(h_);
        float s = core::Color::mapFrom255(s_);
        float l = core::Color::mapFrom255(l_);
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
    core::Color parsedColor = parseHex_(hexEdit_, isValid);
    if (isValid) {
        newColor = parsedColor;
    }

    setSelectedColorNoCheckNoEmit_(newColor);
    if (selectedColor_ != oldColor) {
        colorSelected().emit();
    }
}

void ColorPalette::onPickScreenStarted_() {
    selectedColorOnPickScreenStarted_ = selectedColor_;
}

void ColorPalette::onPickScreenCancelled_() {
    selectColor_(selectedColorOnPickScreenStarted_);
}

void ColorPalette::onPickScreenColorHovered_(const core::Color& color) {
    selectColor_(color);
}

void ColorPalette::onAddToPalette_() {
    colorListView_->appendColor(selectedColor());
    colorListView_->setSelectedColorIndex(colorListView_->numColors() - 1);
}

void ColorPalette::onRemoveFromPalette_() {
    Int selectedColorIndex = colorListView_->selectedColorIndex();
    if (selectedColorIndex != -1) {
        colorListView_->removeColorAt(selectedColorIndex);
    }
}

void ColorPalette::updateStepsLineEdits_() {
    hStepsEdit_->setText(core::toString(selector_->numHueSteps()));
    sStepsEdit_->setText(core::toString(selector_->numSaturationSteps()));
    lStepsEdit_->setText(core::toString(selector_->numLightnessSteps()));
}

// This function is the same as setSelectedColor(), except that it
// also emits selectedColor() if the color changed.
//
void ColorPalette::selectColor_(const core::Color& color) {
    if (selectedColor_ != color) {
        setSelectedColorNoCheckNoEmit_(color);
        colorSelected().emit();
    }
}

void ColorPalette::setSelectedColorNoCheckNoEmit_(const core::Color& color) {

    selectedColor_ = color;

    // Update color preview
    colorPreview_->setColor(selectedColor_);

    // Update selector
    selector_->setSelectedColor(selectedColor_);

    // Update RGB line edits
    rEdit_->setText(core::toString(static_cast<Int>(std::round(color.r() * 255))));
    gEdit_->setText(core::toString(static_cast<Int>(std::round(color.g() * 255))));
    bEdit_->setText(core::toString(static_cast<Int>(std::round(color.b() * 255))));

    // Update HSL line edits
    // For now, we round to the nearest integer. Later, we may
    // want to show the first digit
    auto [h, s, l] = color.toHsl();
    hEdit_->setText(core::toString(static_cast<Int>(std::round(h))));
    sEdit_->setText(core::toString(static_cast<Int>(std::round(s * 255))));
    lEdit_->setText(core::toString(static_cast<Int>(std::round(l * 255))));

    // Update Hex line edit
    hexEdit_->setText(color.toHex());

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
    , selectedLightnessIndex_(0) {

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

    float dh = 360.0f / numHueSteps;
    float ds = 1.0f / (numSaturationSteps - 1);
    float dl = 1.0f / (numLightnessSteps - 1);

    core::Color color = core::Color::hsl( //
        hueIndex * dh,
        saturationIndex * ds,
        lightnessIndex * dl);

    return color.rounded8b();
}

} // namespace

void ColorPaletteSelector::setSelectedColor(const core::Color& color) {

    if (selectedColor_ != color) {
        selectionOrigin_ = SelectionOrigin::External;
        setSelectedColor_(color);

        // Emit signals
        reload_ = true;
        selectedColorChanged().emit();
        requestRepaint();
    }
}

void ColorPaletteSelector::setHslSteps(Int hue, Int saturation, Int lightness) {
    numHueSteps_ = hue;
    numSaturationSteps_ = saturation;
    numLightnessSteps_ = lightness;

    // clamp to valid values.
    //
    // We currently limit values to two digits because:
    // - While technically valid, higher values are quite useless, and
    //   users are better off switching to continue mode at this point
    // - Huge values (e.g., set by accident) might freeze the app due to
    //   huge rendering time.
    // - Two digits makes them fit in smaller line edits.
    //
    numHueSteps_ = core::clamp(numHueSteps_, 2, 98);
    numSaturationSteps_ = core::clamp(numSaturationSteps_, 2, 99);
    numLightnessSteps_ = core::clamp(numLightnessSteps_, 3, 99);

    // Update hovered indices and isSelectedColorExact_ based on new steps
    selectionOrigin_ = SelectionOrigin::External;
    setSelectedColor_(selectedColor_);

    // Repaint
    reload_ = true;
    requestGeometryUpdate();
}

void ColorPaletteSelector::setContinuous(bool isContinuous) {
    isContinuous_ = isContinuous;
    reload_ = true;
    requestRepaint();
}

void ColorPaletteSelector::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

namespace {

constexpr Int numQuarterCircleSamples = 8;
constexpr Int numCircleSamples = 4 * numQuarterCircleSamples;
constexpr Int circleLeftIndexBegin = 0;
constexpr Int circleTopIndex = numQuarterCircleSamples;
//constexpr Int circleRightIndex = 2 * numQuarterCircleSamples;
constexpr Int circleBottomIndex = 3 * numQuarterCircleSamples;
constexpr Int circleLeftIndexEnd = 4 * numQuarterCircleSamples;

// Starts at (-1, 0) then go clockwise (assuming y axis points down).
// Repeats the first and last sample: returned array length is numSamples + 1.
//
geometry::Vec2fArray computeUnitCircle_(Int numSamples) {
    geometry::Vec2fArray res;
    res.reserve(numSamples + 1);
    float dt = 2.0f * static_cast<float>(core::pi) / numSamples;
    for (Int i = 0; i < numSamples; ++i) {
        float t = i * dt;
        float cost = std::cos(t);
        float sint = std::sin(t);
        res.emplaceLast(-cost, -sint);
    }
    res.append(res.first());
    return res;
}

const geometry::Vec2fArray& unitCircle_() {
    static geometry::Vec2fArray unitCircle = computeUnitCircle_(numCircleSamples);
    return unitCircle;
}

// clang-format off

void insertSmoothRect(
    core::FloatArray& a,
    const core::Color& cTopLeft, const core::Color& cTopRight,
    const core::Color& cBottomLeft, const core::Color& cBottomRight,
    float x1, float y1, float x2, float y2) {

    float r1 = cTopLeft[0];
    float g1 = cTopLeft[1];
    float b1 = cTopLeft[2];
    float r2 = cTopRight[0];
    float g2 = cTopRight[1];
    float b2 = cTopRight[2];
    float r3 = cBottomLeft[0];
    float g3 = cBottomLeft[1];
    float b3 = cBottomLeft[2];
    float r4 = cBottomRight[0];
    float g4 = cBottomRight[1];
    float b4 = cBottomRight[2];
    a.extend({
        x1, y1, r1, g1, b1,
        x2, y1, r2, g2, b2,
        x1, y2, r3, g3, b3,
        x2, y1, r2, g2, b2,
        x2, y2, r4, g4, b4,
        x1, y2, r3, g3, b3});
}

void insertQuad_(
    core::FloatArray& a,
    const geometry::Vec2f& p1,
    const geometry::Vec2f& q1,
    const geometry::Vec2f& p2,
    const geometry::Vec2f& q2,
    const core::Color& c) {

    float p1x = p1.x();
    float p1y = p1.y();
    float q1x = q1.x();
    float q1y = q1.y();

    float p2x = p2.x();
    float p2y = p2.y();
    float q2x = q2.x();
    float q2y = q2.y();

    float r = c.r();
    float g = c.g();
    float b = c.b();

    // clang-format off
    a.extend({
              p1x, p1y, r, g, b,
              q1x, q1y, r, g, b,
              p2x, p2y, r, g, b,
              p2x, p2y, r, g, b,
              q1x, q1y, r, g, b,
              q2x, q2y, r, g, b});
    // clang-format on
}

// Insert a quad-shaped cursor for the SL selector.
//
void insertSLCursorQuad_(
    core::FloatArray& a,
    const core::Color& cellColor,
    float x1,
    float y1,
    float x2,
    float y2) {

    core::Color color(cellColor);

    geometry::Rect2f rect1(x1, y1, x2, y2);
    geometry::Rect2f rect2 = rect1 + Margins(1);
    geometry::Rect2f rect3 = rect2 + Margins(1);

    // outer quad
    geometry::Vec2f p1 = rect3.corner(0);
    geometry::Vec2f p2 = rect3.corner(1);
    geometry::Vec2f p3 = rect3.corner(2);
    geometry::Vec2f p4 = rect3.corner(3);

    // mid quad
    geometry::Vec2f q1 = rect2.corner(0);
    geometry::Vec2f q2 = rect2.corner(1);
    geometry::Vec2f q3 = rect2.corner(2);
    geometry::Vec2f q4 = rect2.corner(3);

    // inner quad
    geometry::Vec2f r1 = rect1.corner(0);
    geometry::Vec2f r2 = rect1.corner(1);
    geometry::Vec2f r3 = rect1.corner(2);
    geometry::Vec2f r4 = rect1.corner(3);

    // outer quad strip
    insertQuad_(a, p1, q1, p2, q2, cursorOuterColor);
    insertQuad_(a, p2, q2, p3, q3, cursorOuterColor);
    insertQuad_(a, p3, q3, p4, q4, cursorOuterColor);
    insertQuad_(a, p4, q4, p1, q1, cursorOuterColor);

    // inner quad strip
    insertQuad_(a, q1, r1, q2, r2, cursorInnerColor);
    insertQuad_(a, q2, r2, q3, r3, cursorInnerColor);
    insertQuad_(a, q3, r3, q4, r4, cursorInnerColor);
    insertQuad_(a, q4, r4, q1, r1, cursorInnerColor);

    // fill
    insertQuad_(a, r1, r2, r4, r3, color);
}

void insertCircle_(
    core::FloatArray& a,
    const core::Color& color,
    const geometry::Vec2f& center,
    float radius) {

    const geometry::Vec2fArray& unitCircle = unitCircle_();
    geometry::Vec2f p0 = center + radius * unitCircle[0];
    geometry::Vec2f p1 = center + radius * unitCircle[1];
    for (Int i = 2; i < numCircleSamples - 2; ++i) {
        geometry::Vec2f p2 = center + radius * unitCircle[i];
        detail::insertTriangle(a, color, p0, p1, p2);
        p1 = p2;
    }
}

void insertCircleBorder_(
    core::FloatArray& a,
    const core::Color& color,
    const geometry::Vec2f& center,
    float radius,
    float borderWidth) {

    const geometry::Vec2fArray& unitCircle = unitCircle_();
    for (Int i = 0; i < numCircleSamples; ++i) {
        geometry::Vec2f v1 = unitCircle[i];
        geometry::Vec2f v2 = unitCircle[i + 1];
        geometry::Vec2f p1 = center + radius * v1;
        geometry::Vec2f q1 = center + (radius + borderWidth) * v1;
        geometry::Vec2f p2 = center + radius * v2;
        geometry::Vec2f q2 = center + (radius + borderWidth) * v2;
        insertQuad_(a, p1, q1, p2, q2, color);
    }
}

// Insert a circle-shaped cursor for the SL selector.
//
void insertSLCursorCircle_(
    core::FloatArray& a,
    const core::Color& fillColor,
    const geometry::Vec2f& center,
    float radius) {

    insertCircleBorder_(a, cursorOuterColor, center, radius + 1, 1);
    insertCircleBorder_(a, cursorInnerColor, center, radius, 1);
    insertCircle_(a, fillColor, center, radius);
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

void ColorPaletteSelector::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    float eps = 1e-6f;
    bool hasWidthChanged = std::abs(oldWidth_ - width()) > eps;
    bool hasHeightChanged = std::abs(oldHeight_ - height()) > eps;
    if (reload_ || hasWidthChanged || hasHeightChanged) {

        reload_ = false;
        oldWidth_ = width();
        oldHeight_ = height();
        core::FloatArray a = {};

        core::Color borderColor = detail::getColor(this, style::strings::border_color);
        updateMetrics_(); // TODO: only update if we know that they have changed
        const Metrics& m = metrics_;

        // Get misc color info
        Int lSteps = numLightnessSteps_;
        Int sSteps = numSaturationSteps_;
        Int hSteps = numHueSteps_;
        if (isContinuous_) {
            lSteps = 16;
            sSteps = 16;
            hSteps = 96;
        }
        float dhue = 360.0f / hSteps;
        float hue = isContinuous_ ? selectedHue_ : selectedHueIndex_ * dhue;

        // Radius of Saturation-Lightness cursor in continuous mode
        using namespace style::literals;
        const style::Length slCursorRadius_ = 5.0_dp;
        float slCursorRadius = slCursorRadius_.toPx(styleMetrics());

        // draw saturation/lightness selector
        if (m.borderWidth > 0) {
            detail::insertRect(a, borderColor, m.saturationLightnessRect);
        }
        float x0 = m.saturationLightnessRect.xMin();
        float y0 = m.saturationLightnessRect.yMin();
        float dl = 1.0f / (isContinuous_ ? lSteps : lSteps - 1);
        float ds = 1.0f / (isContinuous_ ? sSteps : sSteps - 1);
        float slDx = m.slDx;
        float slDy = m.slDy;
        if (isContinuous_) {
            slDx = (m.saturationLightnessRect.width() - 2 * m.borderWidth) / lSteps;
            slDy = (m.saturationLightnessRect.height() - 2 * m.borderWidth) / sSteps;
        }
        for (Int i = 0; i < lSteps; ++i) {
            float x1 = x0 + m.borderWidth + i * slDx;
            float x2 = x0 + m.borderWidth + (i + 1) * slDx;
            float l = i * dl;
            if (!isContinuous_) {
                x1 = hint(x1, m.hinting);
                x2 = hint(x2, m.hinting);
                x2 -= m.borderWidth;
            }
            for (Int j = 0; j < sSteps; ++j) {
                float y1 = y0 + m.borderWidth + j * slDy;
                float y2 = y1 + slDy;
                float s = j * ds;
                if (isContinuous_) {
                    auto c1 = core::Color::hsl(hue, s, l);
                    auto c2 = core::Color::hsl(hue, s, l + dl);
                    auto c3 = core::Color::hsl(hue, s + ds, l);
                    auto c4 = core::Color::hsl(hue, s + ds, l + dl);
                    insertSmoothRect(a, c1, c2, c3, c4, x1, y1, x2, y2);
                }
                else {
                    y2 -= m.borderWidth;
                    auto c = core::Color::hsl(hue, s, l).round8b();
                    detail::insertRect(a, c, x1, y1, x2, y2);
                }
            }
        }
        // Draw highlighted color
        if (isContinuous_) {
            if (hoveredLightness_ != -1) {
                float hmargins = m.borderWidth;
                float vmargins = m.borderWidth;
                geometry::Rect2f rect =
                    m.saturationLightnessRect - Margins(vmargins, hmargins);

                geometry::Vec2f center(
                    rect.xMin() + hoveredLightness_ * rect.width(),
                    rect.yMin() + hoveredSaturation_ * rect.height());
                float circleBorderWidth = 1;

                core::Color hoveredColor =
                    core::Color::hsl(selectedHue_, hoveredSaturation_, hoveredLightness_);
                core::Color highlightColor = computeHighlightColor(hoveredColor);
                insertCircleBorder_(
                    a, highlightColor, center, slCursorRadius, circleBorderWidth);
            }
        }
        else {
            if (hoveredLightnessIndex_ != -1) {
                Int i = hoveredLightnessIndex_;
                Int j = hoveredSaturationIndex_;
                float x1 = hint(x0 + m.borderWidth + i * m.slDx, m.hinting);
                float x2 = hint(x0 + m.borderWidth + (i + 1) * m.slDx, m.hinting)
                           - m.borderWidth;
                float y1 = y0 + m.borderWidth + j * m.slDy;
                float y2 = y1 + m.slDy - m.borderWidth;
                float l = i * dl;
                float s = j * ds;
                core::Color hoveredColor = core::Color::hsl(hue, s, l).round8b();
                core::Color highlightColor = computeHighlightColor(hoveredColor);

                geometry::Rect2f rect(x1, y1, x2, y2);
                style::BorderRadiuses radius(style::BorderRadius(0));
                float borderWidth = 1;

                detail::insertRect(
                    a,
                    styleMetrics(),
                    hoveredColor,
                    highlightColor,
                    rect,
                    radius,
                    borderWidth);
            }
        }

        // Draw selected color
        if (isContinuous_ || !isSelectedColorExact_) {
            float hmargins = m.borderWidth;
            float vmargins = m.borderWidth;
            if (!isContinuous_) {
                // remove more margins so that the continuous color
                // corresponding to a quantized-selectable color is centered in
                // the corresponding cell.
                hmargins += 0.5f * slDx;
                vmargins += 0.5f * slDy;
            }
            geometry::Rect2f rect =
                m.saturationLightnessRect - Margins(vmargins, hmargins);

            geometry::Vec2f center(
                rect.xMin() + selectedLightness_ * rect.width(),
                rect.yMin() + selectedSaturation_ * rect.height());

            insertSLCursorCircle_(a, selectedColor_, center, slCursorRadius);
        }
        else {
            Int i = selectedLightnessIndex_;
            Int j = selectedSaturationIndex_;
            float x1 = hint(x0 + m.borderWidth + i * m.slDx, m.hinting);
            float x2 =
                hint(x0 + m.borderWidth + (i + 1) * m.slDx, m.hinting) - m.borderWidth;
            float y1 = y0 + m.borderWidth + j * m.slDy;
            float y2 = y1 + m.slDy - m.borderWidth;
            float l = i * dl;
            float s = j * ds;
            auto c = core::Color::hsl(hue, s, l).round8b();
            insertSLCursorQuad_(a, c, x1, y1, x2, y2);
        }

        // Draw hue selector
        drawHueSelector_(a);

        // Load triangles
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_);
}

namespace {

// repeats first and last
//
geometry::Vec2fArray computeHuePolygon_(
    const geometry::Vec2f& p,
    const geometry::Vec2f& q,
    float r,
    Int numHorizontalSamples) {

    const geometry::Vec2fArray& unitCircle = unitCircle_();
    float dx = (q.x() - p.x()) / numHorizontalSamples;

    geometry::Vec2fArray res;
    res.reserve(numCircleSamples + 2);
    for (Int i = circleLeftIndexBegin; i <= circleTopIndex; ++i) {
        res.append(p + r * unitCircle[i]);
    }
    for (Int i = 1; i < numHorizontalSamples; ++i) {
        res.append(p + geometry::Vec2f(i * dx, -r));
    }
    for (Int i = circleTopIndex; i <= circleBottomIndex; ++i) {
        res.append(q + r * unitCircle[i]);
    }
    for (Int i = 1; i < numHorizontalSamples; ++i) {
        res.append(q - geometry::Vec2f(i * dx, -r));
    }
    for (Int i = circleBottomIndex; i <= circleLeftIndexEnd; ++i) {
        res.append(p + r * unitCircle[i]);
    }
    return res;
}

using HueVec = std::array<geometry::Vec2f, 2>;

core::Array<HueVec> computeHueVecs_(
    const geometry::Vec2f& p,
    const geometry::Vec2f& q,
    Int numHorizontalSamples) {

    const geometry::Vec2fArray& unitCircle = unitCircle_();
    float dx = (q.x() - p.x()) / numHorizontalSamples;

    core::Array<HueVec> res;
    res.reserve(numCircleSamples + 2);
    for (Int i = circleLeftIndexBegin; i <= circleTopIndex; ++i) {
        res.append({p, unitCircle[i]});
    }
    for (Int i = 1; i < numHorizontalSamples; ++i) {
        res.append({p + geometry::Vec2f(i * dx, 0), geometry::Vec2f(0, -1)});
    }
    for (Int i = circleTopIndex; i <= circleBottomIndex; ++i) {
        res.append({q, unitCircle[i]});
    }
    for (Int i = 1; i < numHorizontalSamples; ++i) {
        res.append({q - geometry::Vec2f(i * dx, 0), geometry::Vec2f(0, 1)});
    }
    for (Int i = circleBottomIndex; i <= circleLeftIndexEnd; ++i) {
        res.append({p, unitCircle[i]});
    }
    return res;
}

// Compute the hue corresponding to each sample.
// This is based on the arclength of the capsule at distance r.
// So if you want the hue to "stetch more" in the half-disk parts of the
// hue selector, you can use a lower value of r.
core::FloatArray computeHues_(
    const geometry::Vec2f& p,
    const geometry::Vec2f& q,
    float r,
    Int numHorizontalSamples) {

    geometry::Vec2fArray mid = computeHuePolygon_(p, q, r, numHorizontalSamples);
    Int numSamples = mid.length();
    core::FloatArray res;
    float s = 0;
    res.reserve(numSamples);
    res.append(s);
    for (Int i = 0; i < numSamples - 1; ++i) {
        float ds = (mid[i + 1] - mid[i]).length();
        s += ds;
        res.append(s);
    }
    float multiplier = 360 / s;
    for (Int i = 1; i < numSamples; ++i) {
        res[i] *= multiplier;
    }
    return res;
}

void insertHueQuad_(
    core::FloatArray& a,
    const geometry::Vec2f& p1,
    const geometry::Vec2f& q1,
    const core::Color& c1,
    const geometry::Vec2f& p2,
    const geometry::Vec2f& q2,
    const core::Color& c2) {

    float p1x = p1.x();
    float p1y = p1.y();
    float q1x = q1.x();
    float q1y = q1.y();
    float r1 = c1.r();
    float g1 = c1.g();
    float b1 = c1.b();

    float p2x = p2.x();
    float p2y = p2.y();
    float q2x = q2.x();
    float q2y = q2.y();
    float r2 = c2.r();
    float g2 = c2.g();
    float b2 = c2.b();

    // clang-format off
    a.extend({
        p1x, p1y, r1, g1, b1,
        q1x, q1y, r1, g1, b1,
        p2x, p2y, r2, g2, b2,
        p2x, p2y, r2, g2, b2,
        q1x, q1y, r1, g1, b1,
        q2x, q2y, r2, g2, b2});
    // clang-format on
}

void insertHueQuadStrip_(
    core::FloatArray& a,
    const geometry::Vec2fArray& inner,
    const geometry::Vec2fArray& outer,
    const core::FloatArray& hues,
    float saturation,
    float lightness) {

    Int numSamples = outer.length() - 1;
    for (Int i = 0; i < numSamples; ++i) {
        const geometry::Vec2f& p1 = inner[i];
        const geometry::Vec2f& q1 = outer[i];
        core::Color c1 = core::Color::hsl(hues[i], saturation, lightness);
        const geometry::Vec2f& p2 = inner[i + 1];
        const geometry::Vec2f& q2 = outer[i + 1];
        core::Color c2 = core::Color::hsl(hues[i + 1], saturation, lightness);
        insertHueQuad_(a, p1, q1, c1, p2, q2, c2);
    }
}

// Same as hueBasis_, but without the u vector.
//
HueVec
hueVec_(const core::FloatArray& hues, const core::Array<HueVec>& hueVecs, float hue) {
    VGC_ASSERT(hues.length() == hueVecs.length());
    VGC_ASSERT(hues.length() > 0);
    if (hue < 0) {
        hue += 360;
    }
    else if (hue > 360) {
        hue -= 360;
    }
    auto it = std::lower_bound(hues.begin(), hues.end(), hue);
    Int i = std::distance(hues.begin(), it);
    if (i <= 0 || i >= hues.length()) {
        auto [c, v] = hueVecs[0];
        return {c, v};
    }
    else {
        // Interpolate
        float h1 = hues[i - 1];
        float h2 = hues[i];
        float t = (hue - h1) / (h2 - h1);
        auto [c1, v1] = hueVecs[i - 1];
        auto [c2, v2] = hueVecs[i];
        geometry::Vec2f c = c1 + t * (c2 - c1);
        geometry::Vec2f v = v1 + t * (v2 - v1);
        v.normalize();
        return {c, v};
    }
}

using HueBasis = std::array<geometry::Vec2f, 3>;

// Returns a triplet [c, u, v] representing where to draw a hue cursor
// for the given hue:
//
//            ^
//          v |
//            |
//            o----->
//          c    u
//
// The point c is in the horizontal centerline at the middle of the
// hue selector rectangle.
//
HueBasis
hueBasis_(const core::FloatArray& hues, const core::Array<HueVec>& hueVecs, float hue) {
    auto [c, v] = hueVec_(hues, hueVecs, hue);
    geometry::Vec2f u = v.orthogonalized();
    return {c, u, v};
}

void hintHueQuad_(
    geometry::Vec2f& q1,
    geometry::Vec2f& q2,
    geometry::Vec2f& q3,
    geometry::Vec2f& q4,
    bool hinting) {

    if (hinting) {
        bool isQ1Q2Vertical = q1.x() == q2.x();
        bool isQ3Q4Vertical = q3.x() == q4.x();
        if (isQ1Q2Vertical) {
            float x = std::round(q1.x());
            q1.setX(x);
            q2.setX(x);
        }
        if (isQ3Q4Vertical) {
            float x = std::round(q3.x());
            q3.setX(x);
            q4.setX(x);
        }
    }
}

void insertHueQuad_(
    core::FloatArray& a,
    const core::FloatArray& hues,
    const core::Array<HueVec>& hueVecs,
    float hue,
    const core::Color& color,
    float leftOffset,
    float rightOffset,
    float startHeight,
    float endHeight,
    bool hinting) {

    HueBasis hb = hueBasis_(hues, hueVecs, hue);
    geometry::Vec2f p1 = hb[0] + hb[1] * leftOffset + hb[2] * startHeight;
    geometry::Vec2f p2 = hb[0] + hb[1] * leftOffset + hb[2] * endHeight;
    geometry::Vec2f p3 = hb[0] + hb[1] * rightOffset + hb[2] * endHeight;
    geometry::Vec2f p4 = hb[0] + hb[1] * rightOffset + hb[2] * startHeight;
    hintHueQuad_(p1, p2, p3, p4, hinting);
    insertQuad_(a, p1, p2, p4, p3, color);
}

void insertHuePieSection_(
    core::FloatArray& a,
    const HueVec& hv1,
    const HueVec& hv2,
    const core::Color& color,
    float startHeight,
    float endHeight,
    bool hinting) {

    geometry::Vec2f q1 = hv1[0] + startHeight * hv1[1];
    geometry::Vec2f q2 = hv1[0] + endHeight * hv1[1];
    geometry::Vec2f q3 = hv2[0] + startHeight * hv2[1];
    geometry::Vec2f q4 = hv2[0] + endHeight * hv2[1];
    hintHueQuad_(q1, q2, q3, q4, hinting);
    insertQuad_(a, q1, q2, q3, q4, color);
}

void insertHuePie_(
    core::FloatArray& a,
    const core::FloatArray& hues,
    const core::Array<HueVec>& hueVecs,
    float hue1,
    float hue2,
    const core::Color& color,
    float startHeight,
    float endHeight,
    bool hinting) {

    // Handle red step, which crosses the "0" border
    bool wrappedHue1 = false;
    bool wrappedHue2 = false;
    if (hue1 < 0) {
        hue1 += 360;
        wrappedHue1 = true;
    }
    if (hue2 > 360) {
        hue2 -= 360;
        wrappedHue2 = true;
    }

    auto it1 = std::lower_bound(hues.begin(), hues.end(), hue1);
    auto it2 = std::lower_bound(hues.begin(), hues.end(), hue2);
    Int i1 = std::distance(hues.begin(), it1);
    Int i2 = std::distance(hues.begin(), it2);

    // Ensures 0 <= i1 <= i2
    Int n = hues.length() - 1;
    if (wrappedHue1 || wrappedHue2) {
        i2 += n;
    }

    HueVec hvFirst = hueVec_(hues, hueVecs, hue1);
    HueVec hvLast = hueVec_(hues, hueVecs, hue2);
    if (i1 >= i2) {
        insertHuePieSection_(a, hvFirst, hvLast, color, startHeight, endHeight, hinting);
    }
    else {
        HueVec hv1 = hvFirst;
        for (Int i = i1; i < i2; ++i) {
            Int j = i % n;
            const HueVec& hv2 = hueVecs[j];
            insertHuePieSection_(a, hv1, hv2, color, startHeight, endHeight, hinting);
            hv1 = hv2;
        }
        insertHuePieSection_(a, hv1, hvLast, color, startHeight, endHeight, hinting);
    }
}

struct HueCursorStyle {

    core::Color innerColor;
    core::Color outerColor;

    // vertical offset distances
    float vd1 = 0;
    float vd2 = 1;
    float vd3 = 2;

    // horizontal offset distances
    float hd1 = -2;
    float hd2 = -1;
    float hd3 = 0;

    bool hinting;
};

// Insert a pie-shaped cursor for the hue selector.
//
void insertCursorPie_(
    core::FloatArray& a,
    const core::FloatArray& hues,
    const core::Array<HueVec>& hueVecs,
    const core::Color& fillColor,
    float hue1,
    float hue2,
    float startHeight,
    float endHeight,
    const HueCursorStyle& style) {

    // shorter names to make function calls fit in one line
    const float h1 = startHeight;
    const float h2 = endHeight;
    const core::Color& fc = fillColor;
    const core::Color& oc = style.outerColor;
    const core::Color& ic = style.innerColor;
    const float vd1 = style.vd1;
    const float vd2 = style.vd2;
    const float vd3 = style.vd3;
    const float hd1 = style.hd1;
    const float hd2 = style.hd2;
    const float hd3 = style.hd3;
    const bool hinting = style.hinting;

    // Draw fill color
    insertHuePie_(a, hues, hueVecs, hue1, hue2, fc, h1 - vd1, h2 + vd1, hinting);

    // Draw horizontal (or arc-shaped) bars
    insertHuePie_(a, hues, hueVecs, hue1, hue2, oc, h2 + vd2, h2 + vd3, hinting);
    insertHuePie_(a, hues, hueVecs, hue1, hue2, ic, h2 + vd1, h2 + vd2, hinting);
    insertHuePie_(a, hues, hueVecs, hue1, hue2, ic, h1 - vd2, h1 - vd1, hinting);
    insertHuePie_(a, hues, hueVecs, hue1, hue2, oc, h1 - vd3, h1 - vd2, hinting);

    // Draw vertical bars
    insertHueQuad_(a, hues, hueVecs, hue1, oc, -hd3, -hd2, h1 - vd3, h2 + vd3, hinting);
    insertHueQuad_(a, hues, hueVecs, hue1, ic, -hd2, -hd1, h1 - vd2, h2 + vd2, hinting);
    insertHueQuad_(a, hues, hueVecs, hue2, ic, +hd1, +hd2, h1 - vd2, h2 + vd2, hinting);
    insertHueQuad_(a, hues, hueVecs, hue2, oc, +hd2, +hd3, h1 - vd3, h2 + vd3, hinting);

    // TODO: draw fill color
}

// Insert a pie-shaped border (border width = 1).
//
void insertCursorPieBorder_(
    core::FloatArray& a,
    const core::FloatArray& hues,
    const core::Array<HueVec>& hueVecs,
    const core::Color& color,
    float hue1,
    float hue2,
    float startHeight,
    float endHeight,
    bool hinting) {

    // shorter names to make function calls fit in one line
    const core::Color& c = color;

    // Draw horizontal (or arc-shaped) bars
    insertHuePie_(a, hues, hueVecs, hue1, hue2, c, startHeight, startHeight + 1, hinting);
    insertHuePie_(a, hues, hueVecs, hue1, hue2, c, endHeight - 1, endHeight, hinting);

    // Draw vertical bars
    insertHueQuad_(a, hues, hueVecs, hue1, c, 0, 1, startHeight, endHeight, hinting);
    insertHueQuad_(a, hues, hueVecs, hue2, c, -1, 0, startHeight, endHeight, hinting);
}

float hueFromMousePosition_(
    const geometry::Vec2f& pos,
    const geometry::Vec2f& p,
    const geometry::Vec2f& q,
    const core::FloatArray& hues) {

    constexpr float pi_ = static_cast<float>(core::pi);
    constexpr float twoOverPi = 2.0f / pi_;

    const Int numHSamples = (hues.length() - 1 - numCircleSamples) / 2;
    const Int leftBegin = 0;
    const Int topLeft = leftBegin + numQuarterCircleSamples;
    const Int topRight = topLeft + numHSamples;
    const Int right = topRight + numQuarterCircleSamples;
    const Int bottomRight = right + numQuarterCircleSamples;
    const Int bottomLeft = bottomRight + numHSamples;
    const Int leftEnd = bottomLeft + numQuarterCircleSamples;

    float hue1 = 0;
    float hue2 = 0;
    float t = 0;
    if (pos.y() < p.y()) {
        if (pos.x() < p.x()) {
            // top-left quarter circle
            hue1 = hues[leftBegin];
            hue2 = hues[topLeft];
            geometry::Vec2f v = pos - p;
            t = std::atan2(v.y(), v.x()); // values in [-pi, -pi/2]
            t = twoOverPi * t + 2;        // values in [0, 1]
        }
        else if (pos.x() <= q.x()) {
            // top horizontal line
            hue1 = hues[topLeft];
            hue2 = hues[topRight];
            t = (pos.x() - p.x()) / (q.x() - p.x());
        }
        else {
            // top-right quarter circle
            hue1 = hues[topRight];
            hue2 = hues[right];
            geometry::Vec2f v = pos - q;
            t = std::atan2(v.y(), v.x()); // values in [-pi/2, 0]
            t = twoOverPi * t + 1;        // values in [0, 1]
        }
    }
    else {
        if (pos.x() > q.x()) {
            // bottom-right quarter circle
            hue1 = hues[right];
            hue2 = hues[bottomRight];
            geometry::Vec2f v = pos - q;
            t = std::atan2(v.y(), v.x()); // values in [0, pi/2]
            t = twoOverPi * t;
        }
        else if (pos.x() >= p.x()) {
            // bottom horizontal line
            hue1 = hues[bottomRight];
            hue2 = hues[bottomLeft];
            t = (pos.x() - q.x()) / (p.x() - q.x());
        }
        else {
            // bottom-left quarter circle
            hue1 = hues[bottomLeft];
            hue2 = hues[leftEnd];
            geometry::Vec2f v = pos - p;
            t = std::atan2(v.y(), v.x()); // values in [pi/2, pi]
            t = twoOverPi * t - 1;        // values in [0, 1]
        }
    }
    float hue = hue1 + t * (hue2 - hue1);
    return hue;
}

std::array<geometry::Vec2f, 2> getHueCapsuleCenters_(const geometry::Rect2f& rect) {
    float height = rect.height();
    float right = rect.xMax();
    float left = rect.xMin();
    float top = rect.yMin();
    float r = 0.5f * height;
    geometry::Vec2f p(left + r, top + r);
    geometry::Vec2f q(right - r, top + r);
    return {p, q};
}

// Number of samples in the top and bottom of the "rectangle" part of the hue
// capsule. The given `rect` is the whole hue selector rectangle.
//
Int getNumHSamples_(const geometry::Rect2f&) {
    // For now it's constant, but it may make sense to have
    // more or less samples based on the size of the rectangle.
    return 32;
}

} // namespace

void ColorPaletteSelector::drawHueSelector_(core::FloatArray& a) {

    // The hue selector is an "capsule" made of two half-disks and one rectangle.
    // The diameter of each half-disk is the same as the height of the rectangle.
    //
    //        .──┬───────────────────────┬──.      ^
    //       '   │                       │   `.    │
    //      │    ┼ p                   q ┼    │    │ height
    //      `    │                       │   .'    │
    //       ` ──┴───────────────────────┴──`      v
    //       ^              ^                ^
    //   half-disk      rectangle        half-disk
    //

    const Metrics& m = metrics_;
    const geometry::Rect2f& rect = m.hueRect;
    float height = rect.height();
    float r = 0.5f * height;

    auto [p, q] = getHueCapsuleCenters_(rect);

    const Int numHSamples = getNumHSamples_(rect);

    using namespace style::literals;
    const style::Length borderWidth_ = 1.0_dp;
    const style::Length outerWidth_ = 3.0_dp;
    const style::Length holeRadius_ = 4.0_dp;

    float borderWidth = borderWidth_.toPx(styleMetrics());
    float outerWidth = outerWidth_.toPx(styleMetrics());
    float holeRadius = holeRadius_.toPx(styleMetrics());

    float r1 = holeRadius;
    float r2 = holeRadius + borderWidth;
    float r3 = r - outerWidth;
    float r4 = r - borderWidth;
    float r5 = r;

    geometry::Vec2fArray s1 = computeHuePolygon_(p, q, r1, numHSamples);
    geometry::Vec2fArray s2 = computeHuePolygon_(p, q, r2, numHSamples);
    geometry::Vec2fArray s3 = computeHuePolygon_(p, q, r3, numHSamples);
    geometry::Vec2fArray s4 = computeHuePolygon_(p, q, r4, numHSamples);
    geometry::Vec2fArray s5 = computeHuePolygon_(p, q, r5, numHSamples);

    // Precomputation of hues and hueVecs
    hues_ = computeHues_(p, q, 0.7f * r, numHSamples);
    core::Array<HueVec> hueVecs = computeHueVecs_(p, q, numHSamples);

    // Draw hue selector
    if (isContinuous_) {
        insertHueQuadStrip_(a, s1, s3, hues_, selectedSaturation_, selectedLightness_);
        insertHueQuadStrip_(a, s3, s5, hues_, 1.0f, 0.5f);
    }
    else {
        float dhue = 360.0f / numHueSteps_;
        for (Int i = 0; i < numHueSteps_; ++i) {
            float hue = i * dhue;
            float hue1 = hue - 0.5f * dhue;
            float hue2 = hue1 + dhue;

            core::Color color1 = colorFromHslIndices(
                numHueSteps_,
                numSaturationSteps_,
                numLightnessSteps_,
                i,
                selectedSaturationIndex_,
                selectedLightnessIndex_);

            core::Color::hsl(hue, selectedSaturation_, selectedLightness_);
            core::Color color2 = core::Color::hsl(hue, 1.0f, 0.5f);
            insertHuePie_(a, hues_, hueVecs, hue1, hue2, color1, r1, r3, m.hinting);
            insertHuePie_(a, hues_, hueVecs, hue1, hue2, color2, r3, r5, m.hinting);
        }
    }

    HueCursorStyle continuousStyle;
    continuousStyle.innerColor = cursorInnerColor;
    continuousStyle.outerColor = cursorOuterColor;
    continuousStyle.vd1 = 0;
    continuousStyle.vd2 = 1;
    continuousStyle.vd3 = 2;
    continuousStyle.hd1 = -1;
    continuousStyle.hd2 = 0;
    continuousStyle.hd3 = 1;
    continuousStyle.hinting = m.hinting;

    HueCursorStyle stepsStyle;
    stepsStyle.innerColor = cursorInnerColor;
    stepsStyle.outerColor = cursorOuterColor;
    stepsStyle.vd1 = 0;
    stepsStyle.vd2 = 1;
    stepsStyle.vd3 = 2;
    stepsStyle.hd1 = -2;
    stepsStyle.hd2 = -1;
    stepsStyle.hd3 = 0;
    stepsStyle.hinting = m.hinting;

    HueCursorStyle stepsStyleHighlight;
    stepsStyleHighlight.innerColor = cursorInnerColor;
    stepsStyleHighlight.outerColor = cursorOuterColor;
    stepsStyleHighlight.vd1 = 0;
    stepsStyleHighlight.vd2 = 1;
    stepsStyleHighlight.vd3 = 2;
    stepsStyleHighlight.hd1 = -2;
    stepsStyleHighlight.hd2 = -1;
    stepsStyleHighlight.hd3 = 0;
    stepsStyleHighlight.hinting = m.hinting;

    float cursorStart = r1;
    float cursorEnd = r3;

    // Draw highlighted color cursor
    if (isContinuous_) {
        if (hoveredHue_ != -1) {
            float dhue = 10;
            float hue = hoveredHue_;
            float hue1 = hue - 0.5f * dhue;
            float hue2 = hue1 + dhue;

            core::Color color =
                core::Color::hsl(hue, selectedSaturation_, selectedLightness_);
            core::Color highlightColor = computeHighlightColor(color);
            insertCursorPieBorder_(
                a,
                hues_,
                hueVecs,
                highlightColor,
                hue1,
                hue2,
                cursorStart,
                cursorEnd,
                m.hinting);
        }
    }
    else {
        if (hoveredHueIndex_ != -1) {
            float dhue = 360.0f / numHueSteps_;
            float hue = hoveredHueIndex_ * dhue;
            float hue1 = hue - 0.5f * dhue;
            float hue2 = hue1 + dhue;

            core::Color color = colorFromHslIndices(
                numHueSteps_,
                numSaturationSteps_,
                numLightnessSteps_,
                hoveredHueIndex_,
                selectedSaturationIndex_,
                selectedLightnessIndex_);

            core::Color highlightColor = computeHighlightColor(color);
            insertCursorPieBorder_(
                a,
                hues_,
                hueVecs,
                highlightColor,
                hue1,
                hue2,
                cursorStart,
                cursorEnd,
                m.hinting);
        }
    }

    // Draw selected color cursor
    if (isContinuous_ || !isSelectedColorExact_) {
        float dhue = 10;
        float hue = selectedHue_;
        float hue1 = hue - 0.5f * dhue;
        float hue2 = hue1 + dhue;
        insertCursorPie_(
            a,
            hues_,
            hueVecs,
            selectedColor_,
            hue1,
            hue2,
            cursorStart,
            cursorEnd,
            continuousStyle);
    }
    else {
        float dhue = 360.0f / numHueSteps_;
        float hue = selectedHueIndex_ * dhue;
        float hue1 = hue - 0.5f * dhue;
        float hue2 = hue1 + dhue;

        core::Color color = colorFromHslIndices(
            numHueSteps_,
            numSaturationSteps_,
            numLightnessSteps_,
            selectedHueIndex_,
            selectedSaturationIndex_,
            selectedLightnessIndex_);

        insertCursorPie_(
            a, hues_, hueVecs, color, hue1, hue2, cursorStart, cursorEnd, stepsStyle);
    }
}

void ColorPaletteSelector::computeSlSubMetrics_(float width, Metrics& m) const {

    using namespace style::literals;
    const style::Length minCellWidth_ = 1.0_dp;
    const style::Length maxCellWidth_(core::FloatInfinity, style::LengthUnit::Dp);
    const style::Length minCellHeight_ = 20.0_dp; // can be overidden to fit maxHeight
    const style::Length maxCellHeight_ = 30.0_dp;
    const style::Length maxHeight_ = 300.0_dp;

    const float minCellWidth = minCellWidth_.toPx(styleMetrics());
    const float maxCellWidth = maxCellWidth_.toPx(styleMetrics());
    const float minCellHeight = minCellHeight_.toPx(styleMetrics());
    const float maxCellHeight = maxCellHeight_.toPx(styleMetrics());
    const float maxHeight = maxHeight_.toPx(styleMetrics());

    float maxSlDy =
        (std::min)((maxHeight - m.borderWidth) / numSaturationSteps_, maxCellHeight);
    if (maxSlDy >= 2) {
        maxSlDy = hint(maxSlDy, m.hinting);
    }
    float minSlDy = (std::min)(minCellHeight, maxSlDy);

    float x0 = m.paddingLeft;
    float y0 = m.paddingTop;
    float w = width - (m.paddingLeft + m.paddingRight);
    m.slDx =
        core::clamp((w - m.borderWidth) / numLightnessSteps_, minCellWidth, maxCellWidth);
    m.slDy = core::clamp(m.slDx, minSlDy, maxSlDy);
    if (m.slDy >= 2) { // don't pre-hint if too small
        m.slDy = hint(m.slDy, m.hinting);
    }
    w = hint(m.borderWidth + numLightnessSteps_ * m.slDx, m.hinting);
    float h = hint(m.borderWidth + m.slDy * numSaturationSteps_, m.hinting);
    m.saturationLightnessRect = {x0, y0, x0 + w, y0 + h};
}

void ColorPaletteSelector::computeHueSubMetrics_(float /* width */, Metrics& m) const {

    using namespace style::literals;
    const style::Length minCellWidth_ = 0.0_dp;
    const style::Length maxCellWidth_(core::FloatInfinity, style::LengthUnit::Dp);
    const style::Length minCellHeight_ = 20.0_dp; // can be overidden to fit maxHeight
    const style::Length maxCellHeight_ = 30.0_dp;

    const float minCellWidth = minCellWidth_.toPx(styleMetrics());
    const float maxCellWidth = maxCellWidth_.toPx(styleMetrics());
    const float minCellHeight = minCellHeight_.toPx(styleMetrics());
    const float maxCellHeight = maxCellHeight_.toPx(styleMetrics());

    Int halfNumHueSteps = numHueSteps_ / 2;
    float xMin = m.saturationLightnessRect.xMin();
    float xMax = m.saturationLightnessRect.xMax();
    float y0 = m.saturationLightnessRect.yMax() + m.rowGap;
    float w = xMax - xMin;
    m.hueDx =
        core::clamp((w - m.borderWidth) / halfNumHueSteps, minCellWidth, maxCellWidth);
    m.hueDy = core::clamp(m.hueDx, minCellHeight, maxCellHeight);
    m.hueDy = hint(m.hueDy, m.hinting);
    float h = m.borderWidth + m.hueDy * 2;
    m.hueRect = {xMin, y0, xMax, y0 + h};
}

ColorPaletteSelector::Metrics
ColorPaletteSelector::computeMetricsFromWidth_(float width) const {
    namespace gs = graphics::strings;
    namespace ss = style::strings;
    using detail::getLengthInPx;
    using detail::getLengthOrPercentageInPx;

    // TODO: handle case where padding/gap is a percentage. It's unlikely
    // (doesn't make much sense), but ideally we should support it. For
    // now we simply return 0 if a percentage is given.
    float refLength = 0;

    Metrics m;
    m.hinting = (style(gs::pixel_hinting) == gs::normal);
    m.borderWidth = getLengthInPx(this, ss::border_width);
    m.paddingTop = getLengthOrPercentageInPx(this, ss::padding_top, refLength);
    m.paddingRight = getLengthOrPercentageInPx(this, ss::padding_right, refLength);
    m.paddingBottom = getLengthOrPercentageInPx(this, ss::padding_bottom, refLength);
    m.paddingLeft = getLengthOrPercentageInPx(this, ss::padding_left, refLength);
    m.rowGap = getLengthOrPercentageInPx(this, ui::strings::row_gap, refLength);
    computeSlSubMetrics_(width, m);
    computeHueSubMetrics_(width, m);
    m.width = width;
    m.height = m.hueRect.yMax() + m.paddingBottom;
    return m;
}

void ColorPaletteSelector::updateMetrics_() const {
    metrics_ = computeMetricsFromWidth_(width());
}

void ColorPaletteSelector::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    triangles_.reset();
}

bool ColorPaletteSelector::onMouseMove(MouseEvent* event) {

    const geometry::Vec2f& position = event->position();

    if (isContinuous_) {
        hoveredHue_ = -1;
        hoveredSaturation_ = -1;
        hoveredLightness_ = -1;
        if (scrubbedSelector_ != SelectorType::None) {
            selectContinuousColor_(position);
        }
        else if (metrics_.saturationLightnessRect.contains(position)) {
            auto sl = getContinuousHoveredSaturationLightness_(position);
            hoveredSaturation_ = sl.first;
            hoveredLightness_ = sl.second;
        }
        else if (metrics_.hueRect.contains(position)) {
            hoveredHue_ = getContinuousHoveredHue_(position);
        }
        reload_ = true;
        requestRepaint();
    }
    else {
        // Determine relevant selector
        SelectorType selector = scrubbedSelector_;
        if (selector == SelectorType::None) {
            selector = hoveredSelector_(position);
        }

        // Determine hovered cell
        Int i = -1;
        Int j = -1;
        Int k = -1;
        if (selector == SelectorType::SaturationLightness) {
            auto sl = getHoveredSaturationLightness_(position);
            i = sl.first;
            j = sl.second;
        }
        else if (selector == SelectorType::Hue) {
            k = getHoveredHueIndex_(position);
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
    }
    return true;
}

bool ColorPaletteSelector::onMousePress(MouseEvent* event) {
    if (isContinuous_) {
        geometry::Vec2f position = event->position();
        if (metrics_.saturationLightnessRect.contains(position)) {
            scrubbedSelector_ = SelectorType::SaturationLightness;
        }
        else if (metrics_.hueRect.contains(position)) {
            scrubbedSelector_ = SelectorType::Hue;
        }
        return selectContinuousColor_(position);
    }
    else {
        if (hoveredLightnessIndex_ != -1) {
            scrubbedSelector_ = SelectorType::SaturationLightness;
        }
        else if (hoveredHueIndex_ != -1) {
            scrubbedSelector_ = SelectorType::Hue;
        }
        return selectColorFromHovered_();
    }
}

bool ColorPaletteSelector::onMouseRelease(MouseEvent* /*event*/) {
    scrubbedSelector_ = SelectorType::None;
    return true;
}

bool ColorPaletteSelector::onMouseEnter() {
    return true;
}

bool ColorPaletteSelector::onMouseLeave() {
    hoveredLightnessIndex_ = -1;
    hoveredSaturationIndex_ = -1;
    hoveredHueIndex_ = -1;
    hoveredLightness_ = -1;
    hoveredSaturation_ = -1;
    hoveredHue_ = -1;
    reload_ = true;
    requestRepaint();
    return true;
}

float ColorPaletteSelector::preferredWidthForHeight(float) const {
    // TODO
    return preferredSize()[0];
}

float ColorPaletteSelector::preferredHeightForWidth(float width) const {
    Metrics m = computeMetricsFromWidth_(width);
    if (m.height < 0) {
        VGC_WARNING(
            LogVgcUi,
            "Computed preferredHeightForWidth ({}) of ColorPaletteSelector is negative.",
            m.height);
    }
    return m.height;
}

geometry::Vec2f ColorPaletteSelector::computePreferredSize() const {

    geometry::Vec2f res(0, 0);
    style::LengthOrPercentageOrAuto w = preferredWidth();
    style::LengthOrPercentageOrAuto h = preferredHeight();
    const style::Metrics& sMetrics = styleMetrics();
    float refLength = 0.0f;
    float valueIfAuto = 0.0f;

    if (w.isAuto()) {
        // TODO: something better , e.g., based on the number of
        // hue/saturation/lightness steps?
        using namespace style::literals;
        style::Length preferredWidthIfAuto = 100.0_dp;
        res[0] = preferredWidthIfAuto.toPx(sMetrics);
    }
    else {
        res[0] = w.toPx(sMetrics, refLength, valueIfAuto);
    }

    if (h.isAuto()) {
        Metrics m = computeMetricsFromWidth_(res[0]);
        res[1] = m.height;
    }
    else {
        res[1] = h.toPx(sMetrics, refLength, valueIfAuto);
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
ColorPaletteSelector::getHoveredSaturationLightness_(const geometry::Vec2f& p) {
    const geometry::Rect2f& r = metrics_.saturationLightnessRect;
    float i_ = numLightnessSteps_ * (p.x() - r.xMin()) / r.width();
    float j_ = numSaturationSteps_ * (p.y() - r.yMin()) / r.height();
    Int i = core::clamp(core::ifloor<Int>(i_), Int(0), numLightnessSteps_ - 1);
    Int j = core::clamp(core::ifloor<Int>(j_), Int(0), numSaturationSteps_ - 1);
    return {i, j};
}

Int ColorPaletteSelector::getHoveredHueIndex_(const geometry::Vec2f& p) {
    const geometry::Rect2f& r = metrics_.hueRect;
    auto [p_, q_] = getHueCapsuleCenters_(r);
    float hue = hueFromMousePosition_(p, p_, q_, hues_);
    float dhue = 360.0f / numHueSteps_;
    Int k = static_cast<Int>(std::round(hue / dhue));
    k = core::clamp(k, 0, numHueSteps_);
    if (k == numHueSteps_) {
        k = 0;
    }
    return k;
}

void ColorPaletteSelector::setSelectedColor_(const core::Color& color) {
    selectedColor_ = color;
    if (selectionOrigin_ != SelectionOrigin::Continuous) {
        updateContinuousFromSelectedColor_();
    }
    if (selectionOrigin_ != SelectionOrigin::Steps) {
        // Note: the function below relies on the continuous HSL values, so
        // must be done after updateContinuousFromSelectedColor_().
        updateStepsFromSelectedColor_();
    }
}

void ColorPaletteSelector::updateContinuousFromSelectedColor_() {
    if (selectionOrigin_ == SelectionOrigin::Steps) {
        float dh = 360.0f / numHueSteps_;
        float ds = 1.0f / (numSaturationSteps_ - 1);
        float dl = 1.0f / (numLightnessSteps_ - 1);
        selectedHue_ = selectedHueIndex_ * dh;
        selectedSaturation_ = selectedSaturationIndex_ * ds;
        selectedLightness_ = selectedLightnessIndex_ * dl;
    }
    else {
        auto [h, s, l] = selectedColor_.toHsl();
        bool isChromatic = (l > 0 && l < 1 && s > 0);
        if (isChromatic) { // == has meaningful hue
            selectedHue_ = h;
        }
        bool hasMeaningfulSaturation = (l > 0 && l < 1);
        if (hasMeaningfulSaturation) {
            selectedSaturation_ = s;
        }
        selectedLightness_ = l;
    }
}

void ColorPaletteSelector::updateStepsFromSelectedColor_() {

    // Find closest user-selectable color
    float h = selectedHue_;
    float s = selectedSaturation_;
    float l = selectedLightness_;
    Int hueIndex = static_cast<Int>(std::round(h * numHueSteps_ / 360.0f));
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
    isSelectedColorExact_ = (closestSelectableColor == selectedColor_);

    // Set indices based on closest user-selectable color, regardless
    // of whether there is an exact match or not
    selectedHueIndex_ = hueIndex % numHueSteps_;
    selectedSaturationIndex_ = core::clamp(saturationIndex, 0, numSaturationSteps_ - 1);
    selectedLightnessIndex_ = core::clamp(lightnessIndex, 0, numLightnessSteps_ - 1);
}

bool ColorPaletteSelector::selectColorFromHovered_() {
    bool accepted = false;
    if (hoveredLightnessIndex_ != -1) {
        selectedLightnessIndex_ = hoveredLightnessIndex_;
        selectedSaturationIndex_ = hoveredSaturationIndex_;
        accepted = true;
    }
    else if (hoveredHueIndex_ != -1) {
        selectedHueIndex_ = hoveredHueIndex_;
        accepted = true;
    }

    if (accepted) {
        reload_ = true;
        isSelectedColorExact_ = true;
        core::Color color = colorFromHslIndices(
            numHueSteps_,
            numSaturationSteps_,
            numLightnessSteps_,
            selectedHueIndex_,
            selectedSaturationIndex_,
            selectedLightnessIndex_);

        selectionOrigin_ = SelectionOrigin::Steps;
        setSelectedColor_(color);
        reload_ = true;
        colorSelected().emit();
        selectedColorChanged().emit();
        requestRepaint();
        return true;
    }
    return accepted;
}

float ColorPaletteSelector::getContinuousHoveredHue_(const geometry::Vec2f& position) {
    if (hues_.isEmpty()) {
        return 0;
    }
    else {
        const geometry::Rect2f& r = metrics_.hueRect;
        auto [p_, q_] = getHueCapsuleCenters_(r);
        return hueFromMousePosition_(position, p_, q_, hues_);
    }
}

std::pair<float, float> ColorPaletteSelector::getContinuousHoveredSaturationLightness_(
    const geometry::Vec2f& position) {
    const geometry::Rect2f& r = metrics_.saturationLightnessRect;
    float lightness = core::clamp((position.x() - r.xMin()) / r.width(), 0, 1);
    float saturation = core::clamp((position.y() - r.yMin()) / r.height(), 0, 1);
    return {saturation, lightness};
}

bool ColorPaletteSelector::selectContinuousColor_(const geometry::Vec2f& p) {
    if (scrubbedSelector_ == SelectorType::SaturationLightness) {
        auto sl = getContinuousHoveredSaturationLightness_(p);
        selectedSaturation_ = sl.first;
        selectedLightness_ = sl.second;
    }
    else if (scrubbedSelector_ == SelectorType::Hue) {
        selectedHue_ = getContinuousHoveredHue_(p);
    }
    core::Color color =
        core::Color::hsl(selectedHue_, selectedSaturation_, selectedLightness_);
    selectionOrigin_ = SelectionOrigin::Continuous;
    setSelectedColor_(color);
    reload_ = true;
    colorSelected().emit();
    selectedColorChanged().emit();
    requestRepaint();
    return true;
}

ColorPreview::ColorPreview() {
    addStyleClass(strings::ColorPreview);
}

ColorPreviewPtr ColorPreview::create() {
    return ColorPreviewPtr(new ColorPreview());
}

void ColorPreview::setColor(const core::Color& color) {
    if (color_ != color) {
        color_ = color;
        reload_ = true;
        colorChanged().emit();
        requestRepaint();
    }
}

void ColorPreview::onResize() {
    SuperClass::onResize();
    reload_ = true;
}

void ColorPreview::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void ColorPreview::onPaintDraw(graphics::Engine* engine, PaintOptions options) {
    SuperClass::onPaintDraw(engine, options);
    namespace ss = style::strings;
    if (reload_) {
        reload_ = false;
        core::FloatArray a = {};
        float borderWidth = detail::getLengthInPx(this, ss::border_width);
        core::Color borderColor =
            computeHighlightColor(color_, HighlightStyle::DarkenOnly);
        style::BorderRadiuses radiuses = style::BorderRadiuses(this);
        detail::insertRect(
            a, styleMetrics(), color_, borderColor, rect(), radiuses, borderWidth);
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_);
}

void ColorPreview::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    triangles_.reset();
}

ColorListView::ColorListView()
    : item_(style::StylableObject::create()) {

    appendChildStylableObject(item_.get());
    addStyleClass(strings::ColorListView);
    item_->addStyleClass(strings::ColorListViewItem);
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

void ColorListView::removeColorAt(Int index) {
    core::Color oldSelectedColor = selectedColor();
    Int oldSelectedColorIndex = selectedColorIndex();
    colors_.removeAt(index);
    if (oldSelectedColorIndex >= numColors()) {
        selectedColorIndex_ = numColors() - 1;
    }
    bool hasSelectedColorIndexChanged = (oldSelectedColorIndex != selectedColorIndex_);
    bool hasSelectedColorChanged = (oldSelectedColor != selectedColor());
    colorsChanged().emit();
    if (hasSelectedColorIndexChanged) {
        selectedColorIndexChanged().emit();
    }
    if (hasSelectedColorChanged) {
        selectedColorChanged().emit();
    }
    reload_ = true;
    requestGeometryUpdate();
    requestRepaint();
}

void ColorListView::setColors(const core::Array<core::Color>& colors) {

    // Update selected color index
    bool hasSelectedColorChanged = false;
    Int oldSelectedColorIndex = selectedColorIndex_;
    if (oldSelectedColorIndex >= 0) {
        if (oldSelectedColorIndex >= colors.length()) {
            hasSelectedColorChanged = true;
            selectedColorIndex_ = -1;
        }
        else if (colors[selectedColorIndex_] == colors_[selectedColorIndex_]) {
            hasSelectedColorChanged = false;
        }
        else {
            hasSelectedColorChanged = true;
        }
    }

    // Update colors
    colors_ = colors;

    // Emit signals
    reload_ = true;
    colorsChanged().emit();
    if (hasSelectedColorChanged) {
        selectedColorChanged().emit();
    }
    if (selectedColorIndex_ != oldSelectedColorIndex) {
        selectedColorIndexChanged().emit();
    }
    requestGeometryUpdate();
}

void ColorListView::onResize() {
    SuperClass::onResize();
    reload_ = true;
}

void ColorListView::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

namespace {

float getItemLengthInPx(style::StylableObject* item, core::StringId property) {

    using namespace style::literals;
    const style::Length lengthIfAuto = 10.0_dp;
    const style::Metrics& metrics = item->styleMetrics();

    style::LengthOrPercentageOrAuto p =
        item->style(property).to<style::LengthOrPercentageOrAuto>();

    // TODO: handle percentages
    float refLength = 0.0f;
    float valueIfAuto = lengthIfAuto.toPx(metrics);
    return p.toPx(metrics, refLength, valueIfAuto);
}

} // namespace

void ColorListView::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    namespace ss = style::strings;

    if (reload_) {
        reload_ = false;
        core::FloatArray a = {};
        if (numColors() > 0) {
            updateMetrics_();
            const Metrics& m = metrics_;

            float borderWidth = detail::getLengthInPx(item_.get(), ss::border_width);
            //core::Color borderColor = detail::getColor(item_.get(), gs::border_color);
            style::BorderRadiuses radiuses = style::BorderRadiuses(item_.get());

            geometry::Vec2f itemSize(m.itemWidth, m.itemHeight);

            for (Int i = 0; i < numColors(); ++i) {
                const core::Color& color = colorAt(i);
                Int row = i / m.numColumns;
                Int column = i - m.numColumns * row;
                float x1 = column * (m.itemWidth + m.columnGap);
                float y1 = row * (m.itemHeight + m.rowGap);
                float x2 = hint(x1 + m.itemWidth, m.hinting);
                float y2 = hint(y1 + m.itemHeight, m.hinting);
                x1 = hint(x1, m.hinting);
                x2 = hint(x2, m.hinting);
                geometry::Rect2f itemRect(x1, y1, x2, y2);

                if (i == selectedColorIndex_) {

                    style::BorderRadiusesInPx refRadiuses = radiuses.toPx(
                        styleMetrics(), itemRect.width(), itemRect.height());

                    // XXX should offset be in dp rather than px?
                    geometry::Rect2f itemRect1 = itemRect + ui::Margins(1);
                    style::BorderRadiusesInPx radiuses1 =
                        refRadiuses.offsetted(1, 1, 1, 1);

                    geometry::Rect2f itemRect2 = itemRect + ui::Margins(2);
                    style::BorderRadiusesInPx radiuses2 =
                        refRadiuses.offsetted(2, 2, 2, 2);

                    detail::insertRect(
                        a, color, cursorInnerColor, itemRect1, radiuses1, refRadiuses, 1);

                    detail::insertRect(
                        a,
                        core::colors::transparent,
                        cursorOuterColor,
                        itemRect2,
                        radiuses2,
                        refRadiuses,
                        1);
                }
                else {
                    HighlightStyle style = (i == hoveredColorIndex_)
                                               ? HighlightStyle::LightenOnly
                                               : HighlightStyle::DarkenOnly;
                    core::Color highlightColor = computeHighlightColor(color, style);
                    detail::insertRect(
                        a,
                        styleMetrics(),
                        color,
                        highlightColor,
                        itemRect,
                        radiuses,
                        borderWidth);
                }
            }
        }
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_);
}

void ColorListView::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
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
    Int column = computeTrackIndex(event->x(), m.itemWidth, m.columnGap, m.numColumns);
    Int row = computeTrackIndex(event->y(), m.itemHeight, m.rowGap, m.numRows);

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

float ColorListView::preferredWidthForHeight(float /* height */) const {
    // TODO
    return preferredSize()[0];
}

float ColorListView::preferredHeightForWidth(float width) const {
    Metrics m = computeMetricsFromWidth_(width);
    if (m.height < 0) {
        VGC_WARNING(
            LogVgcUi,
            "Computed preferredHeightForWidth ({}) of ColorListView is negative.",
            m.height);
        m.height = 0;
    }
    return m.height;
}

geometry::Vec2f ColorListView::computePreferredSize() const {

    geometry::Vec2f res(0, 0);
    style::LengthOrPercentageOrAuto w = preferredWidth();
    style::LengthOrPercentageOrAuto h = preferredHeight();
    float refLength = 0.0f;
    float valueIfAuto = 0.0f;

    if (w.isAuto()) {
        // TODO: something better?
        using namespace style::literals;
        style::Length autoWidth = 100.0_dp;
        res[0] = autoWidth.toPx(styleMetrics());
    }
    else {
        res[0] = w.toPx(styleMetrics(), refLength, valueIfAuto);
    }

    if (h.isAuto()) {
        Metrics m = computeMetricsFromWidth_(res[0]);
        res[1] = m.height;
    }
    else {
        res[1] = h.toPx(styleMetrics(), refLength, valueIfAuto);
    }

    return res;
}

ColorListView::Metrics ColorListView::computeMetricsFromWidth_(float width) const {

    namespace gs = graphics::strings;
    using namespace style::literals;

    Metrics m;

    m.hinting = (style(gs::pixel_hinting) == gs::normal);

    // Item preferred size
    m.itemPreferredWidth = getItemLengthInPx(item_.get(), strings::preferred_width);
    m.itemPreferredHeight = getItemLengthInPx(item_.get(), strings::preferred_height);

    // Hinted final size
    //
    // Note: we tried stretching/shrinking the item's preferred width/height,
    // keeping each item as a square, but it didn't look good when resizing the
    // toolbar, as it causes the item size to increase/decrease/increase/etc
    // even when only increasing the toolbar size. The flickering of the height
    // was especially bad. So for now, we don't shrink/stretch the items.
    // Alternatively, we could try just stretching the width (i.e., items
    // wouldn't be square anymore) instead of stretching the gap. Or perhaps
    // keep the items left-aligned. Ideally, it'd be nice if this was
    // controllable via the stylesheet, and perhaps directly handled via a Flex
    // with wrapping behavior.
    //
    m.itemWidth = hint(m.itemPreferredWidth, m.hinting);
    m.itemHeight = hint(m.itemPreferredHeight, m.hinting);

    // Get row and column gaps.
    //
    // For now, when row-gap and/or column-gap is expressed as a percentage, we
    // interpret it to mean a percentage of the size of the main-axis of the
    // widget, for both row-gap and column-gap. This makes is easier to have
    // both row-gap and column-gap the same size even when expressed as a
    // percentage.
    //
    // This is different than CSS, where a row-gap percentage is always relative to
    // the height (if the element has a fixed height), or resolves to zero if the
    // element has an `auto` height. See this discussion:
    //
    // https://github.com/w3c/csswg-drafts/issues/5081
    //
    // It's unclear whether our behavior or CSS is best. Ideally, it'd be nice
    // to be able to specify in the stylesheet whether row-gap or column-gap
    // percentages should refer to the size of the main-axis, the cross-axis,
    // or their respective dimension, e.g.:
    //
    // gap-percentage-relative-to: main-axis | cross-axis | respective-axes
    //
    float refLength = width;
    m.rowGap =
        detail::getLengthOrPercentageInPx(this, strings::row_gap, refLength, m.hinting);
    m.columnGap = detail::getLengthOrPercentageInPx(
        this, strings::column_gap, refLength, m.hinting);

    // Compute number of rows/columns, and update the column-gap accordingly,
    // as if styled with `.Flex { main-alignment: space-between; }`, but only
    // if there are at least 2 rows and 2 columns.
    //
    if (width < 2 * m.itemWidth + m.columnGap) {
        // one column
        m.numColumns = 1;
    }
    else {
        // two or more columns
        m.numColumns = static_cast<Int>(
            std::floor((width + m.columnGap) / (m.itemWidth + m.columnGap)));
    }
    m.numRows = (numColors() + m.numColumns - 1) / m.numColumns;
    if (m.numRows > 1 && m.numColumns > 1) {
        m.columnGap = (width - m.numColumns * m.itemWidth) / (m.numColumns - 1);
    }

    // Compute width/height of widget
    m.width = width;
    if (m.numRows <= 0) {
        m.height = 0;
    }
    else {
        m.height = m.numRows * (m.itemHeight + m.rowGap) - m.rowGap;
    }

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
