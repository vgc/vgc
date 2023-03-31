// Copyright 2022 The VGC Developers
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

#ifndef VGC_UI_PREFERREDSIZECALCULATOR_H
#define VGC_UI_PREFERREDSIZECALCULATOR_H

#include <vgc/core/stringid.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/style/metrics.h>
#include <vgc/style/types.h>
#include <vgc/ui/api.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

namespace detail {

// XXX Move to vgc::style? Make part of public API?
struct VGC_UI_API LengthContributions {
public:
    LengthContributions() = default;

    void
    add(const style::Metrics& metrics, const style::Value& value, float count = 1.0f);

    void addAbsolute(
        const style::Metrics& metrics,
        const style::Length& value,
        float count = 1.0f);

    void addAbsolute(float absolute) {
        absolute_ += absolute;
    }

    void addRelative(float relative) {
        relative_ += relative;
    }

    void addAbsolute(float absolute, float count) {
        absolute_ += count * absolute;
    }

    void addRelative(float relative, float count) {
        relative_ += count * relative;
    }

    float absolute() const {
        return absolute_;
    }

    float relative() const {
        return relative_;
    }

    // returns the length required for the sum of the contributions to be equal
    // to that length, when relative contributions are considered relative to
    // that length.
    //
    // This is essentially equal to `absolute() / (1 - relative())`, with some
    // extra safeguard in case `relative()` is negative or equal or greater
    // than 1.
    //
    float compute() const;

    // returns the sum of the contributions when relative contributions are
    // considered relative to the given `length`.
    //
    // This is equal to `absolute() + length * relative()`.
    //
    float computeFor(float length) const;

private:
    float absolute_ = 0.0f;
    float relative_ = 0.0f;
};

} // namespace detail

/// \class vgc::ui:::PreferredSizeCalculator
/// \brief A helper class to compute a widget's preferred size
///
/// Computing the preferred size of a widget can be tricky and/or repetitive,
/// since some of the lengths can be given in percentage of the widget itself,
/// and one should not forget to add the padding and border, which can also be
/// given in percentage.
///
/// This helper class makes it easier.
///
/// Example:
///
/// ```cpp
/// vgc::geometry::Vec2f MyLabel::computePreferredSize() const {
///     PreferredSizeCalculator calc(this);
///     calc.add(richText->preferredSize());
///     calc.addPaddingAndBorder();
///     return calc.compute();
/// }
/// ```
///
// TODO: apply hinting
class VGC_UI_API PreferredSizeCalculator {
public:
    /// Creates a `PreferredSizeCalculator` for the given widget.
    ///
    PreferredSizeCalculator(const Widget* widget);

    /// Returns the widget associated with this `PreferredSizeCalculator`.
    ///
    const Widget* widget() const {
        return widget_;
    }

    /// Returns whether the `preferred-width` style property of `widget()` is
    /// `auto`.
    ///
    bool isWidthAuto() const {
        return preferredWidth_.isAuto();
    }

    /// Returns whether the `preferred-height` style property of `widget()` is
    /// `auto`.
    ///
    bool isHeightAuto() const {
        return preferredHeight_.isAuto();
    }

    /// Returns the "absolute" part of the preferred size added so far to the
    /// calculator.
    ///
    geometry::Vec2f absoluteSize() const {
        return {absoluteWidth(), absoluteHeight()};
    }

    /// Returns the "absolute" part of the preferred width added so far to the
    /// calculator.
    ///
    float absoluteWidth() const {
        return widthContributions_.absolute();
    }

    /// Returns the "absolute" part of the preferred height added so far to the
    /// calculator.
    ///
    float absoluteHeight() const {
        return heightContributions_.absolute();
    }

    /// Returns the "relative" part of the preferred size added so far to the
    /// calculator.
    ///
    geometry::Vec2f relativeSize() const {
        return {relativeWidth(), relativeHeight()};
    }

    /// Returns the "relative" part of the preferred width added so far to the
    /// calculator.
    ///
    float relativeWidth() const {
        return widthContributions_.relative();
    }

    /// Returns the "relative" part of the preferred height added so far to the
    /// calculator.
    ///
    float relativeHeight() const {
        return heightContributions_.relative();
    }

    /// Adds the given size in px to the "absolute" part of the preferred
    /// size.
    ///
    void add(const geometry::Vec2f& absoluteSize) {
        widthContributions_.addAbsolute(absoluteSize[0]);
        heightContributions_.addAbsolute(absoluteSize[1]);
    }

