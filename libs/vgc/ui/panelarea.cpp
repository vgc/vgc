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

#include <vgc/ui/panelarea.h>

#include <vgc/core/colors.h>
#include <vgc/graphics/strings.h>
#include <vgc/style/parse.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

VGC_DEFINE_ENUM(
    PanelAreaType,
    (HorizontalSplit, "HorizontalSplit"),
    (VerticalSplit, "VerticalSplit"),
    (Tabs, "Tabs"))

PanelArea::PanelArea(PanelAreaType type)
    : Widget()
    , type_(type) {

    addStyleClass(strings::PanelArea);
}

PanelAreaPtr PanelArea::create(PanelAreaType type) {
    return PanelAreaPtr(new PanelArea(type));
}

void PanelArea::setType(PanelAreaType type) {
    if (isSplit_(type_) != isSplit_(type) && numChildren() > 0) {
        VGC_WARNING(
            LogVgcUi,
            "Changing the type of {} from {} to {}. This is only possible for panel "
            "areas without children, so all current children ({}) are destroyed.",
            ptr(this),
            type_,
            type,
            numChildren());
        while (lastChild()) {
            lastChild()->destroy();
        }
    }
    type_ = type;
    requestGeometryUpdate();
    requestRepaint();
}

namespace {

// Fetches minSize and maxSize from style, and enforce the following inequalities:
//
//     0 <= minSize <= preferredSize <= maxSize
//
void updateMinMaxSizes(
    detail::PanelAreaSplitDataArray& splitData,
    Int mainDir,
    float mainSize,
    const style::Metrics& styleMetrics) {

    float scaleFactorInv = 1.0f / styleMetrics.scaleFactor();
    core::StringId minSizeClass = strings::min_width;
    core::StringId maxSizeClass = strings::max_width;
    if (mainDir) {
        minSizeClass = strings::min_height;
        maxSizeClass = strings::max_height;
    }

    for (detail::PanelAreaSplitData& data : splitData) {
        using style::LengthOrPercentage;
        data.minSizeStyle = data.childArea->style(minSizeClass).to<LengthOrPercentage>();
        data.maxSizeStyle = data.childArea->style(maxSizeClass).to<LengthOrPercentage>();
        data.minSize = data.minSizeStyle.toPx(styleMetrics, mainSize);
        data.maxSize = data.maxSizeStyle.toPx(styleMetrics, mainSize);
        data.maxSize = std::abs(data.maxSize);
        data.minSize = core::clamp(data.minSize, 0, data.maxSize);
        data.minSizeInDp = data.minSize * scaleFactorInv;
        data.maxSizeInDp = data.maxSize * scaleFactorInv;
        data.preferredSizeInDp =
            core::clamp(data.preferredSizeInDp, data.minSizeInDp, data.maxSizeInDp);
    }
}

void updateStretch(detail::PanelAreaSplitDataArray& splitData, Int mainDir) {

    core::StringId stretchClass = strings::horizontal_stretch;
    if (mainDir) {
        stretchClass = strings::vertical_stretch;
    }

    // Update all stretch factors and compute their total.
    //
    float totalStretch = 0;
    for (detail::PanelAreaSplitData& data : splitData) {
        data.stretch = std::abs(data.childArea->style(stretchClass).toFloat());
        totalStretch += data.stretch;
    }

    // If all stretch factors are equal to zero, it should behave as if all
    // stretch factors are in fact equal to one.
    //
    if (totalStretch < 1e-6f) {
        for (detail::PanelAreaSplitData& data : splitData) {
            data.stretch = 1.0f;
        }
    }
}

float computeTotalPreferredSizeInDp(const detail::PanelAreaSplitDataArray& splitData) {
    float res = 0;
    for (const detail::PanelAreaSplitData& data : splitData) {
        res += data.preferredSizeInDp;
    }
    return res;
}

void normalStretch(
    detail::PanelAreaSplitDataArray& splitData,
    detail::PanelAreaResizeArray& resizeArray,
    float remainingExtraSizeInDp,
    float scaleFactor) {

    // Initialize resizeArray
    float remainingTotalStretch = 0;
    resizeArray.clear();
    for (detail::PanelAreaSplitData& data : splitData) {
        float stretch = data.stretch;
        float normalizedSlack = 0;
        if (stretch > 0) {
            float slack = data.maxSizeInDp - data.preferredSizeInDp;
            normalizedSlack = slack / stretch;
        }
        resizeArray.append({&data, stretch, normalizedSlack});
        remainingTotalStretch += stretch;
    }

    // Sort resizeArray by increasing pair(isFlexible, normalizedSlack), that
    // is, non-flexible areas first, then flexible areas, sorted by increasing
    // normalizedSlack.
    //
    std::sort(resizeArray.begin(), resizeArray.end());

    // Distribute extra size.
    //
    for (detail::PanelAreaResizeData& resizeData : resizeArray) {
        detail::PanelAreaSplitData& data = *resizeData.splitData;
        float stretch = resizeData.stretch;
        if (stretch > 0) {
            // Stretchable area: we give it its preferred size + some extra size
            float maxExtraSizeInDp = data.maxSizeInDp - data.preferredSizeInDp;
            float extraSizeInDp =
                (remainingExtraSizeInDp / remainingTotalStretch) * stretch;
            extraSizeInDp = (std::min)(extraSizeInDp, maxExtraSizeInDp);
            remainingExtraSizeInDp -= extraSizeInDp;
            remainingTotalStretch -= stretch;
            data.size = (data.preferredSizeInDp + extraSizeInDp) * scaleFactor;
        }
        else {
            // Non-stretchable area: we give it its preferred size
            data.size = data.preferredSizeInDp * scaleFactor;
        }
    }
}

void emergencyStretch(
    detail::PanelAreaSplitDataArray& splitData,
    float mainSizeInDp,
    float totalMaxSizeInDp,
    float scaleFactor) {

    // Compute total stretch. We know it's > 0 (see updateStretch()).
    float totalStretch = 0;
    for (detail::PanelAreaSplitData& data : splitData) {
        totalStretch += data.stretch;
    }
    float totalStretchInv = 1.0f / totalStretch;

    // Distribute extra size
    float extraSizeInDp = mainSizeInDp - totalMaxSizeInDp;
    for (detail::PanelAreaSplitData& data : splitData) {
        float maxSizeInDp =
            (data.stretch > 0) ? data.maxSizeInDp : data.preferredSizeInDp;
        data.size =
            scaleFactor * (maxSizeInDp + extraSizeInDp * data.stretch * totalStretchInv);
    }
}

void stretchChildren(
    detail::PanelAreaSplitDataArray& splitData,
    detail::PanelAreaResizeArray& resizeArray,
    float mainSizeInDp,
    float remainingExtraSizeInDp,
    float scaleFactor) {

    float totalMaxSizeInDp = 0;
    for (detail::PanelAreaSplitData& data : splitData) {
        float maxSizeInDp =
            (data.stretch > 0) ? data.maxSizeInDp : data.preferredSizeInDp;
        totalMaxSizeInDp += maxSizeInDp;
    }
    if (mainSizeInDp < totalMaxSizeInDp) {
        normalStretch(splitData, resizeArray, remainingExtraSizeInDp, scaleFactor);
    }
    else {
        emergencyStretch(splitData, mainSizeInDp, totalMaxSizeInDp, scaleFactor);
    }
}

void normalShrink(
    detail::PanelAreaSplitDataArray& splitData,
    detail::PanelAreaResizeArray& resizeArray,
    float remainingExtraSizeInDp,
    float scaleFactor) {

    // Initialize resizeArray
    resizeArray.clear();
    float remainingTotalStretch = 0;
    for (detail::PanelAreaSplitData& data : splitData) {

        // In shrink mode, we want all child areas with equal stretch
        // factor to reach their min size at the same time. So we multiply
        // the "authored stretch" by the slack, which gives:
        //
        //     stretch         = slack * authoredStretch
        //
        //     normalizedSlack = slack / stretch
        //                     = slack / (slack * authoredStretch)
        //                     = 1 / authoredStretch
        //
        float slack = data.preferredSizeInDp - data.minSizeInDp;
        float stretch = slack * data.stretch;
        float normalizedSlack = 0;
        if (data.stretch > 0) {
            normalizedSlack = 1.0f / data.stretch;
        }
        resizeArray.append({&data, stretch, normalizedSlack});
        remainingTotalStretch += stretch;
    }

    // Sort resizeArray by increasing pair(isFlexible, normalizedSlack), that
    // is, non-flexible areas first, then flexible areas, sorted by increasing
    // normalizedSlack.
    //
    std::sort(resizeArray.begin(), resizeArray.end());

    // Distribute extra size.
    //
    for (detail::PanelAreaResizeData& resizeData : resizeArray) {
        detail::PanelAreaSplitData& data = *resizeData.splitData;
        float stretch = resizeData.stretch;
        if (stretch > 0) {
            // Stretchable area: we give it its preferred size + some extra size
            float minExtraSizeInDp = data.minSizeInDp - data.preferredSizeInDp;
            float extraSizeInDp =
                (remainingExtraSizeInDp / remainingTotalStretch) * stretch;
            extraSizeInDp = (std::max)(extraSizeInDp, minExtraSizeInDp);
            remainingExtraSizeInDp -= extraSizeInDp;
            remainingTotalStretch -= stretch;
            data.size = (data.preferredSizeInDp + extraSizeInDp) * scaleFactor;
        }
        else {
            // Non-stretchable area: we give it its preferred size
            data.size = data.preferredSizeInDp * scaleFactor;
        }
    }
}

void emergencyShrink(
    detail::PanelAreaSplitDataArray& splitData,
    float mainSizeInDp,
    float totalMinSizeInDp,
    float scaleFactor) {

    if (totalMinSizeInDp > 0) {
        float k = scaleFactor * mainSizeInDp / totalMinSizeInDp;
        for (detail::PanelAreaSplitData& data : splitData) {
            data.size = k * data.minSizeInDp;
        }
    }
    else {
        for (detail::PanelAreaSplitData& data : splitData) {
            data.size = 0;
        }
    }
}

void shrinkChildren(
    detail::PanelAreaSplitDataArray& splitData,
    detail::PanelAreaResizeArray& resizeArray,
    float mainSizeInDp,
    float remainingExtraSizeInDp,
    float scaleFactor) {

    float totalMinSizeInDp = 0;
    for (detail::PanelAreaSplitData& data : splitData) {
        totalMinSizeInDp += data.minSizeInDp;
    }
    if (totalMinSizeInDp < mainSizeInDp) {
        normalShrink(splitData, resizeArray, remainingExtraSizeInDp, scaleFactor);
    }
    else {
        emergencyShrink(splitData, mainSizeInDp, totalMinSizeInDp, scaleFactor);
    }
}

float hinted(float x, bool hinting) {
    return hinting ? std::round(x) : x;
}

// Returns the average of all sizes satisfying size >= 0.
// Returns fallback if no sizes satisfy size >= 0.
//
float computeAveragePositiveSizes(
    const detail::PanelAreaSplitDataArray& splitData,
    float fallback = 1.0f) {

    Int numPositiveSizes = 0;
    float totalSize = 0;
    for (const detail::PanelAreaSplitData& data : splitData) {
        if (data.size >= 0) {
            ++numPositiveSizes;
            totalSize += data.size;
        }
    }
    if (numPositiveSizes > 0) {
        return totalSize / numPositiveSizes;
    }
    else {
        return fallback;
    }
}

} // namespace

