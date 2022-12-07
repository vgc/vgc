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

#ifndef VGC_UI_FLEX_H
#define VGC_UI_FLEX_H

#include <vgc/core/enum.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

/// \enum vgc::ui::FlexDirection
/// \brief The direction of a flex layout
///
enum class FlexDirection {
    Row = 0,
    RowReverse = 1,
    Column = 2,
    ColumnReverse = 3
};

VGC_UI_API
VGC_DECLARE_ENUM(FlexDirection)

/// \enum vgc::ui::MainAlignment
/// \brief How to align the children of a Flex along the main axis.
///
/// This enum class stores as an enum the value of the `main-alignment` style
/// property.
///
/// The `main-alignment` style property is used to specify how to distribute
/// any extra space along the main axis of a `Flex`, after all the stretchable
/// child widgets have reached their maximum size:
///
/// - `start`: the widgets are packed at the start of the main axis, with no
///    extra space between them.
///
/// - `end`: the widgets are packed at the end of the main axis, with no extra
///    space between them.
///
/// - `center`: the widgets are packed centered on the middle of the main axis,
///    with no extra space between them.
///
/// - `space-between`: the extra space is equally distributed between the
///    widgets, with no space before the first widget or after the last widget.
///    If there is only widget in the `Flex`, this is equivalent to `start`.
///
/// - `space-around`: the extra space is equally distributed on both sides of
///    widgets. This means that the space before the first widget and after the
///    last widget is half the space between the widgets.
///
/// - `space-evenly`: the extra space is equally distributed not only between
///    the widgets, but also before the first widget and after the last widget.
///    This means that the space before the first widget and after the last
///    widget is equal to the space between the widgets.
///
/// - `force-stretch`: forces stretchable widgets to be larger than their
///    maximum size, and non-stretchable widgets to be larger than their
///    preferred size, effectiveley ensuring that there is no extra space to
///    distribute between the widgets.
///
/// The default value is `start`.
///
enum class MainAlignment {
    Start,
    End,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
    ForceStretch
};

VGC_UI_API
VGC_DECLARE_ENUM(MainAlignment)

/// \enum vgc::ui::FlexWrap
/// \brief How to wrap widgets in a Flex
///
/// For now, only NoWrap is supported. Wrap and WrapReverse will
/// be added in the future.
///
enum class FlexWrap {
    NoWrap //,
    //Wrap,
    //WrapReverse
};

VGC_UI_API
VGC_DECLARE_ENUM(FlexWrap)

VGC_DECLARE_OBJECT(Flex);

namespace detail {

// Stores data about a given Flex.
//
struct FlexData {

    // Data that does not depends on children metrics.
    // All of these are always computed.
    //
    Flex* flex;
    bool hinting;
    bool isRow;
    bool isReverse;
    MainAlignment mainAlignment;
    Int mainDir;
    Int crossDir;
    float gap;
    geometry::Vec2f size;
    float contentMainPosition;
    float contentCrossPosition;
    float contentMainSize;
    float contentCrossSize;

    // Data that depends on children metrics.
    // Some of these are only computed in some modes (e.g., shrink vs stretch).
    //
    float totalShrink;        // Sum of shrink of child widgets
    float totalStretch;       // Sum of shrink of child widgets
    float totalPreferredSize; // Sum of preferred size of child widgets
    float totalMinSize;       // Sum of min sizes of child widgets
    float totalMaxSize;       // Sum of max size of child widgets
    float availableSize;      // Size available for child widgets after removing
                              // Flex's border, padding, gaps, and children fixed margins
    float extraSize;          // Difference between availableSize and totalPreferredSize
    float extraSizeAfterStretch; // Extra size still remaining after stretching children
    float extraSizeAfterShrink;  // Extra size still remaining after shrinking children
};

// Stores data about a given child widget of a Flex.
//
struct FlexChildData {

    Widget* child;

    // Input
    float shrink;
    float stretch;
    float mainPreferredSize;
    float mainMinSize;
    float mainMaxSize;
    geometry::Vec2f minSize;
    geometry::Vec2f maxSize;
    geometry::Vec2f mainMargins;
    geometry::Vec2f crossMargins;