    /// Adds the given `absoluteWidth` and `absoluteHeight` to the "absolute"
    /// part of the preferred size. The `Length` values are converted to `px`
    /// using the given style metrics.
    ///
    void
    add(const style::Metrics& metrics,
        const style::Length& absoluteWidth,
        const style::Length& absoluteHeight) {

        widthContributions_.addAbsolute(metrics, absoluteWidth);
        heightContributions_.addAbsolute(metrics, absoluteHeight);
    }

    /// Adds the given `absoluteWidth` and `absoluteHeight` to the "absolute"
    /// part of the preferred size. The `Length` values are converted to `px`
    /// using the style metrics of the given stylable object `obj`.
    ///
    void
    add(const style::StylableObject* obj,
        const style::Length& absoluteWidth,
        const style::Length& absoluteHeight) {

        add(obj->styleMetrics(), absoluteWidth, absoluteHeight);
    }

    /// Adds the given `absoluteWidth` and `absoluteHeight` to the "absolute"
    /// part of the preferred size. The `Length` values are converted to `px`
    /// using the style metrics of `widget()`.
    ///
    void add(const style::Length& absoluteWidth, const style::Length& absoluteHeight) {
        add(widget(), absoluteWidth, absoluteHeight);
    }

    /// Adds the given value in px to the "absolute" part of the preferred
    /// width.
    ///
    void addWidth(float absoluteWidth) {
        widthContributions_.addAbsolute(absoluteWidth);
    }

    /// Adds the given value in px to the "absolute" part of the preferred
    /// height.
    ///
    void addHeight(float absoluteHeight) {
        heightContributions_.addAbsolute(absoluteHeight);
    }

    /// Adds the given value in px to the "absolute" part of the preferred
    /// width (if direction == 0) or the preferred height (if direction == 1).
    ///
    void addTo(Int i, float absoluteLength) {
        if (i == 0) {
            widthContributions_.addAbsolute(absoluteLength);
        }
        else {
            heightContributions_.addAbsolute(absoluteLength);
        }
    }

    /// Adds the given size to the "relative" part of the preferred size. This
    /// should be a value between 0 and 1 representing a percentage of the
    /// border box of the `widget()`.
    ///
    void addRelativeSize(const geometry::Vec2f& relativeSize) {
        widthContributions_.addRelative(relativeSize[0]);
        heightContributions_.addRelative(relativeSize[1]);
    }

    /// Adds the given value to the "relative" part of the preferred width.
    /// This should be a value between 0 and 1 representing a percentage of the
    /// width of the border box of the `widget()`.
    ///
    void addRelativeWidth(float relativeWidth) {
        widthContributions_.addRelative(relativeWidth);
    }

    /// Adds the given value to the "relative" part of the preferred height.
    /// This should be a value between 0 and 1 representing a percentage of the
    /// height of the border box of the `widget()`.
    ///
    void addRelativeHeight(float relativeHeight) {
        heightContributions_.addRelative(relativeHeight);
    }

    /// Adds the given value to the "relative" part of the preferred width (if i == 0)
    /// or the preferred height (if i == 1).
    ///
    /// This `relativeLength` should be a value between 0 and 1 representing a
    /// percentage of the height of the border box of the `widget()`.
    ///
    void addToRelative(Int i, float relativeLength) {
        if (i == 0) {
            widthContributions_.addRelative(relativeLength);
        }
        else {
            heightContributions_.addRelative(relativeLength);
        }
    }

    /// Adds the given style value to the preferred width, multiplied by
    /// `count`.
    ///
    void addWidth(
        const style::Metrics& metrics,
        const style::Value& value,
        float count = 1.0f) {

        widthContributions_.add(metrics, value, count);
    }

    /// Adds the given style `property` of the given stylable object `obj` to
    /// the preferred width, mutliplied by `count`.
    ///
    void addWidth(
        const style::StylableObject* obj,
        core::StringId property,
        float count = 1.0f) {

        addWidth(obj->styleMetrics(), obj->style(property), count);
    }

    /// Adds the given style `property` of `widget()` to the preferred width,
    /// multiplied by `count`.
    ///
    void addWidth(core::StringId property, float count = 1.0f) {
        addWidth(widget_, property, count);
    }

    /// Adds the given style value to the preferred height, multiplied by
    /// `count`.
    ///
    void addHeight(
        const style::Metrics& metrics,
        const style::Value& value,
        float count = 1.0f) {

        heightContributions_.add(metrics, value, count);
    }

