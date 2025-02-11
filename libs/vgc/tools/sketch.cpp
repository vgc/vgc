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
#include <vgc/vacomplex/operations.h>
#include <vgc/workspace/edge.h>
#include <vgc/workspace/strings.h>

namespace vgc::tools {

namespace {

namespace options_ {

ui::NumberSetting* penWidth() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(), "tools.sketch.penWidth", "Pen Width", 5, 0, 1000);
    return setting.get();
}

ui::NumberSetting* lineSmoothing() {
    static ui::NumberSettingPtr setting = createIntegerNumberSetting(
        ui::settings::session(),
        "tools.sketch.experimental.lineSmoothing",
        "Line Smoothing",
        2,
        0,
        1000);
    return setting.get();
}

ui::NumberSetting* widthSmoothing() {
    static ui::NumberSettingPtr setting = createIntegerNumberSetting(
        ui::settings::session(),
        "tools.sketch.widthSmoothing",
        "Width Smoothing",
        10,
        0,
        1000);
    return setting.get();
}

ui::BoolSetting* snapping() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(), "tools.sketch.snapping", "Snapping", true);
    return setting.get();
}

ui::BoolSetting* snapVertices() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(), "tools.sketch.snapVertices", "Snap Vertices", true);
    return setting.get();
}

ui::BoolSetting* snapEdges() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(), "tools.sketch.snapEdges", "Snap Edges", true);
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

ui::BoolSetting* autoIntersect() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(), "tools.sketch.autoIntersect", "Auto-Intersect", false);
    return setting.get();
}

ui::BoolSetting* autoFill() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(), "tools.sketch.autoFill", "Auto-Fill", false);
    return setting.get();
}

ui::NumberSetting* duplicateThreshold() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(),
        "tools.sketch.experimental.duplicateThreshold",
        "Duplicate Threshold",
        1.5,  // default
        0,    // min
        1000, // max
        10,   // numDecimals
        0.1); // step
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

ui::NumberSetting* samplingLength() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(),
        "tools.sketch.experimental.samplingLength",
        "Sampling Length",
        3.0,  // default
        0.1,  // min
        1000, // max
        10,   // numDecimals
        0.1); // step
    return setting.get();
}

ui::NumberSetting* douglasPeuckerOffset() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(),
        "tools.sketch.experimental.douglasPeuckerOffset",
        "Douglas-Peucker Offset",
        0.8,   // default
        -1000, // min
        1000,  // max
        10,    // numDecimals
        0.1);  // step
    return setting.get();
}

ui::NumberSetting* widthSlopeLimit() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(),
        "tools.sketch.experimental.widthSlopeLimit",
        "Width Slope Limit",
        0.8,  // default
        0,    // min
        1000, // max
        10,   // numDecimals
        0.1); // step
    return setting.get();
}

ui::BoolSetting* improveEndWidths() {
    static ui::BoolSettingPtr setting = ui::BoolSetting::create(
        ui::settings::session(),
        "tools.sketch.experimental.improveEndWidths",
        "Improve End Widths",
        true);
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

bool isAutoIntersectEnabled() {
    return options_::autoIntersect()->value();
}

bool isAutoFillEnabled() {
    return options_::autoFill()->value();
}

bool isSnapVerticesEnabled() {
    return options_::snapVertices()->value();
}

bool isSnapEdgesEnabled() {
    return options_::snapEdges()->value();
}

SketchPreprocessing defaultPreprocessing = SketchPreprocessing::QuadraticBlend;

} // namespace

VGC_DEFINE_ENUM( //
    SketchPreprocessing,
    (Default, "Default (Quadratic Blend)"),
    (NoPreprocessing, "No Preprocessing"),
    (DouglasPeucker, "Douglas-Peucker"),
    (SingleLineSegmentWithFixedEndpoints, "Single Line Segment (Fixed Endpoints)"),
    (SingleLineSegmentWithFreeEndpoints, "Single Line Segment (Free Endpoints)"),
    (SingleQuadraticSegmentWithFixedEndpoints,
     "Single Quadratic Segment (Fixed Endpoints)"),
    (QuadraticSpline, "Quadratic Spline"),
    (QuadraticBlend, "Quadratic Blend"))