    // Output
    float mainSize;
    geometry::Vec2f position;
    geometry::Vec2f size;

    // Hinted output
    geometry::Vec2f hPosition;
    geometry::Vec2f hSize;
};

// Structure used to order child widgets by "normalized slack", that is, how
// much total extra space is required before the child area's size reaches its
// max (or min) size. This order makes it possible to resolved all min/max
// constraints in one pass, since child widgets reaching their min/max size
// faster are processed first.
//
struct FlexChildSlack {

    // Which child this is referring to
    FlexChildData* flexChildData;

    // In stretch mode: weight = data.stretch
    // In shrink mode:  weight = (data.preferredSize - data.minSize) * data.shrink
    float weight;

    // In stretch mode: normalizedSlack = (data.maxSize - data.preferredSize) / data.stretch
    // In shrink mode:  normalizedSlack = 1.0 / data.shrink
    // (as a special case, if data.stretch/shrink == 0 then normalizedSlack = 0)
    float normalizedSlack;

    friend bool operator<(const FlexChildSlack& a, const FlexChildSlack& b) {
        return a.normalizedSlack < b.normalizedSlack;
    };
};

} // namespace detail

/// \class vgc::ui::Flex
/// \brief Arrange a sequence of widgets in rows and/or columns.
///
class VGC_UI_API Flex : public Widget {
private:
    VGC_OBJECT(Flex, Widget)

protected:
    /// This is an implementation details. Please use
    /// Flex::create() instead.
    ///
    Flex(FlexDirection direction = FlexDirection::Row, FlexWrap wrap = FlexWrap::NoWrap);

public:
    /// Creates a Row.
    ///
    static FlexPtr create(
        FlexDirection direction = FlexDirection::Row,
        FlexWrap wrap = FlexWrap::NoWrap);

    /// Returns the FlexDirection of this Flex.
    ///
    FlexDirection direction() const {
        return direction_;
    }

    /// Sets the FlexDirection of this Flex.
    ///
    void setDirection(FlexDirection direction);

    /// Returns true if the flex direction is `Row` or `RowReverse`. Otherwise
    /// returns false.
    ///
    bool isRow() const {
        return core::toUnderlying(direction()) <= 1;
    }

    /// Returns true if the flex direction is `Column` or `ColumnReverse`.
    /// Otherwise returns false.
    ///
    bool isColumn() const {
        return core::toUnderlying(direction()) > 1;
    }

    /// Returns `0` if the flex direction is `Row` or `RowReverse`.
    ///
    /// Returns `1` if the flex direction is `Column` or `ColumnReverse`.
    ///
    Int mainDirectionIndex() const {
        return isColumn();
    }

    /// Returns `1` if the flex direction is `Row` or `RowReverse`.
    ///
    /// Returns `0` if the flex direction is `Column` or `ColumnReverse`.
    ///
    Int crossDirectionIndex() const {
        return isRow();
    }

    /// Returns true if the flex direction is `RowReverse` or `ColumnReverse`.
    /// Otherwise returns flase.
    ///
    bool isReverse() const {
        return direction() == FlexDirection::RowReverse
               || direction() == FlexDirection::ColumnReverse;
    }

    /// Returns the FlexWrap of this Flex.
    ///
    FlexWrap wrap() const {
        return wrap_;
    }

    /// Sets the FlexWrap of this Flex.
    ///
    void setWrap(FlexWrap wrap);

    // reimpl
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;

    // Implementation of StylableObject interface
    static void populateStyleSpecTable(style::SpecTable* table);
    void populateStyleSpecTableVirtual(style::SpecTable* table) override {
        populateStyleSpecTable(table);
    }

protected:
    void onWidgetAdded(Widget* child, bool wasOnlyReordered) override;
    void onWidgetRemoved(Widget* child) override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;

private:
    FlexDirection direction_;
    FlexWrap wrap_;
    core::Array<detail::FlexChildData> childData_;    // visible children only
    core::Array<detail::FlexChildSlack> childSlacks_; // visible children only
};

} // namespace vgc::ui

#endif // VGC_UI_FLEX_H
