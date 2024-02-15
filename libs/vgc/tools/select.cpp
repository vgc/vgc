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

#include <algorithm> // max
#include <set>

#include <vgc/canvas/documentmanager.h>
#include <vgc/canvas/workspaceselection.h>
#include <vgc/core/algorithms.h> // sort, set_difference, appender
#include <vgc/graphics/detail/shapeutil.h>
#include <vgc/graphics/strings.h>
#include <vgc/tools/copypaste.h>
#include <vgc/tools/topology.h>
#include <vgc/ui/boolsettingedit.h>
#include <vgc/ui/column.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/standardmenus.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/detail/operationsimpl.h>
#include <vgc/workspace/colors.h>
#include <vgc/workspace/face.h>
#include <vgc/workspace/workspace.h>

namespace vgc::tools {

namespace commands {

using ui::Key;
using ui::Shortcut;
using ui::modifierkeys::alt;
using ui::modifierkeys::ctrl;
using ui::modifierkeys::shift;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectAll,
    "tools.select.selectAll",
    "Select All",
    Shortcut(ctrl, Key::A));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    deselectAll,
    "tools.select.deselectAll",
    "Deselect All",
    Shortcut(ctrl | shift, Key::A));

// Secondary shortcut for deselectAll.
//
namespace {

VGC_UI_ADD_DEFAULT_SHORTCUT(deselectAll(), Shortcut(Key::Escape))

} // namespace

VGC_UI_DEFINE_WINDOW_COMMAND( //
    invertSelection,
    "tools.select.invertSelection",
    "Invert Selection",
    Shortcut(ctrl, Key::I));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    invertSelectionSameType,
    "tools.select.invertSelectionSameType",
    "Invert Selection (Same Type)",
    Shortcut(ctrl | shift, Key::I));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    invertSelectionExcludeBoundary,
    "tools.select.invertSelectionExcludeBoundary",
    "Invert Selection (Exclude Boundary)",
    Shortcut(ctrl | alt, Key::I));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectBoundary,
    "tools.select.selectBoundary",
    "Select Boundary",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectOuterBoundary,
    "tools.select.selectOuterBoundary",
    "Select Outer Boundary",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectClosure,
    "tools.select.selectClosure",
    "Select Closure (Selection + Boundary)",
    Shortcut(Key::C));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectStar,
    "tools.select.selectStar",
    "Select Star",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectOpening,
    "tools.select.selectOpening",
    "Select Opening (Selection + Star)",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectConnectedObjects,
    "tools.select.selectConnectedObjects",
    "Select Connected Objects",
    Shortcut(shift, Key::C));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectMore,
    "tools.select.selectMore",
    "Select More",
    Shortcut(Key::GreaterThan));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectLess,
    "tools.select.selectLess",
    "Select Less",
    Shortcut(Key::LessThan));

// Secondary shortcuts for select more/less.
//
// We need this because on a QWERTY keyboard, typing `>` requires pressing `Shift`,
// and the KeyEvent reports `Shift + >`, which currently wouldn't match `>`.
//
// TODO: Properly handle shortcuts whose key require to press Shift or AltGr on some
// keyboard layout.
//
namespace {

VGC_UI_ADD_DEFAULT_SHORTCUT(selectMore(), Shortcut(shift, Key::GreaterThan))
VGC_UI_ADD_DEFAULT_SHORTCUT(selectLess(), Shortcut(shift, Key::LessThan))

} // namespace

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectVertices,
    "tools.select.selectVertices",
    "Select Vertices",
    Shortcut(alt, Key::V));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectEdges,
    "tools.select.selectEdges",
    "Select Edges",
    Shortcut(alt, Key::E));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectFaces,
    "tools.select.selectFaces",
    "Select Faces",
    Shortcut(alt, Key::F));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    deselectVertices,
    "tools.select.deselectVertices",
    "Deselect Vertices",
    Shortcut(alt | shift, Key::V));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    deselectEdges,
    "tools.select.deselectEdges",
    "Deselect Edges",
    Shortcut(alt | shift, Key::E));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    deselectFaces,
    "tools.select.deselectFaces",
    "Deselect Faces",
    Shortcut(alt | shift, Key::F));

} // namespace commands

