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

#include <vgc/ui/strings.h>

#include <vgc/ui/internal/paintutil.h>

namespace vgc {
namespace ui {

Flex::Flex(
        FlexDirection direction,
        FlexWrap wrap) :
    Widget(),
    direction_(direction),
    wrap_(wrap)
{
    addStyleClass(strings::Flex);
}

FlexPtr Flex::create(
        FlexDirection direction,
        FlexWrap wrap)
{
    return FlexPtr(new Flex(direction, wrap));
}

void Flex::setDirection(FlexDirection direction)
{
    direction_ = direction;
    updateGeometry();
}

void Flex::setWrap(FlexWrap wrap)
{
    wrap_ = wrap;
    updateGeometry();
}

void Flex::onWidgetAdded(Object*)
{
    updateGeometry();
}

void Flex::onWidgetRemoved(Object*)
{
    updateGeometry();
}

namespace {

float getLeftRightMargins(const Widget* widget)
{
    return internal::getLength(widget, strings::margin_left) +
           internal::getLength(widget, strings::margin_right);
}

float getTopBottomMargins(const Widget* widget)
{
    return internal::getLength(widget, strings::margin_top) +
           internal::getLength(widget, strings::margin_bottom);
}

float getLeftRightPadding(const Widget* widget)
{
    return internal::getLength(widget, strings::padding_left) +
           internal::getLength(widget, strings::padding_right);
}

float getTopBottomPadding(const Widget* widget)
{
    return internal::getLength(widget, strings::padding_top) +
           internal::getLength(widget, strings::padding_bottom);
}

} // namespace

geometry::Vec2f Flex::computePreferredSize() const
{
    bool isRow = (direction_ == FlexDirection::Row) ||
            (direction_ == FlexDirection::RowReverse);
    geometry::Vec2f res(0, 0);
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    if (w.type() != auto_) {
        res[0] = w.value();
    }
    else {
        if (isRow) {
            for (Widget* child : children()) {
                res[0] += child->preferredSize().x() + getLeftRightMargins(child);
            }
        }
        else {
            // For now, we use the max preferred width of all widgets as
            // preferred width for the column. In the future, we could compute
            // minimum/maximum widths based on the stretch and shrink
            // factors of the children, compute an average preferred width
            // (possibly weighted by the stretch/shrink factors?), and clamp
            // using the min/max widths.
            for (Widget* child : children()) {
                res[0] = std::max(res[0], child->preferredSize().x() + getLeftRightMargins(child)) ;
            }
        }
        res[0] += getLeftRightPadding(this);
    }
    if (h.type() != PreferredSizeType::Auto) {
        res[1] = h.value();
    }
    else {
        if (!isRow) {
            for (Widget* child : children()) {
                res[1] += child->preferredSize().y() + getTopBottomMargins(child);
            }
        }
        else {
            for (Widget* child : children()) {
                res[1] = std::max(res[1], child->preferredSize().y() + getTopBottomMargins(child));
            }
        }
        res[1] += getTopBottomPadding(this);
    }
    return res;
}

namespace {

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
float getChildStretch(bool isRow, float freeSpace, Widget* child, float childStretchBonus)
{
    float childAuthoredStretch = 0;
    float childStretchMultiplier = 1;
    if (freeSpace >= 0) {
        childAuthoredStretch = isRow ? child->stretchWidth() : child->stretchHeight();
        childStretchMultiplier = 1;
    }
    else {
        // In the case of shrinking, we need to multiply the shrink factor with
        // the preferred size. This ensures that (assuming all items have the
        // same authored shrink factor) large items shrink faster than small
        // items, so that they reach a zero-size at the same time. This is the
        // same behavior as CSS.
        childAuthoredStretch = isRow ? child->shrinkWidth() : child->shrinkHeight();
        childStretchMultiplier = isRow ? child->preferredSize().x() : child->preferredSize().y();
    }
    childAuthoredStretch = std::max(childAuthoredStretch, 0.0f);
    childStretchMultiplier = std::max(childStretchMultiplier, 0.0f);
    return (childAuthoredStretch + childStretchBonus) * childStretchMultiplier;
}

float computeTotalStretch(bool isRow, float freeSpace, Widget* parent, float childStretchBonus)
{
    float totalStretch = 0;
    for (Widget* child : parent->children()) {
        totalStretch += getChildStretch(isRow, freeSpace, child, childStretchBonus);
    }
    return totalStretch;
}

float hinted(float x, bool hinting) {
    return hinting ? std::round(x) : x;
}

void stretchChild(
        bool isRow, float freeSpace, float crossSize, float extraSpacePerStretch,
        Widget* child, float childStretchBonus, float& childMainPosition,
        float parentCrossPaddingBefore, float parentCrossPaddingAfter,
        bool hinting)
{
    float childPreferredMainSize = isRow ? child->preferredSize().x() : child->preferredSize().y();
    float childStretch = getChildStretch(isRow, freeSpace, child, childStretchBonus);
    float childMainSize = childPreferredMainSize + extraSpacePerStretch * childStretch;
    float childMainMarginBefore = isRow ? internal::getLength(child, strings::margin_left) : internal::getLength(child, strings::margin_top);
    float childMainMarginAfter = isRow ? internal::getLength(child, strings::margin_right) : internal::getLength(child, strings::margin_bottom);
    float childCrossMarginBefore = isRow ? internal::getLength(child, strings::margin_top) : internal::getLength(child, strings::margin_left);
    float childCrossMarginAfter = isRow ? internal::getLength(child, strings::margin_bottom) : internal::getLength(child, strings::margin_right);
    float childCrossSize = crossSize - parentCrossPaddingBefore - parentCrossPaddingAfter - childCrossMarginBefore - childCrossMarginAfter;
    float childCrossPosition = parentCrossPaddingBefore + childCrossMarginBefore;
    childMainPosition += childMainMarginBefore;
    float hChildMainPosition = hinted(childMainPosition, hinting);
    float hChildCrossPosition = hinted(childCrossPosition, hinting);
    float hChildMainSize = hinted(childMainPosition + childMainSize, hinting) - hChildMainPosition;
    float hChildCrossSize = hinted(childCrossPosition + childCrossSize, hinting) - hChildCrossPosition;
    if (isRow) {
        child->setGeometry(hChildMainPosition, hChildCrossPosition, hChildMainSize, hChildCrossSize);
    }
    else {
        child->setGeometry(hChildCrossPosition, hChildMainPosition, hChildCrossSize, hChildMainSize);
    }
    childMainPosition += childMainSize + childMainMarginAfter;
}

} // namespace

void Flex::updateChildrenGeometry()
{
    // Note: we loosely follow the algorithm and terminology from CSS Flexbox:
    // https://www.w3.org/TR/css-flexbox-1/#layout-algorithm
    Int numChildren = 0; // TODO: have a (possibly constant-time) numChildren() method
    for (Widget* child : children()) {
        (void)child; // Unused
        numChildren += 1;
    }
    if (numChildren > 0) {
        bool isRow = (direction_ == FlexDirection::Row) ||
                     (direction_ == FlexDirection::RowReverse);
        bool isReverse =
                (direction_ == FlexDirection::RowReverse) ||
                (direction_ == FlexDirection::ColumnReverse);
        bool hinting = (style(strings::pixel_hinting) == strings::normal);
        float preferredMainSize = isRow ? preferredSize().x() : preferredSize().y();
        float mainPaddingBefore = isRow ? internal::getLength(this, strings::padding_left) : internal::getLength(this, strings::padding_top);
        float crossPaddingBefore = isRow ? internal::getLength(this, strings::padding_top) : internal::getLength(this, strings::padding_left);
        float crossPaddingAfter = isRow ? internal::getLength(this, strings::padding_bottom) : internal::getLength(this, strings::padding_right);
        float mainSize = isRow ? width() : height();
        float crossSize = isRow ? height() : width();
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
        float totalStretch = computeTotalStretch(isRow, freeSpace, this, childStretchBonus);
        if (totalStretch < eps) {
            // For now, we stretch evenly as if all childStretch were equal to
            // one. Later, we should instead insert empty space between the items,
            // based on alignment properties, see:
            // https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Box_Alignment
            childStretchBonus = 1;
            totalStretch = computeTotalStretch(isRow, freeSpace, this, childStretchBonus);
        }
        float extraSpacePerStretch = freeSpace / totalStretch;
        float childMainPosition = mainPaddingBefore;
        Widget* child = isReverse ? lastChild() : firstChild();
        while (child) {
            stretchChild(isRow, freeSpace, crossSize, extraSpacePerStretch,
                         child, childStretchBonus, childMainPosition,
                         crossPaddingBefore, crossPaddingAfter,
                         hinting);
            child = isReverse ? child->previousSibling() : child->nextSibling();
        }
    }
}

} // namespace ui
} // namespace vgc
