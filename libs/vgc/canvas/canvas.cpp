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

#include <vgc/canvas/canvas.h>

#include <QBitmap>
#include <QCursor>
#include <QPainter>

#include <vgc/canvas/logcategories.h>
#include <vgc/canvas/strings.h>
#include <vgc/core/array.h>
#include <vgc/core/history.h>
#include <vgc/core/paths.h>
#include <vgc/core/stopwatch.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/strings.h>
#include <vgc/geometry/stroke.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/window.h>
#include <vgc/workspace/edge.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::canvas {

namespace {

namespace commands {

using ui::Key;

// TODO:
// Rename this to "FitSelectionToView" (F)
// Add "FitDocumentToView" (Shift+F, or the more standard Ctrl + 0, or both)
//
VGC_UI_DEFINE_WINDOW_COMMAND( //
    frameContent,
    "canvas.frameContent",
    "Frame Content",
    Key::F);

// TODO:
//
// D: Toggle between the last two display modes used (default: Normal + OutlineOverlay)
// Shift + D: Cycle between all display modes
//
// Note: using "D" for changing the display mode is nice since it is
// easy to remember (D = Display), is easily accessibly with the left
// hand on QWERTY keyboards, and is rarely used for anything very useful
// on other software (sometimes used for "reset colors to default")
//
VGC_UI_DEFINE_WINDOW_COMMAND( //
    cycleDisplayMode,
    "canvas.cycleDisplayMode",
    "Cycle Display Mode",
    Key::D);

} // namespace commands

} // namespace

SelectionListHistory::SelectionListHistory(CreateKey key, PrivateKey)
    : Object(key) {
}

SelectionListHistoryPtr SelectionListHistory::create() {
    return core::createObject<SelectionListHistory>(SelectionListHistory::PrivateKey{});
}

void SelectionListHistory::pushSelection(const SelectionList& list) {
    lists_.emplaceLast(list);
    selectionChanged().emit();
}

void SelectionListHistory::pushSelection(SelectionList&& list) {
    lists_.emplaceLast(std::move(list));
    selectionChanged().emit();
}

Canvas::Canvas(CreateKey key, workspace::Workspace* workspace)
    : Widget(key)
    , workspace_(workspace)
    , renderTask_("Render")
    , updateTask_("Update")
    , drawTask_("Draw") {

    // Set ClickFocus policy to be able to accept keyboard events (default
    // policy is NoFocus).
    setFocusPolicy(ui::FocusPolicy::Click);

    setClippingEnabled(true);

    if (workspace_) {
        workspace_->changed().connect(onWorkspaceChanged());
        // XXX to remove
        workspace_->document()->emitPendingDiff();
        workspace_->document()->changed().connect(onDocumentChanged());
    }

    addStyleClass(canvas::strings::Canvas);

    ui::Action* frameContentAction = createTriggerAction(commands::frameContent());
    frameContentAction->triggered().connect(onFrameContentSlot_());

    ui::Action* cycleDisplayModeAction =
        createTriggerAction(commands::cycleDisplayMode());
    cycleDisplayModeAction->triggered().connect(cycleDisplayModeSlot_());
}

CanvasPtr Canvas::create(workspace::Workspace* workspace) {
    return core::createObject<Canvas>(workspace);
}

void Canvas::setDisplayMode(DisplayMode displayMode) {
    if (displayMode_ != displayMode) {
        displayMode_ = displayMode;
        requestRepaint();
    }
}

void Canvas::setWorkspace(workspace::Workspace* workspace) {

    if (workspace_ == workspace) {
        return;
    }

    if (workspace_) {
        workspace_->disconnect(this);
        // XXX to remove
        workspace_->document()->disconnect(this);
    }

    workspace_ = workspace;
    if (workspace_) {
        workspace_->changed().connect(onWorkspaceChanged());
        // XXX to remove
        workspace_->document()->changed().connect(onDocumentChanged());
        onWorkspaceChanged_();
    }

    requestRepaint();
    workspaceReplaced().emit();
}

void Canvas::startLoggingUnder(core::PerformanceLog* parent) {
    core::PerformanceLog* renderLog = renderTask_.startLoggingUnder(parent);
    updateTask_.startLoggingUnder(renderLog);
    drawTask_.startLoggingUnder(renderLog);
}