SelectModule::SelectModule(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    documentManager_ = importModule<canvas::DocumentManager>();

    ui::MenuWeakPtr selectMenu;
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        if (auto menuBar = standardMenus->menuBar().lock()) {
            Int index = std::max<Int>(0, menuBar->numItems() - 1);
            selectMenu = menuBar->createSubMenuAt(index, "Select");
        }
    }

    using namespace commands;
    ui::ModuleActionCreator c(this);
    c.setMenu(selectMenu);

    c.addAction(selectAll(), onSelectAll_Slot());
    c.addAction(deselectAll(), onDeselectAll_Slot());

    c.addSeparator();
    c.addAction(invertSelection(), onInvertSelection_Slot());
    c.addAction(invertSelectionSameType(), onInvertSelectionSameType_Slot());
    c.addAction(
        invertSelectionExcludeBoundary(), onInvertSelectionExcludeBoundary_Slot());

    c.addSeparator();
    c.addAction(selectBoundary(), onSelectBoundary_Slot());
    c.addAction(selectOuterBoundary(), onSelectOuterBoundary_Slot());
    c.addAction(selectClosure(), onSelectClosure_Slot());

    c.addSeparator();
    c.addAction(selectStar(), onSelectStar_Slot());
    c.addAction(selectOpening(), onSelectOpening_Slot());

    c.addSeparator();
    c.addAction(selectConnectedObjects(), onSelectConnectedObjects_Slot());

    c.addSeparator();
    c.addAction(selectMore(), onSelectMore_Slot());
    c.addAction(selectLess(), onSelectLess_Slot());

    c.addSeparator();
    c.addAction(selectVertices(), onSelectVertices_Slot());
    c.addAction(selectEdges(), onSelectEdges_Slot());
    c.addAction(selectFaces(), onSelectFaces_Slot());

    c.addSeparator();
    c.addAction(deselectVertices(), onDeselectVertices_Slot());
    c.addAction(deselectEdges(), onDeselectEdges_Slot());
    c.addAction(deselectFaces(), onDeselectFaces_Slot());
}

SelectModulePtr SelectModule::create(const ui::ModuleContext& context) {
    return core::createObject<SelectModule>(context);
}

namespace {

class SelectContextLock {
public:
    SelectContextLock(canvas::DocumentManagerWeakPtr documentManager_) {
        if (auto documentManager = documentManager_.lock()) {
            workspace_ = documentManager->currentWorkspace().lock();
            if (workspace_) {
                workspaceSelection_ = documentManager->currentWorkspaceSelection().lock();
            }
        }
    }

    // Returns whether all locks are acquired.
    //
    explicit operator bool() const {
        return static_cast<bool>(workspaceSelection_);
    }

    workspace::Workspace* workspace() const {
        return workspace_.get();
    }

    canvas::WorkspaceSelection* workspaceSelection() const {
        return workspaceSelection_.get();
    }

    const core::Array<core::Id>& itemIds() const {
        return workspaceSelection_->itemIds();
    }

private:
    workspace::WorkspaceLockPtr workspace_;
    canvas::WorkspaceSelectionLockPtr workspaceSelection_;
};

} // namespace

// Note: when calling "Select All", we don't actually want to select all items
// in the workspace. Instead, we simply want to select the direct children of
// the VGC root element, which already implicitly selects their descendants.
//
// In the future, when group isolation mode will be implemented
// (double-clicking to "enter" a group), then when calling "Select All", it
// should select the direct children of the isolated group.
//
void SelectModule::onSelectAll_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> itemIds;
        if (workspace::Element* root = context.workspace()->vgcElement()) {
            workspace::Element* child = root->firstChild();
            while (child) {
                itemIds.append(child->id());
                child = child->nextSibling();
            }
        }
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

void SelectModule::onDeselectAll_() {
    if (auto context = SelectContextLock(documentManager_)) {
        context.workspaceSelection()->clear();
    }
}

