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
#include <vgc/style/parse.h>
#include <vgc/style/strings.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

VGC_DEFINE_ENUM(
    FlexDirection,
    (Row, "row"),
    (RowReverse, "row-reverse"),
    (Column, "column"),
    (ColumnReverse, "column-reverse"))

VGC_DEFINE_ENUM(
    MainAlignment,
    (Start, "start"),
    (End, "end"),
    (Center, "center"),
    (SpaceBetween, "space-between"),
    (SpaceAround, "space-around"),
    (SpaceEvenly, "space-evenly"),
    (ForceStretch, "force-stretch"))

VGC_DEFINE_ENUM( //
    FlexWrap,
    (NoWrap, "nowrap"))

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

namespace {

using style::StyleTokenIterator;
using style::StyleTokenType;
using style::StyleValue;

StyleValue parseMainAlignment(StyleTokenIterator begin, StyleTokenIterator end_) {

    using namespace strings;

    StyleValue res = StyleValue::invalid();

    // There must be exactly one token
    if (end_ != begin + 1) {
        return res;
    }
    StyleTokenType t = begin->type();

    // The token should be an identifier
    if (t != StyleTokenType::Identifier) {
        return res;
    }
    std::string_view s = begin->stringValue();

    // Converts the identifier to StyleValue storing a MainAlignment enum value.
    if (s == start) {
        res = StyleValue::custom(MainAlignment::Start);
    }
    else if (s == end) {
        res = StyleValue::custom(MainAlignment::End);
    }
    else if (s == center) {
        res = StyleValue::custom(MainAlignment::Center);
    }
    else if (s == space_between) {
        res = StyleValue::custom(MainAlignment::SpaceBetween);
    }
    else if (s == space_around) {
        res = StyleValue::custom(MainAlignment::SpaceAround);
    }
    else if (s == space_evenly) {
        res = StyleValue::custom(MainAlignment::SpaceEvenly);
    }
    else if (s == force_stretch) {
        res = StyleValue::custom(MainAlignment::ForceStretch);
    }

    return res;
}

} // namespace

void Flex::populateStyleSpecTable(style::SpecTable* table) {
    if (!table->setRegistered(staticClassName())) {
        return;
    }
    using namespace strings;
    auto start_ma = StyleValue::custom(MainAlignment::Start);
    table->insert(main_alignment, start_ma, false, &parseMainAlignment);
    SuperClass::populateStyleSpecTable(table);
}

void Flex::onWidgetAdded(Widget*, bool) {
    requestGeometryUpdate();
}

void Flex::onWidgetRemoved(Widget*) {
    requestGeometryUpdate();
}

namespace {

float hinted(float x, bool hinting) {
    return hinting ? std::round(x) : x;
}

float getLeftRightMargins(const Widget* widget) {
    // TODO: handle percentages
    using namespace style::strings;
    float refLength = 0;
    return detail::getLengthOrPercentageInPx(widget, margin_left, refLength)
           + detail::getLengthOrPercentageInPx(widget, margin_right, refLength);
}

float getTopBottomMargins(const Widget* widget) {
    // TODO: handle percentages
    using namespace style::strings;
    float refLength = 0;
    return detail::getLengthOrPercentageInPx(widget, margin_top, refLength)
           + detail::getLengthOrPercentageInPx(widget, margin_bottom, refLength);
}

float getLeftRightPadding(const Widget* widget) {
    // TODO: handle percentages
    using namespace style::strings;
    float refLength = 0;
    return detail::getLengthOrPercentageInPx(widget, padding_left, refLength)
           + detail::getLengthOrPercentageInPx(widget, padding_right, refLength);
}

float getTopBottomPadding(const Widget* widget) {
    // TODO: handle percentages
    using namespace style::strings;
    float refLength = 0;
    return detail::getLengthOrPercentageInPx(widget, padding_top, refLength)
           + detail::getLengthOrPercentageInPx(widget, padding_bottom, refLength);
}

float getGap(bool isRow, const Widget* widget, bool hinting) {
    // - row-gap means the gap between rows, so should be used by Column.
    // - column-gap means the gap between columns, so should be used by Row.
    // TODO: handle percentages
    float refLength = 0;
    float res;
    if (isRow) {
        res = detail::getLengthOrPercentageInPx(widget, strings::column_gap, refLength);
    }
    else {
        res = detail::getLengthOrPercentageInPx(widget, strings::row_gap, refLength);
    }
    return hinted(res, hinting);
}

} // namespace