void PanelArea::populateStyleSpecTable(style::SpecTable* table) {

    if (!table->setRegistered(staticClassName())) {
        return;
    }

    using namespace strings;
    using namespace style::literals;
    using style::Length;
    using style::StyleValue;

    auto ten_l = StyleValue::custom(Length(10_dp));
    auto zero_l = StyleValue::custom(Length(0_dp));
    auto transp = StyleValue::custom(core::colors::transparent);

    table->insert(handle_size, ten_l, false, &Length::parse);
    table->insert(handle_hovered_size, zero_l, false, &Length::parse);
    table->insert(handle_hovered_color, transp, false, &style::parseColor);

    SuperClass::populateStyleSpecTable(table);
}

void PanelArea::onStyleChanged() {

    using namespace strings;
    using style::Length;

    halfHandleSize_ = 0.5f * detail::getLengthInPx(this, handle_size);
    halfHandleHoveredSize_ = 0.5f * detail::getLengthInPx(this, handle_hovered_size);
    handleHoveredColor_ = detail::getColor(this, handle_hovered_color);

    requestGeometryUpdate();
    requestRepaint();

    SuperClass::onStyleChanged();
}

void PanelArea::onWidgetAdded(Widget*, bool) {
    onChildrenChanged_();
}

void PanelArea::onWidgetRemoved(Widget*) {
    onChildrenChanged_();
}