void Canvas::stopLoggingUnder(core::PerformanceLog* parent) {
    core::PerformanceLogPtr renderLog = renderTask_.stopLoggingUnder(parent);
    updateTask_.stopLoggingUnder(renderLog.get());
    drawTask_.stopLoggingUnder(renderLog.get());
}

namespace {

void deleteElements(
    const core::Array<core::Id>& elementIds,
    workspace::Workspace* workspace) {

    if (!workspace || elementIds.isEmpty()) {
        return;
    }

    // Open history group
    static core::StringId Delete_Element("Delete Element");
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(Delete_Element);
    }

    // TODO: use softDelete when implemented.
    workspace->hardDelete(elementIds);

    // Close operation
    if (undoGroup) {
        undoGroup->close();
    }
}

} // namespace

core::Array<core::Id> Canvas::selection() const {
    return selectedElementIds_;
}

void Canvas::setSelection(const core::Array<core::Id>& elementIds) {
    selectedElementIds_.clear();
    for (core::Id id : elementIds) {
        if (!selectedElementIds_.contains(id)) {
            selectedElementIds_.append(id);
        }
    }
    selectionChanged().emit();
    requestRepaint();
}

void Canvas::clearSelection() {
    setSelection({});
}

core::Array<SelectionCandidate>
Canvas::computeSelectionCandidates(const geometry::Vec2f& position) const {

    core::Array<SelectionCandidate> result;

    geometry::Vec2d viewCoords(position);
    geometry::Vec2d worldCoords =
        camera_.viewMatrix().inverted().transformPointAffine(viewCoords);

    using namespace style::literals;
    style::Length dpTol = 7.0_dp;
    double tol = dpTol.toPx(styleMetrics()) / camera_.zoom();

    bool isOutlineOverlay = (displayMode_ == DisplayMode::OutlineOverlay);
    bool isMeshEnabled = isOutlineOverlay || (displayMode_ == DisplayMode::Normal);
    bool isOutlineEnabled = isOutlineOverlay || (displayMode_ == DisplayMode::Outline);

    if (workspace_) {
        workspace_->visitDepthFirst(
            [](workspace::Element*, Int) { return true; },
            [&, tol, worldCoords](workspace::Element* e, Int /*depth*/) {
                if (!e) {
                    return;
                }
                vacomplex::Cell* cell = nullptr;
                if (e->isVacElement()) {
                    vacomplex::Node* node = e->toVacElement()->vacNode();
                    if (node && node->isCell()) {
                        cell = node->toCellUnchecked();
                    }
                }
                double tolerance = tol;
                double dist = 0;
                bool outlineOnly = isOutlineEnabled;
                if (isMeshEnabled) {
                    // keep faces selectable
                    if (cell && cell->cellType() == vacomplex::CellType::KeyFace) {
                        outlineOnly = false;
                    }
                }
                if (e->isSelectableAt(worldCoords, outlineOnly, tolerance, &dist)) {
                    Int priority = 1000;
                    if (isOutlineEnabled && cell) {
                        switch (cell->cellType()) {
                        case vacomplex::CellType::KeyVertex:
                            priority = 3000;
                            break;
                        case vacomplex::CellType::KeyEdge:
                            priority = 2000;
                            break;
                        default:
                            break;
                        }
                    }
                    result.emplaceLast(e->id(), 0, priority);
                }
            });

        // order from front to back
        std::reverse(result.begin(), result.end());

        // sort by selection distance, stable to keep Z order priority
        std::stable_sort(
            result.begin(),
            result.end(),
            [](const SelectionCandidate& a, const SelectionCandidate& b) {
                if (a.priority() != b.priority()) {
                    return a.priority() > b.priority();
                }
                //else {
                //    return a.distance() < b.distance();
                //}
                return false;
            });
    }

    return result;
}

core::Array<core::Id> Canvas::computeRectangleSelectionCandidates(
    const geometry::Vec2f& a,
    const geometry::Vec2f& b) const {

    using geometry::Vec2d;

    core::Array<core::Id> result;

    if (workspace_) {
        geometry::Mat4d invView = camera().viewMatrix().inverted();
        geometry::Rect2d rect = geometry::Rect2d::empty;
        rect.uniteWith(invView.transformPointAffine(Vec2d(a)));
        rect.uniteWith(invView.transformPointAffine(Vec2d(b)));

        workspace_->visitDepthFirst(
            [](workspace::Element*, Int) { return true; },
            [&, rect](workspace::Element* e, Int /*depth*/) {
                if (!e) {
                    return;
                }
                if (e->isSelectableInRect(rect)) {
                    result.emplaceLast(e->id());
                }
            });

        // order from front to back
        std::reverse(result.begin(), result.end());
    }

    return result;
}

