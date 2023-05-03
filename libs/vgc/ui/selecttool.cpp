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

namespace vgc::ui {

SelectTool::SelectTool()
    : CanvasTool() {
}

SelectToolPtr SelectTool::create() {
    return SelectToolPtr(new SelectTool());
}

namespace {

// Assumes `workspace` is not null.
void translateElements(
    workspace::Workspace* workspace,
    const core::Array<core::Id>& elementIds,
    const geometry::Vec2d& delta,
    bool tryAmend) {

    if (elementIds.isEmpty()) {
        return;
    }

    // Open history group
    static core::StringId Translate_Elements("Translate Elements");
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(Translate_Elements);
    }

    // TODO: specific transformations for edges and vertices
    // move this function as a method of the tool to recompute translation
    // from "before drag" data.

    // Iterate over all elements to translate.
    //
    for (core::Id id : elementIds) {
        workspace::Element* element = workspace->find(id);
        if (element) {
            element->setPosition(element->position() + delta);
        }
    }
    workspace->sync();

    // Close operation
    if (undoGroup) {
        bool amend = tryAmend && undoGroup->parent()
                     && undoGroup->parent()->name() == Translate_Elements;
        undoGroup->close(amend);
    }
}

} // namespace

bool SelectTool::onMouseMove(MouseEvent* event) {

    if (!isInAction_) {
        return false;
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
        if (deltaPos >= 5.f || deltaTime > 1000) {
            cursorPositionAtDragStart_ = cursorPositionAtPress_; // event->position();
            cursorPositionAtLastTranslate_ = cursorPositionAtDragStart_;
            isDragging_ = true;
        }
    }

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return isInAction_; // always true
    }

    workspace::Workspace* workspace = canvas->workspace();

    if (isDragging_) {
        geometry::Mat4d inverseViewMatrix = canvas->camera().viewMatrix().inverted();
        geometry::Vec2f cursorPosition = event->position();

        geometry::Vec2d cursorPositionInWorkspace =
            inverseViewMatrix.transformPointAffine(geometry::Vec2d(cursorPosition));
        geometry::Vec2d cursorPositionInWorkspaceAtLastTranslate =
            inverseViewMatrix.transformPointAffine(
                geometry::Vec2d(cursorPositionAtLastTranslate_));

        geometry::Vec2d deltaInWorkspace =
            cursorPositionInWorkspace - cursorPositionInWorkspaceAtLastTranslate;

        switch (dragAction_) {
        case DragAction::Select: {
            // todo
            break;
        }
        case DragAction::TranslateCandidate: {
            if (workspace) {
                core::Array<core::Id> elementIds;
                elementIds.append(candidates_.first().id());
                translateElements(
                    workspace, elementIds, deltaInWorkspace, canAmendUndoGroup_);
                cursorPositionAtLastTranslate_ = cursorPosition;
                canAmendUndoGroup_ = true;
            }
            break;
        }
        case DragAction::TranslateSelection: {
            if (workspace) {
                translateElements(
                    workspace, selectionAtPress_, deltaInWorkspace, canAmendUndoGroup_);
                cursorPositionAtLastTranslate_ = cursorPosition;
                canAmendUndoGroup_ = true;
            }
            break;
        }
        }
    }

    return true;
}

bool SelectTool::onMousePress(MouseEvent* event) {

    if (isInAction_) {
        return true;
    }

    MouseButton button = event->button();
    if (button != MouseButton::Left) {
        return isInAction_; // always false
    }

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return isInAction_; // always false
    }

    ModifierKeys keys = event->modifierKeys();

    ModifierKeys unsupportedKeys =
        (ModifierKey::Ctrl | ModifierKey::Alt | ModifierKey::Shift).toggleAll();
    if (canvas && !keys.hasAny(unsupportedKeys)) {
        // We prepare for a simple click selection action
        isInAction_ = true;
        candidates_ = canvas->computeSelectionCandidates(event->position());
        selectionAtPress_ = canvas->selection();
        cursorPositionAtPress_ = event->position();
        timeAtPress_ = core::int_cast<Int>(event->timestamp());

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
    if (isDragging_) {
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

void SelectTool::resetActionState_() {
    candidates_.clear();
    selectionAtPress_.clear();
    isInAction_ = false;
    isDragging_ = false;
    canAmendUndoGroup_ = false;
}

} // namespace vgc::ui