bool PanelArea::onMouseEnter() {
    return true;
}

bool PanelArea::onMouseLeave() {
    setHoveredSplitHandle_(-1);
    return true;
}

Widget* PanelArea::computeHoverChainChild(MouseEvent* event) const {
    Int hoveredSplitHandle = computeHoveredSplitHandle_(event->position());
    if (hoveredSplitHandle != -1) {
        return nullptr;
    }
    return Widget::computeHoverChainChild(event);
}

void PanelArea::preMouseMove(MouseEvent* event) {
    if (!isHoverLocked() && draggedSplitHandle_ == -1) {
        updateHoveredSplitHandle_(event->position());
    }
    else {
        // Preserve current hoveredSplitHandle_ if we're dragging it
    }
}

bool PanelArea::onMouseMove(MouseEvent* event) {
    if (draggedSplitHandle_ != -1) {
        continueDragging_(event->position());
        return true;
    }
    return false;
}

bool PanelArea::onMousePress(MouseEvent* event) {
    if (event->button() == ui::MouseButton::Left) {
        draggedSplitHandle_ = hoveredSplitHandle_;
        if (draggedSplitHandle_ != -1) {
            startDragging_(event->position());
            return true;
        }
    }
    return false;
}

bool PanelArea::onMouseRelease(MouseEvent* event) {
    if (event->button() == ui::MouseButton::Left) {
        if (draggedSplitHandle_ != -1) {
            draggedSplitHandle_ = -1;
            stopDragging_(event->position());
            return true;
        }
    }
    return false;
}