namespace {

void addPreprocessingSetting(
    canvas::ExperimentalModule& module,
    core::Array<ui::WidgetWeakPtr>& settings,
    ui::NumberSetting* setting) {

    auto edit = ui::NumberSettingEdit::create(setting);
    settings.append(edit);
    module.addWidget(*edit);
}

} // namespace

SketchModule::SketchModule(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    using namespace options_;
    using ui::BoolSettingEdit;
    using ui::EnumSettingEdit;
    using ui::NumberSettingEdit;

    if (auto module = context.importModule<canvas::ExperimentalModule>().lock()) {

        module->addWidget(*NumberSettingEdit::create(duplicateThreshold()));
        module->addWidget(*EnumSettingEdit::create(sketchPreprocessing()));

        // Douglas Peucker settings
        {
            auto& settings = preprocessingSettings_[SketchPreprocessing::DouglasPeucker];
            addPreprocessingSetting(*module, settings, douglasPeuckerOffset());
        }

        // Quadratic Blend settings
        {
            auto& settings = preprocessingSettings_[SketchPreprocessing::QuadraticBlend];
            addPreprocessingSetting(*module, settings, samplingLength());
        }

        module->addWidget(*NumberSettingEdit::create(lineSmoothing()));
        module->addWidget(*NumberSettingEdit::create(widthSlopeLimit()));
        module->addWidget(*BoolSettingEdit::create(improveEndWidths()));
        module->addWidget(*BoolSettingEdit::create(reProcessExistingEdges()));

        // Show/hide conditionnal widgets
        onPreprocessingChanged_();
    }

    duplicateThreshold()->valueChanged().connect(onProcessingChanged_Slot());
    sketchPreprocessing()->valueChanged().connect(onProcessingChanged_Slot());
    sketchPreprocessing()->valueChanged().connect(onPreprocessingChanged_Slot());
    douglasPeuckerOffset()->valueChanged().connect(onProcessingChanged_Slot());
    samplingLength()->valueChanged().connect(onProcessingChanged_Slot());
    lineSmoothing()->valueChanged().connect(onProcessingChanged_Slot());
    widthSmoothing()->valueChanged().connect(onProcessingChanged_Slot());
    widthSlopeLimit()->valueChanged().connect(onProcessingChanged_Slot());
    improveEndWidths()->valueChanged().connect(onProcessingChanged_Slot());
}

SketchModulePtr SketchModule::create(const ui::ModuleContext& context) {
    return core::createObject<SketchModule>(context);
}

namespace {

template<typename TSketchPassClass>
TSketchPassClass& replaceOrAdd(SketchPipeline& pipeline, Int i) {
    if (i < pipeline.numPasses()) {
        if (!pipeline.isPass<TSketchPassClass>(i)) {
            return pipeline.replacePass<TSketchPassClass>(i);
        }
        else {
            return static_cast<TSketchPassClass&>(pipeline[i]);
        }
    }
    else {
        return pipeline.addPass<TSketchPassClass>();
    }
}

} // namespace