// TODO:
// - Use commands/actions instead of hard-coded onKeyPress
// - Review shortcuts (advanced/debug shortcut should probably use key modifiers)
//
bool Canvas::onKeyPress(ui::KeyPressEvent* event) {

    using geometry::CurveSamplingQuality;

    switch (event->key()) {
    case ui::Key::T:
        polygonMode_ = polygonMode_ ? 0 : 1;
        requestRepaint();
        break;
    case ui::Key::I:
        switch (requestedTesselationMode_) {
        case CurveSamplingQuality::Disabled:
            requestedTesselationMode_ = CurveSamplingQuality::UniformVeryLow;
            break;
        case CurveSamplingQuality::UniformVeryLow:
            requestedTesselationMode_ = CurveSamplingQuality::AdaptiveLow;
            break;
        case CurveSamplingQuality::AdaptiveLow:
            requestedTesselationMode_ = CurveSamplingQuality::UniformHigh;
            break;
        case CurveSamplingQuality::UniformHigh:
            requestedTesselationMode_ = CurveSamplingQuality::AdaptiveHigh;
            break;
        case CurveSamplingQuality::AdaptiveHigh:
            requestedTesselationMode_ = CurveSamplingQuality::UniformVeryHigh;
            break;
        case CurveSamplingQuality::UniformVeryHigh:
            requestedTesselationMode_ = CurveSamplingQuality::Disabled;
            break;
        }
        VGC_INFO(
            LogVgcCanvas,
            "Switched edge subdivision quality to: {}",
            core::Enum::prettyName(requestedTesselationMode_));
        reTesselate = true;
        requestRepaint();
        break;
    case ui::Key::P:
        showControlPoints_ = !showControlPoints_;
        requestRepaint();
        break;
    case ui::Key::Backspace:
    case ui::Key::Delete:
        deleteElements(selectedElementIds_, workspace());
        clearSelection();
        break;
    default:
        return false;
    }

    return true;
    // Don't factor out "update()" here, to avoid unnecessary redraws for keys
    // not handled here, including modifiers.
}

void Canvas::onWorkspaceChanged_() {

    //selectionCandidateElements_.clear();
    //selectedElementId_ = 0;

    // ask for redraw
    requestRepaint();
}

void Canvas::onDocumentChanged_(const dom::Diff& /*diff*/) {
}

namespace {

// Time elapsed from press after which the action becomes a drag.
inline constexpr double dragTimeThreshold = 0.5;
inline constexpr float dragDeltaThreshold = 5;

} // namespace

// Reimplementation of Widget virtual methods

