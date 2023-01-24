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

VGC_DEFINE_ENUM( //
    MainAlignment,
    (Start, "start"),
    (End, "end"),
    (Center, "center"))

VGC_DEFINE_ENUM(
    MainSpacing,
    (Packed, "packed"),
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

style::Value parseMainAlignment(style::TokenIterator begin, style::TokenIterator end_) {

    using namespace strings;

    style::Value res = style::Value::invalid();

    // There must be exactly one token
    if (end_ != begin + 1) {
        return res;
    }
    style::TokenType t = begin->type();

    // The token should be an identifier
    if (t != style::TokenType::Identifier) {
        return res;
    }
    std::string_view s = begin->stringValue();

    // Converts the identifier to style::Value storing a MainAlignment enum value.
    if (s == start) {
        res = style::Value::custom(MainAlignment::Start);
    }
    else if (s == end) {
        res = style::Value::custom(MainAlignment::End);
    }
    else if (s == center) {
        res = style::Value::custom(MainAlignment::Center);
    }

    return res;
}

style::Value parseMainSpacing(style::TokenIterator begin, style::TokenIterator end_) {

    using namespace strings;

    style::Value res = style::Value::invalid();

    // There must be exactly one token
    if (end_ != begin + 1) {
        return res;
    }
    style::TokenType t = begin->type();

    // The token should be an identifier
    if (t != style::TokenType::Identifier) {
        return res;
    }
    std::string_view s = begin->stringValue();

    // Converts the identifier to style::Value storing a MainAlignment enum value.
    if (s == packed) {
        res = style::Value::custom(MainSpacing::Packed);
    }
    else if (s == space_between) {
        res = style::Value::custom(MainSpacing::SpaceBetween);
    }
    else if (s == space_around) {
        res = style::Value::custom(MainSpacing::SpaceAround);
    }
    else if (s == space_evenly) {
        res = style::Value::custom(MainSpacing::SpaceEvenly);
    }
    else if (s == force_stretch) {
        res = style::Value::custom(MainSpacing::ForceStretch);
    }

    return res;
}

} // namespace