void SketchModule::setupPipeline(SketchPipeline& pipeline) {

    // Ensures that changing settings of sketch passes is allowed
    pipeline.reset();

    // Convenient index to track which sketch pass we are setting up
    Int i = 0;

    // Remove duplicates
    {
        RemoveDuplicatesPass& pass = replaceOrAdd<RemoveDuplicatesPass>(pipeline, i++);
        RemoveDuplicatesSettings settings;
        settings.setDistanceThreshold(options_::duplicateThreshold()->value());
        pass.setSettings(settings);
    }

    // Preprocessing
    switch (preprocessing()) {
    case SketchPreprocessing::Default:
        // Cannot happen, see implementation of preprocessing()
        break;
    case SketchPreprocessing::NoPreprocessing:
        // We add an empty pass rather than not adding a pass
        // to keep the memory cache of following passes
        replaceOrAdd<EmptyPass>(pipeline, i++);
        break;
    case SketchPreprocessing::DouglasPeucker: {
        DouglasPeuckerPass& pass = replaceOrAdd<DouglasPeuckerPass>(pipeline, i++);
        DouglasPeuckerSettings settings;
        settings.setOffset(options_::douglasPeuckerOffset()->value());
        pass.setSettings(settings);
        break;
    }
    case SketchPreprocessing::SingleLineSegmentWithFixedEndpoints:
        replaceOrAdd<SingleLineSegmentWithFixedEndpointsPass>(pipeline, i++);
        break;
    case SketchPreprocessing::SingleLineSegmentWithFreeEndpoints:
        replaceOrAdd<SingleLineSegmentWithFreeEndpointsPass>(pipeline, i++);
        break;
    case SketchPreprocessing::SingleQuadraticSegmentWithFixedEndpoints:
        replaceOrAdd<SingleQuadraticSegmentWithFixedEndpointsPass>(pipeline, i++);
        break;
    case SketchPreprocessing::QuadraticSpline:
        replaceOrAdd<QuadraticSplinePass>(pipeline, i++);
        break;
    case SketchPreprocessing::QuadraticBlend: {
        QuadraticBlendPass& pass = replaceOrAdd<QuadraticBlendPass>(pipeline, i++);
        experimental::BlendFitSettings settings;
        settings.ds = options_::samplingLength()->value();
        pass.setSettings(settings);
        break;
    }
    }

    // Smoothing
    {
        SmoothingPass& pass = replaceOrAdd<SmoothingPass>(pipeline, i++);
        SmoothingSettings settings;
        settings.setLineSmoothing(options_::lineSmoothing()->intValue());
        settings.setWidthSmoothing(options_::widthSmoothing()->intValue());
        settings.setWidthSlopeLimit(options_::widthSlopeLimit()->value());
        settings.setImproveEndWidths(options_::improveEndWidths()->value());
        pass.setSettings(settings);
    }

    // Transform from Widget to Scene coordinates
    replaceOrAdd<TransformPass>(pipeline, i++);

    // Remove any remaining pass
    pipeline.removePassesFrom(i);
}

SketchPreprocessing SketchModule::preprocessing() const {
    auto res = options_::sketchPreprocessing()->value().get<SketchPreprocessing>();
    if (res == SketchPreprocessing::Default) {
        return defaultPreprocessing;
    }
    else {
        return res;
    }
}

void SketchModule::onPreprocessingChanged_() {
    SketchPreprocessing preprocessing = this->preprocessing();
    for (const auto& [key, widgetWeakPtrArray] : preprocessingSettings_) {
        if (key == preprocessing) {
            for (const auto& widget_ : widgetWeakPtrArray) {
                if (auto widget = widget_.lock()) {
                    widget->show();
                }
            }
        }
        else {
            for (const auto& widget_ : widgetWeakPtrArray) {
                if (auto widget = widget_.lock()) {
                    widget->hide();
                }
            }
        }
    }
}

