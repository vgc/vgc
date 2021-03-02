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

namespace vgc {
namespace ui {

Flex::Flex(
        FlexDirection direction,
        FlexWrap wrap) :
    Widget(),
    direction_(direction),
    wrap_(wrap)
{
    addClass(strings::Flex);
    setWidthPolicy(ui::LengthPolicy::AutoFlexible());
    setHeightPolicy(ui::LengthPolicy::AutoFlexible());
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

void Flex::onChildAdded(Object*)
{
    updateGeometry();
}

void Flex::onChildRemoved(Object*)
{
    updateGeometry();
}

core::Vec2f Flex::computePreferredSize() const
{
    bool isRow = (direction_ == FlexDirection::Row) ||
            (direction_ == FlexDirection::RowReverse);
    float preferredWidth = 0;
    float preferredHeight = 0;
    if (widthPolicy().type() != LengthType::Auto) {
        preferredWidth = widthPolicy().value();
    }
    else {
        if (isRow) {
            for (Widget* child : children()) {
                preferredWidth += child->preferredSize().x();
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
                preferredWidth = std::max(preferredWidth, child->preferredSize().x());
            }
        }
    }
    if (heightPolicy().type() != LengthType::Auto) {
        preferredHeight = heightPolicy().value();
    }
    else {
        if (!isRow) {
            for (Widget* child : children()) {
                preferredHeight += child->preferredSize().y();
            }
        }
        else {
            for (Widget* child : children()) {
                preferredHeight = std::max(preferredHeight, child->preferredSize().y());
            }
        }
    }
    return core::Vec2f(preferredWidth, preferredHeight);
}

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
        float preferredMainSize = isRow ? preferredSize().x() : preferredSize().y();
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
        if (freeSpace >= 0) {
            // Handle stretching
            float totalStretch = 0;
            for (Widget* child : children()) {
                float childStretch = isRow ? child->widthPolicy().stretch() : child->heightPolicy().stretch();
                totalStretch += childStretch;
            }
            if (totalStretch > eps) {
                float extraSpacePerStretch = freeSpace / totalStretch;
                float childMainPosition = 0.0f; // TODO: if isReverse is true, we should start at width() and iterate
                                                // children from first to last. This would better handle future alignment properties.
                Widget* child = isReverse ? lastChild() : firstChild();
                while (child) {
                    float childPreferredMainSize = isRow ? child->preferredSize().x() : child->preferredSize().y();
                    float childStretch = isRow ? child->widthPolicy().stretch() : child->heightPolicy().stretch();
                    float childMainSize = childPreferredMainSize + extraSpacePerStretch * childStretch;
                    if (isRow) {
                        child->setGeometry(childMainPosition, 0, childMainSize, crossSize);
                    }
                    else {
                        child->setGeometry(0, childMainPosition, crossSize, childMainSize);
                    }
                    child = isReverse ? child->previousSibling() : child->nextSibling();
                    childMainPosition += childMainSize;
                }
            }
            else {
                // Stretch evenly. TODO: should we instead leave empty space at the end of the main axis?
                float freeSpacePerChild = freeSpace / numChildren;
                float childMainPosition = 0.0f;
                Widget* child = isReverse ? lastChild() : firstChild();
                while (child) {
                    float childPreferredMainSize = isRow ? child->preferredSize().x() : child->preferredSize().y();
                    float childMainSize = childPreferredMainSize + freeSpacePerChild;
                    if (isRow) {
                        child->setGeometry(childMainPosition, 0, childMainSize, crossSize);
                    }
                    else {
                        child->setGeometry(0, childMainPosition, crossSize, childMainSize);
                    }
                    child = isReverse ? child->previousSibling() : child->nextSibling();
                    childMainPosition += childMainSize;
                }
            }
        }
        else {
            // Handle shrinking. Note that unlike when stretching, the shrink
            // factor is multiplied with the preferred size. This ensures that
            // if they have the same shrink factor, large items shrink faster
            // than small items, so that they reach a zero-size at the same
            // time. This is the same behavior as CSS.
            float totalScaledShrink = 0;
            for (Widget* child : children()) {
                float childPreferredMainSize = isRow ? child->preferredSize().x() : child->preferredSize().y();
                float childShrink = isRow ? child->widthPolicy().shrink() : child->heightPolicy().shrink();
                float childScaledShrink = childShrink * childPreferredMainSize;
                totalScaledShrink += childScaledShrink;
            }
            if (totalScaledShrink > eps) {
                float freeSpacePerScaledShrink = freeSpace / totalScaledShrink;
                float childMainPosition = 0.0f;
                Widget* child = isReverse ? lastChild() : firstChild();
                while (child) {
                    float childPreferredMainSize = isRow ? child->preferredSize().x() : child->preferredSize().y();
                    float childShrink = isRow ? child->widthPolicy().shrink() : child->heightPolicy().shrink();
                    float childScaledShrink = childShrink * childPreferredMainSize;
                    float childMainSize = childPreferredMainSize + freeSpacePerScaledShrink * childScaledShrink;
                    if (isRow) {
                        child->setGeometry(childMainPosition, 0, childMainSize, crossSize);
                    }
                    else {
                        child->setGeometry(0, childMainPosition, crossSize, childMainSize);
                    }
                    child = isReverse ? child->previousSibling() : child->nextSibling();
                    childMainPosition += childMainSize;
                }
            }
            else {
                // Shrink evenly. TODO: should we instead overflow?
                float freeSpacePerChild = freeSpace / numChildren;
                float childMainPosition = 0.0f;
                Widget* child = isReverse ? lastChild() : firstChild();
                while (child) {
                    float childPreferredMainSize = isRow ? child->preferredSize().x() : child->preferredSize().y();
                    float childMainSize = childPreferredMainSize + freeSpacePerChild;
                    if (isRow) {
                        child->setGeometry(childMainPosition, 0, childMainSize, crossSize);
                    }
                    else {
                        child->setGeometry(0, childMainPosition, crossSize, childMainSize);
                    }
                    child = isReverse ? child->previousSibling() : child->nextSibling();
                    childMainPosition += childMainSize;
                }
            }
        }
    }
}

} // namespace ui
} // namespace vgc