namespace {

// When inverting the selection, by "same type", we basically want to mean "tag name".
// Unfortunately, some elements may not have a tag name, such as implicit
// vertices/edges/faces of basic shapes (rectangle, circle, etc.). So we use
// the class `SelectionType` to capture this. For now we do not have basic shapes so it's
// simply the tag name, but we can envision a more complex class later.
//
using SelectionType = core::StringId;

SelectionType getSelectionType(const workspace::Element* element) {
    if (element) {
        return element->tagName();
    }
    else {
        return SelectionType();
    }
}

SelectionType getSelectionType(const workspace::Workspace& workspace, core::Id id) {
    return getSelectionType(workspace.find(id));
}

} // namespace

// Note: like for Select All, inverting the selection should work differently
// when we implement isolation mode in the future.
//
// More discussion on inverting the selection.
// -------------------------------------------
//
// The rationale for not just having the default "Invert Selection" action is
// that if a user selects an edge and does "invert selection", then with the
// default algorithm, this would unfortunately also select the end vertices of
// the edge, which is often not what the user wants. For example, doing "invert
// selection" then "delete" would delete everything including the initally
// selected edge...
//
// Having "Invert Selection (Same Type)" is a simple alternative that
// partially solves this problem: it is useful in many cases, although
// it is not perfect in all cases.
//
// For example, if a user only has edges and vertices in the scene, then it is
// pretty much perfect: the user would typically only select edges, and doing
// "invert selection" would select all the other edges (but no vertices).
//
// However, if there are also faces, or groups (or basic shapes), perhaps the
// user would have liked to select those too? Something like "Invert Selection
// (Exclude Vertices)" might work in some cases, but not always. For example if
// the user selects a face and does "invert selection" probably he also doesn't
// want the edges in the boundary of the face to be selected, otherwise we
// still have the problem that "invert selection + delete" would delete the
// face.
//
// In conclusion, the desired result in many cases seem to be "Invert Selection
// (Exclude Boundary)". This is equivalent to doing "Select Closure" followed
// by "Invert Selection", but it's nice to have a shortcut that does it in one
// shot. This makes sense because the boundary of the selection should often be
// also considered part of the selection. For example, "Copy" (Ctrl + C) also
// copies the boundary, and we want a similar behavior for "Bring
// Forward/Backward".
//
// In any case, "Invert Selection (Same Type)" can also be useful, and not just
// for topology, for example if the user wants to select all text elements
// except a few of them. So we might as well provide it too.
//
// So a list of potentially useful alternatives would be:
// - Invert Selection (Default)
// - Invert Selection (Same Type)
// - Invert Selection (Exclude Boundary)
// - Invert Selection (Exclude Vertices)
// - Invert Selection (Exclude Vertices and Boundary)
//
// However, to avoid bloat, we do not provide the versions that exclude the
// vertices, since these can easily be removed from the selection as a second
// step (Deselect Vertices), and users are used to these being selected anyway,
// for example when doing a rectangle of selection, doing "Select All", or
// simply copy-pasting. There could be a general setting to never select them
// (except isolated vertices, or when clicking on a single vertex), but this
// seems more harmful than helpful: it would add another layer of confusion
// (why these vertices are selected when using this tool but not this one?),
// and it seems best to just let the users become familiar with the concept of
// vertices.

