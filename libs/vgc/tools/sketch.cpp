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

#include <vgc/tools/sketch.h>

#include <QBitmap>
#include <QCursor>
#include <QPainter>

#include <vgc/canvas/documentmanager.h>
#include <vgc/canvas/experimental.h>
#include <vgc/core/arithmetic.h> // roundToSignificantDigits
#include <vgc/core/profile.h>
#include <vgc/core/stringid.h>
#include <vgc/geometry/curve.h>
#include <vgc/graphics/strings.h>
#include <vgc/tools/logcategories.h>
#include <vgc/ui/boolsettingedit.h>
#include <vgc/ui/column.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/enumsettingedit.h>
#include <vgc/ui/numbersettingedit.h>
#include <vgc/ui/settings.h>
#include <vgc/ui/window.h>
#include <vgc/workspace/edge.h>
#include <vgc/workspace/strings.h>

namespace vgc::tools {

namespace options_ {

ui::NumberSetting* penWidth() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(), "tools.sketch.penWidth", "Pen Width", 5, 0, 1000);
    return setting.get();
}

ui::BoolSetting* snapping() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(), "tools.sketch.snapping", "Snapping", true);
    return setting.get();
}

ui::NumberSetting* snapDistance() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(),
        "tools.sketch.snapDistance",
        "Snap Distance",
        10,
        0,
        1000);
    return setting.get();
}

ui::NumberSetting* snapFalloff() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(),
        "tools.sketch.snapFalloff",
        "Snap Falloff",
        100,
        0,
        1000);
    return setting.get();
}

ui::BoolSetting* autoFill() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(), "tools.sketch.autoFill", "Auto-Fill", false);
    return setting.get();
}

ui::EnumSetting* sketchPreprocessing() {
    static ui::EnumSettingSharedPtr setting = ui::EnumSetting::create(
        ui::settings::session(),
        "tools.sketch.experimental.sketchPreprocessing",
        "Sketch Preprocessing",
        SketchPreprocessing::Default);
    return setting.get();
}

ui::BoolSetting* reProcessExistingEdges() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(),
        "tools.sketch.experimental.reProcessExistingEdges",
        "Re-Process Existing Edges",
        false);
    return setting.get();
}

} // namespace options_

namespace {

bool isAutoFillEnabled() {
    return options_::autoFill()->value();
}

} // namespace

VGC_DEFINE_ENUM( //
    SketchPreprocessing,
    (Default, "Default (Quadratic Blend)"),
    (NoPreprocessing, "No Preprocessing"),
    (IndexGaussianSmoothing, "Index-Based Gaussian Smoothing"),
    (DouglasPeucker, "Douglas-Peucker"),
    (SingleLineSegmentWithFixedEndpoints, "Single Line Segment (Fixed Endpoints)"),
    (SingleLineSegmentWithFreeEndpoints, "Single Line Segment (Free Endpoints)"),
    (SingleQuadraticSegmentWithFixedEndpoints,
     "Single Quadratic Segment (Fixed Endpoints)"),
    (QuadraticSpline, "Quadratic Spline"),
    (QuadraticBlend, "Quadratic Blend"))

SketchModule::SketchModule(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    if (auto module = context.importModule<canvas::ExperimentalModule>().lock()) {
        module->addWidget(*ui::EnumSettingEdit::create(options_::sketchPreprocessing()));
        module->addWidget(
            *ui::BoolSettingEdit::create(options_::reProcessExistingEdges()));
    }

    options_::sketchPreprocessing()->valueChanged().connect(
        onPreprocessingChanged_Slot());
}

SketchModulePtr SketchModule::create(const ui::ModuleContext& context) {
    return core::createObject<SketchModule>(context);
}

SketchPreprocessing SketchModule::preprocessing() const {
    return options_::sketchPreprocessing()->value().get<SketchPreprocessing>();
}

void SketchModule::onPreprocessingChanged_() {
    if (options_::reProcessExistingEdges()->value()) {
        reProcessExistingEdges_();
    }
}

