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

#include <vgc/graphics/strings.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

PanelArea::PanelArea(PanelAreaType type)
    : Widget()
    , type_(type) {

    addStyleClass(strings::PanelArea);
}

PanelAreaPtr PanelArea::create(PanelAreaType type) {
    return PanelAreaPtr(new PanelArea(type));
}

namespace {

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

float computeTotalSize(const detail::PanelAreaSplitDataArray& splitData) {
    float totalSize = 0;
    for (const detail::PanelAreaSplitData& data : splitData) {
        totalSize += data.size;
    }
    return totalSize;
}

float hinted(float x, bool hinting) {
    return hinting ? std::round(x) : x;
}

} // namespace

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

Widget* PanelArea::computeHoverChainChild(const geometry::Vec2f& position) const {
    Int hoveredSplitHandle = computeHoveredSplitHandle_(position);
    if (hoveredSplitHandle != -1) {
        return nullptr;
    }
    return Widget::computeHoverChainChild(position);
}

void PanelArea::preMouseMove(MouseEvent* event) {
    if (draggedSplitHandle_ == -1) {
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

    // For now, this is the most minimal implementation possible. It does not
    // take into account whether child area are visible or collapsed, and there
    // is no padding, gap or border between children.

    namespace gs = graphics::strings;

    if (type() == PanelAreaType::Tabs) {
        // For now, we assume there is only one tab, and we simply draw the widget
        // as the full size.
        Widget* child = firstChild();
        if (child) {
            child->updateGeometry(rect());
        }
    }
    else {
        if (splitData_.isEmpty()) {
            return;
        }

        bool hinting = (style(gs::pixel_hinting) == gs::normal);
        Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
        Int crossDir = 1 - mainDir;
        float mainSize = size()[mainDir];
        float crossSize = size()[crossDir];
        geometry::Vec2f childPosition;
        geometry::Vec2f childSize;
        childSize[crossDir] = crossSize;

        if (mainSize > 0) {

            // Update sizes such that total size equals this panel area's size
            float totalSize = computeTotalSize(splitData_);
            float ratio = mainSize / totalSize;
            for (SplitData& data : splitData_) {
                data.size *= ratio;
            }

            // Update positions based on sizes
            float position = 0;
            for (SplitData& data : splitData_) {
                data.position = position;
                position += data.size;
            }

            // Compute hinting
            // Note: we may want to use the smart hinting algo from detail/layoututil.h,
            // which is based on sizes, and thus perform this step before updating
            // the positions based on sizes.
            for (SplitData& data : splitData_) {
                float p1 = data.position;
                float p2 = data.position + data.size;
                data.hPosition = hinted(p1, hinting);
                data.hSize = hinted(p2, hinting) - data.hPosition;
            }

            // Update children geometry
            for (SplitData& data : splitData_) {
                childSize[mainDir] = data.hSize;
                childPosition[mainDir] = data.hPosition;
                data.childArea->updateGeometry(childPosition, childSize);
            }
        }
        else {
            for (const SplitData& data : splitData_) {
                data.childArea->updateGeometry(childPosition, childSize);
            }
        }
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

        core::Color handleHoveredColor(1, 0, 0);
        const float handleRectHalfSize = 1.0f;
        Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
        Int crossDir = 1 - mainDir;
        float crossSize = size()[crossDir];

        float handlePos = splitData_[hoveredSplitHandle_].position;
        geometry::Vec2f handleRectPos;
        geometry::Vec2f handleRectSize;
        handleRectPos[mainDir] = handlePos - handleRectHalfSize;
        handleRectSize[mainDir] = 2 * handleRectHalfSize;
        handleRectSize[crossDir] = crossSize;

        geometry::Rect2f handleRect =
            geometry::Rect2f::fromPositionSize(handleRectPos, handleRectSize);
        detail::insertRect(a, handleHoveredColor, handleRect);

        engine->updateVertexBufferData(triangles_, std::move(a));
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(triangles_, -1, 0);
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
    for (SplitData& data : splitData_) {
        if (data.size < 0) {
            data.size = averageSize;
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

Int PanelArea::computeHoveredSplitHandle_(const geometry::Vec2f& position) const {
    if (isSplit()) {
        const float handleHoverHalfSize = 5.0f;
        Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
        float pos = position[mainDir];
        for (Int i = 0; i < splitData_.length(); ++i) {
            const SplitData& data = splitData_[i];
            if (data.isInteractive
                && std::abs(pos - data.position) < handleHoverHalfSize) {
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
            popCursor();
        }
        else {
            Qt::CursorShape cursor = (type() == PanelAreaType::HorizontalSplit)
                                         ? Qt::SplitHCursor
                                         : Qt::SplitVCursor;
            pushCursor(cursor);
        }
        requestRepaint();
    }
}

void PanelArea::startDragging_(const geometry::Vec2f& position) {
    Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
    dragStartMousePosition_ = position[mainDir];
    dragStartSplitPosition_ = splitData_[draggedSplitHandle_].position;
    dragStartSplitSizeAfter_ = splitData_[draggedSplitHandle_].size;
    dragStartSplitSizeBefore_ = 0;
    if (draggedSplitHandle_ > 0) {
        dragStartSplitSizeBefore_ = splitData_[draggedSplitHandle_ - 1].size;
    }
    else {
        dragStartSplitSizeBefore_ = 0;
    }
}

void PanelArea::continueDragging_(const geometry::Vec2f& position) {

    const float minSplitSize = 50;
    Int mainDir = (type() == PanelAreaType::HorizontalSplit) ? 0 : 1;
    float mousePosition = position[mainDir];
    float deltaPosition = mousePosition - dragStartMousePosition_;

    // Compute new size of split area after the split
    float newSplitSizeAfter = dragStartSplitSizeAfter_ - deltaPosition;
    if (newSplitSizeAfter < minSplitSize) {
        newSplitSizeAfter = minSplitSize;
        deltaPosition = dragStartSplitSizeAfter_ - newSplitSizeAfter;
    }

    // Compute new size of split area before the split
    float newSplitSizeBefore = dragStartSplitSizeBefore_ + deltaPosition;
    if (newSplitSizeBefore < minSplitSize) {
        newSplitSizeBefore = minSplitSize;
        deltaPosition = newSplitSizeBefore - dragStartSplitSizeBefore_;
        newSplitSizeAfter = dragStartSplitSizeAfter_ - deltaPosition;
    }

    // Update splitData_
    float newSplitPosition = dragStartSplitPosition_ + deltaPosition;
    splitData_[draggedSplitHandle_].position = newSplitPosition;
    splitData_[draggedSplitHandle_].size = newSplitSizeAfter;
    if (draggedSplitHandle_ > 0) {
        splitData_[draggedSplitHandle_ - 1].size = newSplitSizeBefore;
    }

    requestGeometryUpdate();
    requestRepaint();
}

void PanelArea::stopDragging_(const geometry::Vec2f& position) {
    updateHoveredSplitHandle_(position);
}

} // namespace vgc::ui
