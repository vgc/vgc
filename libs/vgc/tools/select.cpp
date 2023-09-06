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

#include <vgc/tools/select.h>

#include <set>

#include <vgc/graphics/detail/shapeutil.h>
#include <vgc/graphics/strings.h>
#include <vgc/tools/copypaste.h>
#include <vgc/tools/topology.h>
#include <vgc/ui/boolsettingedit.h>
#include <vgc/ui/column.h>
#include <vgc/vacomplex/detail/operationsimpl.h>
#include <vgc/workspace/colors.h>

namespace vgc::tools {

namespace {

namespace options {

ui::BoolSetting* showTransformBox() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(), "tools.select.showTransformBox", "Transform Box", true);
    return setting.get();
    // Ideally, we'd want "Show Transform Box" to be the name of the command,
    // but "Transform Box" to appear in the tool options.
}

} // namespace options

} // namespace

Select::Select(CreateKey key)
    : CanvasTool(key) {

    canvasChanged().connect(onCanvasChangedSlot_());
    onCanvasChanged_();

    options::showTransformBox()->valueChanged().connect(onShowTransformBoxChangedSlot_());
    onShowTransformBoxChanged_();

    ui::Action* glueAction = createTriggerAction(commands::glue());
    glueAction->triggered().connect(onGlueSlot_());

    ui::Action* unglueAction = createTriggerAction(commands::unglue());
    unglueAction->triggered().connect(onUnglueSlot_());

    ui::Action* simplifyAction = createTriggerAction(commands::simplify());
    simplifyAction->triggered().connect(onSimplifySlot_());

    ui::Action* cutAction = createTriggerAction(commands::cut());
    cutAction->triggered().connect(onCutSlot_());

    ui::Action* copyAction = createTriggerAction(commands::copy());
    copyAction->triggered().connect(onCopySlot_());

    ui::Action* pasteAction = createTriggerAction(commands::paste());
    pasteAction->triggered().connect(onPasteSlot_());
}

SelectPtr Select::create() {
    return core::createObject<Select>();
}

core::Array<core::Id> Select::selection() {
    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return {};
    }
    else {
        return canvas->selection();
    }
}

ui::WidgetPtr Select::createOptionsWidget() const {
    ui::WidgetPtr res = ui::Column::create();
    res->createChild<ui::BoolSettingEdit>(options::showTransformBox());
    return res;
}

namespace {

// Time elapsed from press after which the action becomes a drag.
inline constexpr double dragTimeThreshold = 0.5;
inline constexpr float dragDeltaThreshold = 5;

} // namespace

bool Select::onMouseMove(ui::MouseMoveEvent* event) {

    if (!isInAction_) {
        return false;
    }

    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return isInAction_; // always true
    }

    workspace::Workspace* workspace = canvas->workspace();
    if (!workspace) {
        return isInAction_; // always true
    }

    cursorPosition_ = event->position();

    if (!isDragging_) {

        // Initiate drag if:
        // - mouse position moved more than a few pixels, or
        // - mouse pressed for longer than a few 1/10s of seconds
        //
        double deltaTime = event->timestamp() - timeAtPress_;
        float deltaPos = (cursorPosition_ - cursorPositionAtPress_).length();
        if (deltaPos >= dragDeltaThreshold || deltaTime > dragTimeThreshold) {

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

        geometry::Vec2d cursorPositionInWorkspace =
            inverseViewMatrix.transformPointAffine(geometry::Vec2d(cursorPosition_));
        geometry::Vec2d cursorPositionInWorkspaceAtPress =
            inverseViewMatrix.transformPointAffine(
                geometry::Vec2d(cursorPositionAtPress_));

        switch (dragAction_) {
        case DragAction::Select: {
            rectCandidates_ = canvas->computeRectangleSelectionCandidates(
                cursorPositionAtPress_, cursorPosition_);
            selectionRectangleGeometry_.reset();
            requestRepaint();
            break;
        }
        case DragAction::TranslateCandidate:
        case DragAction::TranslateSelection: {
            deltaInWorkspace_ =
                cursorPositionInWorkspace - cursorPositionInWorkspaceAtPress;
            updateDragMovedElements_(workspace, deltaInWorkspace_);
            break;
        }
        }
    }

    return true;
}

