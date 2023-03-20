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

#include <vgc/core/array.h>
#include <vgc/core/paths.h>
#include <vgc/core/stopwatch.h>
#include <vgc/core/stringid.h>
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

using namespace style::literals;
inline constexpr style::Length snapRadius = 14.0_dp;
inline constexpr style::Length snapDeformationLength = 100.0_dp;

SketchToolPtr SketchTool::create() {
    return SketchToolPtr(new SketchTool());
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

SketchTool::SketchTool()
    : CanvasTool() {

    // Set ClickFocus policy to be able to accept keyboard events (default
    // policy is NoFocus).
    setFocusPolicy(FocusPolicy::Click);
    setClippingEnabled(true);
}

bool SketchTool::onKeyPress(KeyEvent* /*event*/) {
    //switch (event->key()) {
    //default:
    //    return false;
    //}
    //
    //return true;

    return false;
}

void SketchTool::startCurve_(const geometry::Vec2d& p, double width) {
    workspace::Workspace* workspace = this->workspace();
    if (!workspace || !workspace->document()) {
        return;
    }

    namespace ds = dom::strings;

    // XXX CLEAN
    static core::StringId Draw_Curve("Draw Curve");
    core::History* history = workspace->history();
    if (history) {
        drawCurveUndoGroup_ = history->createUndoGroup(Draw_Curve);
        drawCurveUndoGroupConnectionHandle_ = drawCurveUndoGroup_->undone().connect(
            [this](core::UndoGroup*, bool /*isAbort*/) {
                // isAbort should be true since we have no sub-group
                isSketching_ = false;
                drawCurveUndoGroup_ = nullptr;
            });
    }

    workspace::Element* wVgc = workspace->vgcElement();
    dom::Element* dVgc = wVgc->domElement();

    // snapping to vertices
    // in the future we can have the snapping candidates implemented in canvas to be shared by all tools
    workspace::Element* closestVertex = nullptr;
    if (isSnappingEnabled_) {
        ui::Canvas* canvas = this->canvas();
        const double zoom = canvas ? canvas->camera().zoom() : 1.0;
        const double tol = snapRadius.toPx(styleMetrics()) / zoom;
        double minDist = core::DoubleInfinity;
        workspace->visitDepthFirst(
            [](workspace::Element*, Int) { return true; },
            [&, tol, p](workspace::Element* e, Int /*depth*/) {
                if (!e || !e->isVacElement() || !e->vacNode()
                    || !e->vacNode()->isCell()) {
                    return;
                }
                if (!e->vacNode()->toCell()->toKeyVertex()) {
                    return;
                }
                double dist = 0;
                if (e->isSelectableAt(p, false, tol, &dist) && dist < minDist) {
                    minDist = dist;
                    closestVertex = e;
                }
            });
    }

    dom::Element* v0 = nullptr;
    geometry::Vec2d snapPosition = p;
    if (closestVertex) {
        v0 = closestVertex->domElement();
        snapPosition = closestVertex->vacNode()->toCell()->toKeyVertex()->position();
    }
    else {
        v0 = dom::Element::create(dVgc, ds::vertex);
        v0->setAttribute(ds::position, snapPosition);
    }

    dom::Element* v1 = dom::Element::create(dVgc, ds::vertex);
    v1->setAttribute(ds::position, snapPosition);

    dom::Element* edge = dom::Element::create(dVgc, ds::edge);
    edge->setAttribute(ds::positions, geometry::Vec2dArray());
    edge->setAttribute(ds::widths, core::DoubleArray());
    edge->setAttribute(ds::color, penColor_);
    edge->setAttribute(ds::startvertex, v0->getPathFromId());
    edge->setAttribute(ds::endvertex, v1->getPathFromId());

    endVertex_ = v1;
    edge_ = edge;

    snapPosition_ = snapPosition;
    continueCurve_(p, width);

    minimalLatencyStrokePoints_[0] = snapPosition;
    minimalLatencyStrokeWidths_[0] = width * 0.5;
    minimalLatencyStrokePoints_[1] = p;
    minimalLatencyStrokeWidths_[1] = width * 0.5;
    minimalLatencyStrokeReload_ = true;
}

void SketchTool::continueCurve_(const geometry::Vec2d& p, double width) {
    workspace::Workspace* workspace = this->workspace();
    if (!workspace || !workspace->document()) {
        return;
    }

    namespace ds = dom::strings;

    if (!edge_) {
        return;
    }

    if (!lastInputPoints_.isEmpty()) {
        if (lastInputPoints_.last() == p) {
            // skip duplicate point
            return;
        }
    }

    smoothedInputPoints_.append(p);
    points_.append(p);
    widths_.append(width);

    lastInputPoints_.prepend(p);
    if (lastInputPoints_.length() > 5) {
        lastInputPoints_.removeLast();
    }

    const Int num_points = smoothedInputPoints_.size();
    if (lastInputPoints_.length() >= 3) {
        // 1 2 1
        // clang-format off
        smoothedInputPoints_[num_points - 2] =
            (1 / 4.0) * lastInputPoints_[0] +
            (2 / 4.0) * lastInputPoints_[1] +
            (1 / 4.0) * lastInputPoints_[2];
        // clang-format on
    }
    if (lastInputPoints_.length() >= 5) {
        // 1 4 6 4 1
        // clang-format off
        smoothedInputPoints_[smoothedInputPoints_.size() - 3] =
            (1 / 16.0) * lastInputPoints_[0] +
            (4 / 16.0) * lastInputPoints_[1] +
            (6 / 16.0) * lastInputPoints_[2] +
            (4 / 16.0) * lastInputPoints_[3] +
            (1 / 16.0) * lastInputPoints_[4];
        // clang-format on
    }

    if (isSnappingEnabled_) {
        ui::Canvas* canvas = this->canvas();
        const double zoom = canvas ? canvas->camera().zoom() : 1.0;
        const double snapDeformationLengthInObjectSpace =
            snapDeformationLength.toPx(styleMetrics()) / zoom;
        double s = 0;
        geometry::Vec2d previousPoint = smoothedInputPoints_[0];
        geometry::Vec2d delta = snapPosition_ - previousPoint;
        for (Int i = 0; i < smoothedInputPoints_.length(); ++i) {
            geometry::Vec2d sp = smoothedInputPoints_[i];
            s += (previousPoint - sp).length();
            if (s < snapDeformationLengthInObjectSpace) {
                // linear, introduces a non-smooth sync point
                points_[i] = sp + delta * (1 - (s / snapDeformationLengthInObjectSpace));
            }
            else {
                // highly redundant, maybe optimize that in the future
                points_[i] = sp;
            }
            previousPoint = sp;
        }
    }
    else {
        for (Int i = 0; i < smoothedInputPoints_.length(); ++i) {
            geometry::Vec2d sp = smoothedInputPoints_[i];
            points_[i] = sp;
        }
    }

    endVertex_->setAttribute(ds::position, points_.last());

    edge_->setAttribute(ds::positions, points_);
    edge_->setAttribute(ds::widths, widths_);

    workspace->sync();

    // set it to fast tesselation to minimize lag
    workspace::Element* edgeElement = workspace->find(edge_);
    auto edgeCell = dynamic_cast<workspace::VacKeyEdge*>(edgeElement);
    if (edgeCell) {
        edgeCell->setTesselationMode(0);
    }
}

void SketchTool::finishCurve_() {
    workspace::Workspace* workspace = this->workspace();
    if (!workspace || !workspace->document()) {
        return;
    }

    namespace ds = dom::strings;

    if (!edge_) {
        return;
    }

    workspace::Element* edgeElement = workspace->find(edge_);
    auto edgeCell = dynamic_cast<workspace::VacKeyEdge*>(edgeElement);
    if (edgeCell) {
        edgeCell->setTesselationMode(canvas()->requestedTesselationMode());
    }

    const geometry::Vec2d lastInputPoint = lastInputPoints_[0];
    const Int num_points = smoothedInputPoints_.size();
    if (num_points > 1) {
        // snapping to vertices
        // in the future we can have the snapping candidates implemented in canvas to be shared by all tools
        workspace::Element* closestVertex = nullptr;
        if (isSnappingEnabled_) {
            ui::Canvas* canvas = this->canvas();
            const double zoom = canvas ? canvas->camera().zoom() : 1.0;
            const double tol = snapRadius.toPx(styleMetrics()) / zoom;
            double minDist = core::DoubleInfinity;

            workspace->visitDepthFirst(
                [](workspace::Element*, Int) { return true; },
                [&, tol, lastInputPoint](workspace::Element* e, Int /*depth*/) {
                    if (!e || (e->domElement() == endVertex_)) {
                        return;
                    }
                    if (!e->vacNode() || !e->vacNode()->isCell()) {
                        return;
                    }
                    if (!e->vacNode()->toCell()->toKeyVertex()) {
                        return;
                    }
                    double dist = 0;
                    if (e->isSelectableAt(lastInputPoint, false, tol, &dist)
                        && dist < minDist) {

                        minDist = dist;
                        closestVertex = e;
                    }
                });

            if (closestVertex) {
                geometry::Vec2d snapPosition =
                    closestVertex->vacNode()->toCell()->toKeyVertex()->position();

                points_.last() = snapPosition;
                double maxS = 0;
                for (Int i = 1; i < smoothedInputPoints_.length(); ++i) {
                    maxS +=
                        (smoothedInputPoints_[i] - smoothedInputPoints_[i - 1]).length();
                }

                const double snapDeformationLengthInObjectSpace =
                    (std::min)(snapDeformationLength.toPx(styleMetrics()) / zoom, maxS);
                double s = 0;
                geometry::Vec2d delta = snapPosition - lastInputPoint;
                for (Int i = smoothedInputPoints_.length() - 2; i >= 0; --i) {
                    s += (smoothedInputPoints_[i + 1] - smoothedInputPoints_[i]).length();
                    if (s < snapDeformationLengthInObjectSpace) {
                        // linear, introduces a non-smooth sync point
                        points_[i] +=
                            delta * (1 - (s / snapDeformationLengthInObjectSpace));
                    }
                    else {
                        break;
                    }
                }

                endVertex_->remove();
                endVertex_ = closestVertex->domElement();
                edge_->setAttribute(ds::positions, points_);
                edge_->setAttribute(ds::endvertex, endVertex_->getPathFromId());

                workspace->sync();
            }
        }
    }
}

double SketchTool::pressurePenWidth_(const MouseEvent* event) const {
    return event->hasPressure() ? 2 * event->pressure() * penWidth_ : penWidth_;
}

// Reimplementation of Widget virtual methods

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
    double width = pressurePenWidth_(event);
    continueCurve_(worldCoords, width);
    minimalLatencyStrokePoints_[0] = minimalLatencyStrokePoints_[1];
    minimalLatencyStrokeWidths_[0] = minimalLatencyStrokeWidths_[1];
    minimalLatencyStrokePoints_[1] = worldCoords;
    minimalLatencyStrokeWidths_[1] = width;
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

    isSketching_ = true;

    geometry::Vec2f mousePosf = event->position();
    geometry::Vec2d mousePos = geometry::Vec2d(mousePosf.x(), mousePosf.y());

    geometry::Vec2d viewCoords = mousePos;
    geometry::Vec2d worldCoords =
        canvas->camera().viewMatrix().inverted().transformPointAffine(viewCoords);
    startCurve_(worldCoords, pressurePenWidth_(event));
    return true;
}

