// Copyright 2023 The VGC Developers
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

#include <vgc/ui/selecttool.h>

#include <set>

namespace vgc::ui {

SelectTool::SelectTool()
    : CanvasTool() {
}

SelectToolPtr SelectTool::create() {
    return SelectToolPtr(new SelectTool());
}

namespace {

// Time elapsed from press after which the action becomes a drag.
inline constexpr Int dragTimeThresholdInMilliseconds = 1000;
inline constexpr float dragDeltaThresholdInPixels = 5.f;

} // namespace

bool SelectTool::onMouseMove(MouseEvent* event) {

    if (!isInAction_) {
        return false;
    }

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return isInAction_; // always true
    }

    workspace::Workspace* workspace = canvas->workspace();
    if (!workspace) {
        return isInAction_; // always true
    }

    if (!isDragging_) {
        // Initiate drag if conditions are met.
        //
        // Current event implementation uses Qt's timestamps, and according to the
        // documentation, these should "normally be in milliseconds".
        Int deltaTime = core::int_cast<Int>(event->timestamp()) - timeAtPress_;
        float deltaPos = (event->position() - cursorPositionAtPress_).length();
        // Consider the action is a drag if we moved 5 pixels or the button
        // has been pressed for more than 1 second.
        if (deltaPos >= dragDeltaThresholdInPixels
            || deltaTime > dragTimeThresholdInMilliseconds) {

            isDragging_ = true;
            // Initialize drag data
            switch (dragAction_) {
            case DragAction::Select: {
                break;
            }
            case DragAction::TranslateCandidate: {
                // Note: candidates_ is guaranteed to be not empty for this action.
                core::Array<core::Id> elementsIds;
                elementsIds.append(candidates_.first().id());
                initializeDragMoveData_(workspace, elementsIds);
                break;
            }
            case DragAction::TranslateSelection: {
                initializeDragMoveData_(workspace, selectionAtPress_);
                break;
            }
            }
        }
    }

    if (isDragging_) {
        geometry::Mat4d inverseViewMatrix = canvas->camera().viewMatrix().inverted();
        geometry::Vec2f cursorPosition = event->position();

        geometry::Vec2d cursorPositionInWorkspace =
            inverseViewMatrix.transformPointAffine(geometry::Vec2d(cursorPosition));
        geometry::Vec2d cursorPositionInWorkspaceAtPress =
            inverseViewMatrix.transformPointAffine(
                geometry::Vec2d(cursorPositionAtPress_));

        switch (dragAction_) {
        case DragAction::Select: {
            // todo
            break;
        }
        case DragAction::TranslateCandidate:
        case DragAction::TranslateSelection: {
            geometry::Vec2d deltaInWorkspace =
                cursorPositionInWorkspace - cursorPositionInWorkspaceAtPress;
            updateDragMovedElements_(workspace, deltaInWorkspace);
            break;
        }
        }
    }

    return true;
}

bool SelectTool::onMousePress(MouseEvent* event) {

    if (isInAction_) {
        // Prevent parent widget from doing an action
        // if we are in the middle of our own action.
        return true;
    }

    MouseButton button = event->button();
    if (button != MouseButton::Left) {
        return false;
    }

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return false;
    }

    ModifierKeys keys = event->modifierKeys();
    ModifierKeys supportedKeys =
        (ModifierKey::Ctrl | ModifierKey::Alt | ModifierKey::Shift);
    ModifierKeys unsupportedKeys = ~supportedKeys;

    if (!keys.hasAny(unsupportedKeys)) {
        isInAction_ = true;
        candidates_ = canvas->computeSelectionCandidates(event->position());
        selectionAtPress_ = canvas->selection();
        cursorPositionAtPress_ = event->position();
        timeAtPress_ = core::int_cast<Int>(event->timestamp());

        // Prepare for a potential simple click selection action.
        if (keys.hasAll(ModifierKey::Shift | ModifierKey::Ctrl)) {
            selectionMode_ = SelectionMode::Toggle;
        }
        else if (keys.has(ModifierKey::Shift)) {
            selectionMode_ = SelectionMode::Add;
        }
        else if (keys.has(ModifierKey::Ctrl)) {
            selectionMode_ = SelectionMode::Remove;
        }
        else {
            selectionMode_ = SelectionMode::Single;
        }
        isAlternativeMode_ = keys.has(ModifierKey::Alt);

        // Prepare for a potential click-and-drag action.
        if (candidates_.isEmpty()) {
            dragAction_ = DragAction::Select;
        }
        else if (selectionMode_ == SelectionMode::Single && !isAlternativeMode_) {
            // When no modifier keys are used:
            // If some candidates are already selected then the drag action is
            // to translate the current selection.
            // Otherwise we'll translate the candidate that would be selected
            // if no drag occurs.
            dragAction_ = DragAction::TranslateCandidate;
            for (const SelectionCandidate& candidate : candidates_) {
                if (selectionAtPress_.contains(candidate.id())) {
                    dragAction_ = DragAction::TranslateSelection;
                    break;
                }
            }
        }
        else {
            dragAction_ = DragAction::Select;
        }
    }

    return isInAction_;
}