float Flex::preferredWidthForHeight(float height) const {
    style::LengthOrPercentageOrAuto w = preferredWidth();
    float width = 0.0f;
    if (w.isAuto()) {
        bool isRow_ = isRow();
        if (isRow_) {
            float flexTopBottomPadding = getTopBottomPadding(this);
            float flexPaddedHeight = height - flexTopBottomPadding;
            Int numVisibleChildren = 0;
            for (Widget* child : children()) {
                if (child->visibility() == Visibility::Invisible) {
                    continue;
                }
                ++numVisibleChildren;
                float childLeftRightMargins = getLeftRightMargins(child);
                float childTopBottomMargins = getTopBottomMargins(child);
                float childHeight =
                    (std::max)(0.0f, flexPaddedHeight - childTopBottomMargins);
                float childPreferredWidth = child->preferredWidthForHeight(childHeight);
                width += childPreferredWidth + childLeftRightMargins;
            }
            if (numVisibleChildren > 0) {
                namespace gs = graphics::strings;
                bool hinting = (style(gs::pixel_hinting) == gs::normal);
                float gap = getGap(isRow_, this, hinting);
                width += (numVisibleChildren - 1) * gap;
            }
        }
        else {
            for (Widget* child : children()) {
                if (child->visibility() == Visibility::Invisible) {
                    continue;
                }
                float childLeftRightMargins = getLeftRightMargins(child);
                float childPreferredWidth = child->preferredSize().x();
                width = (std::max)(width, childPreferredWidth + childLeftRightMargins);
            }
        }
        width += getLeftRightPadding(this);
    }
    else {
        // fixed width
        // TODO: support percentages
        float refLength = 0.0f;
        float valueIfAuto = 0.0f;
        width = w.toPx(styleMetrics(), refLength, valueIfAuto);
    }
    return width;
}

float Flex::preferredHeightForWidth(float width) const {
    style::LengthOrPercentageOrAuto h = preferredHeight();
    float height = 0.0f;
    if (h.isAuto()) {
        bool isRow_ = isRow();
        if (isRow_) {
            for (Widget* child : children()) {
                if (child->visibility() == Visibility::Invisible) {
                    continue;
                }
                float childTopBottomMargins = getTopBottomMargins(child);
                float childPreferredHeight = child->preferredSize().y();
                height = (std::max)(height, childPreferredHeight + childTopBottomMargins);
            }
        }
        else {
            float flexLeftRightPadding = getLeftRightPadding(this);
            float flexPaddedWidth = width - flexLeftRightPadding;
            Int numVisibleChildren = 0;
            for (Widget* child : children()) {
                if (child->visibility() == Visibility::Invisible) {
                    continue;
                }
                ++numVisibleChildren;
                float childLeftRightMargins = getLeftRightMargins(child);
                float childTopBottomMargins = getTopBottomMargins(child);
                float childWidth =
                    (std::max)(0.0f, flexPaddedWidth - childLeftRightMargins);
                float childPreferredHeight = child->preferredHeightForWidth(childWidth);
                height += childPreferredHeight + childTopBottomMargins;
            }
            if (numVisibleChildren > 0) {
                namespace gs = graphics::strings;
                bool hinting = (style(gs::pixel_hinting) == gs::normal);
                float gap = getGap(isRow_, this, hinting);
                height += (numVisibleChildren - 1) * gap;
            }
        }
        height += getTopBottomPadding(this);
    }
    else {
        // fixed height
        // TODO: support percentages
        float refLength = 0.0f;
        float valueIfAuto = 0.0f;
        height = h.toPx(styleMetrics(), refLength, valueIfAuto);
    }
    return height;
}

