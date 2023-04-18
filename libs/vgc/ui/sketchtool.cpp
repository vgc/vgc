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

#include <vgc/ui/sketchtool.h>

#include <QBitmap>
#include <QCursor>
#include <QPainter>

#include <vgc/core/stringid.h>
#include <vgc/geometry/curve.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/settings.h>
#include <vgc/ui/window.h>
#include <vgc/workspace/edge.h>

namespace vgc::ui {

namespace {

geometry::Vec2d getSnapPosition(workspace::Element* snapVertex) {
    vacomplex::Node* node = snapVertex->vacNode();
    if (node) {
        vacomplex::Cell* cell = node->toCell();
        if (cell) {
            vacomplex::KeyVertex* keyVertex = cell->toKeyVertex();
            if (keyVertex) {
                return keyVertex->position();
            }
        }
    }
    VGC_WARNING(
        LogVgcToolsSketch,
        "Snap vertex didn't have an associated KeyVertex: using (0, 0) as position.");
    return geometry::Vec2d();
}

// Note: for now, the deformation is linear, which introduce a non-smooth
// point at s = snapDeformationLength.
//
geometry::Vec2d snapDeformation(
    const geometry::Vec2d& position,
    const geometry::Vec2d& delta,
    double s,
    double snapDeformationLength) {

    return position + delta * (1 - (s / snapDeformationLength));
}

} // namespace

SketchTool::SketchTool()
    : CanvasTool() {

    Settings* session = ui::settings::session();
    penWidth_ = session->getOrSetDoubleValue("tools.sketch.width", penWidth_);
    setFocusPolicy(FocusPolicy::Click);
    setClippingEnabled(true);
}

SketchToolPtr SketchTool::create() {
    return SketchToolPtr(new SketchTool());
}

bool SketchTool::onKeyPress(KeyEvent* /*event*/) {
    return false;
}

namespace {

double pressurePenWidth(double baseWidth, const MouseEvent* event) {
    return event->hasPressure() ? 2 * event->pressure() * baseWidth : baseWidth;
}

} // namespace

bool SketchTool::onMouseMove(MouseEvent* event) {

    if (!isSketching_) {
        return false;
    }

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return false;
    }

    // Note: event.button() is always NoButton for move events. This is why
    // we use the variable isPanning_ and isSketching_ to remember the current
    // mouse action. In the future, we'll abstract this mechanism in a separate
    // class.

    geometry::Vec2f mousePosf = event->position();
    geometry::Vec2d mousePos = geometry::Vec2d(mousePosf.x(), mousePosf.y());

    // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
    // and should be cached), but let's keep it like this for now for testing.
    geometry::Vec2d viewCoords = mousePos;
    geometry::Vec2d worldCoords =
        canvas->camera().viewMatrix().inverted().transformPointAffine(viewCoords);
    double width = pressurePenWidth(penWidth_, event);
    continueCurve_(worldCoords, width);
    minimalLatencyStrokeReload_ = true;
    return true;
}

bool SketchTool::onMousePress(MouseEvent* event) {

    if (isSketching_ || event->button() != MouseButton::Left || event->modifierKeys()) {
        return false;
    }

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return false;
    }
    canvas->clearSelection_();

    isSketching_ = true;

    geometry::Vec2f mousePosf = event->position();
    geometry::Vec2d mousePos = geometry::Vec2d(mousePosf.x(), mousePosf.y());

    geometry::Vec2d viewCoords = mousePos;
    geometry::Vec2d worldCoords =
        canvas->camera().viewMatrix().inverted().transformPointAffine(viewCoords);
    startCurve_(worldCoords, pressurePenWidth(penWidth_, event));
    return true;
}

bool SketchTool::onMouseRelease(MouseEvent* event) {

    if (event->button() == MouseButton::Left) {
        finishCurve_();
        if (resetData_()) {
            return true;
        }
    }

    return false;
}

