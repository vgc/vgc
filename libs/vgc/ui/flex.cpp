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

#include <vgc/ui/flex.h>

#include <vgc/graphics/strings.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Flex::Flex(FlexDirection direction, FlexWrap wrap)
    : Widget()
    , direction_(direction)
    , wrap_(wrap) {

    addStyleClass(strings::Flex);
}

FlexPtr Flex::create(FlexDirection direction, FlexWrap wrap) {
    return FlexPtr(new Flex(direction, wrap));
}

void Flex::setDirection(FlexDirection direction) {
    direction_ = direction;
    requestGeometryUpdate();
}

void Flex::setWrap(FlexWrap wrap) {
    wrap_ = wrap;
    requestGeometryUpdate();
}

void Flex::onWidgetAdded(Widget*) {
    requestGeometryUpdate();
}

void Flex::onWidgetRemoved(Widget*) {
    requestGeometryUpdate();
}

namespace {

float getLeftRightMargins(const Widget* widget) {
    return detail::getLength(widget, graphics::strings::margin_left)
           + detail::getLength(widget, graphics::strings::margin_right);
}

float getTopBottomMargins(const Widget* widget) {
    return detail::getLength(widget, graphics::strings::margin_top)
           + detail::getLength(widget, graphics::strings::margin_bottom);
}

float getLeftRightPadding(const Widget* widget) {
    return detail::getLength(widget, graphics::strings::padding_left)
           + detail::getLength(widget, graphics::strings::padding_right);
}

float getTopBottomPadding(const Widget* widget) {
    return detail::getLength(widget, graphics::strings::padding_top)
           + detail::getLength(widget, graphics::strings::padding_bottom);
}

float getGap(bool isRow, const Widget* widget) {
    // - row-gap means the gap between rows, so should be used by Column.
    // - column-gap means the gap between columns, so should be used by Row.
    if (isRow) {
        return detail::getLength(widget, strings::column_gap);
    }
    else {
        return detail::getLength(widget, strings::row_gap);
    }
}

} // namespace

float Flex::preferredWidthForHeight(float height) const {
    bool isRow =
        (direction_ == FlexDirection::Row) || (direction_ == FlexDirection::RowReverse);
    float width = 0.0f;
    if (isRow) {
        float flexTopBottomPadding = getTopBottomPadding(this);
        float flexPaddedHeight = height - flexTopBottomPadding;
        float gap = getGap(isRow, this);
        bool isFirst = true;
        for (Widget* child : children()) {
            float childLeftRightMargins = getLeftRightMargins(child);
            float childTopBottomMargins = getTopBottomMargins(child);
            float childHeight =
                (std::max)(0.0f, flexPaddedHeight - childTopBottomMargins);
            float childPreferredWidth = child->preferredWidthForHeight(childHeight);
            width += childPreferredWidth + childLeftRightMargins;
            if (isFirst) {
                isFirst = false;
            }
            else {
                width += gap;
            }
        }
    }
    else {
        for (Widget* child : children()) {
            float childLeftRightMargins = getLeftRightMargins(child);
            float childPreferredWidth = child->preferredSize().x();
            width = (std::max)(width, childPreferredWidth + childLeftRightMargins);
        }
    }
    width += getLeftRightPadding(this);
    return width;
}

float Flex::preferredHeightForWidth(float width) const {
    bool isRow =
        (direction_ == FlexDirection::Row) || (direction_ == FlexDirection::RowReverse);
    float height = 0.0f;
    if (isRow) {
        for (Widget* child : children()) {
            float childTopBottomMargins = getTopBottomMargins(child);
            float childPreferredHeight = child->preferredSize().y();
            height = (std::max)(height, childPreferredHeight + childTopBottomMargins);
        }
    }
    else {
        float flexLeftRightPadding = getLeftRightPadding(this);
        float flexPaddedWidth = width - flexLeftRightPadding;
        float gap = getGap(isRow, this);
        bool isFirst = true;
        for (Widget* child : children()) {
            float childLeftRightMargins = getLeftRightMargins(child);
            float childTopBottomMargins = getTopBottomMargins(child);
            float childWidth = (std::max)(0.0f, flexPaddedWidth - childLeftRightMargins);
            float childPreferredHeight = child->preferredHeightForWidth(childWidth);
            height += childPreferredHeight + childTopBottomMargins;
            if (isFirst) {
                isFirst = false;
            }
            else {
                height += gap;
            }
        }
    }
    height += getTopBottomPadding(this);
    return height;
}

