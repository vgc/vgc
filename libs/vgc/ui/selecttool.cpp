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

bool SelectTool::onMouseMove(MouseEvent* /*event*/) {
    return isSelecting_;

    // TODO: if isMaybeSelecting_ or isMaybeDragging_,
    //       activate dragging after a few pixel
}

bool SelectTool::onMousePress(MouseEvent* event) {

    if (isSelecting_) {
        return true;
    }

    MouseButton button = event->button();
    ModifierKeys keys = event->modifierKeys();
    if (button == MouseButton::Left) {
        ModifierKeys unsupportedKeys =
            (ModifierKey::Ctrl | ModifierKey::Alt | ModifierKey::Shift).toggleAll();
        if (keys.hasAny(unsupportedKeys)) {
            isSelecting_ = false;
        }
        else {
            isSelecting_ = true;
            isAlternativeMode_ = keys.has(ModifierKey::Alt);
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
        }
    }
    else {
        isSelecting_ = false;
    }

    return isSelecting_;

    // TODO: isMaybeSelecting_, isMaybeDragging_
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

    if (!isSelecting_) {
        return false;
    }

    if (event->button() != MouseButton::Left) {
        // Prevent parent widget from doing an action with a different
        // mouse button if we are in the middle of our own action.
        return true;
    }

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        bool wasSelecting = isSelecting_;
        isSelecting_ = false;
        return wasSelecting;
        // Until a better mechanism is implemented, we should return the same
        // value in onMousePress / onMouseRelease (at least for the same mouse
        // button) otherwise this confuses the parent widgets (receiving the
        // press but not the release, or vice-versa).
    }

    // Get current selection and selection candidates
    core::Array<core::Id> selection = canvas->selection();
    core::Array<SelectionCandidate> candidates =
        canvas->computeSelectionCandidates(event->position());

    // Compute new selection
    bool selectionChanged = false;
    switch (selectionMode_) {
    case SelectionMode::Toggle: {
        // TODO: Toggle selection.
        break;
    }
    case SelectionMode::Add: {
        core::Id selectedId =
            addToSelection(selection, candidates, isAlternativeMode_, lastSelectedId_);
        if (selectedId != -1) {
            selectionChanged = true;
            lastSelectedId_ = selectedId;
            lastDeselectedId_ = -1;
        }
        break;
    }
    case SelectionMode::Remove: {
        core::Id deselectedId = removeFromSelection(
            selection, candidates, isAlternativeMode_, lastDeselectedId_);
        if (deselectedId != -1) {
            selectionChanged = true;
            lastSelectedId_ = -1;
            lastDeselectedId_ = deselectedId;
        }
        break;
    }
    case SelectionMode::Single: {
        core::Id selectedId =
            selectSingleItem(candidates, isAlternativeMode_, lastSelectedId_);
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
    isSelecting_ = false;
    return true;
}

} // namespace vgc::ui