namespace {

void drawCrossCursor(QPainter& painter) {
    painter.setPen(QPen(Qt::color1, 1.0));
    painter.drawLine(16, 0, 16, 10);
    painter.drawLine(16, 22, 16, 32);
    painter.drawLine(0, 16, 10, 16);
    painter.drawLine(22, 16, 32, 16);
    painter.drawPoint(16, 16);
}

QCursor createCrossCursor() {
    // Draw bitmap
    QBitmap bitmap(32, 32);
    QPainter bitmapPainter(&bitmap);
    bitmapPainter.fillRect(0, 0, 32, 32, QBrush(Qt::color0));
    drawCrossCursor(bitmapPainter);

    // Draw mask
    QBitmap mask(32, 32);
    QPainter maskPainter(&mask);
    maskPainter.fillRect(0, 0, 32, 32, QBrush(Qt::color0));
#ifndef VGC_CORE_OS_WINDOWS
    // Make the cursor color XOR'd on Windows, black on other platforms. Ideally,
    // we'd prefer XOR'd on all platforms, but it's only supported on Windows.
    // See Qt doc for QCursor(const QBitmap &bitmap, const QBitmap &mask).
    drawCrossCursor(maskPainter);
#endif

    // Create and return cursor
    return QCursor(bitmap, mask);
}

QCursor crossCursor() {
    static QCursor res = createCrossCursor();
    return res;
}

} // namespace

bool SketchTool::onMouseEnter() {
    cursorChanger_.set(crossCursor());
    return false;
}

bool SketchTool::onMouseLeave() {
    cursorChanger_.clear();
    return false;
}

void SketchTool::onResize() {
    reload_ = true;
}

void SketchTool::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    using namespace graphics;
    minimalLatencyStrokeGeometry_ =
        engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XY_iRGBA);
    reload_ = true;
}

void SketchTool::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    if (!isSketching_) {
        return;
    }

    using namespace graphics;
    namespace gs = graphics::strings;

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    // Draw temporary tip of curve between mouse event position and actual current cursor
    // position to reduce visual lag.
    //
    Window* w = window();
    bool cursorMoved = false;
    if (w) {
        geometry::Vec2f pos(w->mapFromGlobal(globalCursorPosition()));
        geometry::Vec2d posd(root()->mapTo(this, pos));
        pos = geometry::Vec2f(
            canvas->camera().viewMatrix().inverted().transformPointAffine(posd));
        if (lastImmediateCursorPos_ != pos) {
            lastImmediateCursorPos_ = pos;
            cursorMoved = true;
            geometry::Vec2d pos2d(pos);
            minimalLatencySnappedCursor_ = pos2d;
            if (!smoothedInputPoints_.isEmpty()) {
                double s = smoothedInputArclengths_.last()
                           + (pos2d - smoothedInputPoints_.last()).length();
                double snapDeformationLength_ = snapDeformationLength();
                geometry::Vec2d delta = startSnapPosition_ - smoothedInputPoints_[0];
                if (s < snapDeformationLength_) {
                    minimalLatencySnappedCursor_ =
                        snapDeformation(pos2d, delta, s, snapDeformationLength_);
                }
            }
        }
    }

    if (cursorMoved || minimalLatencyStrokeReload_) {

        core::Color color = penColor_;
        geometry::Vec2fArray strokeVertices;

        geometry::CurveSample edgeLastSample = {};
        workspace::Element* edgeElement = workspace()->find(edge_);
        vacomplex::KeyEdge* ke = nullptr;
        auto edgeCell = dynamic_cast<workspace::VacKeyEdge*>(edgeElement);
        if (edgeCell) {
            ke = edgeCell->vacKeyEdgeNode();
        }
        if (ke) {
            const geometry::CurveSampleArray& samples = ke->sampling().samples();
            // one sample is not enough to have a well-defined normal
            if (samples.length() >= 2) {
                geometry::CurveSample edgeLastSample = ke->sampling().samples().last();
                geometry::Vec2d tipDir =
                    minimalLatencySnappedCursor_ - edgeLastSample.position();
                geometry::Vec2d tipNormal = tipDir.orthogonalized().normalized();

                double radiusRatio = 0.5;
                geometry::Vec2d tipPoint0 =
                    minimalLatencySnappedCursor_
                    - tipNormal * radiusRatio * edgeLastSample.halfwidth(1);
                geometry::Vec2d tipPoint1 =
                    minimalLatencySnappedCursor_
                    + tipNormal * radiusRatio * edgeLastSample.halfwidth(0);

                strokeVertices.emplaceLast(geometry::Vec2f(edgeLastSample.leftPoint()));
                strokeVertices.emplaceLast(geometry::Vec2f(edgeLastSample.rightPoint()));
                strokeVertices.emplaceLast(geometry::Vec2f(tipPoint0));
                strokeVertices.emplaceLast(geometry::Vec2f(tipPoint1));
            }
        }

        engine->updateBufferData(
            minimalLatencyStrokeGeometry_->vertexBuffer(0), //
            std::move(strokeVertices));

        engine->updateBufferData(
            minimalLatencyStrokeGeometry_->vertexBuffer(1), //
            core::Array<float>({color.r(), color.g(), color.b(), color.a()}));

        minimalLatencyStrokeReload_ = false;
    }

    engine->pushProgram(graphics::BuiltinProgram::Simple);
    geometry::Mat4f vm = engine->viewMatrix();
    geometry::Mat4f cameraViewf(canvas->camera().viewMatrix());
    engine->pushViewMatrix(vm * cameraViewf);
    engine->draw(minimalLatencyStrokeGeometry_);
    engine->popViewMatrix();
    engine->popProgram();
}