geometry::Vec2f Flex::computePreferredSize() const {

    bool isRow =
        (direction_ == FlexDirection::Row) || (direction_ == FlexDirection::RowReverse);
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();

    geometry::Vec2f res;
    if (w.type() == auto_) {
        if (h.type() == auto_) {

            float gap = getGap(isRow, this);
            bool isFirst = true;

            // Compute preferred height not knowing any width
            float height = 0;
            if (!isRow) {
                for (Widget* child : children()) {
                    height += child->preferredSize().y() + getTopBottomMargins(child);
                    if (isFirst) {
                        isFirst = false;
                    }
                    else {
                        height += gap;
                    }
                }
            }
            else {
                for (Widget* child : children()) {
                    height = (std::max)(
                        height, child->preferredSize().y() + getTopBottomMargins(child));
                }
            }
            height += getTopBottomPadding(this);

            // Compute preferred width not knowing any height
            float width = 0;
            if (isRow) {
                for (Widget* child : children()) {
                    width += child->preferredSize().x() + getLeftRightMargins(child);
                    if (isFirst) {
                        isFirst = false;
                    }
                    else {
                        height += gap;
                    }
                }
            }
            else {
                for (Widget* child : children()) {
                    width = (std::max)(
                        width, child->preferredSize().x() + getLeftRightMargins(child));
                }
            }
            width += getLeftRightPadding(this);

            res = {width, height};
        }
        else {
            // (auto, fixed)
            float height = h.value();
            float width = preferredWidthForHeight(height);
            res = {width, height};
        }
    }
    else {
        if (h.type() == auto_) {
            // (fixed, auto)
            float width = w.value();
            float height = preferredHeightForWidth(width);
            res = {width, height};
        }
        else {
            // (fixed, fixed)
            res = {w.value(), h.value()};
        }
    }

    return res;
}

namespace {

// Returns the preferred mainSize (margin excluded) of a child, assuming its
// crossSize (margin included) is paddedCrossSize.
//
float getChildPreferredMainSize(float isRow, float paddedCrossSize, Widget* child) {
    if (isRow) {
        float childCrossMargins = getTopBottomMargins(child);
        float childCrossSize = (std::max)(0.0f, paddedCrossSize - childCrossMargins);
        return child->preferredWidthForHeight(childCrossSize);
    }
    else {
        float childCrossMargins = getLeftRightMargins(child);
        float childCrossSize = (std::max)(0.0f, paddedCrossSize - childCrossMargins);
        return child->preferredHeightForWidth(childCrossSize);
    }
}

// If freeSpace >= 0, returns the stretch factor of this child, otherwise
// returned its scaled shrink factor. Note that we always ensure that stretch
// values and multipliers are positive, so that if totalStretch == 0, then we
// know for sure that childStretch == 0 for all children.
//
// Note: for performance, we may want to cache this computation beforehand in a
// member variable of the child widgets. But it is yet unclear whether this is
// worth it. Note that we already cache the preferredSize(), which requires
// recursion, and if not cached would likely be by far the most time-consuming
// part of getChildStretch().
//
float getChildStretch(
    bool isRow,
    float paddedCrossSize,
    float freeSpace,
    Widget* child,
    float childStretchBonus) {

    float childAuthoredStretch = 0;
    float childStretchMultiplier = 1;
    if (freeSpace >= 0) {
        childAuthoredStretch =
            isRow ? child->horizontalStretch() : child->verticalStretch();
        childStretchMultiplier = 1;
    }
    else {
        // In the case of shrinking, we need to multiply the shrink factor with
        // the preferred size. This ensures that (assuming all items have the
        // same authored shrink factor) large items shrink faster than small
        // items, so that they reach a zero-size at the same time. This is the
        // same behavior as CSS.
        childAuthoredStretch =
            isRow ? child->horizontalShrink() : child->verticalShrink();
        childStretchMultiplier = getChildPreferredMainSize(isRow, paddedCrossSize, child);
    }
    childAuthoredStretch = (std::max)(childAuthoredStretch, 0.0f);
    childStretchMultiplier = (std::max)(childStretchMultiplier, 0.0f);
    return (childAuthoredStretch + childStretchBonus) * childStretchMultiplier;
}

float computeTotalStretch(
    bool isRow,
    float paddedCrossSize,
    float freeSpace,
    Widget* parent,
    float childStretchBonus) {

    float totalStretch = 0;
    for (Widget* child : parent->children()) {
        totalStretch +=
            getChildStretch(isRow, paddedCrossSize, freeSpace, child, childStretchBonus);
    }
    return totalStretch;
}

float hinted(float x, bool hinting) {
    return hinting ? std::round(x) : x;
}

void stretchChild(
    bool isRow,
    float freeSpace,
    float crossSize,
    float extraSpacePerStretch,
    Widget* child,
    float childStretchBonus,
    float& childMainPosition,
    float parentCrossPaddingBefore,
    float parentCrossPaddingAfter,
    float gap,
    bool hinting) {

    float marginLeft = detail::getLength(child, graphics::strings::margin_left);
    float marginRight = detail::getLength(child, graphics::strings::margin_right);
    float marginTop = detail::getLength(child, graphics::strings::margin_top);
    float marginBottom = detail::getLength(child, graphics::strings::margin_bottom);
    float childMainMarginBefore = isRow ? marginLeft : marginTop;
    float childMainMarginAfter = isRow ? marginRight : marginBottom;
    float childCrossMarginBefore = isRow ? marginTop : marginLeft;
    float childCrossMarginAfter = isRow ? marginBottom : marginRight;
    float childCrossMargins = childCrossMarginBefore + childCrossMarginAfter;

    float parentCrossPadding = parentCrossPaddingBefore + parentCrossPaddingAfter;
    float paddedCrossSize = crossSize - parentCrossPadding;
    float childCrossSize = paddedCrossSize - childCrossMargins;

    float childPreferredMainSize =
        getChildPreferredMainSize(isRow, paddedCrossSize, child);

    float childStretch =
        getChildStretch(isRow, paddedCrossSize, freeSpace, child, childStretchBonus);

    float childMainSize = childPreferredMainSize + extraSpacePerStretch * childStretch;
    float childCrossPosition = parentCrossPaddingBefore + childCrossMarginBefore;
    childMainPosition += childMainMarginBefore;
    float hChildMainPosition = hinted(childMainPosition, hinting);
    float hChildCrossPosition = hinted(childCrossPosition, hinting);
    float hChildMainSize =
        hinted(childMainPosition + childMainSize, hinting) - hChildMainPosition;
    float hChildCrossSize =
        hinted(childCrossPosition + childCrossSize, hinting) - hChildCrossPosition;

    if (isRow) {
        child->updateGeometry(
            hChildMainPosition, hChildCrossPosition, hChildMainSize, hChildCrossSize);
    }
    else {
        child->updateGeometry(
            hChildCrossPosition, hChildMainPosition, hChildCrossSize, hChildMainSize);
    }

    childMainPosition += childMainSize + childMainMarginAfter + gap;
}

} // namespace