bool Select::onMousePress(ui::MousePressEvent* event) {

    if (isInAction_) {
        // Prevent parent widget from doing an action
        // if we are in the middle of our own action.
        return true;
    }

    ui::MouseButton button = event->button();
    if (button != ui::MouseButton::Left) {
        return false;
    }

    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return false;
    }

    ui::ModifierKeys keys = event->modifierKeys();
    ui::ModifierKeys supportedKeys =
        (ui::ModifierKey::Ctrl | ui::ModifierKey::Alt | ui::ModifierKey::Shift);
    ui::ModifierKeys unsupportedKeys = ~supportedKeys;

    if (!keys.hasAny(unsupportedKeys)) {
        isInAction_ = true;
        candidates_ = canvas->computeSelectionCandidates(event->position());
        selectionAtPress_ = canvas->selection();
        cursorPositionAtPress_ = event->position();
        timeAtPress_ = event->timestamp();

        // Prepare for a potential simple click selection action.
        if (keys.hasAll(ui::ModifierKey::Shift | ui::ModifierKey::Ctrl)) {
            selectionMode_ = SelectionMode::Toggle;
        }
        else if (keys.has(ui::ModifierKey::Shift)) {
            selectionMode_ = SelectionMode::Add;
        }
        else if (keys.has(ui::ModifierKey::Ctrl)) {
            selectionMode_ = SelectionMode::Remove;
        }
        else {
            selectionMode_ = SelectionMode::New;
        }
        isAlternativeMode_ = keys.has(ui::ModifierKey::Alt);

        // Prepare for a potential click-and-drag action.
        if (candidates_.isEmpty()) {
            dragAction_ = DragAction::Select;
        }
        else if (selectionMode_ == SelectionMode::New && !isAlternativeMode_) {
            // When no modifier keys are used:
            // If some candidates are already selected then the drag action is
            // to translate the current selection.
            // Otherwise we'll translate the candidate that would be selected
            // if no drag occurs.
            dragAction_ = DragAction::TranslateCandidate;
            for (const canvas::SelectionCandidate& candidate : candidates_) {
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

core::Id indexInCandidates(
    const core::Array<canvas::SelectionCandidate>& candidates,
    core::Id itemId) {
    return candidates.index(                               //
        [&](const canvas::SelectionCandidate& candidate) { //
            return candidate.id() == itemId;
        });
}

core::Array<canvas::SelectionCandidate>::iterator
findInCandidates(core::Array<canvas::SelectionCandidate>& candidates, core::Id itemId) {
    return candidates.find(                                //
        [&](const canvas::SelectionCandidate& candidate) { //
            return candidate.id() == itemId;
        });
}

// If the given item is a candidate, then returns the item and rotates the
// candidates such that the item becomes last.
//
// Otherwise, return -1.
//
core::Id
rotateCandidates(core::Array<canvas::SelectionCandidate>& candidates, core::Id item) {
    auto it = findInCandidates(candidates, item);
    if (it != candidates.end()) {
        std::rotate(candidates.begin(), ++it, candidates.end());
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
    core::Array<canvas::SelectionCandidate>& candidates,
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
    // We implement this behavior by:
    // 1. Checking if the last selected item is indeed a candidate (else do nothing).
    // 2. Rotating the candidates to place the last selected item at the end.
    // 3. Remembering to delesect it if we find a candidate to select.
    //
    core::Id itemToDeselect = -1;
    if (isAlternativeMode && lastSelectedId != -1) {
        itemToDeselect = rotateCandidates(candidates, lastSelectedId);
    }

    // Select the first unselected candidate.
    //
    for (const canvas::SelectionCandidate& c : candidates) {
        core::Id id = c.id();
        if (!selection.contains(id)) {
            if (itemToDeselect != -1) {
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
    core::Array<canvas::SelectionCandidate>& candidates,
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
    // We implement this behavior by:
    // 1. Checking if the last deselected item is indeed a candidate (else do nothing).
    // 2. Rotating the candidates to place the last deselected item at the end.
    // 3. Remembering to relesect it if we find a candidate to deselect.
    //
    core::Id itemToReselect = -1;
    if (isAlternativeMode && lastDeselectedId != -1) {
        itemToReselect = rotateCandidates(candidates, lastDeselectedId);
    }

    // Deselect the first selected candidate.
    //
    for (const canvas::SelectionCandidate& c : candidates) {
        core::Id id = c.id();
        if (selection.contains(id)) {
            if (itemToReselect != -1 && !selection.contains(itemToReselect)) {
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
core::Id selectNewItem(
    const core::Array<canvas::SelectionCandidate>& candidates,
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

bool Select::onMouseRelease(ui::MouseReleaseEvent* event) {

    if (!isInAction_) {
        return false;
    }

    if (event->button() != ui::MouseButton::Left) {
        // Prevent parent widget from doing an action with a different
        // mouse button if we are in the middle of our own action.
        return true;
    }

    canvas::Canvas* canvas = this->canvas();
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

    workspace::Workspace* workspace = canvas->workspace();
    if (!workspace) {
        bool wasInAction = isInAction_;
        resetActionState_();
        return wasInAction;
    }

    core::Array<core::Id> selection = selectionAtPress_;
    bool selectionChanged = false;

    // If we were dragging we can stop the action and return.
    if (isDragging_) {
        switch (dragAction_) {
        case DragAction::Select: {
            // Rectangle selection.
            switch (selectionMode_) {
            case SelectionMode::Toggle: {
                for (core::Id id : rectCandidates_) {
                    if (!selection.removeOne(id)) {
                        selection.append(id);
                    }
                }
                selectionChanged = !rectCandidates_.isEmpty();
                break;
            }
            case SelectionMode::Add: {
                for (core::Id id : rectCandidates_) {
                    if (!selection.contains(id)) {
                        selection.append(id);
                        selectionChanged = true;
                    }
                }
                break;
            }
            case SelectionMode::Remove: {
                for (core::Id id : rectCandidates_) {
                    if (selection.removeOne(id)) {
                        selectionChanged = true;
                    }
                }
                break;
            }
            case SelectionMode::New: {
                if (!selection.isEmpty() || !rectCandidates_.isEmpty()) {
                    selection = rectCandidates_;
                    selectionChanged = true;
                }
                break;
            }
            }
            lastSelectedId_ = -1;
            lastDeselectedId_ = -1;
            break;
        }
        case DragAction::TranslateCandidate:
        case DragAction::TranslateSelection: {
            finalizeDragMovedElements_(workspace);
            break;
        }
        }
    }
    else {
        // Point selection.
        switch (selectionMode_) {
        case SelectionMode::Toggle: {
            // TODO: Toggle selection.
            break;
        }
        case SelectionMode::Add: {
            core::Id selectedId = addToSelection(
                selection, candidates_, isAlternativeMode_, lastSelectedId_);
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
        case SelectionMode::New: {
            core::Id selectedId =
                selectNewItem(candidates_, isAlternativeMode_, lastSelectedId_);
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
    }

    if (selectionChanged) {
        canvas->setSelection(selection);
    }

    resetActionState_();
    return true;
}

void Select::onFocusStackIn(ui::FocusReason reason) {
    if (transformBox_) {
        transformBox_->setFocus(reason);
    }
}

void Select::onResize() {
    SuperClass::onResize();
    selectionRectangleGeometry_.reset();
}

void Select::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
}

void Select::onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    using namespace graphics;
    namespace gs = graphics::strings;

    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    using geometry::Vec2d;
    using geometry::Vec2f;

    if (isDragging_ && dragAction_ == DragAction::Select) {
        if (!selectionRectangleGeometry_) {
            geometry::Mat4d invView = canvas->camera().viewMatrix().inverted();
            Vec2f a(invView.transformPointAffine(Vec2d(cursorPositionAtPress_)));
            Vec2f b(invView.transformPointAffine(Vec2d(cursorPosition_)));
            geometry::Rect2f rect = geometry::Rect2f::empty;
            rect.uniteWith(a);
            rect.uniteWith(b);

            const core::Color& color = workspace::colors::selection;

            selectionRectangleGeometry_ =
                graphics::detail::createRectangleWithScreenSpaceThickness(
                    engine, rect, 2.f, color);
        }

        geometry::Mat4f currentView(engine->viewMatrix());
        geometry::Mat4f canvasView(canvas->camera().viewMatrix());
        engine->pushViewMatrix(currentView * canvasView);

        engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
        engine->draw(selectionRectangleGeometry_);

        engine->popViewMatrix();
    }
}

void Select::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    selectionRectangleGeometry_.reset();
}

void Select::updateChildrenGeometry() {
    SuperClass::updateChildrenGeometry();
    if (transformBox_) {
        transformBox_->updateGeometry(rect());
    }
}

void Select::initializeDragMoveData_(
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
        Int n = verticesToTranslate.count(ke->startVertex())
                + verticesToTranslate.count(ke->endVertex());
        if (n != 1) {
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
            draggedEdges_.emplaceLast(element->id(), true);
        }
    }
    for (vacomplex::KeyEdge* ke : affectedEdges) {
        workspace::Element* element = workspace->findVacElement(ke->id());
        if (element) {
            draggedEdges_.emplaceLast(element->id(), false);
        }
    }
}

void Select::updateDragMovedElements_(
    workspace::Workspace* workspace,
    const geometry::Vec2d& translationInWorkspace) {

    // Open history group
    static core::StringId Translate_Elements("Translate Elements");
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(Translate_Elements);
    }

    core::Array<vacomplex::detail::Operations> ops;
    auto initOperationOn = [&ops](vacomplex::Cell* cell) {
        bool found = false;
        for (const vacomplex::detail::Operations& op : ops) {
            if (op.complex() == cell->complex()) {
                found = true;
                break;
            }
        }
        if (!found) {
            ops.emplaceLast(cell->complex());
        }
    };

    // Translate Vertices
    for (const KeyVertexDragData& kvd : draggedVertices_) {
        workspace::Element* element = workspace->find(kvd.elementId);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyVertex* kv =
                element->vacNode()->toCellUnchecked()->toKeyVertex();
            if (kv) {
                initOperationOn(kv);
                vacomplex::ops::setKeyVertexPosition(
                    kv, kvd.position + translationInWorkspace);
            }
        }
    }

    // Translate or snap edges' geometry
    for (const KeyEdgeDragData& ked : draggedEdges_) {
        workspace::Element* element = workspace->find(ked.elementId);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (!ke) {
                continue;
            }
            initOperationOn(ke);
            if (ked.isUniformTranslation) {
                vacomplex::KeyEdgeData* data = ke->data();
                if (data) {
                    if (!ked.isEditStarted) {
                        ked.oldData = data->clone();
                        ked.isEditStarted = true;
                    }
                    else {
                        *data = *ked.oldData;
                    }
                    data->translate(translationInWorkspace);
                }
            }
            else {
                // Vertices are already translated here.
                ke->snapGeometry();
            }
        }
    }

    // Close operation
    ops.clear();
    if (undoGroup) {
        bool amend = canAmendUndoGroup_ && undoGroup->parent()
                     && undoGroup->parent()->name() == Translate_Elements;
        undoGroup->close(amend);
        canAmendUndoGroup_ = true;
    }
}

void Select::finalizeDragMovedElements_(workspace::Workspace* workspace) {
    // Open history group
    static core::StringId Translate_Elements("Translate Elements");
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(Translate_Elements);
    }

    // Finish edges' geometry edit
    for (const KeyEdgeDragData& ked : draggedEdges_) {
        workspace::Element* element = workspace->find(ked.elementId);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke && ked.isEditStarted) {
                vacomplex::KeyEdgeData* data = ke->data();
                if (data) {
                    //data->finishEdit();
                }
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

void Select::resetActionState_() {
    candidates_.clear();
    selectionAtPress_.clear();
    isInAction_ = false;
    isDragging_ = false;
    canAmendUndoGroup_ = false;
    draggedVertices_.clear();
    draggedEdges_.clear();
    if (selectionRectangleGeometry_) {
        selectionRectangleGeometry_.reset();
        requestRepaint();
    }
}

void Select::disconnectCanvas_() {
    if (connectedCanvas_) {
        connectedCanvas_->aboutToBeDestroyed().disconnect(
            onCanvasAboutToBeDestroyedSlot_());
        connectedCanvas_->selectionChanged().disconnect(onSelectionChangedSlot_());
    }
    // TODO: allow `oldCanvas_->disconnect(on..Slot_())` syntax?
}

void Select::onCanvasChanged_() {
    disconnectCanvas_();
    connectedCanvas_ = this->canvas();
    if (connectedCanvas_) {
        connectedCanvas_->aboutToBeDestroyed().connect(onCanvasAboutToBeDestroyedSlot_());
        connectedCanvas_->selectionChanged().connect(onSelectionChangedSlot_());
    }
    onSelectionChanged_();
}

void Select::onCanvasAboutToBeDestroyed_() {
    disconnectCanvas_();
    onSelectionChanged_();
}

void Select::onSelectionChanged_() {
    updateTransformBoxElements_();
}

void Select::onShowTransformBoxChanged_() {
    if (options::showTransformBox()->value()) {
        if (!transformBox_) {
            transformBox_ = createChild<TransformBox>();
            updateTransformBoxElements_();
            if (focusStack().contains(this)) {
                transformBox_->setFocus(ui::FocusReason::Other);
            }
        }
    }
    else {
        if (transformBox_) {
            // Remove from parent and destroy
            bool wasFocused = focusStack().contains(transformBox_);
            transformBox_->reparent(nullptr);
            transformBox_ = nullptr;
            if (wasFocused) {
                this->setFocus(ui::FocusReason::Other);
            }
        }
    }
}

void Select::updateTransformBoxElements_() {
    if (transformBox_) {
        canvas::Canvas* canvas = this->canvas();
        if (canvas) {
            transformBox_->setElements(canvas->selection());
        }
        else {
            transformBox_->setElements({});
        }
    }
}

void Select::onGlue_() {
    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    workspace::Workspace* workspace = canvas->workspace();
    if (!workspace) {
        return;
    }

    // Open history group
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(commands::glue());
    }

    core::Array<core::Id> selection = canvas->selection();
    core::Id gluedId = workspace->glue(selection);
    if (gluedId >= 0) {
        canvas->setSelection({gluedId});
    }

    // Close history group
    if (undoGroup) {
        undoGroup->close();
    }
}

void Select::onUnglue_() {
    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    workspace::Workspace* workspace = canvas->workspace();
    if (!workspace) {
        return;
    }

    core::Array<core::Id> selection = canvas->selection();
    if (selection.isEmpty()) {
        return;
    }

    // Open history group
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(commands::unglue());
    }

    core::Array<core::Id> ungluedIds = workspace->unglue(selection);
    canvas->setSelection(std::move(ungluedIds));

    // Close history group
    if (undoGroup) {
        undoGroup->close();
    }
}

void Select::onSimplify_() {
    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    workspace::Workspace* workspace = canvas->workspace();
    if (!workspace) {
        return;
    }

    core::Array<core::Id> selection = canvas->selection();
    if (selection.isEmpty()) {
        return;
    }

    // Open history group
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(commands::simplify());
    }

    bool smoothJoins = true;
    core::Array<core::Id> uncutIds = workspace->simplify(selection, smoothJoins);
    canvas->setSelection(std::move(uncutIds));

    // Close history group
    if (undoGroup) {
        undoGroup->close();
    }
}

namespace {

dom::DocumentPtr copyDoc_;

} // namespace

void Select::onCut_() {

    workspace::Workspace* workspace = this->workspace();
    if (!workspace) {
        return;
    }

    core::Array<core::Id> selection = this->selection();
    if (selection.isEmpty()) {
        return;
    }

    // Open history group
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(commands::cut());
    }

    copyDoc_ = workspace->cut(selection);

    // Close history group
    if (undoGroup) {
        undoGroup->close();
    }
}

void Select::onCopy_() {

    workspace::Workspace* workspace = this->workspace();
    if (!workspace) {
        return;
    }

    core::Array<core::Id> selection = this->selection();
    if (selection.isEmpty()) {
        return;
    }

    copyDoc_ = workspace->copy(selection);
}

void Select::onPaste_() {

    workspace::Workspace* workspace = this->workspace();
    if (!workspace) {
        return;
    }

    // Open history group
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(commands::paste());
    }

    // Perform the paste operation
    core::Array<core::Id> pasted = workspace->paste(copyDoc_);

    // Set pasted elements as new selection
    canvas::Canvas* canvas = this->canvas();
    if (canvas) {
        canvas->setSelection(pasted);
    }

    // Close history group
    if (undoGroup) {
        undoGroup->close();
    }
}

} // namespace vgc::tools