void Flex::populateStyleSpecTable(style::SpecTable* table) {
    if (!table->setRegistered(staticClassName())) {
        return;
    }
    using namespace strings;
    auto start_ma = style::Value::custom(MainAlignment::Start);
    auto packed_ms = style::Value::custom(MainSpacing::Packed);
    table->insert(main_alignment, start_ma, false, &parseMainAlignment);
    table->insert(main_spacing, packed_ms, false, &parseMainSpacing);
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
    res.mainSpacing = flex->style<MainSpacing>(strings::main_spacing);
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
    return data.contentMainSize - gaps - margins;
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

    if (data.mainSpacing == MainSpacing::ForceStretch) {
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
    data.extraSizeAfterShrink = 0;
    data.flex->setClippingEnabled(false);
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
    data.extraSizeAfterShrink = 0;
    data.flex->setClippingEnabled(false);
}

void emergencyShrink(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData) {

    [[maybe_unused]] bool overflow;
    float totalGivenSize;

    if (data.mainSpacing == MainSpacing::ForceStretch) {

        // Shrink every child past (or equal to) their min size.
        //
        // If availableSize < 0, this means that the sum of gaps and margins is
        // larger than the Flex content size. In this case, we clamp all
        // children size to zero (rather than a negative size), which causes
        // overflow.
        //
        // If availableSize == 0, we also set all children size to zero, but
        // there is technically no overflow.
        //
        // If availableSize > 0, this is the normal case where we shrink
        // the children past their min size, without overflow.
        //
        if (data.availableSize <= 0) { // [2]
            overflow = (data.availableSize < 0);
            for (detail::FlexChildData& d : childData) {
                d.mainSize = 0;
            }
            totalGivenSize = 0;
        }
        else {
            // Note: we know that `data.totalMinSize > 0`, since:
            // [1] data.totalMinSize  >= data.availableSize (see shrinkChildren())
            // [2] data.availableSize > 0
            overflow = false;
            float k = data.availableSize / data.totalMinSize;
            for (detail::FlexChildData& d : childData) {
                d.mainSize = k * d.mainMinSize;
            }
            totalGivenSize = data.availableSize;
        }
    }
    else {
        // Give every child its min size.
        //
        overflow = true;
        for (detail::FlexChildData& d : childData) {
            d.mainSize = d.mainMinSize;
        }
        totalGivenSize = data.totalMinSize;
    }
    data.extraSizeAfterShrink = totalGivenSize - data.availableSize;

    // Note on clipping during emergency shrink:
    //
    // We have the choice to enable clipping either:
    // 1. unconditionally as soon as we enter this function
    // 2. only when a child rect() overflows outside the Flex contentRect()
    //
    // In theory, option 2 seems preferrable: there is no reason
    // to clip if all the child rects are within the Flex contentRect.
    //
    // However, in practice:
    //
    // - When MainAlignment is not ForceStretch, both are equivalent anyway,
    //   as there is always overflow as soon as we enter this function.
    //
    // - When MainAlignment is ForceStretch, overflow only starts when all
    //   child rects become zero, in which case it is (in theory) pointless to
    //   clip as there is nothing to draw. In practice, it useful to clip both
    //   when the rect is zero and when the rect is non-zero, because in both
    //   cases, the child's rect is still smaller than its minSize, and therefore
    //   it is likely that the child might draw outside its rect, which looks
    //   like a bug (a bug of the child, but hard to blame the child when we
    //   didn't respect its advertised minSize). In these cases, it often looks
    //   even worse to start clipping once the child rects become zero, as it
    //   introduces a discontinuity.
    //
    // In the future, we may want to make this configurable in the style sheet,
    // maybe something like:
    //
    // flex-clip: always                         always enable clipping (i.e., even on stretch)
    //          | on-overflow-and-forced-shrink  enable clipping on forced shrink and overflow
    //          | on-overflow                    enable clipping only on actual overflow
    //          | never                          never enable clipping (i.e., even on overflow)
    //
    // For now, we simply keep a bool here to be able to test the two modes
    // "on-overflow-and-forced-shrink" and "on-overflow".
    //
    constexpr bool clipOnOverflowAndForcedShrink = true;
    data.flex->setClippingEnabled(clipOnOverflowAndForcedShrink || overflow);
}

void shrinkChildren(
    detail::FlexData& data,
    core::Array<detail::FlexChildData>& childData,
    core::Array<detail::FlexChildSlack>& childSlacks) {

    data.totalMinSize = 0;
    for (detail::FlexChildData& d : childData) {
        data.totalMinSize += d.mainMinSize;
    }
    if (data.totalMinSize < data.availableSize) { // [1] see emergencyShrink()
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
        setClippingEnabled(false);
        return;
    }

    // Compute how much extra size should be distributed in the main axis
    data.totalPreferredSize = 0;
    for (const detail::FlexChildData& childData : childData_) {
        data.totalPreferredSize += childData.mainPreferredSize;
    }
    data.availableSize = computeAvailableSize(data, childData_);
    data.extraSize = data.availableSize - data.totalPreferredSize;

    // Distribute extra size in the main axis
    if (data.extraSize > 0) {
        stretchChildren(data, childData_, childSlacks_);
    }
    else {
        shrinkChildren(data, childData_, childSlacks_);
    }

    // Compute children 2D sizes
    for (detail::FlexChildData& d : childData_) {
        const float crossMargins = d.crossMargins[0] + d.crossMargins[1];
        d.size[data.mainDir] = d.mainSize;
        d.size[data.crossDir] = data.contentCrossSize - crossMargins;
    }

    // Compute children 2D position
    float mainAlignBetweenSpace = 0;
    float mainAlignOffsetSpace = 0;
    switch (data.mainSpacing) {
    case MainSpacing::Packed:
    case MainSpacing::ForceStretch:
        mainAlignBetweenSpace = 0;
        mainAlignOffsetSpace = 0;
        break;
    case MainSpacing::SpaceBetween:
        mainAlignBetweenSpace =
            (childData_.length() > 1)
                ? data.extraSizeAfterStretch / (childData_.length() - 1)
                : 0;
        mainAlignOffsetSpace = 0;
        break;
    case MainSpacing::SpaceAround:
        mainAlignBetweenSpace = data.extraSizeAfterStretch / childData_.length();
        mainAlignOffsetSpace = 0.5f * mainAlignBetweenSpace;
        break;
    case MainSpacing::SpaceEvenly:
        mainAlignBetweenSpace = data.extraSizeAfterStretch / (childData_.length() + 1);
        mainAlignOffsetSpace = mainAlignBetweenSpace;
        break;
    }
    float remainingExtraSpace =     //
        data.extraSizeAfterStretch  //
        - data.extraSizeAfterShrink //
        - mainAlignBetweenSpace * (childData_.length() - 1);
    float mainAlignmentStartSpace = 0;
    switch (data.mainAlignment) {
    case MainAlignment::Start:
        mainAlignmentStartSpace = mainAlignOffsetSpace;
        break;
    case MainAlignment::End:
        mainAlignmentStartSpace = remainingExtraSpace - mainAlignOffsetSpace;

        break;
    case MainAlignment::Center:
        mainAlignmentStartSpace = 0.5f * remainingExtraSpace;
        break;
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