void PanelArea::onResize() {
    SuperClass::onResize();
}

void PanelArea::updateChildrenGeometry() {

    // TODO: supports isVisible, isCollapsed, and padding/gap/border

    // The algorithm for updating PanelArea sizes is similar to Flex, except
    // that "preferred-size" doesn't come from the stylesheet, but from the
    // user manually dragging a splitter.

    // Useful: https://drafts.csswg.org/css3-tables-algorithms/Overview.src.htm

    namespace gs = graphics::strings;

    // Handle Tabs case
    if (type() == PanelAreaType::Tabs) {
        // For now, we assume there is only one tab, and we simply draw the widget
        // as the full size.
        Widget* child = firstChild();
        if (child) {
            child->updateGeometry(rect());
        }
        return;
    }

    // Handle emtpy Split case
    if (splitData_.isEmpty()) {
        return;
    }

    // Get general metrics, and handle negative mainDir case
    float scaleFactorInv = 1.0f / styleMetrics().scaleFactor();
    bool hinting = (style(gs::pixel_hinting) == gs::normal);
    Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
    Int crossDir = 1 - mainDir;
    float mainSize = size()[mainDir];
    float mainSizeInDp = mainSize * scaleFactorInv;
    float crossSize = size()[crossDir];
    if (mainSize <= 0) {
        geometry::Vec2f childPosition(0, 0);
        geometry::Vec2f childSize;
        childSize[mainDir] = 0;
        childSize[crossDir] = crossSize;
        for (const SplitData& data : splitData_) {
            data.childArea->updateGeometry(childPosition, childSize);
        }
        return;
    }

    // Update min/max/stretch style values
    updateMinMaxSizes(splitData_, mainDir, mainSize, styleMetrics());
    updateStretch(splitData_, mainDir);

    // Compute how much extra dp should be distributed compared to previous sizes
    float totalPreferredSizeInDp = computeTotalPreferredSizeInDp(splitData_);
    float extraSizeInDp = mainSizeInDp - totalPreferredSizeInDp;

    // Distribute extra dp
    float scaleFactor = styleMetrics().scaleFactor();
    if (extraSizeInDp > 0) {
        stretchChildren(
            splitData_, resizeArray_, mainSizeInDp, extraSizeInDp, scaleFactor);
    }
    else {
        shrinkChildren(
            splitData_, resizeArray_, mainSizeInDp, extraSizeInDp, scaleFactor);
    }

    // Update positions based on sizes
    float position = 0;
    for (SplitData& data : splitData_) {
        data.position = position;
        position += data.size;
    }

    // Compute hinting
    // Note: we may want to use the smart hinting algo from detail/layoututil.h
    for (SplitData& data : splitData_) {
        float p1 = data.position;
        float p2 = data.position + data.size;
        data.hPosition = hinted(p1, hinting);
        data.hSize = hinted(p2, hinting) - data.hPosition;
    }

    // Update children geometry
    geometry::Vec2f childSize;
    geometry::Vec2f childPosition;
    childSize[crossDir] = crossSize;
    childPosition[crossDir] = 0;
    for (SplitData& data : splitData_) {
        childSize[mainDir] = data.hSize;
        childPosition[mainDir] = data.hPosition;
        data.childArea->updateGeometry(childPosition, childSize);
    }
}