namespace {

double pressurePen(const ui::MouseEvent* event) {
    return event->hasPressure() ? event->pressure() : 0.5;
}

double pressurePenWidth(double pressure, double baseWidth) {
    return 2.0 * pressure * baseWidth;
}

double pressurePenWidth(double pressure) {
    double baseWidth = options_::penWidth()->value();
    return pressurePenWidth(pressure, baseWidth);
}

// We round double values to float precision since:
// - Most come from a float anyway, so we do not actually lose precision.
// - Even if we did lose precision, the extra precision is overkill/useless anyway.
// - It significantly reduces the size of the XML file output. A striking
//   example is the timestamps, that at least in macOS, are always an exact
//   number of milliseconds, but would otherwise be formatted like 0.128000000004.
//
double roundInput(double x) {
    return core::roundToSignificantDigits(x, 7);
}

void setupPipeline(SketchPipeline& pipeline, SketchPreprocessing preprocessing) {
    pipeline.clear();
    pipeline.addPass<RemoveDuplicatesPass>();
    switch (preprocessing) {
    case SketchPreprocessing::Default:
        pipeline.addPass<QuadraticBlendPass>();
        break;
    case SketchPreprocessing::NoPreprocessing:
        // Nothing
        break;
    case SketchPreprocessing::IndexGaussianSmoothing:
        pipeline.addPass<SmoothingPass>();
        break;
    case SketchPreprocessing::DouglasPeucker:
        pipeline.addPass<DouglasPeuckerPass>();
        break;
    case SketchPreprocessing::SingleLineSegmentWithFixedEndpoints:
        pipeline.addPass<SingleLineSegmentWithFixedEndpointsPass>();
        break;
    case SketchPreprocessing::SingleLineSegmentWithFreeEndpoints:
        pipeline.addPass<SingleLineSegmentWithFreeEndpointsPass>();
        break;
    case SketchPreprocessing::SingleQuadraticSegmentWithFixedEndpoints:
        pipeline.addPass<SingleQuadraticSegmentWithFixedEndpointsPass>();
        break;
    case SketchPreprocessing::QuadraticSpline:
        pipeline.addPass<QuadraticSplinePass>();
        break;
    case SketchPreprocessing::QuadraticBlend:
        pipeline.addPass<QuadraticBlendPass>();
        break;
    }
    pipeline.addPass<SmoothingPass>();
    pipeline.addPass<TransformPass>();
}

// Sets the SketchPoint buffer and the transform matrix from saved input points.
//
// Returns false if there was no saved input points or if the saved data was
// corrupted (unexpected type of array sizes).
//
bool setFromSavedInputPoints(
    SketchPointBuffer& inputPoints,
    geometry::Mat3d& transformMatrix,
    workspace::Element* item) {

    namespace ds = dom::strings;
    using core::DoubleArray;
    using dom::Value;
    using geometry::Mat3d;
    using geometry::Vec2dArray;

    // Check that the item is a key edge
    if (!dynamic_cast<workspace::VacKeyEdge*>(item)) {
        return false;
    }

    // Check that it has a valid DOM element
    dom::Element* e = item->domElement();
    if (!e) {
        return false;
    }

    // Check that it has non-corrupted saved input data
    const auto* transform = e->getAttributeIf<Mat3d>(ds::inputtransform);
    const auto* penWidth = e->getAttributeIf<double>(ds::inputpenwidth);
    const auto* positions = e->getAttributeIf<Vec2dArray>(ds::inputpositions);
    const auto* pressures = e->getAttributeIf<DoubleArray>(ds::inputpressures);
    const auto* timestamps = e->getAttributeIf<DoubleArray>(ds::inputtimestamps);
    if (!(transform && penWidth && positions && pressures && timestamps)) {
        return false;
    }
    Int n = positions->length();
    if (!(pressures->length() == n && timestamps->length() == n)) {
        return false;
    }

    // Sets the transform matrix
    transformMatrix = *transform;

    // Sets the input points
    inputPoints.reset();
    for (Int i = 0; i < n; ++i) {
        inputPoints.emplaceLast(
            (*positions)[i],
            (*pressures)[i],
            (*timestamps)[i],
            roundInput(pressurePenWidth((*pressures)[i], *penWidth)));
    }
    inputPoints.updateChordLengths();
    inputPoints.setNumStablePoints(inputPoints.length());

    return true;
}

void updateEdgeGeometry(const SketchPointBuffer& points, workspace::Element* item) {

    namespace ds = dom::strings;

    // Check that the item is a key edge
    if (!dynamic_cast<workspace::VacKeyEdge*>(item)) {
        return;
    }

    // Check that it has a valid DOM element
    dom::Element* domEdge = item->domElement();
    if (!domEdge) {
        return;
    }

    geometry::Vec2dArray positions;
    core::DoubleArray widths;
    for (const SketchPoint& p : points) {
        positions.append(p.position());
        widths.append(p.width());
    }
    domEdge->setAttribute(ds::positions, positions);
    domEdge->setAttribute(ds::widths, widths);
}

} // namespace

void SketchModule::reProcessExistingEdges_() {

    // Get the workspace.
    //
    workspace::WorkspaceLockPtr workspace;
    if (auto module = importModule<canvas::DocumentManager>().lock()) {
        workspace = module->currentWorkspace().lock();
    }
    if (!workspace) {
        return;
    }

    // Create sketch passes.
    //
    // Note: the recomputation ignores any snapping that may have occured when
    // originally sketching the curve, since this info is not saved. However,
    // if the start/end endpoints of the recomputed curve do not match the
    // current positions of the start/end vertices of the edge, then the curve
    // will anyway be automatically transformed by the workspace/vacomplex as a
    // post-processing step to make these match.
    //
    SketchPointBuffer inputPoints;
    SketchPipeline pipeline;
    setupPipeline(pipeline, preprocessing());

    // Create undo group
    static core::StringId undoGroupName("Re-Fit Existing Edges");
    core::UndoGroupWeakPtr undoGroup_;
    if (core::History* history = workspace->history()) {
        undoGroup_ = history->createUndoGroup(undoGroupName);
    }

    // Apply passes to all curves with saved input sketch points.
    //
    workspace->visitDepthFirstPreOrder([&](workspace::Element* item, Int depth) {
        VGC_UNUSED(depth);
        geometry::Mat3d transform;
        if (setFromSavedInputPoints(inputPoints, transform, item)) {

            // Setup and apply passes
            pipeline.reset();
            pipeline.setTransformMatrix(transform);
            pipeline.updateFrom(inputPoints);

            // Save result to DOM
            updateEdgeGeometry(pipeline.output(), item);
        }
    });
    workspace->sync();

    if (auto undoGroup = undoGroup_.lock()) {
        undoGroup->close();
    }
}

Sketch::Sketch(CreateKey key)
    : CanvasTool(key) {
}

SketchPtr Sketch::create() {
    return core::createObject<Sketch>();
}

double Sketch::penWidth() const {
    return options_::penWidth()->value();
}

void Sketch::setPenWidth(double width) {
    options_::penWidth()->setValue(width);
}

bool Sketch::isSnappingEnabled() const {
    return options_::snapping()->value();
}

void Sketch::setSnappingEnabled(bool enabled) {
    options_::snapping()->setValue(enabled);
}

