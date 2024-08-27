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

#include <vgc/canvas/debugdraw.h>
#include <vgc/canvas/experimental.h>
#include <vgc/canvas/strings.h>
#include <vgc/canvas/workspaceselection.h>
#include <vgc/core/array.h>
#include <vgc/core/colors.h>
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
#include <vgc/workspace/colors.h>
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

void Canvas::setViewSettings(const ViewSettings& viewSettings) {
    if (viewSettings_ != viewSettings) {
        viewSettings_ = viewSettings;
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
        worldCoords = camera().viewMatrix().inverse().transformAffine(worldCoords);
        worldTol /= camera().zoom();
    }

    DisplayMode displayMode = viewSettings().displayMode();
    bool isMeshEnabled = (displayMode != DisplayMode::OutlineOnly);
    bool isOutlineEnabled = (displayMode != DisplayMode::Normal);

    if (auto workspace = workspace_.lock()) {
        bool skip = itemId > 0;
        workspace->visitDepthFirst(
            [](workspace::Element*, Int) { return true; },
            [=, &result, &skip](workspace::Element* e, Int depth) {
                VGC_UNUSED(depth);
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
        geometry::Mat3d invView = camera().viewMatrix().inverse();
        a = invView.transformAffine(a);
        b = invView.transformAffine(b);
    }

    core::Array<core::Id> result;

    DisplayMode displayMode = viewSettings().displayMode();
    bool isMeshEnabled = (displayMode != DisplayMode::OutlineOnly);

    if (auto workspace = workspace_.lock()) {

        geometry::Rect2d rect = geometry::Rect2d::empty;
        rect.uniteWith(a);
        rect.uniteWith(b);

        workspace->visitDepthFirst(
            [](workspace::Element*, Int) { return true; },
            [&, rect, isMeshEnabled](workspace::Element* e, Int depth) {
                VGC_UNUSED(depth);
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
            cameraAtPress_.viewMatrix().inverse().transformAffine(pivotViewCoords);
        geometry::Vec2d pivotViewCoordsNow =
            camera.viewMatrix().transformAffine(pivotWorldCoords);
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
            cameraAtPress_.viewMatrix().inverse().transformAffine(pivotViewCoords);
        geometry::Vec2d pivotViewCoordsNow =
            camera.viewMatrix().transformAffine(pivotWorldCoords);
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
        geometry::Vec2d p1 = camera.viewMatrix().inverse().transformAffine(mousePos);

        // Reset camera rotation
        camera.setRotation(0);

        // Set new camera center so that zoom center = mouse pos at scroll
        geometry::Vec2d p2 = camera.viewMatrix().transformAffine(p1);
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
        geometry::Vec2d p1 = camera.viewMatrix().inverse().transformAffine(mousePos);

        camera.setZoom(newZoom);

        // Set new camera center so that zoom center = mouse pos at scroll
        geometry::Vec2d p2 = camera.viewMatrix().transformAffine(p1);
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
    bgGeometry_ = engine->createTriangleStrip(BuiltinGeometryLayout::XYRGB);

    reload_ = true;
}

namespace {

void drawSubpass(
    graphics::Engine* engine,
    const graphics::RasterizerStatePtr& rasterizerState,
    workspace::Workspace& workspace,
    workspace::PaintOptions paintOptions) {

    engine->setRasterizerState(rasterizerState);
    workspace.visitDepthFirst(
        [](workspace::Element* e, Int depth) {
            VGC_UNUSED(e);
            VGC_UNUSED(depth);
            // we always visit children for now
            return true;
        },
        [=](workspace::Element* e, Int depth) {
            VGC_UNUSED(depth);
            if (e) {
                e->paint(engine, {}, paintOptions);
            }
        });
}

// Note: we do not bother to implement any caching for this since it is
// mostly for debugging purposes and performance is not critical.
//
void doPaintInputSketchPoints(
    graphics::Engine* engine,
    const workspace::VacKeyEdge& edge,
    graphics::GeometryViewPtr& geometryView,
    const core::Color& color) {

    // Get the positions of the input sketch points, in widget coordinates (at
    // time of sketch)
    //
    dom::Element* element = edge.domElement();
    if (!element) {
        return;
    }

    const dom::Value& vPositions = element->getAttribute(dom::strings::inputpositions);
    if (!vPositions.has<geometry::Vec2dArray>()) {
        return;
    }
    const auto& positions = vPositions.getUnchecked<geometry::Vec2dArray>();

    Int n = positions.length();
    if (n <= 0) {
        return;
    }

    // Get the transform matrix from widget coords to scene coords
    //
    const dom::Value& vTransform = element->getAttribute(dom::strings::inputtransform);
    if (!vTransform.has<geometry::Mat3d>()) {
        return;
    }
    const auto& transform = vTransform.getUnchecked<geometry::Mat3d>();

    // Create the graphics resource
    //
    if (!geometryView) {
        geometryView = engine->createTriangleStrip(
            graphics::BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    }

    // Compute, in scene coordinates, the corners of a square centered at the
    // origin, scaled and rotated such that it has the same size and
    // orientation as a pixel when the edge was first sketched. The "disp"
    // component is used to be able to apply a small screen-space displacement,
    // so that we can paint a thin border of w pixels around the square.
    //
    //  x-----------x <- cornerPos + cornerDisp * (w/2)
    //  | x-------x | <- cornerPos + cornerDisp * (-w/2)
    //  | |       | |
    //  | |       |w|
    //  | |       | |
    //  | x-------x |
    //  x-----------x
    //
    constexpr float sqrt2 = 1.4142135f;
    struct PosAndDisp {
        PosAndDisp(const geometry::Vec2d& pos)
            : pos_(pos)
            , disp_(sqrt2 * pos_.normalized()) {
        }
        geometry::Vec2f pos_;
        geometry::Vec2f disp_;
    };
    core::Array<PosAndDisp> sharedInstData = {
        PosAndDisp(transform.transformLinear({-0.5, -0.5})),
        PosAndDisp(transform.transformLinear({0.5, -0.5})),
        PosAndDisp(transform.transformLinear({-0.5, 0.5})),
        PosAndDisp(transform.transformLinear({0.5, 0.5}))};

    // We draw two quads for each input sketch point:
    // - one colored with a small screen-space positive displacement w/2
    // - one white   with a small screen-space negative displacement -w/2
    //
    // An alternative to the (w/2, -w/2) coefficients is to use (w, 0) instead,
    // but the former has the following advantages:
    //
    // - When looking at it at 100% scale (such as when drawing), then
    //   it is rendered exactly as one pixel with the given `color`
    //
    // - When input points are adjacent integer pixels, then they become perfectly
    //   aligned and shared their border
    //
    // It does have the disadvantage to create some artifacts when un-zooming
    // (the smaller white quad becomes inverted and eventually covers the
    // colored quad), but un-zooming is typically rare when inspecting input
    // points (we typically zoom in), and the advantages seem to outweight this
    // disadvantage.
    //
    const core::Color& c = color;
    const float w = 1.0f;
    const float hw = 0.5f * w;
    core::FloatArray perInstData;
    for (Int i = 0; i < n; ++i) {
        geometry::Vec2d pWidget = positions[i];
        geometry::Vec2f pScene(transform.transformAffine(pWidget));
        perInstData.extend({pScene.x(), pScene.y(), 1.f, hw, c.r(), c.g(), c.b(), c.a()});
        perInstData.extend({pScene.x(), pScene.y(), 1.f, -hw, 1.f, 1.f, 1.f, 1.f});
        //                     X           Y        Rot   W    R    G    B    A
    }

    engine->updateVertexBufferData(geometryView, std::move(sharedInstData));
    engine->updateInstanceBufferData(geometryView, std::move(perInstData));

    engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
    engine->drawInstanced(geometryView);
}

graphics::SizedFontWeakPtr getObjectIdFont_() {
    graphics::FontLibraryWeakPtr lib_ = graphics::fontLibrary();
    if (auto lib = lib_.lock()) {
        graphics::FontWeakPtr font_ = lib->defaultFont();
        if (auto font = font_.lock()) {
            return font->getSizedFont({14, graphics::FontHinting::Native});
        }
    }
    return nullptr;
}

graphics::SizedFontWeakPtr getObjectIdFont() {
    static graphics::SizedFontWeakPtr font = getObjectIdFont_();
    return font;
}

geometry::Vec2d getVertexObjectIdAnchor(vacomplex::VertexCell* vertex, core::AnimTime t) {
    return vertex->position(t);
}

geometry::Vec2d getEdgeObjectIdAnchor(vacomplex::EdgeCell* edge, core::AnimTime t) {
    auto sampling = edge->strokeSamplingShared(t);
    const geometry::StrokeSample2dArray& samples = sampling->samples();
    if (samples.length() == 0) {
        return {};
    }
    double halfS = 0.5 * samples.last().s();
    auto comp = [](const geometry::StrokeSample2d& sample, double s) {
        return sample.s() < s;
    };
    auto it2 = std::lower_bound(samples.begin(), samples.end(), halfS, comp);
    if (it2 == samples.end()) {
        // no sample is after halfS
        return samples.last().position();
    }
    else if (it2 == samples.begin()) {
        // the first sample is after halfS
        return samples.first().position();
    }
    else {
        // it2 if the first sample after halfS
        auto it1 = it2 - 1;
        geometry::Vec2d pos1 = it1->position();
        geometry::Vec2d pos2 = it2->position();
        double s1 = it1->s();
        double s2 = it2->s();
        double ds = s2 - s1;
        if (ds > 0) {
            return core::fastLerp(pos1, pos2, (halfS - s1) / (s2 - s1));
        }
        else {
            return it1->position();
        }
    }
}

void drawObjectId(
    graphics::Engine* engine,
    detail::ObjectIdMap& objectIds,
    std::optional<graphics::ShapedText>& shapedText,
    const geometry::Mat4f& oldViewMatrix,
    workspace::Element* e) {

    if (!e) {
        return;
    }

    // For now, we only show IDs of VacElements.
    // We may want to extend this in the future.
    workspace::VacElement* ve = e->toVacElement();
    if (!ve) {
        return;
    }

    // For now, we don't show IDs of groups.
    vacomplex::Cell* cell = ve->vacCell();
    if (!cell) {
        return;
    }

    dom::Element* element = e->domElement();
    if (!element) {
        return;
    }

    core::StringId id = element->id();
    if (id.isEmpty()) {
        return;
    }

    // Create the GeometryViewPtr corresponding to the ID if it doesn't already
    // exist in the `objectIds` cache.
    //
    auto it = objectIds.find(id);
    if (it == objectIds.end()) {
        if (!shapedText) {
            if (auto font = getObjectIdFont().lock()) {
                shapedText = graphics::ShapedText(font.get(), "");
            }
        }
        if (shapedText) {
            shapedText->setText(id);
            graphics::GeometryViewPtr triangles =
                engine->createTriangleList(graphics::BuiltinGeometryLayout::XYRGB);
            core::FloatArray a;
            geometry::Vec2f origin(0, 0);
            core::Color c = core::colors::black;
            shapedText->fill(a, origin, c.r(), c.g(), c.b());
            engine->updateVertexBufferData(triangles, std::move(a));
            it = objectIds.insert_or_assign(id, triangles).first;
        }
    }

    // If we successfully retrieved or created the GeometryViewPtr,
    // draw it at the appropriate location.
    //
    if (it != objectIds.end()) {

        // Compute location where to draw the object ID.
        // XXX: Should we add `anchor(t)` as a virtual method of cell?
        core::AnimTime t;
        geometry::Vec2d anchor;
        if (auto vertex = cell->toVertexCell()) {
            anchor = getVertexObjectIdAnchor(vertex, t);
        }
        else if (auto edge = cell->toEdgeCell()) {
            anchor = getEdgeObjectIdAnchor(edge, t);
        }
        else {
            geometry::Rect2d bb = e->boundingBox(t);
            anchor = 0.5 * (bb.pMin() + bb.pMax());
        }

        // Actually draw the ID
        geometry::Vec2f anchorf(anchor);
        geometry::Vec2f offset(5, -5);
        geometry::Vec2f pos = oldViewMatrix.transformAffine(anchorf) + offset;
        geometry::Mat4f viewMatrix = geometry::Mat4f::identity;
        viewMatrix.translate(pos);
        engine->setViewMatrix(viewMatrix);
        engine->draw(it->second);
    }
}

void drawObjectIds(
    graphics::Engine* engine,
    detail::ObjectIdMap& objectIds,
    workspace::Workspace& workspace) {

    std::optional<graphics::ShapedText> shapedText;
    engine->setProgram(graphics::BuiltinProgram::Simple);
    geometry::Mat4f oldViewMatrix = engine->viewMatrix();
    engine->pushViewMatrix();
    workspace.visitDepthFirstPreOrder( //
        [engine, &objectIds, &shapedText, &oldViewMatrix](
            workspace::Element* e, Int depth) {
            VGC_UNUSED(depth);
            drawObjectId(engine, objectIds, shapedText, oldViewMatrix, e);
        });
    engine->popViewMatrix();
}

} // namespace

// Note: In this override, we intentionally not call SuperClass::onPaintDraw()
// since we draw our own background. We call paintChildren() explicitly at the
// end to call the `onPaintDraw()` method of the CanvasTool children of the
// Canvas.
//
void Canvas::onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) {

    using namespace graphics;
    namespace gs = graphics::strings;
    using workspace::PaintOption;

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
    geometry::Mat3d cameraView = camera().viewMatrix();
    engine->pushViewMatrix(vm * geometry::Mat4f::fromTransform(cameraView));

    core::Array<workspace::Element*> selectedElements = selectedElements_();

    // TODO:
    //  - use transforms
    //  - setup target for layers (painting a layer means using its result)

    if (auto workspace = workspace_.lock()) {
        workspace->sync();
        //VGC_PROFILE_SCOPE("Canvas:WorkspaceVisit");
        DisplayMode displayMode = viewSettings().displayMode();
        bool isMeshEnabled = (displayMode != DisplayMode::OutlineOnly);
        bool isOutlineEnabled = (displayMode != DisplayMode::Normal);
        bool areControlPointsVisible = viewSettings().areControlPointsVisible();
        bool isWireframeMode = viewSettings().isWireframeMode();
        bool showInputSketchPoints = experimental::showInputSketchPoints().value();

        // Draw Normal.
        //
        // If Wireframe = on and ControlPoints = on, then we want this pass to
        // actually be done in two subpasses:
        //
        // 1. First, the Normal option in wireframe.
        // 2. Then, the Editing option in fill mode.
        //
        // Indeed, we never want to draw the control points (=Editing) in
        // wireframe, and when we are in wireframe mode, it isn't a problem to
        // draw all the control points after all the Normal geometry, since the
        // control points of edges that are obscured by other edges/faces would
        // anyway be partially visible since the Normal geometry is drawn in
        // wireframe.
        //
        // However, if Wireframe = off and ControlPoints = on, then we can draw
        // both the Normal and Editing option in one pass, and therefore the
        // control points of edges that are obscured by other edges/faces would
        // also be obscured, as intended (unless the edge is selected, in which
        // case its control points would still be visible since they will be
        // re-drawn in the Selection pass below).
        //
        // Also note that if ControlPoints = on and Outline = on, then there is
        // no need to draw the control points at all in the Normal pass, since
        // they will be drawn in the Outline pass anyway.
        //
        if (isMeshEnabled) {
            bool drawControlPoints = areControlPointsVisible && !isOutlineEnabled;
            if (isWireframeMode) {
                drawSubpass(engine, wireframeRS_, *workspace, PaintOption::Normal);
                if (drawControlPoints) {
                    drawSubpass(engine, fillRS_, *workspace, PaintOption::Editing);
                }
            }
            else {
                workspace::PaintOptions paintOptions = PaintOption::Normal;
                if (drawControlPoints) {
                    paintOptions.set(PaintOption::Editing);
                }
                drawSubpass(engine, fillRS_, *workspace, paintOptions);
            }
        }

        // Note: outline and selection shouldn't be drawn in wireframe, otherwise:
        // - We cannot see which face is selected.
        // - They don't look nice (seem to have "holes") while not providing any
        //   useful data visualization anyway (too thin to see the triangles).

        // Draw non-selected input sketch points
        //
        // Note: drawing them here (between the "Normal" and "Outline" pass)
        // means that when drawControlPoints is true, then the user can choose
        // whether the input sketch points are above the control points (by
        // using the Normal display mode) or below the control points (by using
        // the Outline or Outline Only mode). Both are useful in different
        // circumstances.
        //
        if (showInputSketchPoints) {
            engine->setRasterizerState(fillRS_);
            workspace->visitDepthFirstPreOrder(
                [=, &selectedElements](workspace::Element* e, Int depth) {
                    VGC_UNUSED(depth);
                    if (e && !selectedElements.contains(e)) {
                        if (auto edge = dynamic_cast<workspace::VacKeyEdge*>(e)) {
                            doPaintInputSketchPoints(
                                engine,
                                *edge,
                                inputSketchPointsGeometry_,
                                workspace::colors::outline);
                        };
                    }
                });
        }

        // Draw Outline
        //
        if (isOutlineEnabled) {
            workspace::PaintOptions paintOptions = PaintOption::Outline;
            if (areControlPointsVisible) {
                paintOptions.set(PaintOption::Editing);
            }
            drawSubpass(engine, fillRS_, *workspace, paintOptions);
        }

        // Draw Selection
        //
        if (!selectedElements.isEmpty()) {
            workspace::PaintOptions paintOptions = PaintOption::Selected;
            if (isOutlineEnabled) {
                paintOptions.set(workspace::PaintOption::Outline);
            }
            if (areControlPointsVisible) {
                paintOptions.set(PaintOption::Editing);
            }
            engine->setRasterizerState(fillRS_);
            bool areNonSelectedVerticesVisible =
                isOutlineEnabled || areControlPointsVisible;
            workspace->visitDepthFirst(
                [](workspace::Element* e, Int depth) {
                    VGC_UNUSED(e);
                    VGC_UNUSED(depth);
                    // we always visit children for now
                    return true;
                },
                [=, &selectedElements](workspace::Element* e, Int depth) {
                    VGC_UNUSED(depth);
                    if (e && selectedElements.contains(e)) {
                        auto edge = dynamic_cast<workspace::VacKeyEdge*>(e);

                        // If the element is an edge, we first draw its input points now.
                        // Indeed, we prefer them to be under the control points, since
                        // zooming in makes the input points bigger, but keeps the control
                        // points at the same screen size.
                        //
                        if (edge && showInputSketchPoints) {
                            doPaintInputSketchPoints(
                                engine,
                                *edge,
                                inputSketchPointsGeometry_,
                                workspace::colors::selection);
                        }

                        // We then draw the selected element in "Selected" mode, with
                        // maybe edge outlines and control points (based on settings).
                        //
                        e->paint(engine, {}, paintOptions);

                        // Finally, if the selected element is an edge, we redraw its
                        // end vertices on top, otherwise the edge centerline appears
                        // over its non-selected vertices, which looks ugly.
                        //
                        if (edge && areNonSelectedVerticesVisible) {
                            workspace::VacKeyVertex* startVertex = edge->startVertex();
                            workspace::VacKeyVertex* endVertex = edge->endVertex();
                            workspace::PaintOptions paintOptions2 = paintOptions;
                            paintOptions2.unset(workspace::PaintOption::Selected);
                            if (startVertex && !selectedElements.contains(startVertex)) {
                                startVertex->paint(engine, {}, paintOptions2);
                            }
                            if (endVertex && !selectedElements.contains(endVertex)) {
                                endVertex->paint(engine, {}, paintOptions2);
                            }
                        }
                    }
                });
        }

        // Draw Object IDs
        //
        if (viewSettings().showObjectIds()) {
            engine->setRasterizerState(fillRS_);
            drawObjectIds(engine, objectIds_, *workspace);
        }
    }

    // Call DebugDraw callbacks if any.
    //
    if (!detail::debugDraws().isEmpty()) {
        auto locked = detail::lockDebugDraws();
        for (const auto& x : detail::debugDraws()) {
            x.function(engine);
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