namespace {

core::Id
indexInCandidates(const core::Array<SelectionCandidate>& candidates, core::Id itemId) {
    return candidates.index(                       //
        [&](const SelectionCandidate& candidate) { //
            return candidate.id() == itemId;
        });
}

// If the given item is a candidate, then returns the item and rotates the
// candidates such that the item becomes last.
//
// Otherwise, return -1.
//
core::Id rotateCandidates(core::Array<SelectionCandidate>& candidates, core::Id item) {
    Int i = indexInCandidates(candidates, item);
    if (i >= 0) {
        std::rotate(candidates.begin(), candidates.begin() + i, candidates.end());
        return item;
    }
    else {
        return -1;
    }
}

// Returns the item added to the selection, if any. Otherwise returns -1.
//
core::Id addToSelection(
    core::Array<core::Id>& selection,
    core::Array<SelectionCandidate>& candidates,
    bool isAlternativeMode,
    core::Id lastSelectedId) {

    // If no candidates, then we preserve the current selection.
    //
    if (candidates.isEmpty()) {
        return -1;
    }

    // If Alt is pressed and the last selected item is a candidate, then we
    // want to deselect it and select the next unselected candidate instead.
    //
    // We implement this behavior by rotating the candidates such that the last
    // selected item becomes the last candidate, and we remember whether we
    // should deselect it (unless later re-selected).
    //
    core::Id itemToDeselect = -1;
    if (isAlternativeMode && lastSelectedId != -1) {
        itemToDeselect = rotateCandidates(candidates, lastSelectedId);
    }

    // Select the first unselected candidate.
    //
    for (const SelectionCandidate& c : candidates) {
        core::Id id = c.id();
        if (!selection.contains(id)) {
            if (itemToDeselect != -1 && itemToDeselect != id) {
                selection.removeOne(itemToDeselect);
            }
            selection.append(id);
            return id;
        }
    }
    return -1;
}

// Returns the item removed from the selection, if any. Otherwise returns -1.
//
core::Id removeFromSelection(
    core::Array<core::Id>& selection,
    core::Array<SelectionCandidate>& candidates,
    bool isAlternativeMode,
    core::Id lastDeselectedId) {

    // If no candidates, then we preserve the current selection.
    //
    if (candidates.isEmpty()) {
        return -1;
    }

    // If Alt is pressed and the last deselected item is a candidate, then we
    // want to reselect it and deselect the next selected candidate instead.
    //
    // We implement this behavior by rotating the candidates such that the last
    // deselected item becomes the last candidate, and we remember whether we
    // should reselect it (unless later re-deselected).
    //
    core::Id itemToReselect = -1;
    if (isAlternativeMode && lastDeselectedId != -1) {
        itemToReselect = rotateCandidates(candidates, lastDeselectedId);
    }

    // Deselect the first selected candidate.
    //
    for (const SelectionCandidate& c : candidates) {
        core::Id id = c.id();
        if (selection.contains(id)) {
            if (itemToReselect != -1 && itemToReselect != id) {
                selection.append(itemToReselect);
            }
            selection.removeOne(id);
            return id;
        }
    }
    return -1;
}

// Returns the item to select, if any. Otherwise returns -1.
//
core::Id selectSingleItem(
    const core::Array<SelectionCandidate>& candidates,
    bool isAlternativeMode,
    core::Id lastSelectedId) {

    // If no candidates, then we clear selection.
    //
    if (candidates.isEmpty()) {
        return -1;
    }

    // Return the first candidate, unless in alternative mode when we return
    // the candidate after the last selected item.
    //
    Int j = 0;
    if (isAlternativeMode && lastSelectedId != -1) {
        Int i = indexInCandidates(candidates, lastSelectedId);
        if (i != -1) {
            j = (i + 1) % candidates.length();
        }
    }
    return candidates[j].id();
}

} // namespace

