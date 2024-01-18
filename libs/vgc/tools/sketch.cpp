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

#include <array>

#include <QBitmap>
#include <QCursor>
#include <QPainter>

#include <vgc/core/profile.h>
#include <vgc/core/stringid.h>
#include <vgc/geometry/curve.h>
#include <vgc/graphics/strings.h>
#include <vgc/tools/logcategories.h>
#include <vgc/ui/boolsettingedit.h>
#include <vgc/ui/column.h>
#include <vgc/ui/cursor.h>
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

} // namespace options_

namespace {

bool isAutoFillEnabled() {
    return options_::autoFill()->value();
}

} // namespace

void SketchPointsProcessingPass::updateCumulativeChordalDistances() {
    Int n = numPoints();
    if (n == 0) {
        return;
    }

    Int start = std::max<Int>(numStablePoints(), 1);
    const SketchPoint* p1 = &getPoint(start - 1);
    double s = p1->s();
    for (Int i = start; i < n; ++i) {
        SketchPoint& p2 = getPointRef(i);
        s += (p2.position() - p1->position()).length();
        p2.setS(s);
        p1 = &p2;
    }
}

namespace detail {

Int EmptyPass::update_(const SketchPointBuffer& input, Int /*lastNumStableInputPoints*/) {

    const SketchPointArray& inputPoints = input.data();
    Int numStablePoints = this->numStablePoints();
    resizePoints(numStablePoints);
    extendPoints(inputPoints.begin() + numStablePoints, inputPoints.end());
    return numStablePoints;
}

void EmptyPass::reset_() {
    // no custom state
}

namespace {

// Returns the binomial coefficient C(n, k) for 0 <= k <= n.
//
// The returned array is of size n + 1.
//
// These are computed using the Pascal's triangle:
//
//  C(0, k) =       1
//  C(1, k) =      1 1
//  C(2, k) =     1 2 1
//  C(3, k) =    1 3 3 1
//  C(4, k) =   1 4 6 4 1
//
template<size_t n>
constexpr std::array<Int, n + 1> binomialCoefficients() {
    std::array<Int, n + 1> res{}; // Value-initialization required for constexprness.
    res[0] = 1;
    for (size_t m = 1; m <= n; ++m) {
        // Compute C(m, k) coefficients from C(m-1, k) coefficients
        res[m] = 1;
        for (size_t k = m - 1; k >= 1; --k) {
            res[k] = res[k] + res[k - 1];
        }
    }
    return res;
}

bool clampMin_(double k, SketchPoint& p, const SketchPoint& minLimitor, double ds) {
    double minWidth = minLimitor.width() - k * ds;
    if (p.width() < minWidth) {
        p.setWidth(minWidth);
        return true;
    }
    return false;
}

void clampMax_(double k, SketchPoint& p, const SketchPoint& maxLimitor, double ds) {
    double maxWidth = maxLimitor.width() + k * ds;
    if (p.width() > maxWidth) {
        p.setWidth(maxWidth);
    }
}

// Clamps a point p based on a minLimitor before p.
// Returns whether p was widened according to minLimitor.
//
bool clampMinForward(double k, SketchPoint& p, const SketchPoint& minLimitor) {
    double dsMinLimitor = p.s() - minLimitor.s();
    return clampMin_(k, p, minLimitor, dsMinLimitor);
}

// Clamps a point p based on a minLimitor after p.
// Returns whether p was widened according to minLimitor.
//
bool clampMinBackward(double k, SketchPoint& p, const SketchPoint& minLimitor) {
    double dsMinLimitor = minLimitor.s() - p.s();
    return clampMin_(k, p, minLimitor, dsMinLimitor);
}

// Clamps a point p based on a maxLimitor before p.
//
void clampMaxForward(double k, SketchPoint& p, const SketchPoint& maxLimitor) {
    double dsMaxLimitor = p.s() - maxLimitor.s();
    clampMax_(k, p, maxLimitor, dsMaxLimitor);
}

// Clamps a point p based on a minLimitor and maxLimitor before p.
// Returns whether p was widened according to minLimitor.
//
bool clampMinMaxForward(
    double k,
    SketchPoint& p,
    const SketchPoint& minLimitor,
    const SketchPoint& maxLimitor) {

    if (clampMinForward(k, p, minLimitor)) {
        return true;
    }
    else {
        clampMaxForward(k, p, maxLimitor);
        return false;
    }
}

// Ensures that |dw/ds| <= k (e.g., k = 0.5).
//
// Importantly, we should have at least |dw/ds| <= 1, otherwise in the current
// tesselation model with round caps, it causes the following ugly artifact:
//
// Mouse move #123:   (pos = (100, 0), width = 3)
//
// -------_
//         =
//      +   |
//         =
// -------'
//
// Mouse move #124:   (pos = (101, 0), width = 1)
//
// -------   <- Ugly temporal disontinuity (previously existing geometry disappears)
//        .  <- Ugly geometric discontinuity (prone to cusps, etc.)
//       + |
//        '
// -------
//
// The idea is that what users see should be as close as possible as the
// "integral of disks" interpretation of a brush stroke. With a physical round
// paint brush, if you push the brush more then it creates a bigger disk. If
// you then pull a little without moving lateraly, then it doesn't remove what
// was previously already painted.
//
// Algorithm pseudo-code when applied to a global list of points:
//
// 1. Sort samples by width in a list
// 2. While the list isn't empty:
//    a. Pop sample with largest width
//    b. Modify the width of its two siblings to enforce |dw/ds| <= k
//    c. Update the location of the two siblings in the sorted list to keep it sorted
//
// Unfortunately, the above algorithm have global effect: adding one point with
// very large width might increase the width of all previous points. This is
// undesirable for performance and user-predictibility, as we want to keep the
// "unstable points" part of the sketched curve as small as possible.
// Therefore, in the implementation below, we only allow for a given point to
// affect `windowSize` points before itself.
//
void applyWidthRoughnessLimitor(
    double k,
    Int windowSize,
    const SketchPoint* lastStablePoint, // nullptr if no stable points
    core::Span<SketchPoint> unstablePoints) {

    if (unstablePoints.isEmpty()) {
        return;
    }

    // Apply width-limitor to first unstable point.
    //
    const SketchPoint* minLimitor = lastStablePoint;
    const SketchPoint* maxLimitor = lastStablePoint;
    if (lastStablePoint) {
        clampMinMaxForward(k, unstablePoints[0], *minLimitor, *maxLimitor);
    }

    // Apply width-limitor to subsequent unstable points.
    //
    for (Int i = 1; i < unstablePoints.length(); ++i) {

        //                   window size = 3    (each point influences up to
        //                <------------------|    3 points before itself)
        //
        //          p[windowStart]   p[i-1] p[i]
        // x-----x--------x--------x----x----x
        //   maxLimitor
        //
        Int windowStart = i - windowSize;
        if (windowStart > 0) {
            maxLimitor = &unstablePoints[windowStart - 1];
        }
        else {
            windowStart = 0;
            // maxLimitor == lastStablePoint
        }
        SketchPoint& p = unstablePoints[i];

        // Widen current point p[i] based on p[windowStart - 1]
        // Shorten current point p[i] based on p[i - 1]
        minLimitor = &unstablePoints[i - 1];
        bool widened = maxLimitor ? clampMinMaxForward(k, p, *minLimitor, *maxLimitor)
                                  : clampMinForward(k, p, *minLimitor);

        // Widen previous points within window if necessary.
        // Note: whenever a point is not itself widened, we know that previous
        // points will not be widened either, so we can skip computation.
        if (!widened) {
            for (Int j = i - 1; j >= windowStart; --j) {
                if (!clampMinBackward(k, unstablePoints[j], p)) {
                    break;
                }
            }
        }
    }
}

} // namespace

Int SmoothingPass::update_(
    const SketchPointBuffer& input,
    Int /*lastNumStableInputPoints*/) {

    const SketchPointArray& inputPoints = input.data();
    Int numPoints = inputPoints.length();
    Int numStablePoints = this->numStablePoints();
    if (numPoints == numStablePoints) {
        return numPoints;
    }

    // Keep our stable points, fill the rest with the input points.
    Int unstableIndexStart = numStablePoints;
    resizePoints(numStablePoints);
    extendPoints(inputPoints.begin() + unstableIndexStart, inputPoints.end());

    Int instabilityDelta = 0;

    Int pointsSmoothingLevel = 2;
    if (pointsSmoothingLevel > 0 && numPoints >= 3) {
        // Apply gaussian smoothing.
        Int iStart = unstableIndexStart;
        if (pointsSmoothingLevel == 1) {
            iStart = std::max<Int>(1, iStart);
            for (Int i = iStart; i < numPoints - 1; ++i) {
                getPointRef(i).setPosition(                                  //
                    (1 / 4.0) * inputPoints.getUnchecked(i - 1).position() + //
                    (2 / 4.0) * inputPoints.getUnchecked(i + 0).position() + //
                    (1 / 4.0) * inputPoints.getUnchecked(i + 1).position());
            }
        }
        else if (pointsSmoothingLevel == 2) {
            if (iStart <= 1) {
                getPointRef(1).setPosition(                              //
                    (1 / 4.0) * inputPoints.getUnchecked(0).position() + //
                    (2 / 4.0) * inputPoints.getUnchecked(1).position() + //
                    (1 / 4.0) * inputPoints.getUnchecked(2).position());
                iStart = 2;
            }
            for (Int i = iStart; i < numPoints - 2; ++i) {
                getPointRef(i).setPosition(                                   //
                    (1 / 16.0) * inputPoints.getUnchecked(i - 2).position() + //
                    (4 / 16.0) * inputPoints.getUnchecked(i - 1).position() + //
                    (6 / 16.0) * inputPoints.getUnchecked(i + 0).position() + //
                    (4 / 16.0) * inputPoints.getUnchecked(i + 1).position() + //
                    (1 / 16.0) * inputPoints.getUnchecked(i + 2).position());
            }
            if (numPoints - 2 >= iStart) {
                Int i = numPoints - 2;
                getPointRef(i).setPosition(                                  //
                    (1 / 4.0) * inputPoints.getUnchecked(i - 1).position() + //
                    (2 / 4.0) * inputPoints.getUnchecked(i + 0).position() + //
                    (1 / 4.0) * inputPoints.getUnchecked(i + 1).position());
            }
        }
    }
    instabilityDelta = std::max<Int>(instabilityDelta, pointsSmoothingLevel);

    // Smooth width.
    //
    // This is different as smoothing points since we don't need to keep
    // the first/last width unchanged.
    //
    constexpr size_t widthSmoothingLevel = 2;
    if constexpr (widthSmoothingLevel != 0) {

        // Get binomial coefficients
        constexpr Int l = widthSmoothingLevel;
        constexpr Int m = 2 * widthSmoothingLevel + 1;
        constexpr std::array<Int, m> coeffs =
            binomialCoefficients<2 * widthSmoothingLevel>();

        // Apply convolution with coefficients
        for (Int i = unstableIndexStart; i < numPoints; ++i) {
            double value = {};
            double sumCoeffs = 0;
            Int j = i - l;
            for (Int k = 0; k < m; ++k, ++j) {
                if (0 <= j && j < numPoints) {
                    sumCoeffs += coeffs[k];
                    value += coeffs[k] * inputPoints[j].width();
                }
            }
            getPointRef(i).setWidth(value / sumCoeffs);
        }
        instabilityDelta = std::max<Int>(instabilityDelta, widthSmoothingLevel);
    }

    // compute chordal lengths
    updateCumulativeChordalDistances();

    // Width limitor
    constexpr double widthRoughness = 0.8;
    constexpr Int roughnessLimitorWindowSize = 3;
    const SketchPoint* lastStablePoint =
        numStablePoints == 0 ? nullptr : &getPoint(numStablePoints - 1);
    applyWidthRoughnessLimitor(
        widthRoughness, roughnessLimitorWindowSize, lastStablePoint, unstablePoints());
    instabilityDelta += roughnessLimitorWindowSize;

    return std::max<Int>(0, input.numStablePoints() - instabilityDelta);
}

void SmoothingPass::reset_() {
    // no custom state
}

} // namespace detail

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

