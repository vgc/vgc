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

#include <vgc/tools/currentcolor.h>

#include <vgc/canvas/documentmanager.h>
#include <vgc/canvas/workspaceselection.h>
#include <vgc/core/algorithm.h> // contains
#include <vgc/core/boolguard.h>
#include <vgc/dom/strings.h>
#include <vgc/workspace/workspace.h>

namespace vgc::tools {

namespace commands {

using ui::Key;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    colorSelectSync,
    "colors.colorSelectSync",
    "Synchronize Current Color With Selection",
    Key::None,
    "tools/icons/colorSelectSync.svg")

} // namespace commands

CurrentColor::CurrentColor(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    // Color-Select Sync
    documentManager_ = importModule<canvas::DocumentManager>();
    colorSelectSyncAction_ = createTriggerAction(commands::colorSelectSync());
    if (auto action = colorSelectSyncAction_.lock()) {
        action->setCheckable(true); // XXX Make this part of the Command?
        action->checkStateChanged().connect(onColorSelectSyncCheckStateChanged_Slot());
        onColorSelectSyncCheckStateChanged_();
    }
}

CurrentColorPtr CurrentColor::create(const ui::ModuleContext& context) {
    return core::createObject<CurrentColor>(context);
}

void CurrentColor::setColor(const core::Color& color) {
    if (color_ == color) {
        return;
    }
    color_ = color;
    core::Color copy = color;
    colorChanged().emit(copy);
}

void CurrentColor::onColorSelectSyncCheckStateChanged_() {
    if (auto action = colorSelectSyncAction_.lock()) {
        bool isChecked = action->isChecked();
        if (isChecked) {

            // Update current color when selection color changes
            if (auto documentManager = documentManager_.lock()) {
                documentManager->currentWorkspaceSelectionChanged().connect(
                    updateCurrentColorFromSelectionColor_Slot());

                // TODO: updateCurrentColorFromSelectionColor_ should also be
                // called when the color of the selection changes (e.g., via
                // direct DOM manipulation), not just when what's selected
                // changes.
            }

            // Update selection color when current color changes
            colorChanged().connect(updateSelectionColorFromCurrentColor_Slot());

            // Update current color from selection now
            updateCurrentColorFromSelectionColor_();
        }
        else {
            if (auto documentManager = documentManager_.lock()) {
                documentManager->currentWorkspaceSelectionChanged().disconnect(
                    updateCurrentColorFromSelectionColor_Slot());
            }
            colorChanged().disconnect(updateSelectionColorFromCurrentColor_Slot());
        }
    }
}

namespace {

std::array<core::StringId, 2> colorableElements() {
    namespace S = dom::strings;
    return {S::edge, S::face};
}

} // namespace

void CurrentColor::updateCurrentColorFromSelectionColor_() {

    // Prevent mutual recursion between the Color-Select Sync update functions
    if (isUpdatingSelectionColorFromCurrentColor_) {
        return;
    }
    core::BoolGuard guard(isUpdatingCurrentColorFromSelectionColor_);

    // Disallow amending whenever the selection change, so that the following
    // sequence of user actions results in two undo groups, not just one:
    //
    // 1. Selecting an edge
    // 2. Changing its color via the current color
    // 3. Selectin another edge
    // 4. Changing its color via the current color
    //
    canAmendUpdateSelectionColor_ = false;

    auto documentManager = documentManager_.lock();
    if (!documentManager) {
        return;
    }

    auto workspace = documentManager->currentWorkspace().lock();
    if (!workspace) {
        return;
    }

    auto selection = documentManager->currentWorkspaceSelection().lock();
    if (!selection) {
        return;
    }

    // Update current color from the first colorable selected elements, if any
    for (core::Id id : selection->itemIds()) {
        workspace::Element* item = workspace->find(id);
        dom::Element* element = item ? item->domElement() : nullptr;
        if (element && core::contains(colorableElements(), element->tagName())) {
            core::Color color = element->getAttribute(dom::strings::color).getColor();
            setColor(color);
            break;
        }
    }
}

void CurrentColor::updateSelectionColorFromCurrentColor_() {

    // Prevent mutual recursion between the Color-Select Sync update functions
    if (isUpdatingCurrentColorFromSelectionColor_) {
        return;
    }
    core::BoolGuard guard(isUpdatingSelectionColorFromCurrentColor_);

    auto documentManager = documentManager_.lock();
    if (!documentManager) {
        return;
    }

    auto workspace = documentManager->currentWorkspace().lock();
    if (!workspace) {
        return;
    }

    auto selection = documentManager->currentWorkspaceSelection().lock();
    if (!selection) {
        return;
    }

    const core::Array<core::Id>& itemIds = selection->itemIds();
    if (itemIds.isEmpty()) {
        return;
    }

    // Open undo group
    static core::StringId undoName("Update Selection Color From Current Color");
    core::UndoGroupWeakPtr undoGroup_;
    core::HistoryWeakPtr history_ = workspace->history();
    if (auto history = history_.lock()) {
        undoGroup_ = history->createUndoGroup(undoName);
    }

    // Update color of colorable selected elements
    for (core::Id id : itemIds) {
        workspace::Element* item = workspace->find(id);
        dom::Element* element = item ? item->domElement() : nullptr;
        if (element && core::contains(colorableElements(), element->tagName())) {
            element->setAttribute(dom::strings::color, color());
        }
    }
    workspace->sync();

    // Close undo operation
    if (auto undoGroup = undoGroup_.lock()) {
        bool amend = canAmendUpdateSelectionColor_ && undoGroup->parent()
                     && undoGroup->parent()->name() == undoName;
        undoGroup->close(amend);

        // Re-allow amending (see updateCurrentColorFromSelectionColor_())
        canAmendUpdateSelectionColor_ = true;
    }
}

} // namespace vgc::tools
