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

#include <vgc/canvas/strings.h>
#include <vgc/canvas/workspaceselection.h>
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

Canvas::Canvas(CreateKey key)
    : Widget(key)
    , renderTask_("Render")
    , updateTask_("Update")
    , drawTask_("Draw") {

    // Set ClickFocus policy to be able to accept keyboard events (default
    // policy is NoFocus).
    setFocusPolicy(ui::FocusPolicy::Click);

    setClippingEnabled(true);

    addStyleClass(canvas::strings::Canvas);
}

CanvasPtr Canvas::create() {
    return core::createObject<Canvas>();
}

void Canvas::setWorkspace(workspace::WorkspaceWeakPtr newWorkspace_) {

    if (workspace_ == newWorkspace_) {
        return;
    }

    if (auto oldWorkspace = workspace_.lock()) {
        oldWorkspace->disconnect(this);
    }

    workspace_ = newWorkspace_;
    if (auto newWorkspace = workspace_.lock()) {
        newWorkspace->changed().connect(onWorkspaceChanged_Slot());
    }

    onWorkspaceChanged_();
    workspaceReplaced().emit();
}

void Canvas::setWorkspaceSelection(WorkspaceSelectionWeakPtr newWorkspaceSelection_) {

    if (workspaceSelection_ == newWorkspaceSelection_) {
        return;
    }

    if (auto oldWorkspaceSelection = workspaceSelection_.lock()) {
        oldWorkspaceSelection->disconnect(this);
    }

    workspaceSelection_ = newWorkspaceSelection_;
    if (auto newWorkspaceSelection = workspaceSelection_.lock()) {
        newWorkspaceSelection->changed().connect(onWorkspaceSelectionChanged_Slot());
    }

    onWorkspaceSelectionChanged_();
    workspaceSelectionReplaced().emit();
}