void SelectModule::onInvertSelection_() {
    if (auto context = SelectContextLock(documentManager_)) {
        const core::Array<core::Id>& oldItemIds = context.workspaceSelection()->itemIds();
        core::Array<core::Id> itemIds;
        if (workspace::Element* root = context.workspace()->vgcElement()) {
            workspace::Element* child = root->firstChild();
            while (child) {
                if (!oldItemIds.contains(child->id())) {
                    itemIds.append(child->id());
                }
                child = child->nextSibling();
            }
        }
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

// XXX:
// - behavior if selection is empty? For now we do nothing.
// - behavior if selection contains more than one type? For now we only use the first.
//
void SelectModule::onInvertSelectionSameType_() {
    if (auto context = SelectContextLock(documentManager_)) {
        const core::Array<core::Id>& oldItemIds = context.workspaceSelection()->itemIds();
        if (oldItemIds.isEmpty()) {
            return;
        }
        SelectionType targetType =
            getSelectionType(*context.workspace(), oldItemIds.first());
        if (targetType.isEmpty()) {
            return;
        }
        core::Array<core::Id> itemIds;
        if (workspace::Element* root = context.workspace()->vgcElement()) {
            workspace::Element* child = root->firstChild();
            while (child) {
                SelectionType type = getSelectionType(child);
                if (type == targetType && !oldItemIds.contains(child->id())) {
                    itemIds.append(child->id());
                }
                child = child->nextSibling();
            }
        }
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

void SelectModule::onInvertSelectionExcludeBoundary_() {
    if (auto context = SelectContextLock(documentManager_)) {
        const core::Array<core::Id>& oldItemIds = context.workspaceSelection()->itemIds();
        core::Array<core::Id> closure = context.workspace()->closure(oldItemIds);
        core::Array<core::Id> itemIds;
        if (workspace::Element* root = context.workspace()->vgcElement()) {
            workspace::Element* child = root->firstChild();
            while (child) {
                if (!closure.contains(child->id())) {
                    itemIds.append(child->id());
                }
                child = child->nextSibling();
            }
        }
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

void SelectModule::onSelectBoundary_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> oldItemIds = context.workspaceSelection()->itemIds();
        core::Array<core::Id> newItemIds = context.workspace()->boundary(oldItemIds);
        context.workspaceSelection()->setItemIds(newItemIds);
    }
}

void SelectModule::onSelectOuterBoundary_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> oldItemIds = context.workspaceSelection()->itemIds();
        core::Array<core::Id> newItemIds = context.workspace()->outerBoundary(oldItemIds);
        context.workspaceSelection()->setItemIds(newItemIds);
    }
}

void SelectModule::onSelectClosure_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> oldItemIds = context.workspaceSelection()->itemIds();
        core::Array<core::Id> newItemIds = context.workspace()->closure(oldItemIds);
        context.workspaceSelection()->setItemIds(newItemIds);
    }
}

void SelectModule::onSelectStar_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> oldItemIds = context.workspaceSelection()->itemIds();
        core::Array<core::Id> newItemIds = context.workspace()->star(oldItemIds);
        context.workspaceSelection()->setItemIds(newItemIds);
    }
}

void SelectModule::onSelectOpening_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> oldItemIds = context.workspaceSelection()->itemIds();
        core::Array<core::Id> newItemIds = context.workspace()->opening(oldItemIds);
        context.workspaceSelection()->setItemIds(newItemIds);
    }
}

void SelectModule::onSelectConnectedObjects_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> oldItemIds = context.workspaceSelection()->itemIds();
        core::Array<core::Id> newItemIds = context.workspace()->connected(oldItemIds);
        context.workspaceSelection()->setItemIds(newItemIds);
    }
}

void SelectModule::onSelectMore_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> input = context.workspaceSelection()->itemIds();
        core::Array<core::Id> opening = context.workspace()->opening(input);
        core::Array<core::Id> closure = context.workspace()->closure(opening);
        context.workspaceSelection()->setItemIds(closure);
    }
}

namespace {

// Shrinks the input from its boundary, if any.
// Assumes input is sorted.
//
core::Array<core::Id> selectLessOneStep_(
    const workspace::Workspace& workspace,
    core::ConstSpan<core::Id> input) {

    core::Array<core::Id> boundary = workspace.boundary(input);
    core::Array<core::Id> opening = workspace.opening(boundary);

    core::sort(opening);
    core::Array<core::Id> output;
    core::set_difference(input, opening, core::appender(output));

    return output;
}

} // namespace

void SelectModule::onSelectLess_() {
    if (auto context = SelectContextLock(documentManager_)) {

        // Initialize output
        core::Array<core::Id> output;

        // Separate closure into connected components.
        //
        // We use the closure so that it behaves in a more intuitive way in the
        // typical case where the user only select faces (or edges).
        //
        // Indeed, if the user only select faces (or edges), they are all
        // technically isolated to each other, since their shared boundary
        // isn't selected. So without the closure step, `Select Less` would
        // deselect them all, which is unexpected.
        //
        core::Array<core::Id> input = context.workspaceSelection()->itemIds();
        core::Array<core::Id> closure = context.workspace()->closure(input);
        core::Array<core::Array<core::Id>> connectedComponents =
            context.workspace()->connectedComponents(closure);

        // For each component
        for (core::Array<core::Id>& ids : connectedComponents) {

            // Fast skip if only one element (isolated vertex or non-VAC element)
            if (ids.length() == 1) {
                continue;
            }

            // Attempt to shrink it from its boundary
            core::sort(ids);
            core::Array<core::Id> newIds = selectLessOneStep_(*context.workspace(), ids);

            // If unchanged (loop of edges, sphere, or any set of cells without
            // boundary), then we randomly remove one of the elements and try
            // again.
            //
            if (newIds.length() == ids.length()) {
                core::ConstSpan<core::Id> ids2(ids.data(), ids.length() - 1);
                newIds = selectLessOneStep_(*context.workspace(), ids2);
            }

            // Add to output, except elements that were not initially
            // in the input (they were added in the closure step).
            //
            output.reserve(output.length() + newIds.length());
            for (core::Id id : newIds) {
                if (input.contains(id)) {
                    output.append(id);
                }
            }
        }

        context.workspaceSelection()->setItemIds(output);
    }
}

