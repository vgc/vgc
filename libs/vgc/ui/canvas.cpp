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

#include <vgc/ui/canvas.h>

#include <QBitmap>
#include <QCursor>
#include <QPainter>

#include <vgc/core/array.h>
#include <vgc/core/history.h>
#include <vgc/core/paths.h>
#include <vgc/core/stopwatch.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/strings.h>
#include <vgc/geometry/curve.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/strings.h>
#include <vgc/ui/window.h>
#include <vgc/workspace/edge.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

SelectionListHistoryPtr SelectionListHistory::create() {
    return SelectionListHistoryPtr(new SelectionListHistory());
}

void SelectionListHistory::pushSelection(const SelectionList& list) {
    lists_.emplaceLast(list);
    selectionChanged().emit();
}

void SelectionListHistory::pushSelection(SelectionList&& list) {
    lists_.emplaceLast(std::move(list));
    selectionChanged().emit();
}

CanvasPtr Canvas::create(workspace::Workspace* workspace) {
    return CanvasPtr(new Canvas(workspace));
}

Canvas::Canvas(workspace::Workspace* workspace)
    : Widget()
    , workspace_(workspace)
    , renderTask_("Render")
    , updateTask_("Update")
    , drawTask_("Draw") {

    // Set ClickFocus policy to be able to accept keyboard events (default
    // policy is NoFocus).
    setFocusPolicy(FocusPolicy::Click);

    setClippingEnabled(true);

    if (workspace_) {
        workspace_->changed().connect(onWorkspaceChanged());
        // XXX to remove
        workspace_->document()->emitPendingDiff();
        workspace_->document()->changed().connect(onDocumentChanged());
    }

    addStyleClass(strings::Canvas);
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

// Note: VAC elements have soft and hard delete, non-VAC elements may have the
// same, so it would be best to have the delete method virtual in
// workspace::Element.
//
// For now we simply use hard delete since it's the only deletion method
// implemented. Later, the default for VAC cells should probably be soft
// delete.
//
void deleteElement(workspace::Element* element) {
    vacomplex::Node* node = element->vacNode();
    bool deleteIsolatedVertices = true;
    vacomplex::ops::hardDelete(node, deleteIsolatedVertices);
}

void deleteElements(
    const core::Array<core::Id>& elementIds,
    workspace::Workspace* workspace) {

    if (elementIds.isEmpty()) {
        return;
    }

    // Open history group
    static core::StringId Delete_Element("Delete Element");
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace->history();
    if (history) {
        undoGroup = history->createUndoGroup(Delete_Element);
    }

    // Iterate over all elements to delete.
    //
    // For now, deletion is done via the DOM, so we need to sync() before
    // finding the next selected ID to check whether it still exists.
    //
    for (core::Id id : elementIds) {
        workspace::Element* element = workspace->find(id);
        if (element) {
            deleteElement(element);
        }
    }

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

    if (workspace_) {
        workspace_->visitDepthFirst(
            [](workspace::Element*, Int) { return true; },
            [&, tol, worldCoords](workspace::Element* e, Int /*depth*/) {
                if (!e) {
                    return;
                }
                double dist = 0;
                if (e->isSelectableAt(worldCoords, false, tol, &dist)) {
                    result.emplaceLast(e->id(), dist);
                }
            });

        // order from front to back
        std::reverse(result.begin(), result.end());

        // sort by selection distance, stable to keep Z order priority
        std::stable_sort(
            result.begin(),
            result.end(),
            [](const SelectionCandidate& a, const SelectionCandidate& b) {
                return a.distance() < b.distance();
            });
    }

    return result;
}

bool Canvas::onKeyPress(KeyEvent* event) {

    using geometry::CurveSamplingQuality;

    switch (event->key()) {
    case Key::T:
        polygonMode_ = polygonMode_ ? 0 : 1;
        requestRepaint();
        break;
    case Key::I:
        switch (requestedTesselationMode_) {
        case CurveSamplingQuality::Disabled:
            requestedTesselationMode_ = CurveSamplingQuality::UniformLow;
            break;
        case CurveSamplingQuality::UniformLow:
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
            LogVgcToolsSketch,
            "Switched edge subdivision quality to: {}",
            core::Enum::prettyName(requestedTesselationMode_));
        reTesselate = true;
        requestRepaint();
        break;
    case Key::P:
        showControlPoints_ = !showControlPoints_;
        requestRepaint();
        break;
    case Key::Backspace:
    case Key::Delete:
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

bool Canvas::onMouseMove(MouseEvent* event) {

    if (!mousePressed_) {
        return false;
    }

    if (!isDragging_) {
        // Initiate drag if:
        // - mouse position moved more than a few pixels, or
        // - mouse pressed for longer than a few 1/10s of seconds
        //
        double deltaTime = event->timestamp() - timeAtPress_;
        float deltaPos = (event->position() - cursorPositionAtPress_).length();
        if (deltaPos >= dragDeltaThreshold || deltaTime > dragTimeThreshold) {

            isDragging_ = true;
        }
    }

    // Note: event.button() is always NoButton for move events. This is why
    // we use the variable isPanning_ and isSketching_ to remember the current
    // mouse action. In the future, we'll abstract this mechanism in a separate
    // class.

    geometry::Vec2f mousePosf = event->position();
    geometry::Vec2d mousePos = geometry::Vec2d(mousePosf.x(), mousePosf.y());
    if (isPanning_) {
        geometry::Vec2d delta = mousePosAtPress_ - mousePos;
        camera_.setCenter(cameraAtPress_.center() + delta);
        requestRepaint();
        return true;
    }
    else if (isRotating_) {
        // Set new camera rotation
        // XXX rotateViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double rotateViewSensitivity = 0.01;
        geometry::Vec2d deltaPos = mousePosAtPress_ - mousePos;
        double deltaRotation = rotateViewSensitivity * (deltaPos.x() - deltaPos.y());
        camera_.setRotation(cameraAtPress_.rotation() + deltaRotation);

        // Set new camera center so that rotation center = mouse pos at press
        geometry::Vec2d pivotViewCoords = mousePosAtPress_;
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
        geometry::Vec2d deltaPos = mousePosAtPress_ - mousePos;
        const double s = std::exp(zoomViewSensitivity * (deltaPos.y() - deltaPos.x()));
        camera_.setZoom(cameraAtPress_.zoom() * s);

        // Set new camera center so that zoom center = mouse pos at press
        geometry::Vec2d pivotViewCoords = mousePosAtPress_;
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

bool Canvas::onMousePress(MouseEvent* event) {

    if (mousePressed_ || tabletPressed_) {
        return true;
    }
    mousePressed_ = true;
    mouseButtonAtPress_ = event->button();

    if (isPanning_ || isRotating_ || isZooming_) {
        return true;
    }

    ModifierKeys keys = event->modifierKeys();
    MouseButton button = event->button();
    if (keys == ModifierKey::None && button == MouseButton::Middle) {
        isPanning_ = true;
    }
    else if (keys == ModifierKey::Alt && button == MouseButton::Right) {
        isRotating_ = true;
    }
    else if (keys == ModifierKey::Alt && button == MouseButton::Middle) {
        isZooming_ = true;
    }

    if (isPanning_ || isRotating_ || isZooming_) {
        mousePosAtPress_ = geometry::Vec2d(event->position());
        cameraAtPress_ = camera_;
        cursorPositionAtPress_ = event->position();
        timeAtPress_ = event->timestamp();
        return true;
    }

    return false;
}

bool Canvas::onMouseRelease(MouseEvent* event) {

    if (!mousePressed_ || mouseButtonAtPress_ != event->button()) {
        return false;
    }

    if (!isDragging_) {
        double deltaTime = event->timestamp() - timeAtPress_;
        isDragging_ = deltaTime > dragTimeThreshold;
    }

    if (isRotating_ && !isDragging_) {
        // Save mouse pos in world coords before modifying camera
        geometry::Vec2d mousePos = geometry::Vec2d(cursorPositionAtPress_);
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

bool Canvas::onMouseEnter() {
    return false;
}

bool Canvas::onMouseLeave() {
    return false;
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
void Canvas::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

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

    // render visit
    // todo:
    //  - use transforms
    //  - setup target for layers (painting a layer means using its result)
    bool paintOutline = showControlPoints_;
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
        workspace_->visitDepthFirst(
            [](workspace::Element* /*e*/, Int /*depth*/) {
                // we always visit children for now
                return true;
            },
            [=](workspace::Element* e, Int /*depth*/) {
                if (!e) {
                    return;
                }
                e->paint(engine);
                if (paintOutline) {
                    e->paint(engine, {}, workspace::PaintOption::Outline);
                }
            });
        reTesselate = false;
    }

    for (workspace::Element* selectedElement : selectedElements_()) {
        selectedElement->paint(engine, {}, workspace::PaintOption::Selected);
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

} // namespace vgc::ui