void Canvas::setDisplayMode(DisplayMode displayMode) {
    if (displayMode_ != displayMode) {
        displayMode_ = displayMode;
        requestRepaint();
    }
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

void Canvas::setCamera(const geometry::Camera2d& camera) {
    camera_ = camera;
    requestRepaint();
}

core::Array<SelectionCandidate> Canvas::computeSelectionCandidates(
    const geometry::Vec2d& positionInWidgetSpace,
    style::Length toleranceInWidgetSpace) const {

    double tol = toleranceInWidgetSpace.toPx(styleMetrics());
    return computeSelectionCandidatesAboveOrAt(0, positionInWidgetSpace, tol);
}

core::Array<SelectionCandidate> Canvas::computeSelectionCandidatesAboveOrAt(
    core::Id itemId,
    const geometry::Vec2d& position,
    double tolerance,
    CoordinateSpace coordinateSpace) const {

    core::Array<SelectionCandidate> result;

    geometry::Vec2d worldCoords = position;
    double worldTol = tolerance;

    if (coordinateSpace == CoordinateSpace::Widget) {
        worldCoords = camera().viewMatrix().inverted().transformPointAffine(worldCoords);
        worldTol /= camera().zoom();
    }

    bool isMeshEnabled = (displayMode_ != DisplayMode::OutlineOnly);
    bool isOutlineEnabled = (displayMode_ != DisplayMode::Normal);

    if (auto workspace = workspace_.lock()) {
        bool skip = itemId > 0;
        workspace->visitDepthFirst(
            [](workspace::Element*, Int) { return true; },
            [=, &result, &skip](workspace::Element* e, Int /*depth*/) {
                if (!e || (skip && e->id() != itemId)) {
                    return;
                }
                skip = false;

                vacomplex::Cell* cell = nullptr;
                if (e->isVacElement()) {
                    vacomplex::Node* node = e->toVacElement()->vacNode();
                    if (node && node->isCell()) {
                        cell = node->toCellUnchecked();
                    }
                }
                double dist = 0;
                bool outlineOnly = isOutlineEnabled;
                if (isMeshEnabled) {
                    // keep faces selectable
                    if (cell && cell->cellType() == vacomplex::CellType::KeyFace) {
                        outlineOnly = false;
                    }
                }
                if (e->isSelectableAt(worldCoords, outlineOnly, worldTol, &dist)) {
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

// XXX: add softSnappingCandidates() for alignment, nearest edge..

core::Array<core::Id> Canvas::computeRectangleSelectionCandidates(
    const geometry::Vec2d& a_,
    const geometry::Vec2d& b_,
    CoordinateSpace coordinateSpace) const {

    using geometry::Vec2d;

    geometry::Vec2d a = a_;
    geometry::Vec2d b = b_;
    if (coordinateSpace == CoordinateSpace::Widget) {
        geometry::Mat4d invView = camera().viewMatrix().inverted();
        a = invView.transformPointAffine(a);
        b = invView.transformPointAffine(b);
    }

    core::Array<core::Id> result;

    bool isMeshEnabled = (displayMode_ != DisplayMode::OutlineOnly);

    if (auto workspace = workspace_.lock()) {

        geometry::Rect2d rect = geometry::Rect2d::empty;
        rect.uniteWith(a);
        rect.uniteWith(b);

        workspace->visitDepthFirst(
            [](workspace::Element*, Int) { return true; },
            [&, rect, isMeshEnabled](workspace::Element* e, Int /*depth*/) {
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
                if (!isMeshEnabled && cell
                    && cell->cellType() == vacomplex::CellType::KeyFace) {
                    // don't select face when mesh display is disabled.
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

void Canvas::setWireframeMode(bool isWireframeMode) {
    if (isWireframeMode_ != isWireframeMode) {
        isWireframeMode_ = isWireframeMode;
        requestRepaint();
    }
}

void Canvas::setControlPointsVisible(bool areControlPointsVisible) {
    if (areControlPointsVisible_ != areControlPointsVisible) {
        areControlPointsVisible_ = areControlPointsVisible;
        requestRepaint();
    }
}

void Canvas::onWorkspaceChanged_() {
    requestRepaint();
}

void Canvas::onWorkspaceSelectionChanged_() {
    requestRepaint();
    workspaceSelectionChanged().emit();
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

    bool hasCameraChanged = true;
    geometry::Camera2d camera = this->camera();

    if (isPanning_) {
        geometry::Vec2d delta = mousePosAtPress - mousePos;
        camera.setCenter(cameraAtPress_.center() + delta);
    }
    else if (isRotating_) {
        // Set new camera rotation
        // XXX rotateViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double rotateViewSensitivity = 0.01;
        geometry::Vec2d deltaPos = mousePosAtPress - mousePos;
        double deltaRotation = rotateViewSensitivity * (deltaPos.x() - deltaPos.y());
        camera.setRotation(cameraAtPress_.rotation() + deltaRotation);

        // Set new camera center so that rotation center = mouse pos at press
        geometry::Vec2d pivotViewCoords = mousePosAtPress;
        geometry::Vec2d pivotWorldCoords =
            cameraAtPress_.viewMatrix().inverted().transformPointAffine(pivotViewCoords);
        geometry::Vec2d pivotViewCoordsNow =
            camera.viewMatrix().transformPointAffine(pivotWorldCoords);
        camera.setCenter(camera.center() - pivotViewCoords + pivotViewCoordsNow);
    }
    else if (isZooming_) {
        // Set new camera zoom
        // XXX zoomViewSensitivity should be a user preference
        //     (the signs in front of dx and dy too)
        const double zoomViewSensitivity = 0.005;
        geometry::Vec2d deltaPos = mousePosAtPress - mousePos;
        const double s = std::exp(zoomViewSensitivity * (deltaPos.y() - deltaPos.x()));
        camera.setZoom(cameraAtPress_.zoom() * s);

        // Set new camera center so that zoom center = mouse pos at press
        geometry::Vec2d pivotViewCoords = mousePosAtPress;
        geometry::Vec2d pivotWorldCoords =
            cameraAtPress_.viewMatrix().inverted().transformPointAffine(pivotViewCoords);
        geometry::Vec2d pivotViewCoordsNow =
            camera.viewMatrix().transformPointAffine(pivotWorldCoords);
        camera.setCenter(camera.center() - pivotViewCoords + pivotViewCoordsNow);
    }
    else {
        hasCameraChanged = false;
    }

    if (hasCameraChanged) {
        setCamera(camera);
        return true;
    }
    else {
        return false;
    }
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
        cameraAtPress_ = camera();
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
        geometry::Camera2d camera = this->camera();

        // Save mouse pos in world coords before modifying camera
        geometry::Vec2d mousePos(mousePosAtPress_);
        geometry::Vec2d p1 =
            camera.viewMatrix().inverted().transformPointAffine(mousePos);

        // Reset camera rotation
        camera.setRotation(0);

        // Set new camera center so that zoom center = mouse pos at scroll
        geometry::Vec2d p2 = camera.viewMatrix().transformPointAffine(p1);
        camera.setCenter(camera.center() - mousePos + p2);

        setCamera(camera);
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

    double oldZoom = camera().zoom();
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
        geometry::Camera2d camera = this->camera();

        // Save mouse pos in world coords before applying zoom
        geometry::Vec2d mousePos = geometry::Vec2d(event->position());
        geometry::Vec2d p1 =
            camera.viewMatrix().inverted().transformPointAffine(mousePos);

        camera.setZoom(newZoom);

        // Set new camera center so that zoom center = mouse pos at scroll
        geometry::Vec2d p2 = camera.viewMatrix().transformPointAffine(p1);
        camera.setCenter(camera.center() - mousePos + p2);

        setCamera(camera);
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

    geometry::Mat4f vm = engine->viewMatrix();
    geometry::Mat4f cameraViewf(camera().viewMatrix());
    engine->pushViewMatrix(vm * cameraViewf);

    core::Array<workspace::Element*> selectedElements = selectedElements_();

    // render visit
    // todo:
    //  - use transforms
    //  - setup target for layers (painting a layer means using its result)

    workspace::PaintOptions commonPaintOptions = {};
    if (areControlPointsVisible_) {
        commonPaintOptions.set(workspace::PaintOption::Editing);
    }

    if (auto workspace = workspace_.lock()) {
        workspace->sync();
        //VGC_PROFILE_SCOPE("Canvas:WorkspaceVisit");

        bool isMeshEnabled = (displayMode_ != DisplayMode::OutlineOnly);
        bool isOutlineEnabled = (displayMode_ != DisplayMode::Normal);

        // Draw Normal
        //
        if (isMeshEnabled) {
            engine->setRasterizerState(isWireframeMode_ ? wireframeRS_ : fillRS_);
            workspace::PaintOptions paintOptions = commonPaintOptions;
            workspace->visitDepthFirst(
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

        // Note: outline and selection shouldn't be drawn in wireframe, otherwise:
        // - We cannot see which face is selected.
        // - They don't look nice (seem to have "holes") while not providing any
        //   useful data visualization anyway (too thin to see the triangles).

        // Draw Outline
        //
        if (isOutlineEnabled) {
            engine->setRasterizerState(fillRS_);
            workspace::PaintOptions paintOptions = commonPaintOptions;
            paintOptions.set(workspace::PaintOption::Outline);
            workspace->visitDepthFirst(
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
        //
        if (!selectedElements.isEmpty()) {
            engine->setRasterizerState(fillRS_);
            workspace::PaintOptions paintOptions = commonPaintOptions;
            paintOptions.set(workspace::PaintOption::Selected);
            if (isOutlineEnabled) {
                paintOptions.set(workspace::PaintOption::Outline);
            }
            workspace->visitDepthFirst(
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
    if (auto workspace = workspace_.lock()) {
        if (auto workspaceSelection = workspaceSelection_.lock()) {

            for (core::Id id : workspaceSelection->itemIds()) {
                workspace::Element* element = workspace->find(id);
                if (element && !result.contains(element)) {
                    result.append(element);
                }
            }
        }
    }
    return result;
}

} // namespace vgc::canvas