bool Canvas::onMouseMove(ui::MouseMoveEvent* event) {

    if (!mousePressed_) {
        return false;
    }

    if (!isDragging_) {
        // Initiate drag if:
        // - mouse position moved more than a few pixels, or
        // - mouse pressed for longer than a few 1/10s of seconds
        //
        double deltaTime = event->timestamp() - timeAtPress_;
        float deltaPos = (event->position() - mousePosAtPress_).length();
        if (deltaPos >= dragDeltaThreshold || deltaTime > dragTimeThreshold) {

            isDragging_ = true;
        }
    }

    // Note: event.button() is always NoButton for move events. This is why
    // we use the variable isPanning_ and isSketching_ to remember the current
    // mouse action. In the future, we'll abstract this mechanism in a separate
    // class.

    geometry::Vec2d mousePosAtPress(mousePosAtPress_);
    geometry::Vec2d mousePos(event->position());

    if (isPanning_) {
        geometry::Vec2d delta = mousePosAtPress - mousePos;
        camera_.setCenter(cameraAtPress_.center() + delta);
        requestRepaint();
        return true;
    }
    else if (isRotating_) {
        // Set new camera rotation
        // XXX rotateViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double rotateViewSensitivity = 0.01;
        geometry::Vec2d deltaPos = mousePosAtPress - mousePos;
        double deltaRotation = rotateViewSensitivity * (deltaPos.x() - deltaPos.y());
        camera_.setRotation(cameraAtPress_.rotation() + deltaRotation);

        // Set new camera center so that rotation center = mouse pos at press
        geometry::Vec2d pivotViewCoords = mousePosAtPress;
        geometry::Vec2d pivotWorldCoords =
            cameraAtPress_.viewMatrix().inverted().transformPointAffine(pivotViewCoords);
        geometry::Vec2d pivotViewCoordsNow =
            camera_.viewMatrix().transformPointAffine(pivotWorldCoords);
        camera_.setCenter(camera_.center() - pivotViewCoords + pivotViewCoordsNow);

        requestRepaint();
        return true;
    }
    else if (isZooming_) {
        // Set new camera zoom
        // XXX zoomViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double zoomViewSensitivity = 0.005;
        geometry::Vec2d deltaPos = mousePosAtPress - mousePos;
        const double s = std::exp(zoomViewSensitivity * (deltaPos.y() - deltaPos.x()));
        camera_.setZoom(cameraAtPress_.zoom() * s);

        // Set new camera center so that zoom center = mouse pos at press
        geometry::Vec2d pivotViewCoords = mousePosAtPress;
        geometry::Vec2d pivotWorldCoords =
            cameraAtPress_.viewMatrix().inverted().transformPointAffine(pivotViewCoords);
        geometry::Vec2d pivotViewCoordsNow =
            camera_.viewMatrix().transformPointAffine(pivotWorldCoords);
        camera_.setCenter(camera_.center() - pivotViewCoords + pivotViewCoordsNow);

        requestRepaint();
        return true;
    }

    return false;
}

bool Canvas::onMousePress(ui::MousePressEvent* event) {

    if (mousePressed_ || tabletPressed_) {
        return true;
    }
    mousePressed_ = true;
    mouseButtonAtPress_ = event->button();

    if (isPanning_ || isRotating_ || isZooming_) {
        return true;
    }

    using K = ui::ModifierKey;
    using B = ui::MouseButton;
    ui::ModifierKeys keys = event->modifierKeys();
    ui::MouseButton button = event->button();
    if (keys == K::None && button == B::Middle) {
        isPanning_ = true;
    }
    else if (keys == K::Alt && button == B::Right) {
        isRotating_ = true;
    }
    else if (keys == K::Alt && button == B::Middle) {
        isZooming_ = true;
    }

    if (isPanning_ || isRotating_ || isZooming_) {
        mousePosAtPress_ = event->position();
        cameraAtPress_ = camera_;
        timeAtPress_ = event->timestamp();
        return true;
    }

    return false;
}

bool Canvas::onMouseRelease(ui::MouseReleaseEvent* event) {

    if (!mousePressed_ || mouseButtonAtPress_ != event->button()) {
        return false;
    }

    if (!isDragging_) {
        double deltaTime = event->timestamp() - timeAtPress_;
        isDragging_ = deltaTime > dragTimeThreshold;
    }

    if (isRotating_ && !isDragging_) {
        // Save mouse pos in world coords before modifying camera
        geometry::Vec2d mousePos(mousePosAtPress_);
        geometry::Vec2d p1 =
            camera_.viewMatrix().inverted().transformPointAffine(mousePos);

        // Reset camera rotation
        camera_.setRotation(0);

        // Set new camera center so that zoom center = mouse pos at scroll
        geometry::Vec2d p2 = camera_.viewMatrix().transformPointAffine(p1);
        camera_.setCenter(camera_.center() - mousePos + p2);

        requestRepaint();
    }

    isRotating_ = false;
    isPanning_ = false;
    isZooming_ = false;
    mousePressed_ = false;
    isDragging_ = false;

    return true;
}