void SketchTool::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    minimalLatencyStrokeGeometry_.reset();
}

void SketchTool::updateSmoothedData_() {

    // Add the latest raw input points to smoothed data.
    //
    if (lastInputPoints_.length() > 0) {
        smoothedInputPoints_.append(lastInputPoints_.first());
    }
    else {
        return;
    }
    Int numInputPoints = lastInputPoints_.length();
    Int numSmoothedPoints = smoothedInputPoints_.length();
    VGC_ASSERT(numSmoothedPoints > 0);
    VGC_ASSERT(numSmoothedPoints >= numInputPoints);

    // Apply gaussian smoothing. We only need to update at most 2 points.
    //
    Int lastUnchangedIndex = 0;
    if (numInputPoints >= 3) {
        smoothedInputPoints_.getUnchecked(numSmoothedPoints - 2) = //
            (1 / 4.0) * lastInputPoints_.getUnchecked(0) +         //
            (2 / 4.0) * lastInputPoints_.getUnchecked(1) +         //
            (1 / 4.0) * lastInputPoints_.getUnchecked(2);
        lastUnchangedIndex = numSmoothedPoints - 3;
    }
    if (numInputPoints >= 5) {
        smoothedInputPoints_.getUnchecked(numSmoothedPoints - 3) = //
            (1 / 16.0) * lastInputPoints_.getUnchecked(0) +        //
            (4 / 16.0) * lastInputPoints_.getUnchecked(1) +        //
            (6 / 16.0) * lastInputPoints_.getUnchecked(2) +        //
            (4 / 16.0) * lastInputPoints_.getUnchecked(3) +        //
            (1 / 16.0) * lastInputPoints_.getUnchecked(4);
        lastUnchangedIndex = numSmoothedPoints - 4;
    }
    VGC_ASSERT(lastUnchangedIndex >= 0);

    // Update arclengths.
    //
    smoothedInputArclengths_.resizeNoInit(numSmoothedPoints);
    if (numSmoothedPoints == 1) {
        smoothedInputArclengths_[0] = 0;
    }
    else {
        double s = smoothedInputArclengths_[lastUnchangedIndex];
        geometry::Vec2d p = smoothedInputPoints_[lastUnchangedIndex];
        for (Int i = lastUnchangedIndex + 1; i < numSmoothedPoints; ++i) {
            geometry::Vec2d q = smoothedInputPoints_[i];
            s += (q - p).length();
            smoothedInputArclengths_[i] = s;
        }
    }
}