void Flex::updateChildrenGeometry() {

    namespace gs = graphics::strings;

    // Note: we loosely follow the algorithm and terminology from CSS Flexbox:
    // https://www.w3.org/TR/css-flexbox-1/#layout-algorithm
    Int numChildren = 0; // TODO: have a (possibly constant-time) numChildren() method
    for (Widget* child : children()) {
        (void)child; // Unused
        numChildren += 1;
    }
    if (numChildren > 0) {
        bool isRow = (direction_ == FlexDirection::Row)
                     || (direction_ == FlexDirection::RowReverse);
        bool isReverse = (direction_ == FlexDirection::RowReverse)
                         || (direction_ == FlexDirection::ColumnReverse);
        bool hinting = (style(gs::pixel_hinting) == gs::normal);
        float paddingLeft = detail::getLength(this, gs::padding_left);
        float paddingRight = detail::getLength(this, gs::padding_right);
        float paddingTop = detail::getLength(this, gs::padding_top);
        float paddingBottom = detail::getLength(this, gs::padding_bottom);
        float mainSize = isRow ? width() : height();
        float crossSize = isRow ? height() : width();
        float gap = getGap(isRow, this);
        float preferredMainSize = isRow ? preferredWidthForHeight(crossSize)
                                        : preferredHeightForWidth(crossSize);
        float mainPaddingBefore = isRow ? paddingLeft : paddingTop;
        float crossPaddingBefore = isRow ? paddingTop : paddingLeft;
        float crossPaddingAfter = isRow ? paddingBottom : paddingRight;
        float paddedCrossSize = crossSize - crossPaddingBefore - crossPaddingAfter;
        float freeSpace = mainSize - preferredMainSize;
        float eps = 1e-6f;
        // TODO: have a loop to resolve constraint violations, as per 9.7.4:
        // https://www.w3.org/TR/css-flexbox-1/#resolve-flexible-length
        // Indeed, although we currently don't have explicit min/max constraints,
        // we still have an implicit min-size = 0 constraint. The algorithm
        // below don't properly handle this constraint: if the size of one of
        // the items is shrinked to a negative size, then it is clamped to zero,
        // but the lost space due to clamping isn't redistributed to other items,
        // causing an overflow.
        float childStretchBonus = 0;
        float totalStretch = computeTotalStretch(
            isRow, paddedCrossSize, freeSpace, this, childStretchBonus);
        if (totalStretch < eps) {
            // For now, we stretch evenly as if all childStretch were equal to
            // one. Later, we should instead insert empty space between the items,
            // based on alignment properties, see:
            // https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Box_Alignment
            childStretchBonus = 1;
            totalStretch = computeTotalStretch(
                isRow, paddedCrossSize, freeSpace, this, childStretchBonus);
        }
        float extraSpacePerStretch = freeSpace / totalStretch;
        float childMainPosition = mainPaddingBefore;
        Widget* child = isReverse ? lastChild() : firstChild();
        while (child) {
            stretchChild(
                isRow,
                freeSpace,
                crossSize,
                extraSpacePerStretch,
                child,
                childStretchBonus,
                childMainPosition,
                crossPaddingBefore,
                crossPaddingAfter,
                gap,
                hinting);
            child = isReverse ? child->previousSibling() : child->nextSibling();
        }
    }
}

} // namespace vgc::ui