bool Canvas::onMouseScroll(ui::ScrollEvent* event) {
    if (mousePressed_) {
        return true;
    }

    double oldZoom = camera_.zoom();
    double newZoom = 0;

    // Cubic Root of 2 (we want to double zoom every 3 steps)
    constexpr double fallbackZoomFactor = 1.25992104989487;

    if (event->modifierKeys().isEmpty()) {
        Int steps = event->verticalSteps();
        constexpr double q23 = 2.0 / 3;
        constexpr auto levels = std::array{
            q23 / 32,   0.8 / 32,  1.0 / 32,  q23 / 16,   0.8 / 16,  1.0 / 16,
            q23 / 8,    0.8 / 8,   1.0 / 8,   q23 / 4,    0.8 / 4,   1.0 / 4,
            q23 / 2,    0.8 / 2,   1.0 / 2,   q23,        0.8,       1.0,
            1.25,       1.5,       1.0 * 2,   1.25 * 2,   1.5 * 2,   1.0 * 4,
            1.25 * 4,   1.5 * 4,   1.0 * 8,   1.25 * 8,   1.5 * 8,   1.0 * 16,
            1.25 * 16,  1.5 * 16,  1.0 * 32,  1.25 * 32,  1.5 * 32,  1.0 * 64,
            1.25 * 64,  1.5 * 64,  1.0 * 128, 1.25 * 128, 1.5 * 128, 1.0 * 256,
            1.25 * 256, 1.5 * 256, 1.0 * 512, 1.25 * 512, 1.5 * 512,
        };
        Int n = core::int_cast<Int>(std::size(levels));

        // Use case: user is at level 0, zooms out N times and zooms in N times.
        // Due to floating point inaccuracy, zoom may not be back to level 0 value.
        // Applying this factor snaps zoom to level 0.
        // Similar reasoning for last level and inverse zoom operations.
        constexpr double snapZoomFactor = 1.001;

        if (steps == 0) {
            // no change
        }
        else if (steps > 0) {
            // zoom in
            newZoom = oldZoom * fallbackZoomFactor;
            if (n > 0 && newZoom * snapZoomFactor > levels.at(0)
                && oldZoom < levels.at(n - 1)) {
                for (Int i = 0; i < n; ++i) {
                    newZoom = levels.at(i);
                    if (newZoom > oldZoom) {
                        break;
                    }
                }
            }
        }
        else { // steps < 0
            // zoom out
            newZoom = oldZoom / fallbackZoomFactor;
            if (n > 0 && newZoom / snapZoomFactor < levels.at(n - 1)
                && oldZoom > levels.at(0)) {
                for (Int i = n - 1; i >= 0; --i) {
                    newZoom = levels.at(i);
                    if (newZoom < oldZoom) {
                        break;
                    }
                }
            }
        }
    }
    else if (event->modifierKeys() == ui::ModifierKey::Alt) {
        // At least on Linux KDE, scrolling on a touchpad with Alt pressed
        // switches from vertical to horizontal scrolling.
        // So we use horizontal if vertical delta is zero.
        geometry::Vec2f deltas = event->scrollDelta();
        float d = deltas.y() != 0 ? deltas.y() : deltas.x();
        newZoom = oldZoom * std::pow(fallbackZoomFactor, d);
    }

    if (newZoom != 0) {
        // Save mouse pos in world coords before applying zoom
        geometry::Vec2d mousePos = geometry::Vec2d(event->position());
        geometry::Vec2d p1 =
            camera_.viewMatrix().inverted().transformPointAffine(mousePos);

        camera_.setZoom(newZoom);

        // Set new camera center so that zoom center = mouse pos at scroll
        geometry::Vec2d p2 = camera_.viewMatrix().transformPointAffine(p1);
        camera_.setCenter(camera_.center() - mousePos + p2);

        requestGeometryUpdate();
        requestRepaint();
    }

    return true;
}

void Canvas::onMouseEnter() {
}

void Canvas::onMouseLeave() {
}

void Canvas::onVisible() {
}

void Canvas::onHidden() {
}

void Canvas::onResize() {
    camera_.setViewportSize(width(), height());
    reload_ = true;
}

geometry::Vec2f Canvas::computePreferredSize() const {
    return geometry::Vec2f(160, 120);
}

void Canvas::onPaintCreate(graphics::Engine* engine) {

    SuperClass::onPaintCreate(engine);

    using namespace graphics;

    RasterizerStateCreateInfo createInfo = {};
    fillRS_ = engine->createRasterizerState(createInfo);
    createInfo.setFillMode(FillMode::Wireframe);
    wireframeRS_ = engine->createRasterizerState(createInfo);
    bgGeometry_ = engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XYRGB);

    reload_ = true;
}