    /// Adds the given style `property` of the given stylable object `obj` to
    /// the preferred height, mutliplied by `count`.
    ///
    void addHeight(
        const style::StylableObject* obj,
        core::StringId property,
        float count = 1.0f) {

        addHeight(obj->styleMetrics(), obj->style(property), count);
    }

    /// Adds the given style `property` of `widget()` to the preferred height,
    /// mutliplied by `count`.
    ///
    void addHeight(core::StringId property, float count = 1.0f) {
        addHeight(widget_, property, count);
    }

    /// Adds the given style value to the preferred width (if i == 0) or the
    /// preferred height (if i == 1), multiplied by `count`.
    ///
    void addTo(
        Int i,
        const style::Metrics& metrics,
        const style::Value& value,
        float count = 1.0f) {

        if (i == 0) {
            widthContributions_.add(metrics, value, count);
        }
        else {
            heightContributions_.add(metrics, value, count);
        }
    }

    /// Adds the given style `property` of the given stylable object `obj` to
    /// the preferred width (if i == 0) or the preferred height (if i == 1),
    /// mutliplied by `count`.
    ///
    void addTo(
        Int i,
        const style::StylableObject* obj,
        core::StringId property,
        float count = 1.0f) {

        addTo(i, obj->styleMetrics(), obj->style(property), count);
    }

    /// Adds the given style `property` of `widget()` to the preferred width
    /// (if i == 0) or the preferred height (if i == 1), multiplied by `count`.
    ///
    void addTo(Int i, core::StringId property, float count = 1.0f) {
        addTo(i, widget_, property, count);
    }

    /// Adds the padding and border of the widget to the preferred size.
    ///
    void addPaddingAndBorder();

    /// Computes and returns the preferred size based on the given absolute and
    /// relative lengths as well as the style properties `preferred-width` and
    /// `preferred-height` of the widget.
    ///
    geometry::Vec2f compute() const;

private:
    const Widget* widget_;
    detail::LengthContributions widthContributions_;
    detail::LengthContributions heightContributions_;
    style::LengthOrPercentageOrAuto preferredWidth_;
    style::LengthOrPercentageOrAuto preferredHeight_;
    bool hint_;
};

/// \class vgc::ui:::PreferredWidthForHeightCalculator
/// \brief A helper class to compute a widget's preferred width for height.
///
/// Implementing the virtual method `Widget::preferredWidthForHeight()` can be
/// tricky and/or repetitive, since some of the lengths can be given in
/// percentage of the widget itself, and one should not forget to add the
/// padding and border, which can also be given in percentage.
///
/// This helper class makes it easier.
///
// TODO: apply hinting
class VGC_UI_API PreferredWidthForHeightCalculator {
public:
    /// Creates a `PreferredWidthForHeightCalculator` for the given `widget` and `targetHeight`.
    ///
    PreferredWidthForHeightCalculator(const Widget* widget, float targetHeight);

    /// Returns the widget associated with this `PreferredWidthForHeightCalculator`.
    ///
    const Widget* widget() const {
        return widget_;
    }

    /// Returns the target height associated with this `PreferredWidthForHeightCalculator`.
    ///
    float targetHeight() const {
        return targetHeight_;
    }

    /// Returns the remaining height for children after removing this widget's
    /// border and padding from `targetHeight()`.
    ///
    /// This function ensures that the returned value is never negative.
    ///
    float getChildrenTargetHeight();

    /// Returns whether the `preferred-width` style property of `widget()` is
    /// `auto`.
    ///
    bool isWidthAuto() const {
        return preferredWidth_.isAuto();
    }

    /// Returns the "absolute" part of the preferred width added so far to the
    /// calculator.
    ///
    float absoluteWidth() const {
        return widthContributions_.absolute();
    }

    /// Returns the "relative" part of the preferred width added so far to the
    /// calculator.
    ///
    float relativeWidth() const {
        return widthContributions_.relative();
    }

    /// Adds the given value in px to the "absolute" part of the preferred
    /// width.
    ///
    void addWidth(float absoluteWidth) {
        widthContributions_.addAbsolute(absoluteWidth);
    }

    /// Adds the given value to the "relative" part of the preferred width.
    /// This should be a value between 0 and 1 representing a percentage of the
    /// width of the border box of the `widget()`.
    ///
    void addRelativeWidth(float relativeWidth) {
        widthContributions_.addRelative(relativeWidth);
    }

    /// Adds the given style value to the preferred width, multiplied by
    /// `count`.
    ///
    void addWidth(
        const style::Metrics& metrics,
        const style::Value& value,
        float count = 1.0f) {

        widthContributions_.add(metrics, value, count);
    }