geometry::Vec2f Flex::computePreferredSize() const {
    style::LengthOrPercentageOrAuto w = preferredWidth();
    style::LengthOrPercentageOrAuto h = preferredHeight();
    geometry::Vec2f res;
    if (w.isAuto()) {
        if (h.isAuto()) {

            // Compute preferred height not knowing any width
            float height = 0;
            if (isColumn()) {
                for (Widget* child : children()) {
                    if (child->visibility() == Visibility::Invisible) {
                        continue;
                    }
                    height += child->preferredSize().y() + getTopBottomMargins(child);
                }
            }
            else {
                for (Widget* child : children()) {
                    if (child->visibility() == Visibility::Invisible) {
                        continue;
                    }
                    height = (std::max)(
                        height, child->preferredSize().y() + getTopBottomMargins(child));
                }
            }
            height += getTopBottomPadding(this);

            // Compute preferred width not knowing any height
            float width = 0;
            if (isRow()) {
                for (Widget* child : children()) {
                    if (child->visibility() == Visibility::Invisible) {
                        continue;
                    }
                    width += child->preferredSize().x() + getLeftRightMargins(child);
                }
            }
            else {
                for (Widget* child : children()) {
                    if (child->visibility() == Visibility::Invisible) {
                        continue;
                    }
                    width = (std::max)(
                        width, child->preferredSize().x() + getLeftRightMargins(child));
                }
            }
            width += getLeftRightPadding(this);

            // Add gap
            Int numVisibleChildren = 0;
            for (Widget* child : children()) {
                if (child->visibility() != Visibility::Invisible) {
                    ++numVisibleChildren;
                }
            }
            if (numVisibleChildren > 0) {
                namespace gs = graphics::strings;
                bool hinting = (style(gs::pixel_hinting) == gs::normal);
                float gap = getGap(isRow(), this, hinting);
                float totalGap = (numVisibleChildren - 1) * gap;
                if (isRow()) {
                    width += totalGap;
                }
                else {
                    height += totalGap;
                }
            }

            res = {width, height};
        }
        else {
            // (auto, fixed)
            // TODO: support percentages
            float refLength = 0.0f;
            float valueIfAuto = 0.0f;
            float height = h.toPx(styleMetrics(), refLength, valueIfAuto);
            float width = preferredWidthForHeight(height);
            res = {width, height};
        }
    }
    else {
        if (h.isAuto()) {
            // (fixed, auto)
            // TODO: support percentages
            float refLength = 0.0f;
            float valueIfAuto = 0.0f;
            float width = w.toPx(styleMetrics(), refLength, valueIfAuto);
            float height = preferredHeightForWidth(width);
            res = {width, height};
        }
        else {
            // (fixed, fixed)
            // TODO: support percentages
            float refLength = 0.0f;
            float valueIfAuto = 0.0f;
            float width = w.toPx(styleMetrics(), refLength, valueIfAuto);
            float height = h.toPx(styleMetrics(), refLength, valueIfAuto);
            res = {width, height};
        }
    }
    return res;
}