void SketchTool::startCurve_(const geometry::Vec2d& p, double width) {

    // Fast return if missing required context
    workspace::Workspace* workspace = this->workspace();
    if (!workspace || !workspace->document()) {
        return;
    }
    dom::Element* parentDomElement = workspace->vgcElement()->domElement();
    if (!parentDomElement) {
        return;
    }

    // Create undo group.
    // XXX: Cleanup this?
    static core::StringId Draw_Curve("Draw Curve");
    core::History* history = workspace->history();
    if (history) {
        drawCurveUndoGroup_ = history->createUndoGroup(Draw_Curve);
        drawCurveUndoGroupConnectionHandle_ = drawCurveUndoGroup_->undone().connect(
            [this]([[maybe_unused]] core::UndoGroup* undoGroup, bool /*isAbort*/) {
                // isAbort should be true since we have no sub-group
                if (drawCurveUndoGroup_) {
                    VGC_ASSERT(undoGroup == drawCurveUndoGroup_);
                    drawCurveUndoGroup_->undone().disconnect(
                        drawCurveUndoGroupConnectionHandle_);
                    drawCurveUndoGroup_ = nullptr;
                }
                resetData_();
                requestRepaint();
            });
    }

    // Compute start vertex to snap to
    workspace::Element* snapVertex = nullptr;
    hasStartSnap_ = false;
    startSnapPosition_ = p;
    if (isSnappingEnabled_) {
        snapVertex = computeSnapVertex_(p, nullptr);
        if (snapVertex) {
            hasStartSnap_ = true;
            startSnapPosition_ = getSnapPosition(snapVertex);
        }
    }

    // Get or create start vertex
    //
    // XXX What to do if snapVertex is non-null, but if there is no DOM element
    // corresponding to this workspace element, e.g., due to composite shapes?
    //
    // For now, computeSnapVertex_() ensures that snapVertex->domElement() is
    // never null, but we do not rely on it here. If snapVertex->domElement()
    // is null, we create a new DOM vertex but keep hasStartSnap to true.
    //
    namespace ds = dom::strings;
    dom::Element* startVertex = nullptr;
    if (snapVertex) {
        startVertex = snapVertex->domElement();
    }
    if (!startVertex) {
        startVertex = dom::Element::create(parentDomElement, ds::vertex);
        startVertex->setAttribute(ds::position, startSnapPosition_);
    }

    // Create end vertex
    endVertex_ = dom::Element::create(parentDomElement, ds::vertex);
    endVertex_->setAttribute(ds::position, startSnapPosition_);

    // Create edge
    edge_ = dom::Element::create(parentDomElement, ds::edge);
    edge_->setAttribute(ds::positions, geometry::Vec2dArray());
    edge_->setAttribute(ds::widths, core::DoubleArray());
    edge_->setAttribute(ds::color, penColor_);
    edge_->setAttribute(ds::startvertex, startVertex->getPathFromId());
    edge_->setAttribute(ds::endvertex, endVertex_->getPathFromId());

    // Append start point to geometry
    continueCurve_(p, width);

    // Set curve to fast tesselation to minimize lag. We do this after
    // continueCurve_() to rely on workspace->sync() called there.
    workspace::Element* edgeElement = workspace->find(edge_);
    auto edgeCell = dynamic_cast<workspace::VacKeyEdge*>(edgeElement);
    if (edgeCell) {
        edgeCell->setTesselationMode(geometry::CurveSamplingQuality::AdaptiveLow);
    }

    // Update stroke tip
    minimalLatencyStrokeReload_ = true;
}

void SketchTool::continueCurve_(const geometry::Vec2d& p, double width) {

    // Fast return if missing required context
    workspace::Workspace* workspace = this->workspace();
    if (!workspace || !workspace->document() || !edge_ || !endVertex_) {
        return;
    }

    // Skip duplicate points
    if (!lastInputPoints_.isEmpty()) {
        if (lastInputPoints_.last() == p) {
            return;
        }
    }

    // Append raw new data
    widths_.append(width);
    lastInputPoints_.prepend(p);
    if (lastInputPoints_.length() > 5) {
        lastInputPoints_.removeLast();
    }

    // Update smoothed data from raw data
    updateSmoothedData_();

    // Handle snapping
    if (hasStartSnap_) {

        Int numPoints = smoothedInputPoints_.length();
        double snapDeformationLength_ = snapDeformationLength();
        geometry::Vec2d delta = startSnapPosition_ - smoothedInputPoints_[0];

        points_.resizeNoInit(numPoints);
        for (Int i = 0; i < numPoints; ++i) {
            geometry::Vec2d sp = smoothedInputPoints_[i];
            double s = smoothedInputArclengths_[i];
            if (s < snapDeformationLength_) {
                points_[i] = snapDeformation(sp, delta, s, snapDeformationLength_);
            }
            else {
                points_[i] = sp; // maybe optimize in the future
            }
        }
    }
    else {
        points_ = smoothedInputPoints_;
    }

    // Update DOM and workspace
    namespace ds = dom::strings;
    endVertex_->setAttribute(ds::position, points_.last());
    edge_->setAttribute(ds::positions, points_);
    edge_->setAttribute(ds::widths, widths_);
    workspace->sync();
}