bool Sketch::onKeyPress(ui::KeyPressEvent* /*event*/) {
    return false;
}

namespace {

double pressurePen(const ui::MouseEvent* event) {
    return event->hasPressure() ? event->pressure() : 0.5;
}

double pressurePenWidth(double pressure) {
    double baseWidth = options_::penWidth()->value();
    return 2.0 * pressure * baseWidth;
}

} // namespace

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

    auto canvas = this->canvas().lock();
    if (!canvas) {
        return false;
    }
    canvas->clearSelection();

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

void Sketch::onMouseEnter() {
    cursorChanger_.set(crossCursor());
}

void Sketch::onMouseLeave() {
    cursorChanger_.clear();
}

void Sketch::onResize() {
    reload_ = true;
}

void Sketch::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    using namespace graphics;
    minimalLatencyStrokeGeometry_ =
        engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XY_iRGBA);
    mouseInputGeometry_ =
        engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    reload_ = true;
}

void Sketch::onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    using namespace graphics;
    namespace gs = graphics::strings;

    auto context = contextLock();
    if (!context) {
        return;
    }
    auto workspace = context.workspace();
    auto canvas = context.canvas();

    // Draw temporary tip of curve between mouse event position and actual current cursor
    // position to reduce visual lag.
    //
    ui::Window* w = window();
    bool cursorMoved = false;
    if (isSketching_ && w) {
        geometry::Vec2f pos(w->mapFromGlobal(ui::globalCursorPosition()));
        geometry::Vec2d posd(root()->mapTo(this, pos));
        pos = geometry::Vec2f(
            canvas->camera().viewMatrix().inverted().transformPointAffine(posd));
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

    const bool showInputPixels = false;

    if (showInputPixels) { // TODO: only update on new points
        float hp = static_cast<float>(
            (canvasToWorkspaceMatrix_.transformPointAffine(geometry::Vec2d(0, 0.5))
             - canvasToWorkspaceMatrix_.transformPointAffine(geometry::Vec2d(0, 0)))
                .length());
        core::FloatArray pointVertices(
            {-hp, -hp, 0, 0, hp, -hp, 0, 0, -hp, hp, 0, 0, hp, hp, 0, 0});

        core::FloatArray pointInstData;
        const Int numPoints = inputPoints_.length();
        for (Int i = 0; i < numPoints; ++i) {
            geometry::Vec2d pd = inputPoints_[i].position();
            geometry::Vec2f p =
                geometry::Vec2f(canvasToWorkspaceMatrix_.transformPointAffine(pd));
            pointInstData.extend({p.x(), p.y(), 0.f, 4.f, 0.f, 1.f, 0.f, 0.5f});
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

    if (showInputPixels) {
        engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
        engine->drawInstanced(mouseInputGeometry_);
    }

    engine->popViewMatrix();
}

void Sketch::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    minimalLatencyStrokeGeometry_.reset();
}

namespace {

// This is a variant of Douglas-Peucker designed to dequantize mouse inputs
// from integer to float coordinates.
//
// To this end, the distance test checks if the current line segment AB passes
// through all pixel squares of the samples in the interval. We call
// `threshold` the minimal distance between AB and the pixel center such that
// AB does not passes through the pixel square.
//
//     ---------------
//    |               |
//    |     pixel     |             threshold = distance(pixelCenter, A'B'),
//    |     center    |         B'  where A'B' is a line parallel to AB and
//    |       x       |       '     touching the pixel square.
//    |               |     '
//    |               |   '
//    |               | '
//     ---------------'
//                  '
//                ' A'
//
// This threshold only depends on the angle AB. If AB is perfectly horizontal
// or vertical, the threshold is equal to 0.5. If AB is at 45°, the threshold
// is equal to sqrt(2)/2 (the half-diagonal).
//
// We then scale this threshold is scaled with the given `thresholdCoefficient`
// and add the given `tolerance`.
//
// If the current segment does not pass the test, then the farthest samples is
// selected for the next iteration.
//
// In some variants of this algorithm, we may also slighly move the position of
// selected samples towards the segment AB (e.g., by 0.75 * threshold), which
// seems to empirically give nicer results in some circumstances.
//
//Int reconstructInputStep(
//    core::Span<SketchPoint> points,
//    core::IntArray& indices,
//    Int intervalStart,
//    double thresholdCoefficient,
//    double tolerance = 1e-10) {
//
//    Int i = intervalStart;
//    Int endIndex = indices[i + 1];
//    while (indices[i] != endIndex) {
//
//        // Get line AB. Fast discard if AB too small.
//        Int iA = indices[i];
//        Int iB = indices[i + 1];
//        geometry::Vec2d a = points[iA].position();
//        geometry::Vec2d b = points[iB].position();
//        geometry::Vec2d ab = b - a;
//        double abLen = ab.length();
//        if (abLen < core::epsilon) {
//            ++i;
//            continue;
//        }
//
//        // Compute `threshold`
//        constexpr double sqrtOf2 = 1.4142135623730950488017;
//        double abMaxNormalizedAbsoluteCoord =
//            (std::max)(std::abs(ab.x()), std::abs(ab.y())) / abLen;
//        double abAngleWithClosestAxis = std::acos(abMaxNormalizedAbsoluteCoord);
//        double threshold =
//            std::cos(core::pi / 4 - abAngleWithClosestAxis) * (sqrtOf2 / 2);
//
//        // Apply threshold coefficient and additive tolerance
//        double adjustedThreshold = thresholdCoefficient * threshold + tolerance;
//
//        // Compute which sample between A and B is furthest from the line AB
//        double maxDist = 0;
//        Int farthestPointSide = 0;
//        Int farthestPointIndex = -1;
//        for (Int j = iA + 1; j < iB; ++j) {
//            geometry::Vec2d p = points[j].position();
//            geometry::Vec2d ap = p - a;
//            double dist = ab.det(ap) / abLen;
//            // ┌─── x
//            // │    ↑ side 1
//            // │ A ───→ B
//            // y    ↓ side 0
//            Int side = 0;
//            if (dist < 0) {
//                dist = -dist;
//                side = 1;
//            }
//            if (dist > maxDist) {
//                maxDist = dist;
//                farthestPointSide = side;
//                farthestPointIndex = j;
//            }
//        }
//
//        // If the furthest point is too far from AB, then recurse.
//        // Otherwise, stop the recursion and move one to the next segment.
//        if (maxDist > adjustedThreshold) {
//
//            // Add sample to the list of selected samples
//            indices.insert(i + 1, farthestPointIndex);
//
//            // Move the position of the selected sample slightly towards AB
//            constexpr bool isMoveEnabled = true;
//            if (isMoveEnabled) {
//                geometry::Vec2d n = ab.orthogonalized() / abLen;
//                if (farthestPointSide != 0) {
//                    n = -n;
//                }
//                // TODO: scale delta based on some data to prevent shrinkage?
//                float delta = 0.8f * threshold;
//                SketchPoint& p = points[farthestPointIndex];
//                p.setPosition(p.position() - delta * n);
//            }
//        }
//        else {
//            ++i;
//        }
//    }
//    return i;
//}

} // namespace

void Sketch::updatePreTransformPassesResult_() {
    dummyPreTransformPass_.updateResultFrom(inputPoints_);
}

const SketchPointBuffer& Sketch::preTransformPassesResult_() {
    return dummyPreTransformPass_.buffer();
}

void Sketch::updateTransformedPoints_() {
    const SketchPointBuffer& input = Sketch::preTransformPassesResult_();
    const SketchPointArray& inputPoints = input.data();

    Int n = transformedPoints_.length();
    transformedPoints_.reserve(input.length());

    for (auto it = inputPoints.begin() + n; it != inputPoints.end(); ++it) {
        SketchPoint& p = transformedPoints_.append(*it);
        p.setPosition(canvasToWorkspaceMatrix_.transformPointAffine(p.position()));
    }
}

void Sketch::updatePostTransformPassesResult_() {
    // Note: last pass must produce points with computed arclengths.
    smoothingPass_.updateResultFrom(transformedPoints_);
}

const SketchPointBuffer& Sketch::postTransformPassesResult_() const {
    return smoothingPass_.buffer();
}

core::ConstSpan<SketchPoint> Sketch::cleanInputPoints_() const {
    const SketchPointBuffer& allCleanInputPoints = postTransformPassesResult_();
    return core::ConstSpan<SketchPoint>(
        allCleanInputPoints.begin() + cleanInputStartIndex_, allCleanInputPoints.end());
}

Int Sketch::numStableCleanInputPoints_() const {
    const SketchPointBuffer& allCleanInputPoints = postTransformPassesResult_();
    return allCleanInputPoints.numStablePoints() - cleanInputStartIndex_;
}

// Assumes postTransformPassesResult_() points have computed arclengths.
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

    const SketchPointBuffer& cleanInputBuffer = postTransformPassesResult_();
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

    // Clear the points now. We don't to it on finishCurve_() for
    // debugging purposes.
    //
    inputPoints_.clear();

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
    canvasToWorkspaceMatrix_ = canvas->camera().viewMatrix().inverted();

    // Snapping: Compute start vertex to snap to
    workspace::Element* snapVertex = nullptr;
    geometry::Vec2d startPosition =
        canvasToWorkspaceMatrix_.transformPointAffine(eventPos2d);
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
    edgeItemId_ = domEdge->internalId();

    // Append start point to geometry
    continueCurve_(event);
    workspace->sync(); // required for toKeyEdge() below

    // Append start vertex to snap/cut info
    appendVertexInfo_(startPosition, startVertexItemId_);

    if (vacomplex::KeyEdge* keyEdge = toKeyEdge(workspace.get(), edgeItemId_)) {

        // Use low sampling quality override to minimize lag.
        keyEdge->data().setSamplingQualityOverride(
            geometry::CurveSamplingQuality::AdaptiveLow);

        // Move edge to proper depth location.
        vacomplex::ops::moveBelowBoundary(keyEdge);
    }

    // Update stroke tip
    minimalLatencyStrokeReload_ = true;
}

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

    dom::Element* domEndVertex = document->elementFromInternalId(endVertexItemId_);
    dom::Element* domEdge = document->elementFromInternalId(edgeItemId_);
    if (!domEndVertex || !domEdge) {
        return;
    }

    geometry::Vec2d eventPos2d(event->position());

    // Append the input point
    double pressure = pressurePen(event);
    inputPoints_.emplaceLast(
        eventPos2d,
        pressure,
        event->timestamp() - startTime_,
        pressurePenWidth(pressure));
    inputPoints_.setNumStablePoints(inputPoints_.length());

    // Apply all processing steps

    updatePreTransformPassesResult_();
    updateTransformedPoints_();
    updatePostTransformPassesResult_();
    updatePendingPositions_();
    updatePendingWidths_();

    // TODO: auto cut algorithm

    // Update DOM and workspace
    namespace ds = dom::strings;
    domEndVertex->setAttribute(ds::position, pendingPositions_.last());
    domEdge->setAttribute(ds::positions, pendingPositions_);
    domEdge->setAttribute(ds::widths, pendingWidths_);
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

void Sketch::finishCurve_(ui::MouseEvent* /*event*/) {

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

    // pre-transform passes
    dummyPreTransformPass_.reset();

    // transform (fixed pass)
    transformedPoints_.clear();

    // post-transform passes
    smoothingPass_.reset();

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