void PanelArea::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void PanelArea::onPaintDraw(graphics::Engine* engine, PaintOptions options) {
    SuperClass::onPaintDraw(engine, options);

    namespace gs = graphics::strings;

    if (hoveredSplitHandle_ != -1) {
        core::FloatArray a;

        Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
        Int crossDir = 1 - mainDir;
        float crossSize = size()[crossDir];

        float handlePos = splitData_[hoveredSplitHandle_].hPosition;
        geometry::Vec2f handleRectPos;
        geometry::Vec2f handleRectSize;
        handleRectPos[mainDir] = handlePos - halfHandleHoveredSize_;
        handleRectSize[mainDir] = 2 * halfHandleHoveredSize_;
        handleRectSize[crossDir] = crossSize;

        geometry::Rect2f handleRect =
            geometry::Rect2f::fromPositionSize(handleRectPos, handleRectSize);
        detail::insertRect(a, handleHoveredColor_, handleRect);

        engine->updateVertexBufferData(triangles_, std::move(a));
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(triangles_);
    }
}

void PanelArea::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    triangles_.reset();
}

void PanelArea::onChildrenChanged_() {

    // There's nothing to do if we're not a splitter.
    //
    if (!isSplit()) {
        return;
    }

    // Defer updating if we're already in the middle of some updating. We'll do
    // a second pass anyway at the end of this function.
    //
    if (isUpdatingSplitData_) {
        return;
    }
    isUpdatingSplitData_ = true;

    // While we iterate children(), we remember those who are not PanelArea
    // in order to delete them later.
    //
    core::Array<Widget*> toRemove;

    // Update splitData_ to keep it in sync with children().
    //
    // Loop invariant:
    //
    // The half-open range [splitData_.begin(), ..., nextSplitData) contains the same
    // childAreas as in children() up to the current loop iteration.
    //
    auto nextSplitData = splitData_.begin();
    Int numAreas = 0;
    for (Widget* child : children()) {
        PanelArea* childArea = dynamic_cast<PanelArea*>(child);
        if (!childArea) {
            VGC_WARNING(
                LogVgcUi,
                "PanelArea splitters only support PanelAreas as children. "
                "Destroying unsupported child.");
            toRemove.append(child);
            continue;
        }
        ++numAreas;

        // Find SplitData `it` such that `it->childArea == childArea`.
        auto end = splitData_.end();
        auto it = std::find_if(nextSplitData, end, [childArea](const SplitData& data) {
            return data.childArea == childArea;
        });
        const bool found = (it != end);

        // Ensure next entry in splitData_ satisfies data.childArea == childArea
        if (found) {
            nextSplitData = splitData_.relocate(it, nextSplitData);
        }
        else {
            float size = -1;     // updated after this loop
            float position = -1; // updated after this loop
            nextSplitData = splitData_.insert(nextSplitData, {childArea, position, size});
        }
        ++nextSplitData;
    }
    splitData_.erase(nextSplitData, splitData_.end());

    // Give a new size to new elements
    float averageSize = computeAveragePositiveSizes(splitData_);
    float scaleFactor = styleMetrics().scaleFactor();
    float invScaleFactor = 1.0f / scaleFactor;
    for (SplitData& data : splitData_) {
        if (data.size < 0) {
            data.size = averageSize;
            data.preferredSizeInDp = data.size * invScaleFactor;
        }
    }

    // Make all splitters interactive except the first
    bool isInteractive = false;
    for (SplitData& data : splitData_) {
        data.isInteractive = isInteractive;
        isInteractive = true;
    }

    // Remove children which are not supported. This implicitly calls
    // onChildrenChanged_, which is swallowed thanks to isUpdatingSplitData_.
    //
    for (Widget* child : toRemove) {
        child->destroy();
    }

    // Make a second pass, in case destroying the children above had side
    // effects such as adding or removing other children. If there was no side
    // effects, then the second pass should be a no-op in which toRemove is
    // empty, so there will be no third pass.
    //
    isUpdatingSplitData_ = false;
    if (!toRemove.isEmpty()) {
        onChildrenChanged_();
    }
    else {
        // request update in the inner-most pass.
        requestGeometryUpdate();
    }
}