// Note: In this override, we intentionally not call SuperClass::onPaintDraw()
// since we draw our own background. We call paintChildren() explicitly at the
// end to call the `onPaintDraw()` method of the CanvasTool children of the
// Canvas.
//
void Canvas::onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) {

    using namespace graphics;
    namespace gs = graphics::strings;

    drawTask_.start();

    auto modifiedParameters = PipelineParameter::RasterizerState;
    engine->pushPipelineParameters(modifiedParameters);

    engine->setProgram(BuiltinProgram::Simple);

    // Draw background as a (triangle strip) quad
    //
    engine->setRasterizerState(fillRS_);
    if (reload_) {
        reload_ = false;
        core::FloatArray a;
        geometry::Vec2f sz = size();
        a.extend({
            0.f,    0.f,    1.f, 1.f, 1.f, //
            0.f,    sz.y(), 1.f, 1.f, 1.f, //
            sz.x(), 0.f,    1.f, 1.f, 1.f, //
            sz.x(), sz.y(), 1.f, 1.f, 1.f  //
        });
        engine->updateVertexBufferData(bgGeometry_, std::move(a));
    }
    engine->draw(bgGeometry_);

    engine->setRasterizerState((polygonMode_ == 1) ? wireframeRS_ : fillRS_);

    geometry::Mat4f vm = engine->viewMatrix();
    geometry::Mat4f cameraViewf(camera_.viewMatrix());
    engine->pushViewMatrix(vm * cameraViewf);

    core::Array<workspace::Element*> selectedElements = selectedElements_();

    // render visit
    // todo:
    //  - use transforms
    //  - setup target for layers (painting a layer means using its result)

    workspace::PaintOptions commonPaintOptions = {};
    if (showControlPoints_) {
        commonPaintOptions.set(workspace::PaintOption::Editing);
    }

    if (workspace_) {
        workspace_->sync();
        //VGC_PROFILE_SCOPE("Canvas:WorkspaceVisit");
        if (reTesselate) {
            workspace_->visitDepthFirstPreOrder( //
                [=](workspace::Element* e, Int /*depth*/) {
                    if (!e) {
                        return;
                    }
                    if (e->isVacElement()) {
                        // todo: should we use an enum to avoid dynamic_cast ?
                        // if an error happens with the Element creation we cannot rely on vac node type.
                        auto edge = dynamic_cast<workspace::VacKeyEdge*>(e);
                        if (edge) {
                            //profileName = "Canvas:WorkspaceDrawEdgeElement";
                            edge->setTesselationMode(requestedTesselationMode_);
                        }
                    }
                });
        }

        // Draw Normal
        if (displayMode_ == DisplayMode::Normal
            || displayMode_ == DisplayMode::OutlineOverlay) {
            workspace::PaintOptions paintOptions = commonPaintOptions;
            workspace_->visitDepthFirst(
                [](workspace::Element* /*e*/, Int /*depth*/) {
                    // we always visit children for now
                    return true;
                },
                [=](workspace::Element* e, Int /*depth*/) {
                    if (e) {
                        e->paint(engine, {}, paintOptions);
                    }
                });
        }

        // Draw Outline
        bool isOutlineEnabled = displayMode_ == DisplayMode::Outline
                                || displayMode_ == DisplayMode::OutlineOverlay;
        if (isOutlineEnabled) {
            workspace::PaintOptions paintOptions = commonPaintOptions;
            paintOptions.set(workspace::PaintOption::Outline);
            workspace_->visitDepthFirst(
                [](workspace::Element* /*e*/, Int /*depth*/) {
                    // we always visit children for now
                    return true;
                },
                [=](workspace::Element* e, Int /*depth*/) {
                    if (e) {
                        e->paint(engine, {}, paintOptions);
                    }
                });
        }

        // Draw Selection
        if (!selectedElements.isEmpty()) {
            workspace::PaintOptions paintOptions = commonPaintOptions;
            paintOptions.set(workspace::PaintOption::Selected);
            if (isOutlineEnabled) {
                paintOptions.set(workspace::PaintOption::Outline);
            }
            workspace_->visitDepthFirst(
                [](workspace::Element* /*e*/, Int /*depth*/) {
                    // we always visit children for now
                    return true;
                },
                [=, &selectedElements](workspace::Element* e, Int /*depth*/) {
                    if (e && selectedElements.contains(e)) {
                        e->paint(engine, {}, paintOptions);
                        if (isOutlineEnabled
                            || paintOptions.has(workspace::PaintOption::Editing)) {
                            // Redraw outline of end vertices on top of selected edges,
                            // otherwise the centerline of the selected edge is on top of the
                            // outline of its end vertices and it doesn't look good.
                            if (auto edge = dynamic_cast<workspace::VacKeyEdge*>(e)) {
                                workspace::VacKeyVertex* startVertex =
                                    edge->startVertex();
                                workspace::VacKeyVertex* endVertex = edge->endVertex();
                                workspace::PaintOptions paintOptions2 = paintOptions;
                                paintOptions2.unset(workspace::PaintOption::Selected);
                                if (startVertex
                                    && !selectedElements.contains(startVertex)) {
                                    startVertex->paint(engine, {}, paintOptions2);
                                }
                                if (endVertex && !selectedElements.contains(endVertex)) {
                                    endVertex->paint(engine, {}, paintOptions2);
                                }
                            }
                        }
                    }
                });
        }

        reTesselate = false;
    }

    engine->popViewMatrix();
    engine->popPipelineParameters(modifiedParameters);

    drawTask_.stop();

    // Paint CanvasTool children overlays
    paintChildren(engine, options);
}