namespace {

detail::FlexData computeData(Flex* flex) {

    namespace gs = graphics::strings;
    namespace ss = style::strings;
    using detail::getLengthOrPercentageInPx;

    detail::FlexData res;
    res.flex = flex;
    res.hinting = (flex->style(gs::pixel_hinting) == gs::normal);
    res.isRow = flex->isRow();
    res.isReverse = flex->isReverse();
    res.mainAlignment = flex->style<MainAlignment>(strings::main_alignment);
    res.mainDir = 0;
    res.crossDir = 1;
    res.gap = getGap(res.isRow, flex, res.hinting);
    res.size = flex->size();

    geometry::Rect2f contentRect = flex->contentRect();
    res.contentMainPosition = contentRect.x();
    res.contentCrossPosition = contentRect.y();
    res.contentMainSize = contentRect.width();
    res.contentCrossSize = contentRect.height();

    if (!res.isRow) {
        std::swap(res.contentMainPosition, res.contentCrossPosition);
        std::swap(res.contentMainSize, res.contentCrossSize);
        std::swap(res.mainDir, res.crossDir);
    }

    return res;
}

detail::FlexChildData computeChildData(const detail::FlexData& data, Widget* child) {

    namespace ss = style::strings;
    using detail::getLengthOrPercentageInPx;

    const bool isRow = data.isRow;
    const geometry::Vec2f parentSize = data.size;

    detail::FlexChildData res;

    res.child = child;

    res.minSize = {
        getLengthOrPercentageInPx(child, strings::min_width, parentSize[0]),
        getLengthOrPercentageInPx(child, strings::min_height, parentSize[1])};

    res.maxSize = {
        getLengthOrPercentageInPx(child, strings::max_width, parentSize[0]),
        getLengthOrPercentageInPx(child, strings::max_height, parentSize[1])};

    res.maxSize[0] = std::abs(res.maxSize[0]);
    res.maxSize[1] = std::abs(res.maxSize[1]);

    res.minSize[0] = core::clamp(res.minSize[0], 0, res.maxSize[0]);
    res.minSize[1] = core::clamp(res.minSize[1], 0, res.maxSize[1]);

    // TODO: handle percentages
    float refLength = 0;
    res.mainMargins = {
        getLengthOrPercentageInPx(child, ss::margin_left, refLength),
        getLengthOrPercentageInPx(child, ss::margin_right, refLength)};
    res.crossMargins = {
        getLengthOrPercentageInPx(child, ss::margin_top, refLength),
        getLengthOrPercentageInPx(child, ss::margin_bottom, refLength)};

    if (isRow) {
        res.shrink = child->horizontalShrink();
        res.stretch = child->horizontalStretch();
        res.mainMinSize = res.minSize[0];
        res.mainMaxSize = res.maxSize[0];

        float childCrossMargins = res.crossMargins[0] + res.crossMargins[1];
        float childCrossSize =
            (std::max)(0.0f, data.contentCrossSize - childCrossMargins);
        res.mainPreferredSize = child->preferredWidthForHeight(childCrossSize);
    }
    else {
        res.shrink = child->verticalShrink();
        res.stretch = child->verticalStretch();
        res.mainMinSize = res.minSize[1];
        res.mainMaxSize = res.maxSize[1];

        std::swap(res.mainMargins, res.crossMargins);

        float childCrossMargins = res.crossMargins[0] + res.crossMargins[1];
        float childCrossSize =
            (std::max)(0.0f, data.contentCrossSize - childCrossMargins);
        res.mainPreferredSize = child->preferredHeightForWidth(childCrossSize);
    }

    // For non-stretchable or non-shrinkable child widgets, update their
    // effective min/max size based on their preferred size
    //
    if (res.shrink <= 0) {
        res.mainMinSize = (std::max)(res.mainMinSize, res.mainPreferredSize);
    }
    if (res.stretch <= 0) {
        res.mainMaxSize = (std::min)(res.mainMaxSize, res.mainPreferredSize);
    }

    return res;
}

void updateChildData(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData) {

    // Update most child data
    childData.clear();
    for (Widget* child : data.flex->children()) {
        if (child->visibility() == Visibility::Invisible) {
            continue;
        }
        childData.append(computeChildData(data, child));
    }

    // Nothing more to do if empty
    if (childData.isEmpty()) {
        return;
    }

    // If all shrink factors are equal to zero, they should behave as
    // if they are all equal to one.
    //
    // XXX Maybe we should only do this if the widget has some 'force-shrink'
    //     style, similar to 'force-stretch'.
    //
    data.totalShrink = 0;
    data.totalStretch = 0;
    for (const detail::FlexChildData& d : childData) {
        data.totalShrink += d.shrink;
        data.totalStretch += d.stretch;
    }
    if (data.totalShrink <= 0) {
        for (detail::FlexChildData& d : childData) {
            d.shrink = 1.0f;
        }
        data.totalShrink = childData.length();
    }
}

// Computes the main size available for child widgets of a Flex, that is, the
// main size of the Flex subtracted by:
// - the Flex's border
// - the Flex's padding
// - the Flex's gaps between its children
// - the fixed margins of the Flex's children
//
// Note that margins of children expressed in percentages are not yet
// implemented. When implemented, they will still not be subtracted here, but
// instead integrated within the "weight" of the FlexChildSlack. For more
// information, see Grid that already implements this.
//
// Precondition: childData.size() >= 1.
//
float computeAvailableSize(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData) {

    float gaps = static_cast<float>(childData.length() - 1) * data.gap;
    float margins = 0;
    for (detail::FlexChildData& d : childData) {
        margins += d.mainMargins[0] + d.mainMargins[1];
    }
    return (std::max)(data.contentMainSize - gaps - margins, 0.0f);
}

void normalStretch(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData,
    core::Array<detail::FlexChildSlack>& childSlacks) {

    // Initialize slacks
    float remainingTotalStretch = 0;
    childSlacks.clear();
    for (detail::FlexChildData& d : childData) {
        float stretch = d.stretch;
        float normalizedSlack = 0;
        if (stretch > 0) {
            float slack = d.mainMaxSize - d.mainPreferredSize;
            normalizedSlack = slack / stretch;
        }
        childSlacks.append({&d, stretch, normalizedSlack});
        remainingTotalStretch += stretch;
    }

    // Sort resizeArray by increasing pair(isFlexible, normalizedSlack), that
    // is, non-flexible areas first, then flexible areas, sorted by increasing
    // normalizedSlack.
    //
    std::sort(childSlacks.begin(), childSlacks.end());

    // Distribute extra size.
    //
    float remainingExtraSize = data.extraSize;
    for (detail::FlexChildSlack& childSlack : childSlacks) {
        detail::FlexChildData& d = *childSlack.flexChildData;
        float stretch = childSlack.weight;
        if (stretch > 0) {
            // Stretchable widget: we give it its preferred size + some extra size
            float maxExtraSize = d.mainMaxSize - d.mainPreferredSize;
            float extraSize = (remainingExtraSize / remainingTotalStretch) * stretch;
            extraSize = (std::min)(extraSize, maxExtraSize);
            remainingExtraSize -= extraSize;
            remainingTotalStretch -= stretch;
            d.mainSize = d.mainPreferredSize + extraSize;
        }
        else {
            // Non-stretchable widget: we give it its preferred size
            d.mainSize = d.mainPreferredSize;
        }
    }
    data.extraSizeAfterStretch = remainingExtraSize;
}

void emergencyStretch(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData) {

    float extraSize = data.availableSize - data.totalMaxSize;

    if (data.mainAlignment == MainAlignment::ForceStretch) {
        // Stretch every child past their max size
        float extraSizePerChild = extraSize / childData.length();
        for (detail::FlexChildData& d : childData) {
            d.mainSize = d.mainMaxSize + extraSizePerChild;
        }
        data.extraSizeAfterStretch = 0;
    }
    else {
        // Give every child its max size
        for (detail::FlexChildData& d : childData) {
            d.mainSize = d.mainMaxSize;
        }
        data.extraSizeAfterStretch = extraSize;
    }
}

void stretchChildren(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData,
    core::Array<detail::FlexChildSlack>& childSlacks) {

    data.totalMaxSize = 0;
    for (detail::FlexChildData& d : childData) {
        data.totalMaxSize += d.mainMaxSize;
    }
    if (data.availableSize < data.totalMaxSize && data.totalStretch > 0) {
        normalStretch(data, childData, childSlacks);
    }
    else {
        emergencyStretch(data, childData);
    }
}

void normalShrink(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData,
    core::Array<detail::FlexChildSlack>& childSlacks) {

    // Initialize slacks
    float remainingTotalShrink = 0;
    childSlacks.clear();
    for (detail::FlexChildData& d : childData) {
        // In shrink mode, we want all child areas with equal shrink
        // factor to reach their min size at the same time. So we multiply
        // the "authored shrink" by the slack, which gives:
        //
        //     shrink          = slack * authoredShrink
        //
        //     normalizedSlack = slack / shrink
        //                     = slack / (slack * authoredShrink)
        //                     = 1 / authoredShrink
        //
        float slack = d.mainPreferredSize - d.mainMinSize;
        float shrink = slack * d.shrink;
        float normalizedSlack = 0;
        if (d.shrink > 0) {
            normalizedSlack = 1.0f / d.shrink;
        }
        childSlacks.append({&d, shrink, normalizedSlack});
        remainingTotalShrink += shrink;
    }

    // Sort resizeArray by increasing pair(isFlexible, normalizedSlack), that
    // is, non-flexible areas first, then flexible areas, sorted by increasing
    // normalizedSlack.
    //
    std::sort(childSlacks.begin(), childSlacks.end());

    // Distribute extra size.
    //
    float remainingExtraSize = data.extraSize;
    for (detail::FlexChildSlack& childSlack : childSlacks) {
        detail::FlexChildData& d = *childSlack.flexChildData;
        float shrink = childSlack.weight;
        if (shrink > 0) {
            // Stretchable widget: we give it its preferred size + some extra size
            float minExtraSize = d.mainMinSize - d.mainPreferredSize;
            float extraSize = (remainingExtraSize / remainingTotalShrink) * shrink;
            extraSize = (std::max)(extraSize, minExtraSize);
            remainingExtraSize -= extraSize;
            remainingTotalShrink -= shrink;
            d.mainSize = d.mainPreferredSize + extraSize;
        }
        else {
            // Non-stretchable widget: we give it its preferred size
            d.mainSize = d.mainPreferredSize;
        }
    }
}

void emergencyShrink(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData) {

    if (data.totalMinSize > 0) {
        float k = data.availableSize / data.totalMinSize;
        for (detail::FlexChildData& d : childData) {
            d.mainSize = k * d.mainMinSize;
        }
    }
    else {
        for (detail::FlexChildData& d : childData) {
            d.mainSize = 0;
        }
    }
}

void shrinkChildren(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData,
    core::Array<detail::FlexChildSlack>& childSlacks) {

    data.totalMinSize = 0;
    for (detail::FlexChildData& d : childData) {
        data.totalMinSize += d.mainMinSize;
    }
    if (data.totalMinSize < data.availableSize) {
        normalShrink(data, childData, childSlacks);
    }
    else {
        emergencyShrink(data, childData);
    }
    data.extraSizeAfterStretch = 0;
}

} // namespace