    /// Adds the given style `property` of the given stylable object `obj` to
    /// the preferred width, mutliplied by `count`.
    ///
    void addWidth(
        const style::StylableObject* obj,
        core::StringId property,
        float count = 1.0f) {

        addWidth(obj->styleMetrics(), obj->style(property), count);
    }

    /// Adds the given style `property` of `widget()` to the preferred width,
    /// mutliplied by `count`.
    ///
    void addWidth(core::StringId property, float count = 1.0f) {
        addWidth(widget_, property, count);
    }

    /// Adds the padding and border of the widget to the preferred width.
    ///
    void addPaddingAndBorder();

    /// Computes and returns the preferred width based on the given absolute
    /// and relative lengths as well as the style property `preferred-width` of
    /// the widget.
    ///
    float compute() const;

private:
    const Widget* widget_;
    float targetHeight_;
    detail::LengthContributions widthContributions_;
    style::LengthOrPercentageOrAuto preferredWidth_;
    bool hint_;
};

/// \class vgc::ui:::PreferredHeightForWidthCalculator
/// \brief A helper class to compute a widget's preferred height for width.
///
/// Implementing the virtual method `Widget::preferredHeightForWidth()` can be
/// tricky and/or repetitive, since some of the lengths can be given in
/// percentage of the widget itself, and one should not forget to add the
/// padding and border, which can also be given in percentage.
///
/// This helper class makes it easier.
///
// TODO: apply hinting
class VGC_UI_API PreferredHeightForWidthCalculator {
public:
    /// Creates a `PreferredHeightForWidthCalculator` for the given `widget` and `targetWidth`.
    ///
    PreferredHeightForWidthCalculator(const Widget* widget, float targetWidth);

    /// Returns the widget associated with this `PreferredHeightForWidthCalculator`.
    ///
    const Widget* widget() const {
        return widget_;
    }

    /// Returns the target width associated with this `PreferredHeightForWidthCalculator`.
    ///
    float targetWidth() const {
        return targetWidth_;
    }

    /// Returns the remaining width for children after removing this widget's
    /// border and padding from `targetWidth()`.
    ///
    /// This function ensures that the returned value is never negative.
    ///
    float getChildrenTargetWidth();

    /// Returns whether the `preferred-height` style property of `widget()` is
    /// `auto`.
    ///
    bool isHeightAuto() const {
        return preferredHeight_.isAuto();
    }

    /// Returns the "absolute" part of the preferred height added so far to the
    /// calculator.
    ///
    float absoluteHeight() const {
        return heightContributions_.absolute();
    }

    /// Returns the "relative" part of the preferred height added so far to the
    /// calculator.
    ///
    float relativeHeight() const {
        return heightContributions_.relative();
    }

    /// Adds the given value in px to the "absolute" part of the preferred
    /// width.
    ///
    void addHeight(float absoluteHeight) {
        heightContributions_.addAbsolute(absoluteHeight);
    }

    /// Adds the given value to the "relative" part of the preferred height.
    /// This should be a value between 0 and 1 representing a percentage of the
    /// height of the border box of the `widget()`.
    ///
    void addRelativeHeight(float relativeHeight) {
        heightContributions_.addRelative(relativeHeight);
    }

    /// Adds the given style value to the preferred height, multiplied by
    /// `count`.
    ///
    void addHeight(
        const style::Metrics& metrics,
        const style::Value& value,
        float count = 1.0f) {

        heightContributions_.add(metrics, value, count);
    }

    /// Adds the given style `property` of the given stylable object `obj` to
    /// the preferred height, mutliplied by `count`.
    ///
    void addHeight(
        const style::StylableObject* obj,
        core::StringId property,
        float count = 1.0f) {

        addHeight(obj->styleMetrics(), obj->style(property), count);
    }

    /// Adds the given style `property` of `widget()` to the preferred height,
    /// mutliplied by `count`.
    ///
    void addHeight(core::StringId property, float count = 1.0f) {
        addHeight(widget_, property, count);
    }

    /// Adds the padding and border of the widget to the preferred height.
    ///
    void addPaddingAndBorder();

    /// Computes and returns the preferred size based on the given absolute and
    /// relative lengths as well as the style properties `preferred-width` and
    /// `preferred-height` of the widget.
    ///
    float compute() const;

private:
    const Widget* widget_;
    float targetWidth_;
    detail::LengthContributions heightContributions_;
    style::LengthOrPercentageOrAuto preferredHeight_;
    bool hint_;
};

} // namespace vgc::ui

#endif // VGC_UI_PREFERREDSIZECALCULATOR_H