void SketchModule::onProcessingChanged_() {

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
    setupPipeline(pipeline_);

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
            pipeline_.reset();
            pipeline_.setTransformMatrix(transform);
            pipeline_.updateFrom(inputPoints);

            // Save result to DOM
            updateEdgeGeometry(pipeline_.output(), item);
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

namespace {

void setVisibility(const ui::WidgetWeakPtr& w, ui::Visibility visibility) {
    if (auto wl = w.lock()) {
        wl->setVisibility(visibility);
    }
}

struct SnappingSubWidgets {
    ui::WidgetWeakPtr snapVertices;
    ui::WidgetWeakPtr snapEdges;
    ui::WidgetWeakPtr snapDistance;
    ui::WidgetWeakPtr snapFalloff;

    void setVisibility(bool isVisible) const {
        ui::Visibility visibility =
            isVisible ? ui::Visibility::Inherit : ui::Visibility::Invisible;
        tools::setVisibility(snapVertices, visibility);
        tools::setVisibility(snapEdges, visibility);
        tools::setVisibility(snapDistance, visibility);
        tools::setVisibility(snapFalloff, visibility);
    }
};

} // namespace

ui::WidgetPtr Sketch::doCreateOptionsWidget() const {
    ui::WidgetPtr res = ui::Column::create();
    res->createChild<ui::NumberSettingEdit>(options_::penWidth());
    res->createChild<ui::NumberSettingEdit>(options_::widthSmoothing());

    res->createChild<ui::BoolSettingEdit>(options_::snapping());
    SnappingSubWidgets ssw;
    ssw.snapVertices = res->createChild<ui::BoolSettingEdit>(options_::snapVertices());
    ssw.snapEdges = res->createChild<ui::BoolSettingEdit>(options_::snapEdges());
    ssw.snapDistance = res->createChild<ui::NumberSettingEdit>(options_::snapDistance());
    ssw.snapFalloff = res->createChild<ui::NumberSettingEdit>(options_::snapFalloff());
    ssw.setVisibility(options_::snapping()->value());
    options_::snapping()->valueChanged().connect(
        [ssw](bool value) { ssw.setVisibility(value); });

    res->createChild<ui::BoolSettingEdit>(options_::autoIntersect());
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

// Note: in the future we may want to have the snapping candidates implemented
// in canvas to be shared by all tools.

namespace {

// Retuns whether there is a selectable item at the given position that is
// above the given position.
//
// Note that this takes the current display mode into account. For example, if
// `itemId` is an edge and if there is a face above the edge at the given
// position, then:
//
// - In "Normal" display mode: the edge is considered occluded.
//
// - In "Outline Overlay" or "Outline Only" mode: the edge is not considered
//   occluded, since users can see the edge through the face, and therefore it
//   is expected that users may want to snap to that edge.
//
// Due to numerical errors, it's important to use a non-zero tolerance so
// that in "Outline Overlay" mode, the outline of `itemID` can be in the
// list of occluders, above faces.
//
// However, we cannot use a tolerance too large, otherwise a vertex near a face
// (but not occluded by it) might be considered occluded.
//
bool computeIsOccludedAt(
    const canvas::Canvas& canvas,
    core::Id itemId,
    const geometry::Vec2d& position,
    double tolerance) {

    auto workspace = canvas.workspace().lock();
    if (!workspace) {
        return false;
    }

    core::Array<canvas::SelectionCandidate> occluders =
        canvas.computeSelectionCandidatesAboveOrAt(
            itemId, position, tolerance, canvas::CoordinateSpace::Workspace);

    for (const canvas::SelectionCandidate& occluder : occluders) {
        if (occluder.id() == itemId) {
            return false;
        }
        if (workspace::Element* occluderItem = workspace->find(occluder.id())) {
            if (workspace::VacElement* occluderVacItem = occluderItem->toVacElement()) {
                if (vacomplex::Cell* occluderCell = occluderVacItem->vacCell()) {
                    if (occluderCell->spatialType() == vacomplex::CellSpatialType::Face) {
                        // face are occluders
                        return true;
                    }
                }
            }
            else {
                // not a vac element, let's consider it prevents snapping.
                return true;
            }
        }
    }
    return false;
}

} // namespace

// Compute which vertex or edge to snap to.
//
// If snapping to an edge, this performs a topological cut and returns the new
// vertex as well as the new edges and the ID of the old cut edge.
//
// If `vertexItemId` is non-zero, then we ignore snapping to the given vertex.
//
// If `edgeItemId` is non-zero, then it is assumed that we are snapping the end
// position of the given edge (and that `vertexItemId` is the end vertex), and
// therefore we prevent snapping not only to the end vertex, but also to the
// "tip" of the edge within the snap tolerance.
//
Sketch::SnapVertexResult Sketch::computeSnapVertex_(
    const geometry::Vec2d& position,
    core::Id vertexItemId,
    core::Id edgeItemId) {

    SnapVertexResult res;

    auto context = contextLock();
    if (!context) {
        return res;
    }
    auto workspace = context.workspace();
    auto canvas = context.canvas();

    // For simplicy, we recompute the cache each time for now.
    // In the future, we want to optimize this by keeping the cache
    // up to date progressively instead of recomputing it from scratch.
    initCellInfoArrays_();

    float snapDistanceFloat = static_cast<float>(options_::snapDistance()->value());
    style::Length snapDistanceLength(snapDistanceFloat, style::LengthUnit::Dp);

    double zoom = canvas ? canvas->camera().zoom() : 1.0;
    double snapDistance = snapDistanceLength.toPx(styleMetrics()) / zoom;
    double tolerance = snapDistance * 0.01;

    // Define data structure to store candidate vertices/edges for snapping
    struct SnapCandidate {
        Int infoIdx;
        Int dimension;
        double dist;
        geometry::SampledCurveProjection proj;

        bool operator<(const SnapCandidate& rhs) const {
            return std::pair(dimension, dist) < std::pair(rhs.dimension, rhs.dist);
        }
    };
    core::Array<SnapCandidate> candidates;

    // Find all candidate vertices
    if (isSnapVerticesEnabled()) {
        for (Int i = 0; i < vertexInfos_.length(); ++i) {
            const VertexInfo& info = vertexInfos_[i];
            if (info.itemId == vertexItemId) {
                continue;
            }
            double d = (info.position - position).length();
            if (d < snapDistance) {
                SnapCandidate& candidate = candidates.emplaceLast();
                candidate.infoIdx = i;
                candidate.dimension = 0;
                candidate.dist = d;
            }
        }
    }

    // Find all candidate edges
    //
    if (isSnapEdgesEnabled()) {
        for (Int i = 0; i < edgeInfos_.length(); ++i) {
            const EdgeInfo& info = edgeInfos_[i];
            geometry::StrokeSample2dConstSpan samples = info.sampling->samples();
            if (info.itemId == edgeItemId) {
                // When snapping the end vertex of a stroke, the closest point
                // to the stroke is always the end position itself.
                //
                // Therefore, we need to exclude the tip of the stroke from
                // candidate projections. We do this by removing all end
                // samples within a given radius of the end position.
                //
                double tipRadius = snapDistance * 1.2;
                Int j = samples.length() - 1; // index of last non-tip sample
                while (j >= 0
                       && (samples[j].position() - position).length() < tipRadius) {
                    --j;
                }
                Int numNonTipSamples = j + 1;
                samples = samples.subspan(0, numNonTipSamples);
            }
            if (samples.isEmpty()) {
                continue;
            }
            geometry::SampledCurveProjection proj =
                projectToCenterline(samples, position);
            double d = (proj.position() - position).length();
            if (d < snapDistance) {
                SnapCandidate& candidate = candidates.emplaceLast();
                candidate.infoIdx = i;
                candidate.dimension = 1;
                candidate.dist = d;
                candidate.proj = proj;
            }
        }
    }

    // Choose the best candidate vertex or edge:
    // 1. It should not be occluded at the projected position
    // 2. We prioritize vertices rather than edges
    // 3. Otherwise we prioritize the closest candidate
    std::sort(candidates.begin(), candidates.end());
    const SnapCandidate* bestCandidate = nullptr;
    vacomplex::KeyEdge* bestEdge = nullptr;
    core::Id bestEdgeItemId = 0;
    for (const SnapCandidate& candidate : candidates) {
        if (candidate.dimension == 0) {
            VertexInfo& info = vertexInfos_[candidate.infoIdx];
            auto vertex = workspace->findCellByItemId<vacomplex::KeyVertex>(info.itemId);
            if (!vertex) {
                continue;
            }
            bool isOccluded =
                computeIsOccludedAt(*canvas, info.itemId, info.position, tolerance);
            if (!isOccluded) {
                bestCandidate = &candidate;
                res.vertex = vertex;
                break;
            }
        }
        else if (candidate.dimension == 1) {
            EdgeInfo& info = edgeInfos_[candidate.infoIdx];
            auto edge = workspace->findCellByItemId<vacomplex::KeyEdge>(info.itemId);
            if (!edge) {
                continue;
            }
            bool isOccluded = computeIsOccludedAt(
                *canvas, info.itemId, candidate.proj.position(), tolerance);
            if (!isOccluded) {
                bestCandidate = &candidate;
                bestEdge = edge;
                bestEdgeItemId = info.itemId;
                break;
            }
        }
    }

    // If the best candidate is an edge, cut it.
    if (bestEdge) {
        geometry::SampledCurveParameter sParam = bestCandidate->proj.parameter();
        geometry::CurveParameter param = bestEdge->stroke()->resolveParameter(sParam);
        auto result = vacomplex::ops::cutEdge(bestEdge, param);
        res.vertex = result.vertex();
        res.cutEdgeItemId = bestEdgeItemId;
        res.newEdges = result.edges();

        // TODO: cutting removes and creates new edges, so we need to update
        // the cached geometry.
    }

    return res;
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
    updateStartSnappedCleanInputPositions_();
    pendingPositions_.assign(startSnappedCleanInputPositions_);
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

// Sketch::VertexInfo* Sketch::searchVertexInfo_(core::Id itemId) {
//     for (Sketch::VertexInfo& vi : vertexInfos_) {
//         if (vi.itemId == itemId) {
//             return &vi;
//         }
//     }
//     return nullptr;
// }

// void Sketch::appendVertexInfo_(const geometry::Vec2d& position, core::Id itemId) {
//     VertexInfo& vi = vertexInfos_.emplaceLast();
//     vi.position = position;
//     vi.itemId = itemId;
// }

// void Sketch::updateVertexInfo_(const geometry::Vec2d& position, core::Id itemId) {
//     // Reverse iteration because currently, the updated itemId is always last
//     for (auto it = vertexInfos_.rbegin(); it != vertexInfos_.rend(); ++it) {
//         if (it->itemId == itemId) {
//             it->position = position;
//             break;
//         }
//     }
// }

// Sketch::EdgeInfo* Sketch::searchEdgeInfo_(core::Id itemId) {
//     for (Sketch::EdgeInfo& ei : edgeInfos_) {
//         if (ei.itemId == itemId) {
//             return &ei;
//         }
//     }
//     return nullptr;
// }

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

    // Cache geometry of existing vertices/edges.
    // TODO: Improve cache management. See computeSnapVertex_().
    //initCellInfoArrays_();

    // Snapping: Compute start vertex to snap to
    vacomplex::KeyVertex* snapVertex = nullptr;
    geometry::Vec2d startPosition = pipeline_.transformAffine(eventPos2d);
    if (isSnappingEnabled()) {
        snapVertex = computeSnapVertex_(startPosition).vertex;
        if (snapVertex) {
            startPosition = snapVertex->position();
            snapStartPosition_ = startPosition;
        }
    }

    // Get or create start vertex
    //
    // XXX What to do if snapVertex is non-null, but if there is no DOM element
    // corresponding to this workspace element, e.g., due to composite shapes?
    //
    namespace ds = dom::strings;
    dom::Element* domStartVertex = nullptr;
    if (snapVertex) {
        domStartVertex = workspace->findDomElement(snapVertex);
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
    if (auto module = sketchModule_.lock()) {
        module->setupPipeline(pipeline_);
    }

    // Append start point to geometry
    continueCurve_(event);
    workspace->sync(); // required for toKeyEdge() below

    // Append new start vertex (if any) to snap/cut info
    //if (!snapStartPosition_) {
    //    appendVertexInfo_(startPosition, startVertexItemId_);
    //}

    if (auto edge = workspace->findCellByItemId<vacomplex::KeyEdge>(edgeItemId_)) {

        // Use low sampling quality override to minimize lag, unless current quality
        // is already even lower.
        //
        if (auto complex = workspace->vac().lock()) {
            geometry::CurveSamplingQuality quality = complex->samplingQuality();
            bool isAdaptive = geometry::isAdaptiveSampling(quality);
            Int8 level = geometry::getSamplingQualityLevel(quality);
            Int8 newLevel = std::min<Int8>(level, 2); // 2 = Low
            quality = geometry::getSamplingQuality(newLevel, isAdaptive);
            edge->data().setSamplingQualityOverride(quality);
        }

        // Move edge to proper depth location.
        vacomplex::ops::moveBelowBoundary(edge);
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
    // XXX: it might be interesting to also record the current time (now) as
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

    // Update DOM and workspace
    //
    namespace ds = dom::strings;
    if (!snapStartPosition_) {
        // Unless start-snapped, processing passes may have modified the start point
        domStartVertex->setAttribute(ds::position, pendingPositions_.first());
        //updateVertexInfo_(pendingPositions_.first(), startVertexItemId_);
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

void autoFill(vacomplex::KeyEdge* ke) {
    if (!ke) {
        return;
    }
    if (ke->startVertex() == ke->endVertex()) { // closed or pseudo-closed
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

    auto cleanup = [this]() {
        resetData_();
        requestRepaint();
    };

    // Fast return if missing required context
    auto workspace = this->workspace().lock();
    if (!workspace) {
        cleanup();
        return;
    }
    auto document = workspace->document().lock();
    if (!document) {
        cleanup();
        return;
    }

    // Get the sequence of edges corresponding to the sketched stroke.
    //
    // At this point in the code, there is only one edge, but after end vertex
    // snapping (see below) there may be multiple edges.
    //
    core::Array<vacomplex::KeyEdge*> edges;
    if (auto edge = workspace->findCellByItemId<vacomplex::KeyEdge>(edgeItemId_)) {
        edges.append(edge);
    }
    else {
        cleanup();
        return;
    }

    // Compute end vertex snapping.
    //
    if (isSnappingEnabled() && startSnappedCleanInputPositions_.length() > 1) {

        // Compute which vertex to snap the end vertex to, if any
        geometry::Vec2d endPosition = startSnappedCleanInputPositions_.last();
        SnapVertexResult sr =
            computeSnapVertex_(endPosition, endVertexItemId_, edgeItemId_);
        vacomplex::KeyVertex* snapVertex = sr.vertex;

        if (sr.cutEdgeItemId == edgeItemId_) {
            // Handle self-snapping.
            edgeItemId_ = 0;
            edges = sr.newEdges;
        }
        VGC_ASSERT(!edges.isEmpty());

        // If found, do the snapping
        if (snapVertex) {
            // Get position to snap to
            vacomplex::KeyEdge* edge = edges.last();
            vacomplex::KeyVertex* endVertex = edge->endVertex();
            geometry::Vec2d snapPosition = snapVertex->position();

            // Set snap settings
            auto oldSettings = edge->complex()->snapSettings();
            auto settings = geometry::CurveSnapSettings::falloff(snapFalloff_());
            edge->complex()->setSnapSettings(settings);

            // Modify vertex position and apply snapping now (otherwise, it could
            // be deferred in case we are within an operation group)
            vacomplex::ops::setKeyVertexPosition(endVertex, snapPosition);
            edge->snapGeometry();

            // Restore snap settings
            edge->complex()->setSnapSettings(oldSettings);

            // Glue current end vertex to snap vertex.
            // TODO: avoid creating a new vertex (e.g., `glueKeyVerticesInto(kvs, snapVertex)`
            std::array kvs = {endVertex, snapVertex};
            vacomplex::ops::glueKeyVertices(kvs, snapPosition);

            // Note: snapVertex and the old edges.last()->endVertex() are now invalid
            // Create a close edge if possible
            vacomplex::KeyVertex* kv = edges.last()->endVertex();
            vacomplex::CellRangeView kvStar = kv->star();
            if (kvStar.length() == 1) {
                bool smoothJoin = true;
                if (vacomplex::Cell* cell =
                        vacomplex::ops::uncutAtKeyVertex(kv, smoothJoin)) {
                    edges.last() = cell->toKeyEdgeUnchecked();
                }
            }

            // Ensure edge is below its end-snap vertex
            vacomplex::ops::moveBelowBoundary(edges.last());

            // Auto-fill
            if (isAutoFillEnabled()) {
                autoFill(edges.last());
            }
        }
    }

    // Clear sampling quality override to use default sampling
    for (auto edge : edges) {
        edge->data().clearSamplingQualityOverride();
    }

    if (isAutoIntersectEnabled()) {
        vacomplex::ops::intersectWithGroup(edges);
    }

    cleanup();
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