ui::WidgetPtr Sketch::doCreateOptionsWidget() const {
    ui::WidgetPtr res = ui::Column::create();
    res->createChild<ui::NumberSettingEdit>(options_::penWidth());
    res->createChild<ui::BoolSettingEdit>(options_::snapping());
    res->createChild<ui::NumberSettingEdit>(options_::snapDistance());
    res->createChild<ui::NumberSettingEdit>(options_::snapFalloff());
    res->createChild<ui::BoolSettingEdit>(options_::autoFill());
    return res;
}

bool Sketch::onKeyPress(ui::KeyPressEvent* event) {
    VGC_UNUSED(event);
    return false;
}

bool Sketch::onMouseMove(ui::MouseMoveEvent* event) {

    if (!isSketching_) {
        return false;
    }

    auto canvas = this->canvas().lock();
    if (!canvas) {
        return false;
    }

    bool isPressureZero = hasPressure_ && !(event->pressure() > 0);
    if (isCurveStarted_) {

        // Ends the curve if the pressure becomes zero. If we receive a
        // non-zero pressure later, this has the intended effect of splitting
        // the curve into several curves.
        //
        if (isPressureZero) {
            finishCurve_(event);
            isCurveStarted_ = false;
        }
        else {
            continueCurve_(event);
        }
    }
    else {
        // Starts the curve as soon as the pressure is non-zero.
        //
        if (!isPressureZero) {
            isCurveStarted_ = true;
            startCurve_(event);
        }
    }

    minimalLatencyStrokeReload_ = true;
    return true;
}

bool Sketch::onMousePress(ui::MousePressEvent* event) {

    if (isSketching_ || event->button() != ui::MouseButton::Left
        || event->modifierKeys()) {
        return false;
    }

    auto context = contextLock();
    if (!context) {
        return false;
    }
    auto workspaceSelection = context.workspaceSelection();

    workspaceSelection->clear();

    // XXX: could be done when not sketching yet but we need to
    //      do it on undo/redo too. use workspace signal ?
    initCellInfoArrays_();

    isSketching_ = true;
    hasPressure_ = event->hasPressure();

    // If the device is pressure-enabled, we wait for the pressure to actually
    // be positive before starting the curve. This fixes issues on some devices
    // where the first/last samples have a null pressure.
    //
    bool isPressureZero = hasPressure_ && !(event->pressure() > 0);
    if (!isPressureZero) {
        isCurveStarted_ = true;
        startCurve_(event);
    }

    return true;
}