namespace {

core::Array<core::Id> selectCellType(
    const workspace::Workspace& workspace,
    const core::Array<core::Id>& itemIds,
    vacomplex::CellType cellType) {

    core::Array<core::Id> res;
    for (core::Id id : itemIds) {
        if (workspace::Element* item = workspace.find(id)) {
            if (vacomplex::Cell* cell = item->vacCell()) {
                if (cell->cellType() == cellType) {
                    res.append(id);
                }
            }
        }
    }
    return res;
}

core::Array<core::Id> deselectCellType(
    const workspace::Workspace& workspace,
    const core::Array<core::Id>& itemIds,
    vacomplex::CellType cellType) {

    core::Array<core::Id> res;
    for (core::Id id : itemIds) {
        if (workspace::Element* item = workspace.find(id)) {
            if (vacomplex::Cell* cell = item->vacCell()) {
                if (cell->cellType() == cellType) {
                    continue;
                }
            }
        }
        res.append(id);
    }
    return res;
}

} // namespace

void SelectModule::onSelectVertices_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> itemIds = selectCellType(
            *context.workspace(),
            context.workspaceSelection()->itemIds(),
            vacomplex::CellType::KeyVertex);
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

void SelectModule::onSelectEdges_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> itemIds = selectCellType(
            *context.workspace(),
            context.workspaceSelection()->itemIds(),
            vacomplex::CellType::KeyEdge);
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

void SelectModule::onSelectFaces_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> itemIds = selectCellType(
            *context.workspace(),
            context.workspaceSelection()->itemIds(),
            vacomplex::CellType::KeyFace);
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

void SelectModule::onDeselectVertices_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> itemIds = deselectCellType(
            *context.workspace(),
            context.workspaceSelection()->itemIds(),
            vacomplex::CellType::KeyVertex);
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

void SelectModule::onDeselectEdges_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> itemIds = deselectCellType(
            *context.workspace(),
            context.workspaceSelection()->itemIds(),
            vacomplex::CellType::KeyEdge);
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

void SelectModule::onDeselectFaces_() {
    if (auto context = SelectContextLock(documentManager_)) {
        core::Array<core::Id> itemIds = deselectCellType(
            *context.workspace(),
            context.workspaceSelection()->itemIds(),
            vacomplex::CellType::KeyFace);
        context.workspaceSelection()->setItemIds(itemIds);
    }
}

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

VGC_DECLARE_OBJECT(CutWithVertexAction);

