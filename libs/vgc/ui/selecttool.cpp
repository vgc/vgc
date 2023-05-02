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
    if (pressedButtons() == MouseButton::Left) {
        if (!isDrag) {
            // todo: activate after a few pixels
            //isDrag = true;
        }
    }
    return false;
}

bool SelectTool::onMousePress(MouseEvent* /*event*/) {
    return false;
}

bool SelectTool::onMouseRelease(MouseEvent* event) {

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return false;
    }

    MouseButton button = event->button();
    ModifierKeys keys = event->modifierKeys();

    if (button == MouseButton::Left) {
        if (isDrag) {
            isDrag = false;
            // todo
        }
        else {
            ModifierKeys unsupportedKeys =
                (ModifierKey::Ctrl | ModifierKey::Alt | ModifierKey::Shift).toggleAll();
            if (keys.hasAny(unsupportedKeys)) {
                // unsupported modifier key
                return false;
            }

            bool updateSelection = false;
            core::Array<core::Id> newSelection;
            core::Array<SelectionCandidate> candidates =
                canvas->computeSelectionCandidates(event->position());

            if (keys.hasAll(ModifierKey::Shift | ModifierKey::Ctrl)) {
                // toggle, todo
                lastSelectedId = -1;
                lastDeselectedId = -1;
            }
            else if (keys.has(ModifierKey::Shift)) {
                // add to multi-selection

                lastDeselectedId = -1;

                if (candidates.isEmpty()) {
                    // do not change the current selection
                    lastSelectedId = -1;
                }
                else {
                    newSelection = canvas->selection();
                    // If alternative mode is on and the last selected item is in the candidates:
                    //      Find the first unselected item starting from the last selected item
                    //      in the candidates list after having rotated it in such a way that the
                    //      last selected item is the first candidate. If such item exists and is
                    //      not the last selected item then deselect the last selected item.
                    // Otherwise find the first unselected item in the candidates list.
                    //
                    // First, rotate candidates so that last selected becomes first if present.
                    if (keys.has(ModifierKey::Alt) && lastSelectedId != -1) {
                        Int i = candidates.index([&](const SelectionCandidate& c) {
                            return c.id() == lastSelectedId;
                        });
                        if (i >= 0) {
                            std::rotate(
                                candidates.begin(),
                                candidates.begin() + i,
                                candidates.end());
                        }
                        else {
                            lastDeselectedId = -1;
                            keys.unset(ModifierKey::Alt);
                        }
                    }
                    // Then search first unselected candidate in candidates.
                    for (const SelectionCandidate& c : candidates) {
                        core::Id id = c.id();
                        if (!newSelection.contains(id)) {
                            // Deselect last selected if not being reselected
                            if (keys.has(ModifierKey::Alt) && lastDeselectedId != -1
                                && id != lastSelectedId) {
                                //
                                newSelection.removeOne(lastSelectedId);
                            }
                            // select new item
                            newSelection.append(id);
                            updateSelection = true;
                            lastSelectedId = id;
                            break;
                        }
                    }
                }
            }
            else if (keys == ModifierKey::Ctrl) {
                // remove from multi-selection

                lastSelectedId = -1;

                if (candidates.isEmpty()) {
                    // do not change the current selection
                    lastDeselectedId = -1;
                }
                else {
                    newSelection = canvas->selection();
                    // If alternative mode is on and the last deselected item is in the candidates:
                    //      Find the first selected item starting from the last deselected item
                    //      in the candidates list after having rotated it in such a way that the
                    //      last deselected item is the first candidate. If such item exists and is
                    //      not the last deselected item then select the last deselected item.
                    // Otherwise find the first selected item in the candidates list.
                    //
                    // First, rotate candidates so that last deselected becomes first if present.
                    if (keys.has(ModifierKey::Alt) && lastDeselectedId != -1) {
                        Int i = candidates.index([&](const SelectionCandidate& c) {
                            return c.id() == lastDeselectedId;
                        });
                        if (i >= 0) {
                            std::rotate(
                                candidates.begin(),
                                candidates.begin() + i,
                                candidates.end());
                        }
                        else {
                            lastDeselectedId = -1;
                            keys.unset(ModifierKey::Alt);
                        }
                    }
                    // Then search first selected candidate in candidates.
                    for (const SelectionCandidate& c : candidates) {
                        core::Id id = c.id();
                        if (newSelection.contains(id)) {
                            // Select last deselected if not being deselected
                            if (keys.has(ModifierKey::Alt) && lastDeselectedId != -1
                                && id != lastDeselectedId) {
                                //
                                if (!newSelection.contains(lastDeselectedId)) {
                                    newSelection.append(lastDeselectedId);
                                }
                            }
                            // deselect new item
                            newSelection.removeOne(id);
                            updateSelection = true;
                            lastDeselectedId = id;
                            break;
                        }
                    }
                }
            }
            else if (keys == ModifierKey::Alt) {
                // single selection with alternative

                lastDeselectedId = -1;

                if (!candidates.isEmpty()) {
                    // find first candidate after last selected item in candidates
                    // wrapped if last item is in the candidates, otherwise use
                    // first candidate entry in candidates.
                    Int j = 0;
                    if (lastSelectedId != -1) {
                        Int i = candidates.index([&](const SelectionCandidate& c) {
                            return c.id() == lastSelectedId;
                        });
                        j = (i + 1) % candidates.length();
                    }
                    core::Id id = candidates[j].id();
                    if (id != lastSelectedId || !newSelection.contains(id)) {
                        newSelection.append(id);
                        updateSelection = true;
                        lastSelectedId = id;
                    }
                }
            }
            else if (!candidates.isEmpty()) {
                // single selection

                lastDeselectedId = -1;

                core::Id id = candidates.first().id();
                newSelection.append(id);
                updateSelection = true;
                lastSelectedId = id;
            }
            else {
                // single selection of nothing (deselect all)

                lastDeselectedId = -1;
                lastSelectedId = -1;

                newSelection.clear();
                updateSelection = true;
            }

            if (updateSelection) {
                canvas->setSelection(newSelection);
            }
            return true;
        }
    }

    return false;
}

} // namespace vgc::ui