void Flex::updateChildrenGeometry() {

    // Note: we loosely follow the algorithm and terminology from CSS Flexbox:
    // https://www.w3.org/TR/css-flexbox-1/#layout-algorithm

    namespace gs = graphics::strings;
    namespace ss = style::strings;
    using detail::getLengthOrPercentageInPx;

    // Compute / update input metrics about this Flex and its children.
    // Fast return if no visible child.
    //
    // TODO: Keep those in cache and only update them on styleChanged() / childrenChanged(), etc.
    //
    detail::FlexData data = computeData(this);
    updateChildData(data, childData_);
    if (childData_.isEmpty()) {
        return;
    }

    // Main layout algorithm: compute children main sizes.
    //
    if (data.contentMainSize <= 0) {
        // Fast computation of d.mainSize when the content size of the Flex is zero.
        for (detail::FlexChildData& d : childData_) {
            d.mainSize = 0;
        }
    }
    else {
        // Compute how much extra size should be distributed
        data.totalPreferredSize = 0;
        for (const detail::FlexChildData& childData : childData_) {
            data.totalPreferredSize += childData.mainPreferredSize;
        }
        data.availableSize = computeAvailableSize(data, childData_);
        data.extraSize = data.availableSize - data.totalPreferredSize;

        // Distribute extra size
        if (data.extraSize > 0) {
            stretchChildren(data, childData_, childSlacks_);
        }
        else {
            shrinkChildren(data, childData_, childSlacks_);
        }
    }

    // Compute children 2D sizes
    for (detail::FlexChildData& d : childData_) {
        const float crossMargins = d.crossMargins[0] + d.crossMargins[1];
        d.size[data.mainDir] = d.mainSize;
        d.size[data.crossDir] = data.contentCrossSize - crossMargins;
    }

    // Compute children 2D position
    float mainAlignmentStartSpace = 0;
    float mainAlignBetweenSpace = 0;
    if (data.mainAlignment == MainAlignment::End) {
        mainAlignmentStartSpace = data.extraSizeAfterStretch;
    }
    else if (data.mainAlignment == MainAlignment::Center) {
        mainAlignmentStartSpace = 0.5f * data.extraSizeAfterStretch;
    }
    else if (data.mainAlignment == MainAlignment::SpaceBetween) {
        if (childData_.length() > 1) {
            mainAlignBetweenSpace =
                data.extraSizeAfterStretch / (childData_.length() - 1);
        }
    }
    else if (data.mainAlignment == MainAlignment::SpaceAround) {
        mainAlignBetweenSpace = data.extraSizeAfterStretch / childData_.length();
        mainAlignmentStartSpace = 0.5f * mainAlignBetweenSpace;
    }
    else if (data.mainAlignment == MainAlignment::SpaceEvenly) {
        mainAlignBetweenSpace = data.extraSizeAfterStretch / (childData_.length() + 1);
        mainAlignmentStartSpace = mainAlignBetweenSpace;
    }
    if (data.isReverse) {
        float mainPosition =
            data.contentMainPosition + data.contentMainSize - mainAlignmentStartSpace;
        for (detail::FlexChildData& d : childData_) {
            mainPosition -= d.mainSize + d.mainMargins[1];
            d.position[data.mainDir] = mainPosition;
            d.position[data.crossDir] = data.contentCrossPosition + d.crossMargins[0];
            mainPosition -= d.mainMargins[0] + data.gap + mainAlignBetweenSpace;
        }
    }
    else {
        float mainPosition = data.contentMainPosition + mainAlignmentStartSpace;
        for (detail::FlexChildData& d : childData_) {
            mainPosition += d.mainMargins[0];
            d.position[data.mainDir] = mainPosition;
            d.position[data.crossDir] = data.contentCrossPosition + d.crossMargins[0];
            mainPosition +=
                d.mainSize + d.mainMargins[1] + data.gap + mainAlignBetweenSpace;
        }
    }

    // Compute hinting
    // Note: we may want to use the smart hinting algo from detail/layoututil.h
    bool hinting = data.hinting;
    for (detail::FlexChildData& d : childData_) {
        geometry::Vec2f p1 = d.position;
        geometry::Vec2f p2 = d.position + d.size;
        geometry::Vec2f hp1 = {hinted(p1[0], hinting), hinted(p1[1], hinting)};
        geometry::Vec2f hp2 = {hinted(p2[0], hinting), hinted(p2[1], hinting)};
        d.hPosition = hp1;
        d.hSize = hp2 - hp1;
    }

    // Update children geometry
    for (detail::FlexChildData& d : childData_) {
        d.child->updateGeometry(d.hPosition, d.hSize);
    }
}

} // namespace vgc::ui