bool Sketch::onMouseRelease(ui::MouseReleaseEvent* event) {

    if (event->button() == ui::MouseButton::Left) {
        if (isSketching_) {
            if (isCurveStarted_) {
                finishCurve_(event);
                isCurveStarted_ = false;
            }
            isSketching_ = false;
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
#ifndef VGC_OS_WINDOWS
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

void Sketch::onMouseEnter() {
    cursorChanger_.set(crossCursor());
}

void Sketch::onMouseLeave() {
    cursorChanger_.clear();
}

void Sketch::onResize() {
    reload_ = true;
}

namespace {

// The "minimal latency tip" is an extension of the sketched edge drawn as an
// overlay to decrease the perceived lag between the mouse cursor and the
// sketched edge.
//
// It is basically a straight line between the edge endpoint (as set in the DOM
// when processing the mouse event) and the current mouse position given by
// ui::globalCursorPosition() at the time of drawing, which typically is a
// "more recent" mouse position than the one provided in the last mouse event.
//
// This is an experimental feature which is currently disabled since it doesn't
// properly support outline-only display mode, or objects above the sketched
// edge, or constraining the sketch edge to be a single line segment. It also
// sometimes doesn't look good and is a bit distracting due to being a straight
// line.
//
// We might want to enable it after some polishing, most likely only shown as
// an "outline" so that it doesn't look bad when there are objects above the
// sketched edge, if if the sketched edge has some effect applied to it (e.g.,
// blur).
//
constexpr bool isMinimalLatencyTipEnabled = false;

} // namespace

void Sketch::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    if (isMinimalLatencyTipEnabled) {
        minimalLatencyStrokeGeometry_ =
            engine->createTriangleStrip(graphics::BuiltinGeometryLayout::XY_iRGBA);
    }
    reload_ = true;
}

namespace {

// Assumes 0 <= s <= snapFalloff
//
geometry::Vec2d applySnapFalloff(
    const geometry::Vec2d& position,
    const geometry::Vec2d& delta,
    double s,
    double snapFalloff) {

    // Cubic Ease-Out
    double t = s / snapFalloff;
    double x = 1 - t;
    return position + delta * x * x * x;
}

} // namespace

void Sketch::onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    if (!isMinimalLatencyTipEnabled) {
        return;
    }

    using namespace graphics;
    namespace gs = graphics::strings;

    auto context = contextLock();
    if (!context) {
        return;
    }
    auto workspace = context.workspace();
    auto canvas = context.canvas();

    ui::Window* w = window();
    bool cursorMoved = false;
    if (isSketching_ && w) {
        geometry::Vec2f pos(w->mapFromGlobal(ui::globalCursorPosition()));
        geometry::Vec2d posd(root()->mapTo(this, pos));
        pos = geometry::Vec2f(
            canvas->camera().viewMatrix().inverse().transformAffine(posd));
        if (lastImmediateCursorPos_ != pos) {
            lastImmediateCursorPos_ = pos;
            cursorMoved = true;
            geometry::Vec2d pos2d(pos);
            minimalLatencySnappedCursor_ = pos2d;
            if (snapStartPosition_.has_value()) {
                core::ConstSpan<SketchPoint> cleanInputPoints = cleanInputPoints_();
                if (cleanInputPoints.length() > 0) {
                    SketchPoint firstCleanInputPoint =
                        cleanInputStartPointOverride_.value_or(cleanInputPoints[0]);
                    const SketchPoint& lastCleanInputPoint = cleanInputPoints.last();
                    double startS = firstCleanInputPoint.s();
                    double s = lastCleanInputPoint.s() - startS;
                    s += (pos2d - lastCleanInputPoint.position()).length();
                    double falloff = snapFalloff_();
                    if (s < falloff) {
                        geometry::Vec2d delta =
                            snapStartPosition_.value() - firstCleanInputPoint.position();
                        minimalLatencySnappedCursor_ =
                            applySnapFalloff(pos2d, delta, s, falloff);
                    }
                }
            }
        }
    }

    if (isSketching_ && (cursorMoved || minimalLatencyStrokeReload_)) {

        core::Color color = penColor_;
        geometry::Vec2fArray strokeVertices;

        workspace::Element* edgeItem = workspace->find(this->edgeItemId_);
        vacomplex::KeyEdge* ke = nullptr;
        auto edgeCell = dynamic_cast<workspace::VacKeyEdge*>(edgeItem);
        if (edgeCell) {
            ke = edgeCell->vacKeyEdgeNode();
        }
        if (ke) {
            const geometry::StrokeSample2dArray& samples = ke->strokeSampling().samples();
            // one sample is not enough to have a well-defined normal
            if (samples.length() >= 2) {
                geometry::StrokeSample2d edgeLastSample =
                    ke->strokeSampling().samples().last();
                geometry::Vec2d tipDir =
                    minimalLatencySnappedCursor_ - edgeLastSample.position();
                double width = edgeLastSample.halfwidth(0) + edgeLastSample.halfwidth(1);

                // We only draw the curve tip if it is long enough w.r.t. the
                // stroke width, otherwise it looks really bad when drawing
                // thick strokes (lots of flickering between [-90°, 90°] angles
                // due to mouse inputs being integer pixels)
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
                        geometry::Vec2f(edgeLastSample.offsetPoint(1)));
                    strokeVertices.emplaceLast(
                        geometry::Vec2f(edgeLastSample.offsetPoint(0)));
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

    geometry::Mat4f vm = engine->viewMatrix();
    geometry::Mat3d cameraView = canvas->camera().viewMatrix();
    engine->pushViewMatrix(vm * geometry::Mat4f::fromTransform(cameraView));

    if (isSketching_) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(minimalLatencyStrokeGeometry_);
    }

    engine->popViewMatrix();
}

void Sketch::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    if (isMinimalLatencyTipEnabled) {
        minimalLatencyStrokeGeometry_.reset();
    }
}

core::ConstSpan<SketchPoint> Sketch::cleanInputPoints_() const {
    const SketchPointBuffer& allCleanInputPoints = pipeline_.output();
    return core::ConstSpan<SketchPoint>(
        allCleanInputPoints.begin() + cleanInputStartIndex_, allCleanInputPoints.end());
}

Int Sketch::numStableCleanInputPoints_() const {
    const SketchPointBuffer& allCleanInputPoints = pipeline_.output();
    return allCleanInputPoints.numStablePoints() - cleanInputStartIndex_;
}

namespace {

geometry::Vec2d getSnapPosition(workspace::Element* snapVertex) {
    if (vacomplex::Cell* cell = snapVertex->vacCell()) {
        vacomplex::KeyVertex* keyVertex = cell->toKeyVertex();
        if (keyVertex) {
            return keyVertex->position();
        }
    }
    VGC_WARNING(
        LogVgcToolsSketch,
        "Snap vertex didn't have an associated KeyVertex: using (0, 0) as position.");
    return geometry::Vec2d();
}

} // namespace

void Sketch::updateStartSnappedCleanInputPositions_() {

    core::ConstSpan<SketchPoint> cleanInputPoints = cleanInputPoints_();
    if (cleanInputPoints.isEmpty()) {
        return;
    }

    Int numStableCleanInputPoints = numStableCleanInputPoints_();
    Int newNumPendingPoints = cleanInputPoints.length();
    Int updateStartIndex = numStableStartSnappedCleanInputPositions_;

    SketchPoint firstCleanInputPoint =
        cleanInputStartPointOverride_.value_or(cleanInputPoints[0]);

    geometry::Vec2dArray& result = startSnappedCleanInputPositions_;
    result.resize(updateStartIndex);
    result.reserve(newNumPendingPoints);

    if (updateStartIndex == 0) {
        geometry::Vec2d position0 =
            snapStartPosition_.value_or(firstCleanInputPoint.position());
        result.append(position0);
        updateStartIndex = 1;
    }

    for (Int i = updateStartIndex; i < newNumPendingPoints; ++i) {
        const SketchPoint& p = cleanInputPoints[i];
        result.append(p.position());
    }

    if (snapStartPosition_.has_value()) {
        geometry::Vec2d ssp = snapStartPosition_.value();
        double falloff = snapFalloff_();
        double startS = firstCleanInputPoint.s();
        geometry::Vec2d delta = ssp - firstCleanInputPoint.position();
        for (Int i = updateStartIndex; i < newNumPendingPoints; ++i) {
            const SketchPoint& p = cleanInputPoints[i];
            double s = p.s() - startS;
            if (s < falloff) {
                result[i] = applySnapFalloff(result[i], delta, s, falloff);
            }
            else {
                break;
            }
        }
    }

    numStableStartSnappedCleanInputPositions_ = numStableCleanInputPoints;
}

void Sketch::endSnapStartSnappedCleanInputPositions_(geometry::Vec2dArray& result) {

    updateStartSnappedCleanInputPositions_();

    result.assign(startSnappedCleanInputPositions_);
    if (snapEndVertexItemId_ == 0 || result.length() < 2) {
        return;
    }

    auto workspace = this->workspace().lock();
    if (!workspace) {
        return;
    }

    workspace::Element* snapEndVertexItem = workspace->find(snapEndVertexItemId_);
    if (!snapEndVertexItem) {
        return;
    }

    geometry::Vec2d snapEndPosition = getSnapPosition(snapEndVertexItem);
    result.last() = snapEndPosition;

    core::ConstSpan<SketchPoint> cleanInputPoints = cleanInputPoints_();
    VGC_ASSERT(cleanInputPoints.length() == startSnappedCleanInputPositions_.length());

    SketchPoint firstCleanInputPoint =
        cleanInputStartPointOverride_.value_or(cleanInputPoints[0]);
    const SketchPoint& lastCleanInputPoint = cleanInputPoints.last();
    double maxS = lastCleanInputPoint.s();
    double totalS = maxS - firstCleanInputPoint.s();
    double falloff = (std::min)(snapFalloff_(), totalS);
    geometry::Vec2d delta = snapEndPosition - lastCleanInputPoint.position();
    for (Int i = result.length() - 2; i >= 0; --i) {
        const SketchPoint& p = cleanInputPoints[i];
        double reversedS = maxS - p.s();
        if (reversedS < falloff) {
            result[i] = applySnapFalloff(result[i], delta, reversedS, falloff);
        }
        else {
            break;
        }
    }
}

// Note: in the future we may want to have the snapping candidates implemented
// in canvas to be shared by all tools.

workspace::Element*
Sketch::computeSnapVertex_(const geometry::Vec2d& position, core::Id tmpVertexItemId) {

    auto context = contextLock();
    if (!context) {
        return nullptr;
    }
    auto workspace = context.workspace();
    auto canvas = context.canvas();

    float snapDistanceFloat = static_cast<float>(options_::snapDistance()->value());
    style::Length snapDistanceLength(snapDistanceFloat, style::LengthUnit::Dp);

    double zoom = canvas ? canvas->camera().zoom() : 1.0;
    double snapDistance = snapDistanceLength.toPx(styleMetrics()) / zoom;

    struct SnapCandidate {
        Int infoIdx;
        double dist;

        bool operator<(const SnapCandidate& rhs) const {
            return dist < rhs.dist;
        }
    };

    core::Array<SnapCandidate> candidates;
    for (Int i = 0; i < vertexInfos_.length(); ++i) {
        const VertexInfo& info = vertexInfos_[i];
        if (info.itemId == tmpVertexItemId) {
            continue;
        }
        double d = (info.position - position).length();
        if (d < snapDistance) {
            SnapCandidate& candidate = candidates.emplaceLast();
            candidate.infoIdx = i;
            candidate.dist = d;
        }
    }
    std::sort(candidates.begin(), candidates.end());

    for (const SnapCandidate& candidate : candidates) {
        VertexInfo& info = vertexInfos_[candidate.infoIdx];
        auto vertexItem =
            dynamic_cast<workspace::VacVertexCell*>(workspace->find(info.itemId));
        if (!vertexItem) {
            continue;
        }
        if (!info.isSelectable.has_value()) {
            core::Array<canvas::SelectionCandidate> occluders =
                canvas->computeSelectionCandidatesAboveOrAt(
                    info.itemId,
                    info.position,
                    snapDistance * core::epsilon,
                    canvas::CoordinateSpace::Workspace);
            bool isSelectable = false;
            for (const canvas::SelectionCandidate& occluder : occluders) {
                if (occluder.id() == info.itemId) {
                    isSelectable = true;
                    break;
                }
                if (workspace::Element* occluderItem = workspace->find(occluder.id())) {
                    if (workspace::VacElement* occluderVacItem =
                            occluderItem->toVacElement()) {
                        if (vacomplex::Cell* occluderCell = occluderVacItem->vacCell()) {
                            if (occluderCell->spatialType()
                                == vacomplex::CellSpatialType::Face) {
                                // face are occluders
                                break;
                            }
                        }
                    }
                    else {
                        // not a vac element, let's consider it prevents snapping.
                        break;
                    }
                }
            }
            info.isSelectable = isSelectable;
        }
        if (info.isSelectable.value()) {
            return vertexItem;
        }
    }

    return nullptr;
}

double Sketch::snapFalloff_() const {

    float snapFalloffFloat = static_cast<float>(options_::snapFalloff()->value());
    style::Length snapFalloffLength(snapFalloffFloat, style::LengthUnit::Dp);

    double zoom = 1.0;
    if (auto canvas = this->canvas().lock()) {
        zoom = canvas->camera().zoom();
    }

    return snapFalloffLength.toPx(styleMetrics()) / zoom;
}

void Sketch::updatePendingPositions_() {
    endSnapStartSnappedCleanInputPositions_(pendingPositions_);
}

void Sketch::updatePendingWidths_() {

    const SketchPointBuffer& cleanInputBuffer = pipeline_.output();
    const SketchPointArray& cleanInputPoints = cleanInputBuffer.data();
    if (cleanInputPoints.isEmpty()) {
        return;
    }

    Int numStableCleanInputPoints = numStableCleanInputPoints_();
    Int newNumPendingPoints = cleanInputPoints.length();
    Int updateStartIndex = numStablePendingWidths_;

    SketchPoint firstCleanInputPoint =
        cleanInputStartPointOverride_.value_or(cleanInputPoints[0]);

    core::DoubleArray& result = pendingWidths_;
    result.resize(updateStartIndex);
    result.reserve(newNumPendingPoints);

    if (updateStartIndex == 0) {
        double width0 = firstCleanInputPoint.width();
        result.append(width0);
        updateStartIndex = 1;
    }

    for (Int i = updateStartIndex; i < newNumPendingPoints; ++i) {
        const SketchPoint& p = cleanInputPoints[i];
        result.append(p.width());
    }

    numStablePendingWidths_ = numStableCleanInputPoints;
}

void Sketch::initCellInfoArrays_() {

    auto context = contextLock();
    if (!context) {
        return;
    }
    auto workspace = context.workspace();
    auto canvas = context.canvas();
    auto vac = workspace->vac().lock();
    if (!vac) {
        return;
    }
    core::AnimTime t = canvas->currentTime();

    vertexInfos_.clear();
    for (vacomplex::VertexCell* vc : vac->vertices(t)) {
        VertexInfo& vi = vertexInfos_.emplaceLast();
        vi.position = vc->position(t);
        vi.itemId = workspace->findVacElement(vc->id())->id();
    }

    edgeInfos_.clear();
    for (vacomplex::EdgeCell* ec : vac->edges(t)) {
        auto sampling = ec->strokeSamplingShared(t);
        if (sampling) {
            EdgeInfo& ei = edgeInfos_.emplaceLast();
            ei.sampling = ec->strokeSamplingShared(t);
            ei.itemId = workspace->findVacElement(ec->id())->id();
        }
    }
}

Sketch::VertexInfo* Sketch::searchVertexInfo_(core::Id itemId) {
    for (Sketch::VertexInfo& vi : vertexInfos_) {
        if (vi.itemId == itemId) {
            return &vi;
        }
    }
    return nullptr;
}

void Sketch::appendVertexInfo_(const geometry::Vec2d& position, core::Id itemId) {
    VertexInfo& vi = vertexInfos_.emplaceLast();
    vi.position = position;
    vi.itemId = itemId;
}

void Sketch::updateVertexInfo_(const geometry::Vec2d& position, core::Id itemId) {
    // Reverse iteration because currently, the updated itemId is always last
    for (auto it = vertexInfos_.rbegin(); it != vertexInfos_.rend(); ++it) {
        if (it->itemId == itemId) {
            it->position = position;
            break;
        }
    }
}

Sketch::EdgeInfo* Sketch::searchEdgeInfo_(core::Id itemId) {
    for (Sketch::EdgeInfo& ei : edgeInfos_) {
        if (ei.itemId == itemId) {
            return &ei;
        }
    }
    return nullptr;
}

namespace {

workspace::VacKeyEdge* toKeyEdgeItem(workspace::Workspace* workspace, core::Id itemId) {
    if (workspace) {
        if (workspace::Element* item = workspace->find(itemId)) {
            return dynamic_cast<workspace::VacKeyEdge*>(item);
        }
    }
    return nullptr;
}

vacomplex::KeyEdge* toKeyEdge(workspace::VacKeyEdge* keyEdgeItem) {
    if (keyEdgeItem) {
        if (vacomplex::Cell* cell = keyEdgeItem->vacCell()) {
            return cell->toKeyEdge();
        }
    }
    return nullptr;
}

vacomplex::KeyEdge* toKeyEdge(workspace::Workspace* workspace, core::Id itemId) {
    return toKeyEdge(toKeyEdgeItem(workspace, itemId));
}

} // namespace

void Sketch::startCurve_(ui::MouseEvent* event) {

    // Resets the input points now. We don't to it on finishCurve_() for
    // debugging purposes.
    //
    inputPoints_.reset();

    // Fast return if missing required context
    auto context = contextLock();
    if (!context) {
        return;
    }
    auto workspace = context.workspace();
    auto canvas = context.canvas();
    dom::Element* parentDomElement = workspace->vgcElement()->domElement();
    if (!parentDomElement) {
        return;
    }

    // Create undo group.
    // XXX: Cleanup this?
    static core::StringId Draw_Curve("Draw Curve");
    core::History* history = workspace->history();
    if (history) {
        SketchPtr self(this);
        drawCurveUndoGroup_ = history->createUndoGroup(Draw_Curve);
        drawCurveUndoGroupConnectionHandle_ = drawCurveUndoGroup_->undone().connect(
            [self]([[maybe_unused]] core::UndoGroup* undoGroup, bool /*isAbort*/) {
                if (!self.isAlive()) {
                    return;
                }
                // isAbort should be true since we have no sub-group
                if (self->drawCurveUndoGroup_) {
                    VGC_ASSERT(undoGroup == self->drawCurveUndoGroup_);
                    self->drawCurveUndoGroup_->undone().disconnect(
                        self->drawCurveUndoGroupConnectionHandle_);
                    self->drawCurveUndoGroup_ = nullptr;
                }
                self->isSketching_ = false;
                self->resetData_();
                self->requestRepaint();
            });
    }

    geometry::Vec2f eventPos2f = event->position();
    geometry::Vec2d eventPos2d = geometry::Vec2d(eventPos2f.x(), eventPos2f.y());

    startTime_ = event->timestamp();

    // Transform: Save inverse view matrix
    pipeline_.setTransformMatrix(canvas->camera().viewMatrix().inverse());

    // Snapping: Compute start vertex to snap to
    workspace::Element* snapVertex = nullptr;
    geometry::Vec2d startPosition = pipeline_.transformAffine(eventPos2d);
    if (isSnappingEnabled()) {
        snapVertex = computeSnapVertex_(startPosition, 0);
        if (snapVertex) {
            startPosition = getSnapPosition(snapVertex);
            snapStartPosition_ = startPosition;
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
    dom::Element* domStartVertex = nullptr;
    if (snapVertex) {
        domStartVertex = snapVertex->domElement();
    }
    if (!domStartVertex) {
        domStartVertex = dom::Element::create(parentDomElement, ds::vertex);
        domStartVertex->setAttribute(ds::position, startPosition);
    }
    startVertexItemId_ = domStartVertex->internalId();

    // Create end vertex
    dom::Element* domEndVertex = dom::Element::create(parentDomElement, ds::vertex);
    domEndVertex->setAttribute(ds::position, startPosition);
    endVertexItemId_ = domEndVertex->internalId();

    // Create edge
    dom::Element* domEdge = dom::Element::create(parentDomElement, ds::edge);
    domEdge->setAttribute(ds::positions, geometry::Vec2dArray());
    domEdge->setAttribute(ds::widths, core::DoubleArray());
    domEdge->setAttribute(ds::color, penColor_);
    domEdge->setAttribute(ds::startvertex, domStartVertex->getPathFromId());
    domEdge->setAttribute(ds::endvertex, domEndVertex->getPathFromId());
    if (canvas::experimental::saveInputSketchPoints()) {
        domEdge->setAttribute(ds::inputtransform, pipeline_.transformMatrix());
        domEdge->setAttribute(ds::inputpenwidth, options_::penWidth()->value());
    }
    edgeItemId_ = domEdge->internalId();

    // Configure sketch passes
    SketchPreprocessing preprocessing = SketchPreprocessing::Default;
    if (auto module = sketchModule_.lock()) {
        preprocessing = module->preprocessing();
    }
    if (!lastPreprocessing_ || *lastPreprocessing_ != preprocessing) {
        lastPreprocessing_ = preprocessing;
        setupPipeline(pipeline_, preprocessing);
    }

    // Append start point to geometry
    continueCurve_(event);
    workspace->sync(); // required for toKeyEdge() below

    // Append new start vertex (if any) to snap/cut info
    if (!snapStartPosition_) {
        appendVertexInfo_(startPosition, startVertexItemId_);
    }

    if (vacomplex::KeyEdge* keyEdge = toKeyEdge(workspace.get(), edgeItemId_)) {

        // Use low sampling quality override to minimize lag, unless current quality
        // is already even lower.
        //
        if (auto complex = workspace->vac().lock()) {
            geometry::CurveSamplingQuality quality = complex->samplingQuality();
            bool isAdaptive = geometry::isAdaptiveSampling(quality);
            Int8 level = geometry::getSamplingQualityLevel(quality);
            Int8 newLevel = std::min<Int8>(level, 2); // 2 = Low
            quality = geometry::getSamplingQuality(newLevel, isAdaptive);
            keyEdge->data().setSamplingQualityOverride(quality);
        }

        // Move edge to proper depth location.
        vacomplex::ops::moveBelowBoundary(keyEdge);
    }

    // Update stroke tip
    minimalLatencyStrokeReload_ = true;
}

namespace {

// Note: one may be tempted to try to optimize the function below by not
// recreating the arrays from scratch every time. However, this function is in
// fact already as optimized as possible, since we need anyway to create a new
// Value storing a new Array. We cannot do better than creating the Array here
// then moving it to the Value.
//
void doSaveInputPoints(dom::Element* edge, const SketchPointBuffer& inputPoints) {

    Int n = inputPoints.length();

    geometry::Vec2dArray inputPositions;
    core::DoubleArray inputPressures;
    core::DoubleArray inputTimestamps;

    inputPositions.reserve(n);
    inputPressures.reserve(n);
    inputTimestamps.reserve(n);

    for (const SketchPoint& p : inputPoints) {

        inputPositions.append(p.position());
        inputPressures.append(p.pressure());
        inputTimestamps.append(p.timestamp());
    }

    namespace ds = dom::strings;
    edge->setAttribute(ds::inputpositions, std::move(inputPositions));
    edge->setAttribute(ds::inputpressures, std::move(inputPressures));
    edge->setAttribute(ds::inputtimestamps, std::move(inputTimestamps));
}

} // namespace

void Sketch::continueCurve_(ui::MouseEvent* event) {

    // Fast return if missing required context
    auto workspace = this->workspace().lock();
    if (!workspace) {
        return;
    }
    auto document = workspace->document().lock();
    if (!document) {
        return;
    }

    dom::Element* domStartVertex = document->elementFromInternalId(startVertexItemId_);
    dom::Element* domEndVertex = document->elementFromInternalId(endVertexItemId_);
    dom::Element* domEdge = document->elementFromInternalId(edgeItemId_);
    if (!domStartVertex || !domEndVertex || !domEdge) {
        return;
    }

    // Append the input point
    //
    // XXX: it might to interesting to also record the current time (now) as
    // useful log info for performance analysis, so that we can answer
    // questions such as: which points where processed at the same time? Is
    // there significant delay between the event time and the processing time?
    // Are we processing them in batch every 16ms, or in real time when they
    // occur?
    //
    double pressure = pressurePen(event);
    inputPoints_.emplaceLast(
        geometry::Vec2d(roundInput(event->x()), roundInput(event->y())),
        roundInput(pressure),
        roundInput(event->timestamp() - startTime_),
        roundInput(pressurePenWidth(pressure)));
    inputPoints_.updateChordLengths();
    inputPoints_.setNumStablePoints(inputPoints_.length());

    // Apply all sketch passes
    pipeline_.updateFrom(inputPoints_);

    updatePendingPositions_();
    updatePendingWidths_();

    // TODO: auto cut algorithm

    // Update DOM and workspace
    //
    namespace ds = dom::strings;
    if (!snapStartPosition_) {
        // Unless start-snapped, processing passes may have modified the start point
        domStartVertex->setAttribute(ds::position, pendingPositions_.first());
        updateVertexInfo_(pendingPositions_.first(), startVertexItemId_);
    }
    domEndVertex->setAttribute(ds::position, pendingPositions_.last());
    domEdge->setAttribute(ds::positions, pendingPositions_);
    domEdge->setAttribute(ds::widths, pendingWidths_);
    if (canvas::experimental::saveInputSketchPoints()) {
        doSaveInputPoints(domEdge, inputPoints_);
    }
    workspace->sync();
}

namespace {

void autoFill(vacomplex::Node* keNode) {
    vacomplex::Cell* keCell = keNode->toCell();
    if (!keCell) {
        return;
    }
    vacomplex::KeyEdge* ke = keCell->toKeyEdge();
    if (!ke) {
        return;
    }
    // Note: test works if closed too.
    if (ke->startVertex() == ke->endVertex()) {
        vacomplex::KeyCycle cycle({vacomplex::KeyHalfedge(ke, true)});
        vacomplex::KeyFace* kf = vacomplex::ops::createKeyFace(
            std::move(cycle), ke->parentGroup(), ke, ke->time());
        const vacomplex::CellProperty* styleProp =
            ke->data().findProperty(workspace::strings::style);
        if (styleProp) {
            kf->data().insertProperty(styleProp->clone());
        }
    }
}

} // namespace

void Sketch::finishCurve_(ui::MouseEvent* event) {

    VGC_UNUSED(event);

    namespace ds = dom::strings;

    // Fast return if missing required context
    auto workspace = this->workspace().lock();
    if (!workspace) {
        return;
    }
    auto document = workspace->document().lock();
    if (!document) {
        return;
    }

    dom::Element* domEndVertex = document->elementFromInternalId(endVertexItemId_);
    dom::Element* domEdge = document->elementFromInternalId(edgeItemId_);
    if (!domEndVertex || !domEdge) {
        return;
    }

    // Clear sampling quality override to use default sampling
    if (vacomplex::KeyEdge* keyEdge = toKeyEdge(workspace.get(), edgeItemId_)) {
        keyEdge->data().clearSamplingQualityOverride();
    }

    // Compute end vertex snapping
    if (isSnappingEnabled() && startSnappedCleanInputPositions_.length() > 1
        && snapEndVertexItemId_ == 0) {

        // Compute end vertex to snap to
        geometry::Vec2d endPosition = startSnappedCleanInputPositions_.last();
        workspace::Element* snapEndVertexItem =
            computeSnapVertex_(endPosition, endVertexItemId_);

        // If found, do the snapping
        if (snapEndVertexItem) {

            // Compute end-snapped positions
            snapEndVertexItemId_ = snapEndVertexItem->id();
            updatePendingPositions_();

            // Update DOM and workspace
            domEndVertex->remove();
            domEndVertex = snapEndVertexItem->domElement();

            bool canClose = false;
            auto snapKvItem = dynamic_cast<workspace::VacKeyVertex*>(snapEndVertexItem);
            workspace::VacKeyEdge* keItem = toKeyEdgeItem(workspace.get(), edgeItemId_);
            if (snapKvItem && keItem) {
                vacomplex::KeyVertex* kv = snapKvItem->vacKeyVertexNode();
                vacomplex::KeyEdge* ke = keItem->vacKeyEdgeNode();
                vacomplex::CellRangeView kvStar = kv->star();
                canClose = (kvStar.length() == 1) && (*kvStar.begin() == ke);
            }
            if (canClose) {
                domEndVertex->remove();
                domEndVertex = nullptr;
                domEdge->clearAttribute(ds::startvertex);
                domEdge->clearAttribute(ds::endvertex);
                if (pendingPositions_.length() > 1) {
                    pendingPositions_.removeLast();
                    pendingWidths_.removeLast();
                    --numStablePendingWidths_;
                }
            }
            else {
                domEdge->setAttribute(ds::endvertex, domEndVertex->getPathFromId());
            }
            domEdge->setAttribute(ds::positions, std::move(pendingPositions_));
            domEdge->setAttribute(ds::widths, std::move(pendingWidths_));

            workspace->sync();

            if (keItem) {
                auto ke = keItem->vacNode();
                if (ke) {
                    vacomplex::ops::moveBelowBoundary(ke);
                    if (isAutoFillEnabled()) {
                        autoFill(ke);
                    }
                }
            }
        }
    }

    resetData_();
    requestRepaint();
}

void Sketch::resetData_() {
    if (drawCurveUndoGroup_) {
        drawCurveUndoGroup_->undone().disconnect(drawCurveUndoGroupConnectionHandle_);
        drawCurveUndoGroup_->close();
        drawCurveUndoGroup_ = nullptr;
    }

    // inputs are kept until next curve starts
    // for debugging purposes.
    //inputPoints_.clear();

    // sketch passes
    pipeline_.reset();

    // pending clean input
    cleanInputStartIndex_ = 0;
    cleanInputStartPointOverride_ = std::nullopt;

    // snapping
    snapStartPosition_ = std::nullopt;
    startSnappedCleanInputPositions_.clear();
    numStableStartSnappedCleanInputPositions_ = 0;
    snapEndVertexItemId_ = 0;

    // pending edge
    startVertexItemId_ = 0;
    endVertexItemId_ = 0;
    edgeItemId_ = 0;
    pendingPositions_.clear();
    pendingWidths_.clear();
    numStablePendingWidths_ = 0;

    // snap/cut cache
    vertexInfos_.clear();
    edgeInfos_.clear();
}

} // namespace vgc::tools