bool SelectTool::onMouseRelease(MouseEvent* event) {

    if (!isInAction_) {
        return false;
    }

    if (event->button() != MouseButton::Left) {
        // Prevent parent widget from doing an action with a different
        // mouse button if we are in the middle of our own action.
        return true;
    }

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        bool wasInAction = isInAction_;
        resetActionState_();
        return wasInAction;
        // Until a better mechanism is implemented, we should return the same
        // value in onMousePress / onMouseRelease (at least for the same mouse
        // button) otherwise this confuses the parent widgets (receiving the
        // press but not the release, or vice-versa).
        // Here we stop the action early so our parent may receive releases for
        // buttons it didn't receive any press event for.
    }

    // If we were dragging we can stop the action and return.
    Int deltaTime = core::int_cast<Int>(event->timestamp()) - timeAtPress_;
    if (isDragging_ || deltaTime > dragTimeThresholdInMilliseconds) {
        resetActionState_();
        return true;
    }

    // Otherwise, we compute the new selection.
    bool selectionChanged = false;
    core::Array<core::Id> selection = selectionAtPress_;
    switch (selectionMode_) {
    case SelectionMode::Toggle: {
        // TODO: Toggle selection.
        break;
    }
    case SelectionMode::Add: {
        core::Id selectedId =
            addToSelection(selection, candidates_, isAlternativeMode_, lastSelectedId_);
        if (selectedId != -1) {
            selectionChanged = true;
            lastSelectedId_ = selectedId;
            lastDeselectedId_ = -1;
        }
        break;
    }
    case SelectionMode::Remove: {
        core::Id deselectedId = removeFromSelection(
            selection, candidates_, isAlternativeMode_, lastDeselectedId_);
        if (deselectedId != -1) {
            selectionChanged = true;
            lastSelectedId_ = -1;
            lastDeselectedId_ = deselectedId;
        }
        break;
    }
    case SelectionMode::Single: {
        core::Id selectedId =
            selectSingleItem(candidates_, isAlternativeMode_, lastSelectedId_);
        if (selectedId != -1) {
            if (selection.length() != 1 || selection.first() != selectedId) {
                selection.assign(1, selectedId);
                selectionChanged = true;
            }
            lastSelectedId_ = selectedId;
            lastDeselectedId_ = -1;
        }
        else {
            if (!selection.isEmpty()) {
                selection.clear();
                selectionChanged = true;
            }
            lastSelectedId_ = -1;
            lastDeselectedId_ = -1;
        }
        break;
    }
    }

    if (selectionChanged) {
        canvas->setSelection(selection);
    }

    resetActionState_();
    return true;
}

void SelectTool::initializeDragMoveData_(
    workspace::Workspace* workspace,
    const core::Array<core::Id>& elementsIds) {

    // Only key vertices and edges have intrinsic spatial data amongst
    // vac cells, so we identify those first.
    //
    std::set<vacomplex::KeyVertex*> verticesToTranslate;
    std::set<vacomplex::KeyEdge*> edgesToTranslate;
    auto insertCellToTranslate = [&](vacomplex::Cell* cell) {
        switch (cell->cellType()) {
        case vacomplex::CellType::KeyVertex: {
            verticesToTranslate.insert(cell->toKeyVertexUnchecked());
            break;
        }
        case vacomplex::CellType::KeyEdge: {
            edgesToTranslate.insert(cell->toKeyEdgeUnchecked());
            break;
        }
        default:
            break;
        }
    };

    for (const core::Id& id : elementsIds) {
        workspace::Element* element = workspace->find(id);
        if (!element) {
            continue;
        }
        vacomplex::Node* node = element->vacNode();
        if (!node || !node->isCell()) {
            continue;
        }
        vacomplex::Cell* cell = node->toCellUnchecked();
        insertCellToTranslate(cell);
        for (vacomplex::Cell* boundaryCell : cell->boundary()) {
            insertCellToTranslate(boundaryCell);
        }
    }

    // Every edge connected to translated vertices has to be either
    // partially modified (snapped) or translated (both vertices are
    // translated).
    std::set<vacomplex::KeyEdge*> affectedEdges;
    for (vacomplex::KeyVertex* kv : verticesToTranslate) {
        for (vacomplex::Cell* cell : kv->star()) {
            if (cell->cellType() == vacomplex::CellType::KeyEdge) {
                vacomplex::KeyEdge* ke = cell->toKeyEdgeUnchecked();
                if (edgesToTranslate.count(ke) == 0) {
                    affectedEdges.insert(ke);
                }
            }
        }
    }
    // Now transfer edges of affectedEdges that have both end vertices
    // in verticesToTranslate to edgesToTranslate.
    for (auto it = affectedEdges.begin(); it != affectedEdges.end();) {
        vacomplex::KeyEdge* ke = *it;
        // It is guaranteed that these edges have start and end vertices,
        // otherwise they would not be in any vertex star.
        if (verticesToTranslate.count(ke->startVertex()) > 0
            && verticesToTranslate.count(ke->endVertex()) > 0) {

            edgesToTranslate.insert(ke);
            it = affectedEdges.erase(it);
        }
        else {
            ++it;
        }
    }

    // Save original intrinsic geometry data for translation
    for (vacomplex::KeyVertex* kv : verticesToTranslate) {
        workspace::Element* element = workspace->findVacElement(kv->id());
        if (element) {
            draggedVertices_.append({element->id(), kv->position()});
        }
    }
    for (vacomplex::KeyEdge* ke : edgesToTranslate) {
        workspace::Element* element = workspace->findVacElement(ke->id());
        if (element) {
            draggedEdges_.append({element->id(), ke->points(), false});
        }
    }
    for (vacomplex::KeyEdge* ke : affectedEdges) {
        workspace::Element* element = workspace->findVacElement(ke->id());
        if (element) {
            draggedEdges_.append({element->id(), ke->points(), true});
        }
    }
}