// Post-condition: returns either -1 (no handle) or a value in [1..n-1]
// The value "0" makes no sense as it would correspond to a handle located
// on the left of the first split area: there is no handle there.
Int PanelArea::computeHoveredSplitHandle_(const geometry::Vec2f& position) const {
    if (isSplit()) {
        Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
        float pos = position[mainDir];
        for (Int i = 1; i < splitData_.length(); ++i) {
            const SplitData& data = splitData_[i];
            if (data.isInteractive && std::abs(pos - data.position) < halfHandleSize_) {
                return i;
            }
        }
    }
    return -1;
}

void PanelArea::updateHoveredSplitHandle_(const geometry::Vec2f& position) {
    Int hoveredSplitHandle = computeHoveredSplitHandle_(position);
    setHoveredSplitHandle_(hoveredSplitHandle);
}

void PanelArea::setHoveredSplitHandle_(Int hoveredSplitHandle) {
    if (hoveredSplitHandle_ != hoveredSplitHandle) {
        hoveredSplitHandle_ = hoveredSplitHandle;
        if (hoveredSplitHandle_ == -1) {
            cursorChanger_.clear();
        }
        else {
            Qt::CursorShape cursor = (type() == PanelAreaType::HorizontalSplit)
                                         ? Qt::SplitHCursor
                                         : Qt::SplitVCursor;
            cursorChanger_.set(cursor);
        }
        requestRepaint();
    }
}

// Pre-condition: draggedSplitHandle_ in [1..n-1]
void PanelArea::startDragging_(const geometry::Vec2f& position) {

    Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
    dragStartMousePosition_ = position[mainDir];

    const SplitData& splitDataBefore = splitData_[draggedSplitHandle_ - 1];
    const SplitData& splitDataAfter = splitData_[draggedSplitHandle_];
    dragStartSplitSizeBefore_ = splitDataBefore.size;
    dragStartSplitSizeAfter_ = splitDataAfter.size;
}

void PanelArea::continueDragging_(const geometry::Vec2f& position) {

    // Get raw deltaPosition (before applying min/max constraints)
    Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
    float mousePosition = position[mainDir];
    float deltaPosition = mousePosition - dragStartMousePosition_;

    // Get min/max constraints of child areas before and after the splitter
    SplitData& splitDataBefore = splitData_[draggedSplitHandle_ - 1];
    SplitData& splitDataAfter = splitData_[draggedSplitHandle_];
    float minSizeBefore = splitDataBefore.minSize;
    float maxSizeBefore = splitDataBefore.maxSize;
    float minSizeAfter = splitDataAfter.minSize;
    float maxSizeAfter = splitDataAfter.maxSize;

    // Apply constraints of the child area after the split
    float newSplitSizeAfter = dragStartSplitSizeAfter_ - deltaPosition;
    newSplitSizeAfter = core::clamp(newSplitSizeAfter, minSizeAfter, maxSizeAfter);
    deltaPosition = dragStartSplitSizeAfter_ - newSplitSizeAfter;

    // Apply constraints of the child area before the split
    float newSplitSizeBefore = dragStartSplitSizeBefore_ + deltaPosition;
    newSplitSizeBefore = core::clamp(newSplitSizeBefore, minSizeBefore, maxSizeBefore);
    deltaPosition = newSplitSizeBefore - dragStartSplitSizeBefore_;
    newSplitSizeAfter = dragStartSplitSizeAfter_ - deltaPosition;

    // Update splitData_
    splitDataBefore.size = newSplitSizeBefore;
    splitDataAfter.size = newSplitSizeAfter;
    splitDataAfter.position = splitDataBefore.position + newSplitSizeBefore;
    float scaleFactor = styleMetrics().scaleFactor();
    float invScaleFactor = 1.0f / scaleFactor;
    for (SplitData& data : splitData_) {
        data.preferredSizeInDp = data.size * invScaleFactor;
    }

    requestGeometryUpdate();
    requestRepaint();
}

void PanelArea::stopDragging_(const geometry::Vec2f& position) {
    updateHoveredSplitHandle_(position);
}

} // namespace vgc::ui
