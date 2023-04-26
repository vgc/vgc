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
#include <vgc/ui/boolsettingedit.h>
#include <vgc/ui/column.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/numbersettingedit.h>
#include <vgc/ui/settings.h>
#include <vgc/ui/window.h>
#include <vgc/workspace/edge.h>

namespace vgc::ui {

namespace {

namespace options {

NumberSetting* penWidth() {
    static NumberSettingPtr setting = createDecimalNumberSetting(
        settings::session(), "tools.sketch.penWidth", "Pen Width", 5, 0, 1000);
    return setting.get();
}

BoolSetting* snapping() {
    static BoolSettingPtr setting = BoolSetting::create(
        settings::session(), "tools.sketch.snapping", "Snapping", true);
    return setting.get();
}

NumberSetting* snapDistance() {
    static NumberSettingPtr setting = createDecimalNumberSetting(
        settings::session(), "tools.sketch.snapDistance", "Snap Distance", 10, 0, 1000);
    return setting.get();
}

NumberSetting* snapFalloff() {
    static NumberSettingPtr setting = createDecimalNumberSetting(
        settings::session(), "tools.sketch.snapFalloff", "Snap Falloff", 100, 0, 1000);
    return setting.get();
}

} // namespace options

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
// point at s = snapFalloff.
//
geometry::Vec2d applySnapFalloff(
    const geometry::Vec2d& position,
    const geometry::Vec2d& delta,
    double s,
    double snapFalloff) {

    return position + delta * (1 - (s / snapFalloff));
}

} // namespace

SketchTool::SketchTool()
    : CanvasTool() {
}

SketchToolPtr SketchTool::create() {
    return SketchToolPtr(new SketchTool());
}

double SketchTool::penWidth() const {
    return options::penWidth()->value();
}

void SketchTool::setPenWidth(double width) {
    options::penWidth()->setValue(width);
}

bool SketchTool::isSnappingEnabled() const {
    return options::snapping()->value();
}

void SketchTool::setSnappingEnabled(bool enabled) {
    options::snapping()->setValue(enabled);
}

ui::WidgetPtr SketchTool::createOptionsWidget() const {
    ui::WidgetPtr res = ui::Column::create();
    res->createChild<ui::NumberSettingEdit>(options::penWidth());
    res->createChild<ui::BoolSettingEdit>(options::snapping());
    res->createChild<ui::NumberSettingEdit>(options::snapDistance());
    res->createChild<ui::NumberSettingEdit>(options::snapFalloff());
    return res;
}

bool SketchTool::onKeyPress(KeyEvent* /*event*/) {
    return false;
}

namespace {

double pressurePenWidth(const MouseEvent* event) {
    double baseWidth = options::penWidth()->value();
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

    continueCurve_(event);
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
    canvas->clearSelection();

    isSketching_ = true;

    startCurve_(event);
    return true;
}

bool SketchTool::onMouseRelease(MouseEvent* event) {

    if (event->button() == MouseButton::Left) {
        //VGC_DEBUG_TMP_EXPR(mouseInputPoints_);
        //VGC_DEBUG_TMP_EXPR(mouseInputPointsTimestamps_);
        finishCurve_(event);
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
    mouseInputGeometry_ =
        engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    reload_ = true;
}

void SketchTool::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

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
    if (isSketching_ && w) {
        geometry::Vec2f pos(w->mapFromGlobal(globalCursorPosition()));
        geometry::Vec2d posd(root()->mapTo(this, pos));
        pos = geometry::Vec2f(
            canvas->camera().viewMatrix().inverted().transformPointAffine(posd));
        if (lastImmediateCursorPos_ != pos) {
            lastImmediateCursorPos_ = pos;
            cursorMoved = true;
            geometry::Vec2d pos2d(pos);
            minimalLatencySnappedCursor_ = pos2d;
            if (!smoothedPoints_.isEmpty()) {

                double s = 0;
                geometry::Vec2d previousSp = smoothedPoints_.first();
                for (Int i = 0; i < smoothedPoints_.length(); ++i) {
                    geometry::Vec2d sp = smoothedPoints_[i];
                    s += (sp - previousSp).length();
                    previousSp = sp;
                }
                s += (pos2d - smoothedPoints_.last()).length();

                double snapFalloff_ = snapFalloff();
                geometry::Vec2d delta = startSnapPosition_ - smoothedPoints_[0];
                if (s < snapFalloff_) {
                    minimalLatencySnappedCursor_ =
                        applySnapFalloff(pos2d, delta, s, snapFalloff_);
                }
            }
        }
    }

    if (isSketching_ && (cursorMoved || minimalLatencyStrokeReload_)) {

        core::Color color = penColor_;
        geometry::Vec2fArray strokeVertices;

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
                double width = edgeLastSample.halfwidth(0) + edgeLastSample.halfwidth(1);

                // We only draw the curve tip if it is long enough w.r.t. the
                // stroke width, otherwise it looks really bad when drawing
                // thick strokes (lots of flickering between [-90°, 90°] angles
                // due to mouse inputs being integer pixels)
                //
                // TODO: improve visualization of the tip. It might make more
                // sense to only draw curve outlines as thin overlay lines,
                // since we draw tip as an overlay anyway, that is, not at the
                // same depth as the actual curve, and without any effects
                // (blur, layer opacity, etc.), so the tip as currently drawn
                // may look very different from the actual curve anyway.
                //
                if (tipDir.length() > width) {

                    geometry::Vec2d tipNormal = tipDir.orthogonalized().normalized();

                    double widthRatio = 0.5;
                    geometry::Vec2d tipPoint0 =
                        minimalLatencySnappedCursor_
                        - tipNormal * widthRatio * edgeLastSample.halfwidth(1);
                    geometry::Vec2d tipPoint1 =
                        minimalLatencySnappedCursor_
                        + tipNormal * widthRatio * edgeLastSample.halfwidth(0);

                    strokeVertices.emplaceLast(
                        geometry::Vec2f(edgeLastSample.leftPoint()));
                    strokeVertices.emplaceLast(
                        geometry::Vec2f(edgeLastSample.rightPoint()));
                    strokeVertices.emplaceLast(geometry::Vec2f(tipPoint0));
                    strokeVertices.emplaceLast(geometry::Vec2f(tipPoint1));
                }
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

    if (true) { // todo only update on new points

        core::Array<geometry::Vec4f> pointVertices;
        Int n = 32;
        pointVertices.reserve(n * 2 + 1);
        for (Int i = 0; i < n; ++i) {
            double a = core::pi * 2.0 * i / 32.f;
            pointVertices.emplaceLast(0, 0, 0, 0);
            pointVertices.emplaceLast(0, 0, std::cos(a), std::sin(a));
        }
        pointVertices.emplaceLast(0, 0, 1, 0);

        core::FloatArray pointInstData;
        const Int numPoints = inputPoints_.length();
        for (Int i = 0; i < numPoints; ++i) {
            geometry::Vec2f p = geometry::Vec2f(inputPoints_[i]);
            pointInstData.extend({p.x(), p.y(), 0.f, 4.f, 0.f, 1.f, 0.f, 1.f});
        }

        engine->updateBufferData(
            mouseInputGeometry_->vertexBuffer(0), std::move(pointVertices));
        engine->updateBufferData(
            mouseInputGeometry_->vertexBuffer(1), std::move(pointInstData));
    }

    geometry::Mat4f vm = engine->viewMatrix();
    geometry::Mat4f cameraViewf(canvas->camera().viewMatrix());
    engine->pushViewMatrix(vm * cameraViewf);

    if (isSketching_) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(minimalLatencyStrokeGeometry_);
    }

    engine->popViewMatrix();

    // the input points geometry is in widget space
    //engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
    //engine->drawInstanced(mouseInputGeometry_);
}

void SketchTool::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    minimalLatencyStrokeGeometry_.reset();
}

void SketchTool::updateUnquantizedData_() {
    unquantizedPoints_ = inputPoints_;
    unquantizedWidths_ = inputWidths_;

    // Skip duplicate points
    //if (!inputPoints_.isEmpty()) {
    //    if (inputPoints_.last() == p) {
    //        // TODO: update last point width if width increases
    //        return;
    //    }
    //}
}

void SketchTool::updateTransformedData_() {
    Int n = unquantizedPoints_.length();
    transformedPoints_.resize(n);
    for (Int i = 0; i < n; ++i) {
        geometry::Vec2d pointd(unquantizedPoints_[i]);
        transformedPoints_[i] = canvasToWorkspaceMatrix_.transformPointAffine(pointd);
    }
    transformedWidths_ = unquantizedWidths_;
}

void SketchTool::updateSmoothedData_() {

    smoothedPoints_ = transformedPoints_;
    smoothedWidths_ = transformedWidths_;

    Int numPoints = smoothedPoints_.length();

    if (numPoints >= 3) {
        // Apply gaussian smoothing.

        const geometry::Vec2dArray& inPoints = transformedPoints_;

        smoothedPoints_.getUnchecked(1) =          //
            (1 / 4.0) * inPoints.getUnchecked(0) + //
            (2 / 4.0) * inPoints.getUnchecked(1) + //
            (1 / 4.0) * inPoints.getUnchecked(2);

        smoothedPoints_.getUnchecked(numPoints - 2) =          //
            (1 / 4.0) * inPoints.getUnchecked(numPoints - 1) + //
            (2 / 4.0) * inPoints.getUnchecked(numPoints - 2) + //
            (1 / 4.0) * inPoints.getUnchecked(numPoints - 3);

        for (Int i = 2; i < numPoints - 2; ++i) {
            smoothedPoints_.getUnchecked(i) =               //
                (1 / 16.0) * inPoints.getUnchecked(i - 2) + //
                (4 / 16.0) * inPoints.getUnchecked(i - 1) + //
                (6 / 16.0) * inPoints.getUnchecked(i + 0) + //
                (4 / 16.0) * inPoints.getUnchecked(i + 1) + //
                (1 / 16.0) * inPoints.getUnchecked(i + 2);
        }
    }

    // TODO: could remove second and before last sample here
    //       since they are less smooth than the others.
}

void SketchTool::startCurve_(MouseEvent* event) {

    // Clear the points now. We don't to it on finishCurve_() for
    // debugging purposes.
    //
    inputPoints_.clear();
    inputWidths_.clear();
    inputPointsTimestamps_.clear();

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

    geometry::Vec2f eventPos2f = event->position();
    geometry::Vec2d eventPos2d = geometry::Vec2d(eventPos2f.x(), eventPos2f.y());

    // Save inverse view matrix
    canvasToWorkspaceMatrix_ = canvas()->camera().viewMatrix().inverted();
    isInputQuantized_ = !event->isTablet();

    // Compute start vertex to snap to
    workspace::Element* snapVertex = nullptr;
    hasStartSnap_ = false;
    startSnapPosition_ = canvasToWorkspaceMatrix_.transformPointAffine(eventPos2d);
    if (isSnappingEnabled()) {
        snapVertex = computeSnapVertex_(startSnapPosition_, nullptr);
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
    continueCurve_(event);

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

void SketchTool::continueCurve_(MouseEvent* event) {

    // Fast return if missing required context
    workspace::Workspace* workspace = this->workspace();
    if (!workspace || !workspace->document() || !edge_ || !endVertex_) {
        return;
    }

    geometry::Vec2f eventPos2f = event->position();

    // Append the input point
    inputPoints_.append(eventPos2f);
    inputWidths_.append(pressurePenWidth(event));
    inputPointsTimestamps_.append(event->timestamp());

    updateUnquantizedData_();
    updateTransformedData_();
    updateSmoothedData_();

    // Handle snapping
    if (hasStartSnap_) {

        Int numPoints = smoothedPoints_.length();
        double snapFalloff_ = snapFalloff();
        geometry::Vec2d delta = startSnapPosition_ - smoothedPoints_[0];

        double s = 0;
        points_.resizeNoInit(numPoints);
        geometry::Vec2d previousSp = smoothedPoints_.first();
        for (Int i = 0; i < numPoints; ++i) {
            geometry::Vec2d sp = smoothedPoints_[i];
            s += (sp - previousSp).length();
            if (s < snapFalloff_) {
                points_[i] = applySnapFalloff(sp, delta, s, snapFalloff_);
            }
            else {
                points_[i] = sp; // maybe optimize in the future
            }
            previousSp = sp;
        }

        widths_ = smoothedWidths_;
    }
    else {
        points_ = smoothedPoints_;
        widths_ = smoothedWidths_;
    }

    // Update DOM and workspace
    namespace ds = dom::strings;
    endVertex_->setAttribute(ds::position, points_.last());
    edge_->setAttribute(ds::positions, points_);
    edge_->setAttribute(ds::widths, widths_);
    workspace->sync();
}

void SketchTool::finishCurve_(MouseEvent* /*event*/) {

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
    if (isSnappingEnabled() && smoothedPoints_.length() > 1) {

        // Compute start vertex to snap to
        geometry::Vec2d endPoint = canvasToWorkspaceMatrix_.transformPointAffine(
            geometry::Vec2d(inputPoints_.last()));
        workspace::Element* snapVertex = computeSnapVertex_(endPoint, endVertex_);

        // If found, do the snapping
        if (snapVertex) {

            Int numPoints = smoothedPoints_.length();
            core::DoubleArray arclengthsFromLast(numPoints, core::noInit);
            arclengthsFromLast.last() = 0;
            geometry::Vec2d previousSp = smoothedPoints_.last();
            double s = 0;
            for (Int i = numPoints - 2; i >= 0; --i) {
                geometry::Vec2d sp = smoothedPoints_[i];
                s += (sp - previousSp).length();
                arclengthsFromLast[i] = s;
                previousSp = sp;
            }

            // Cap snap deformation length to ensure start point isn't modified
            double maxS = arclengthsFromLast.first();
            double snapFalloff_ = (std::min)(snapFalloff(), maxS);

            // Deform end of stroke to match snap position
            geometry::Vec2d snapPosition = getSnapPosition(snapVertex);
            geometry::Vec2d delta = snapPosition - endPoint;
            points_.last() = snapPosition;
            for (Int i = smoothedPoints_.length() - 2; i >= 0; --i) {
                s = arclengthsFromLast[i];
                if (s < snapFalloff_) {
                    points_[i] = applySnapFalloff(points_[i], delta, s, snapFalloff_);
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
        // inputs are kept until next curve starts
        // for debugging purposes.
        //inputPoints_.clear();
        //inputWidths_.clear();
        //inputPointsTimestamps_.clear();
        unquantizedPoints_.clear();
        unquantizedWidths_.clear();
        transformedPoints_.clear();
        transformedWidths_.clear();
        smoothedPoints_.clear();
        smoothedWidths_.clear();
        points_.clear();
        widths_.clear();
        return true;
    }
    return false;
}

double SketchTool::snapFalloff() const {

    float snapFalloffFloat = static_cast<float>(options::snapFalloff()->value());
    style::Length snapFalloffLength(snapFalloffFloat, style::LengthUnit::Dp);

    ui::Canvas* canvas = this->canvas();
    double zoom = canvas ? canvas->camera().zoom() : 1.0;

    return snapFalloffLength.toPx(styleMetrics()) / zoom;
}

// Note: in the future we may want to have the snapping candidates implemented
// in canvas to be shared by all tools.

workspace::Element* SketchTool::computeSnapVertex_(
    const geometry::Vec2d& position,
    dom::Element* excludedElement_) {

    workspace::Workspace* workspace = this->workspace();
    if (!workspace) {
        return nullptr;
    }

    float snapDistanceFloat = static_cast<float>(options::snapDistance()->value());
    style::Length snapDistanceLength(snapDistanceFloat, style::LengthUnit::Dp);

    ui::Canvas* canvas = this->canvas();
    double zoom = canvas ? canvas->camera().zoom() : 1.0;
    double snapDistance = snapDistanceLength.toPx(styleMetrics()) / zoom;
    double minDist = core::DoubleInfinity;

    workspace::Element* res = nullptr;

    workspace->visitDepthFirst(
        [](workspace::Element*, Int) { return true; },
        [&, snapDistance, position](workspace::Element* e, Int /*depth*/) {
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
            if (e->isSelectableAt(position, false, snapDistance, &dist)
                && dist < minDist) {
                minDist = dist;
                res = e;
            }
        });

    return res;
}

} // namespace vgc::ui