void SelectTool::updateDragMovedElements_(
    workspace::Workspace* workspace,
    const geometry::Vec2d& translationInWorkspace) {

    // Open history group
    static core::StringId Translate_Elements("Translate Elements");
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(Translate_Elements);
    }

    // Translate cells
    for (const KeyVertexDragData& kvd : draggedVertices_) {
        workspace::Element* element = workspace->find(kvd.elementId);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyVertex* kv =
                element->vacNode()->toCellUnchecked()->toKeyVertex();
            if (kv) {
                vacomplex::ops::setKeyVertexPosition(
                    kv, kvd.position + translationInWorkspace);
            }
        }
    }
    for (const KeyEdgeDragData& ked : draggedEdges_) {
        workspace::Element* element = workspace->find(ked.elementId);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                geometry::Vec2dArray newPoints = ked.points;
                if (newPoints.isEmpty()) {
                    // do nothing
                }
                else if (ked.isPartialTranslation) {
                    // Vertices are already translated here.
                    geometry::Vec2d a = ke->startVertex()->position();
                    geometry::Vec2d b = ke->endVertex()->position();
                    if (newPoints.length() == 1) {
                        // We would have to deal with "widths" if we want
                        // to change the number of points.
                        newPoints[0] = (a + b) * 0.5;
                    }
                    else if (newPoints.length() == 2) {
                        // We would have to deal with "widths" if we want
                        // to change the number of points.
                        newPoints[0] = a;
                        newPoints[1] = b;
                    }
                    else {
                        geometry::Vec2d d1 = a - newPoints.first();
                        geometry::Vec2d d2 = b - newPoints.last();

                        // linear deformation in rough "s"
                        double totalS = 0;
                        geometry::Vec2d lastP = newPoints.first();
                        for (const geometry::Vec2d& p : newPoints) {
                            totalS += (p - lastP).length();
                            lastP = p;
                        }
                        if (totalS > 0) {
                            double currentS = 0;
                            lastP = newPoints.first();
                            for (geometry::Vec2d& p : newPoints) {
                                currentS += (p - lastP).length();
                                lastP = p;
                                double t = currentS / totalS;
                                p += (d1 + t * (d2 - d1));
                            }
                        }
                        else {
                            for (geometry::Vec2d& p : newPoints) {
                                p += d1;
                            }
                        }
                    }
                }
                else {
                    for (geometry::Vec2d& p : newPoints) {
                        p += translationInWorkspace;
                    }
                }
                vacomplex::ops::setKeyEdgeCurvePoints(ke, std::move(newPoints));
            }
        }
    }

    // Close operation
    if (undoGroup) {
        bool amend = canAmendUndoGroup_ && undoGroup->parent()
                     && undoGroup->parent()->name() == Translate_Elements;
        undoGroup->close(amend);
        canAmendUndoGroup_ = true;
    }
}

void SelectTool::resetActionState_() {
    candidates_.clear();
    selectionAtPress_.clear();
    isInAction_ = false;
    isDragging_ = false;
    canAmendUndoGroup_ = false;
    draggedVertices_.clear();
    draggedEdges_.clear();
}

} // namespace vgc::ui