class CutWithVertexAction : public ui::Action {
private:
    VGC_OBJECT(CutWithVertexAction, ui::Action)

protected:
    /// This is an implementation details.
    /// Please use `CutWithVertexAction::create()` instead.
    ///
    CutWithVertexAction(CreateKey key)
        : ui::Action(key, commands::cutWithVertex()) {
    }

public:
    /// Creates a `CutWithVertexAction`.
    ///
    static CutWithVertexActionPtr create() {
        return core::createObject<CutWithVertexAction>();
    }

public:
    void onMouseClick(ui::MouseEvent* event) override {

        auto tool = tool_.lock();
        if (!tool) {
            return;
        }
        auto context = tool->contextLock();
        if (!context) {
            return;
        }
        auto workspace = context.workspace();
        auto canvas = context.canvas();
        auto workspaceSelection = context.workspaceSelection();

        // Open history group
        core::UndoGroup* undoGroup = nullptr;
        core::History* history = workspace->history();
        if (history) {
            undoGroup = history->createUndoGroup(actionName());
        }

        geometry::Vec2d position(event->position());

        geometry::Mat4d inverseViewMatrix = canvas->camera().viewMatrix().inverted();
        geometry::Vec2d cursorPositionInWorkspace =
            inverseViewMatrix.transformPointAffine(position);

        core::Array<canvas::SelectionCandidate> candidates =
            canvas->computeSelectionCandidates(position);

        for (const auto& candidate : candidates) {
            workspace::Element* item = workspace->find(candidate.id());
            auto keItem = dynamic_cast<workspace::VacKeyEdge*>(item);
            if (keItem) {
                if (vacomplex::KeyEdge* ke = keItem->vacKeyEdgeNode()) {
                    const geometry::AbstractStroke2d* stroke = ke->data().stroke();
                    auto sampling = stroke->computeSampling(
                        geometry::CurveSamplingQuality::AdaptiveHigh);
                    // find closest location on curve
                    geometry::SampledCurveLocation closestLoc =
                        geometry::closestCenterlineLocation(
                            sampling.samples(), cursorPositionInWorkspace)
                            .location();
                    // convert to curve parameter
                    geometry::CurveParameter param =
                        stroke->resolveSampledLocation(closestLoc);
                    // do the cut
                    auto result = vacomplex::ops::cutEdge(ke, param);
                    // select resulting vertex
                    workspace::Element* vertexItem =
                        workspace->findVacElement(result.vertex());
                    if (vertexItem) {
                        workspaceSelection->setItemIds(std::array{vertexItem->id()});
                    }
                    break;
                }
            }
            auto kfItem = dynamic_cast<workspace::VacKeyFace*>(item);
            if (kfItem) {
                if (vacomplex::KeyFace* kf = kfItem->vacKeyFaceNode()) {
                    // do the cut
                    auto result =
                        vacomplex::ops::cutFaceWithVertex(kf, cursorPositionInWorkspace);
                    // select resulting vertex
                    workspace::Element* vertexItem = workspace->findVacElement(result);
                    if (vertexItem) {
                        workspaceSelection->setItemIds(std::array{vertexItem->id()});
                    }
                    break;
                }
            }
        }

        // Close operation
        if (undoGroup) {
            undoGroup->close();
        }
    }

public:
    SelectWeakPtr tool_;

    core::StringId actionName() const {
        static core::StringId actionName_("Vertex-Cut Edge");
        return actionName_;
    }
};

} // namespace