void Canvas::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    bgGeometry_.reset();
    fillRS_.reset();
    wireframeRS_.reset();
}

void Canvas::updateChildrenGeometry() {
    for (auto c : children()) {
        c->updateGeometry(rect());
    }
}

core::Array<workspace::Element*> Canvas::selectedElements_() const {
    core::Array<workspace::Element*> result;
    if (workspace()) {
        for (core::Id id : selectedElementIds_) {
            workspace::Element* element = workspace()->find(id);
            if (element && !result.contains(element)) {
                result.append(element);
            }
        }
    }
    return result;
}

void Canvas::onFrameContent_() {
    if (!workspace_) {
        return;
    }

    geometry::Rect2d frame = geometry::Rect2d::empty;
    if (selectedElementIds_.isEmpty()) {
        // Frame All
        // TODO: implement Workspace::boundingBox().
        workspace_->visitDepthFirstPreOrder( //
            [&frame](workspace::Element* e, Int /*depth*/) {
                frame.uniteWith(e->boundingBox());
            });
    }
    else {
        // Frame Selection
        for (core::Id id : selectedElementIds_) {
            workspace::Element* e = workspace_->find(id);
            if (e) {
                frame.uniteWith(e->boundingBox());
            }
        }
    }

    if (!frame.isDegenerate()) {

        double oldRotation = camera_.rotation();
        geometry::Vec2d boundingCircleDiameter(frame.width(), frame.height());
        double a = boundingCircleDiameter.length();
        double b = camera_.viewportWidth() / camera_.viewportHeight();
        double z = 1;
        if (b <= 1) {
            z = camera_.viewportWidth() / (a * 1.1);
        }
        else {
            z = camera_.viewportHeight() / (a * 1.1);
        }

        geometry::Vec2d bboxCenter = 0.5 * (frame.pMin() + frame.pMax());

        camera_.setRotation(0);
        camera_.setZoom(z);
        camera_.setCenter(z * bboxCenter);

        // now re-rotate.
        // camera setters are not trivial.
        geometry::Vec2d c0 =
            0.5 * geometry::Vec2d(camera_.viewportWidth(), camera_.viewportHeight());
        geometry::Vec2d c1 = camera_.viewMatrix().inverted().transformPointAffine(c0);
        camera_.setRotation(oldRotation);
        geometry::Vec2d c2 = camera_.viewMatrix().transformPointAffine(c1);
        camera_.setCenter(camera_.center() - c0 + c2);

        requestRepaint();
    }
}

void Canvas::cycleDisplayMode_() {
    switch (displayMode_) {
    case DisplayMode::Normal:
        displayMode_ = DisplayMode::Outline;
        break;
    case DisplayMode::Outline:
        displayMode_ = DisplayMode::OutlineOverlay;
        break;
    case DisplayMode::OutlineOverlay:
        displayMode_ = DisplayMode::Normal;
        break;
    }
    requestRepaint();
}

} // namespace vgc::canvas