bool SketchTool::onMouseRelease(MouseEvent* event) {
    if (event->button() == MouseButton::Left) {
        if (isSketching_) {
            finishCurve_();
        }
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
            requestRepaint();
            return true;
        }
    }

    return false;
}

bool SketchTool::onMouseEnter() {
    cursorChanger_.set(crossCursor());
    return false;
}

bool SketchTool::onMouseLeave() {
    cursorChanger_.clear();
    return false;
}

void SketchTool::onVisible() {
}

void SketchTool::onHidden() {
}

void SketchTool::onResize() {
    reload_ = true;
}

void SketchTool::onPaintCreate(graphics::Engine* engine) {
    using namespace graphics;
    minimalLatencyStrokeGeometry_ =
        engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XY_iRGBA);
    reload_ = true;
}

void SketchTool::onPaintDraw(graphics::Engine* engine, PaintOptions /*options*/) {
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
            minimalLatencyStrokePoints_[2] = geometry::Vec2d(pos);
            minimalLatencyStrokeWidths_[2] = minimalLatencyStrokeWidths_[1] * 0.5;
        }
    }

    if (cursorMoved || minimalLatencyStrokeReload_) {

        //core::Color color(1.f, 0.f, 0.f, 1.f);
        core::Color color = penColor_;
        geometry::Vec2fArray strokeVertices;

        geometry::Curve curve;
        curve.setPositions(minimalLatencyStrokePoints_);
        curve.setWidths(minimalLatencyStrokeWidths_);

        geometry::CurveSamplingParameters samplingParams = {};
        samplingParams.setMaxAngle(0.05);
        samplingParams.setMinIntraSegmentSamples(10);
        samplingParams.setMaxIntraSegmentSamples(20);
        geometry::CurveSampleArray csa;
        curve.sampleRange(samplingParams, csa, 1);

        for (const geometry::CurveSample& s : csa) {
            geometry::Vec2d p0 = s.leftPoint();
            strokeVertices.emplaceLast(geometry::Vec2f(p0));
            geometry::Vec2d p1 = s.rightPoint();
            strokeVertices.emplaceLast(geometry::Vec2f(p1));
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

void SketchTool::onPaintDestroy(graphics::Engine*) {
    minimalLatencyStrokeGeometry_.reset();
}

} // namespace vgc::ui