void SketchTool::finishCurve_() {

    namespace ds = dom::strings;

    // Fast return if missing required context
    workspace::Workspace* workspace = this->workspace();
    if (!workspace || !workspace->document() || !edge_ || !endVertex_) {
        return;
    }

    // Set curve to final requested tesselation mode
    workspace::Element* edgeElement = workspace->find(edge_);
    auto edgeCell = dynamic_cast<workspace::VacKeyEdge*>(edgeElement);
    if (edgeCell && canvas()) {
        edgeCell->setTesselationMode(canvas()->requestedTesselationMode());
    }

    // Compute end vertex snapping
    if (isSnappingEnabled_ && smoothedInputPoints_.length() > 1) {

        // Compute start vertex to snap to
        geometry::Vec2d lastInputPoint = lastInputPoints_[0];
        workspace::Element* snapVertex = computeSnapVertex_(lastInputPoint, endVertex_);

        // If found, do the snapping
        if (snapVertex) {

            // Cap snap deformation length to ensure start point isn't modified
            double maxS = smoothedInputArclengths_.last();
            double snapDeformationLength_ = (std::min)(snapDeformationLength(), maxS);

            // Deform end of stroke to match snap position
            double s = 0;
            geometry::Vec2d snapPosition = getSnapPosition(snapVertex);
            geometry::Vec2d delta = snapPosition - lastInputPoint;
            points_.last() = snapPosition;
            for (Int i = smoothedInputPoints_.length() - 2; i >= 0; --i) {
                s = maxS - smoothedInputArclengths_[i];
                if (s < snapDeformationLength_) {
                    points_[i] =
                        snapDeformation(points_[i], delta, s, snapDeformationLength_);
                }
                else {
                    break;
                }
            }

            // Update DOM and workspace
            endVertex_->remove();
            endVertex_ = snapVertex->domElement();
            edge_->setAttribute(ds::positions, points_);
            edge_->setAttribute(ds::endvertex, endVertex_->getPathFromId());
            workspace->sync();
        }
    }

    requestRepaint();
}

bool SketchTool::resetData_() {
    if (drawCurveUndoGroup_) {
        drawCurveUndoGroup_->close();
        drawCurveUndoGroup_->undone().disconnect(drawCurveUndoGroupConnectionHandle_);
        drawCurveUndoGroup_ = nullptr;
    }
    if (isSketching_) {
        isSketching_ = false;
        endVertex_ = nullptr;
        edge_ = nullptr;
        points_.clear();
        widths_.clear();
        lastInputPoints_.clear();
        smoothedInputPoints_.clear();
        smoothedInputArclengths_.clear();
        return true;
    }
    return false;
}

double SketchTool::snapDeformationLength() const {

    using namespace style::literals;
    constexpr style::Length snapDeformationLength_ = 100.0_dp;

    ui::Canvas* canvas = this->canvas();
    double zoom = canvas ? canvas->camera().zoom() : 1.0;

    return snapDeformationLength_.toPx(styleMetrics()) / zoom;
}

// Note: in the future we may want to have the snapping candidates implemented
// in canvas to be shared by all tools.

workspace::Element* SketchTool::computeSnapVertex_(
    const geometry::Vec2d& position,
    dom::Element* excludedElement_) {

    using namespace style::literals;
    constexpr style::Length snapRadius_ = 14.0_dp;

    workspace::Workspace* workspace = this->workspace();
    if (!workspace) {
        return nullptr;
    }

    ui::Canvas* canvas = this->canvas();
    double zoom = canvas ? canvas->camera().zoom() : 1.0;
    double snapRadius = snapRadius_.toPx(styleMetrics()) / zoom;
    double minDist = core::DoubleInfinity;

    workspace::Element* res = nullptr;

    workspace->visitDepthFirst(
        [](workspace::Element*, Int) { return true; },
        [&, snapRadius, position](workspace::Element* e, Int /*depth*/) {
            if (!e || !e->domElement() || e->domElement() == excludedElement_) {
                //    ^^^^^^^^^^^^^^^^
                //    For now, we forbid snapping to vertices with no corresponding
                //    DOM elements, e.g., vertices that belong to composite shapes.
                //
                return;
            }
            if (!e->vacNode() || !e->vacNode()->isCell()) {
                return;
            }
            if (!e->vacNode()->toCell()->toKeyVertex()) {
                return;
            }
            double dist = 0;
            if (e->isSelectableAt(position, false, snapRadius, &dist) && dist < minDist) {
                minDist = dist;
                res = e;
            }
        });

    return res;
}

} // namespace vgc::ui