Select::Select(CreateKey key)
    : CanvasTool(key) {

    canvasChanged().connect(onCanvasChangedSlot_());
    onCanvasChanged_();

    options::showTransformBox()->valueChanged().connect(onShowTransformBoxChangedSlot_());
    onShowTransformBoxChanged_();

    CutWithVertexAction* cutWithVertexAction = createAction<CutWithVertexAction>();
    cutWithVertexAction->tool_ = this;

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

core::Array<core::Id> Select::selectedItemIds() {
    if (auto context = contextLock()) {
        return context.workspaceSelection()->itemIds();
    }
    else {
        return {};
    }
}

ui::WidgetPtr Select::doCreateOptionsWidget() const {
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

    auto context = contextLock();
    if (!context) {
        return isInAction_; // always true
    }
    auto workspace = context.workspace();
    auto canvas = context.canvas();

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
                initializeDragMoveData_(workspace.get(), elementsIds);
                break;
            }
            case DragAction::TranslateSelection: {
                initializeDragMoveData_(workspace.get(), selectionAtPress_);
                break;
            }
            }
        }
    }

    if (isDragging_) {
        geometry::Mat4d inverseViewMatrix = canvas->camera().viewMatrix().inverted();

        geometry::Vec2d cursorPosition(cursorPosition_);
        geometry::Vec2d cursorPositionAtPress(cursorPositionAtPress_);

        geometry::Vec2d cursorPositionInWorkspace =
            inverseViewMatrix.transformPointAffine(cursorPosition);
        geometry::Vec2d cursorPositionInWorkspaceAtPress =
            inverseViewMatrix.transformPointAffine(cursorPositionAtPress);

        switch (dragAction_) {
        case DragAction::Select: {
            rectCandidates_ = canvas->computeRectangleSelectionCandidates(
                cursorPositionAtPress, cursorPosition);
            selectionRectangleGeometry_.reset();
            requestRepaint();
            break;
        }
        case DragAction::TranslateCandidate:
        case DragAction::TranslateSelection: {
            deltaInWorkspace_ =
                cursorPositionInWorkspace - cursorPositionInWorkspaceAtPress;
            updateDragMovedElements_(workspace.get(), deltaInWorkspace_);
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

    auto context = contextLock();
    if (!context) {
        return false;
    }
    auto canvas = context.canvas();
    auto workspaceSelection = context.workspaceSelection();

    cursorPosition_ = event->position();

    ui::ModifierKeys keys = event->modifierKeys();
    ui::ModifierKeys supportedKeys =
        (ui::ModifierKey::Ctrl | ui::ModifierKey::Alt | ui::ModifierKey::Shift);
    ui::ModifierKeys unsupportedKeys = ~supportedKeys;

    if (!keys.hasAny(unsupportedKeys)) {
        isInAction_ = true;
        geometry::Vec2d position(event->position());
        candidates_ = canvas->computeSelectionCandidates(position);
        selectionAtPress_ = workspaceSelection->itemIds();
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
    auto context = contextLock();
    if (!context) {
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
    auto canvas = context.canvas();
    auto workspace = context.workspace();
    auto workspaceSelection = context.workspaceSelection();

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
            finalizeDragMovedElements_(workspace.get());
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
        workspaceSelection->setItemIds(selection);
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

    auto context = contextLock();
    if (!context) {
        return;
    }
    auto canvas = context.canvas();

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
                vacomplex::KeyEdgeData& data = ke->data();
                if (!ked.isEditStarted) {
                    ked.oldData = data;
                    ked.isEditStarted = true;
                }
                else {
                    data = ked.oldData;
                }
                data.translate(translationInWorkspace);
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
                const vacomplex::KeyEdgeData& data = ke->data();
                std::ignore = data;
                //data.finishEdit();
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
    if (auto connectedCanvas = connectedCanvas_.lock()) {
        connectedCanvas->aboutToBeDestroyed().disconnect(
            onCanvasAboutToBeDestroyedSlot_());
        connectedCanvas->workspaceSelectionChanged().disconnect(
            onSelectionChangedSlot_());
    }
    // TODO: allow `oldCanvas_->disconnect(on..Slot_())` syntax?
}

void Select::onCanvasChanged_() {
    disconnectCanvas_();
    connectedCanvas_ = canvas();
    if (auto connectedCanvas = connectedCanvas_.lock()) {
        connectedCanvas->aboutToBeDestroyed().connect(onCanvasAboutToBeDestroyedSlot_());
        connectedCanvas->workspaceSelectionChanged().connect(onSelectionChangedSlot_());
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
        if (auto canvas = this->canvas().lock()) {
            if (auto workspaceSelection = canvas->workspaceSelection().lock()) {
                transformBox_->setElements(workspaceSelection->itemIds());
                return;
            }
        }
        transformBox_->setElements({});
    }
}

namespace {

dom::DocumentPtr copyDoc_;

} // namespace

void Select::onCut_() {

    auto workspace = this->workspace().lock();
    if (!workspace) {
        return;
    }

    core::Array<core::Id> selection = this->selectedItemIds();
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

    auto workspace = this->workspace().lock();
    if (!workspace) {
        return;
    }

    core::Array<core::Id> selection = this->selectedItemIds();
    if (selection.isEmpty()) {
        return;
    }

    copyDoc_ = workspace->copy(selection);
}

void Select::onPaste_() {

    auto context = contextLock();
    if (!context) {
        return;
    }
    auto workspace = context.workspace();
    auto workspaceSelection = context.workspaceSelection();

    // Open history group
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(commands::paste());
    }

    // Perform the paste operation
    core::Array<core::Id> pasted = workspace->paste(copyDoc_);

    // Set pasted elements as new selection
    workspaceSelection->setItemIds(pasted);

    // Close history group
    if (undoGroup) {
        undoGroup->close();
    }
}

} // namespace vgc::tools
