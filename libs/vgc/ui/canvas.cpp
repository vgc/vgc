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
#include <vgc/core/paths.h>
#include <vgc/core/stopwatch.h>
#include <vgc/core/stringid.h>
#include <vgc/geometry/curve.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

SelectionListHistoryPtr SelectionListHistory::create() {
    return SelectionListHistoryPtr(new SelectionListHistory());
}

void SelectionListHistory::setSelection(const SelectionList& list) {
    lists_.emplaceLast(list);
    selectionChanged().emit();
}

void SelectionListHistory::setSelection(SelectionList&& list) {
    lists_.emplaceLast(std::move(list));
    selectionChanged().emit();
}

CanvasPtr Canvas::create(workspace::Workspace* workspace) {
    return CanvasPtr(new Canvas(workspace));
}

namespace {

double width_(const MouseEvent* event) {
    const double defaultWidth = 6.0;
    return event->hasPressure() ? 2 * event->pressure() * defaultWidth : defaultWidth;
}

core::StringId PATH("path");
core::StringId POSITIONS("positions");
core::StringId WIDTHS("widths");
core::StringId COLOR("color");

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

Canvas::Canvas(workspace::Workspace* workspace)
    : Widget()
    , workspace_(workspace)
    , mousePressed_(false)
    , tabletPressed_(false)
    , polygonMode_(0)
    , showControlPoints_(false)
    , requestedTesselationMode_(2)
    , currentTesselationMode_(2)
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

SelectionList Canvas::getSelectableItemsAt(const geometry::Vec2f& /*position*/) {
    SelectionList l = {};

    return l;
}

void Canvas::setWorkspace(workspace::Workspace* workspace) {

    clearGraphics_();

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

        namespace ss = dom::strings;

        auto isEdge = [](dom::Element* e) {
            core::StringId tagName = e->tagName();
            return tagName == PATH || tagName == ss::edge;
        };

        dom::Element* root = workspace_->document()->rootElement();

        for (dom::Node* node : root->children()) {
            dom::Element* e = dom::Element::cast(node);
            if (!(e && isEdge(e))) {
                continue;
            }
            if (e->parent() == root) {
                auto graphicsIt = getOrCreateEdgeGraphics_(e);
                toUpdate_.insert(graphicsIt);
            }
        }
    }

    requestRepaint();
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

bool Canvas::onKeyPress(KeyEvent* event) {
    switch (event->key()) {
    case Key::T:
        polygonMode_ = polygonMode_ ? 0 : 1;
        requestRepaint();
        break;
    case Key::I:
        requestedTesselationMode_ = (requestedTesselationMode_ + 1) % 4;
        requestRepaint();
        break;
    case Key::P:
        showControlPoints_ = !showControlPoints_;
        requestRepaint();
        break;
    default:
        return false;
    }

    return true;
    // Don't factor out "update()" here, to avoid unnecessary redraws for keys
    // not handled here, including modifiers.
}

void Canvas::onWorkspaceChanged_() {
    //
}

void Canvas::onDocumentChanged_(const dom::Diff& diff) {

    namespace ss = dom::strings;

    //VGC_DEBUG_TMP("onDocumentChanged_");

    auto isEdge = [](dom::Element* e) {
        core::StringId tagName = e->tagName();
        return tagName == PATH || tagName == ss::edge;
    };

    for (dom::Node* node : diff.removedNodes()) {
        dom::Element* e = dom::Element::cast(node);
        if (!(e && isEdge(e))) {
            continue;
        }
        auto it = edgeGraphicsMap_.find(e);
        if (it != edgeGraphicsMap_.end()) {
            removedEdgeGraphics_.splice(
                removedEdgeGraphics_.begin(), edgeGraphics_, it->second);
            edgeGraphicsMap_.erase(it);
        }
    }

    bool needsSort = false;

    dom::Element* root = workspace_->document()->rootElement();

    for (dom::Node* node : diff.createdNodes()) {
        dom::Element* e = dom::Element::cast(node);
        if (!(e && isEdge(e))) {
            continue;
        }
        if (e->parent() == root) {
            needsSort = true;
            auto graphicsIt = getOrCreateEdgeGraphics_(e);
            toUpdate_.insert(graphicsIt);
        }
    }

    for (dom::Node* node : diff.reparentedNodes()) {
        dom::Element* e = dom::Element::cast(node);
        if (!(e && isEdge(e))) {
            continue;
        }
        auto it = edgeGraphicsMap_.find(e);
        if (it != edgeGraphicsMap_.end()) {
            if (e->parent() != root) {
                removedEdgeGraphics_.splice(
                    removedEdgeGraphics_.begin(), edgeGraphics_, it->second);
                edgeGraphicsMap_.erase(it);
            }
            else {
                needsSort = true;
            }
        }
        else if (e->parent() == root) {
            needsSort = true;
            auto graphicsIt = getOrCreateEdgeGraphics_(e);
            toUpdate_.insert(graphicsIt);
        }
    }

    if (!needsSort) {
        for (dom::Node* node : diff.childrenReorderedNodes()) {
            if (node == root) {
                needsSort = true;
                break;
            }
        }
    }

    if (needsSort) {
        auto insertIt = edgeGraphics_.begin();
        for (dom::Node* node : root->children()) {
            dom::Element* e = dom::Element::cast(node);
            if (!(e && isEdge(e))) {
                continue;
            }
            auto it = edgeGraphicsMap_.find(e);
            if (it != edgeGraphicsMap_.end()) {
                EdgeGraphicsIterator graphicsIt = it->second;
                if (insertIt == graphicsIt) {
                    ++insertIt;
                }
                else {
                    edgeGraphics_.splice(insertIt, edgeGraphics_, graphicsIt);
                    insertIt = graphicsIt;
                }
            }
            else {
                //VGC_DEBUG_TMP("pb");
            }
        }
    }

    // XXX it's possible that update is done twice if the element is both modified and reparented..

    const auto& modifiedElements = diff.modifiedElements();
    for (EdgeGraphicsIterator it = edgeGraphics_.begin(); it != edgeGraphics_.end();
         ++it) {

        auto it2 = modifiedElements.find(it->element);
        if (it2 != modifiedElements.end()) {
            toUpdate_.insert(it);
        }
    }

    // ask for redraw
    requestRepaint();
}

void Canvas::clearGraphics_() {
    for (EdgeGraphics& r : edgeGraphics_) {
        destroyEdgeGraphics_(r);
    }
    edgeGraphics_.clear();
    edgeGraphicsMap_.clear();
}

void Canvas::updateEdgeGraphics_(graphics::Engine* engine) {
    updateTask_.start();

    removedEdgeGraphics_.clear();
    bool tesselationModeChanged = requestedTesselationMode_ != currentTesselationMode_;
    if (tesselationModeChanged) {
        currentTesselationMode_ = requestedTesselationMode_;
        for (EdgeGraphics& r : edgeGraphics_) {
            updateEdgeGraphics_(engine, r);
        }
    }
    else {
        for (auto it : toUpdate_) {
            updateEdgeGraphics_(engine, *it);
        }
    }
    toUpdate_.clear();

    updateTask_.stop();
}

Canvas::EdgeGraphicsIterator Canvas::getOrCreateEdgeGraphics_(dom::Element* element) {
    auto it = edgeGraphicsMap_.find(element);
    if (it == edgeGraphicsMap_.end()) {
        auto newEdgeGraphicsIterator =
            edgeGraphics_.emplace(edgeGraphics_.end(), EdgeGraphics(element));
        edgeGraphicsMap_.emplace(element, newEdgeGraphicsIterator);
        return newEdgeGraphicsIterator;
    }
    return it->second;
}

void Canvas::updateEdgeGraphics_(graphics::Engine* engine, EdgeGraphics& r) {
    using namespace graphics;
    if (!r.inited_) {
        // XXX use indexing when triangulate() implements indices.
        r.strokeGeometry_ =
            engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XY_iRGBA);

        r.pointsGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotRGBA);

        r.dispLineGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotRGBA);

        r.inited_ = true;
    }

    dom::Element* path = r.element;
    geometry::SharedConstVec2dArray positions =
        path->getAttribute(POSITIONS).getVec2dArray();
    core::SharedConstDoubleArray widths = path->getAttribute(WIDTHS).getDoubleArray();
    core::Color color = path->getAttribute(COLOR).getColor();

    // Convert the dom::Path to a geometry::Curve
    // XXX move this logic to dom::Path

    VGC_CORE_ASSERT(positions.get().size() == widths.get().size());
    //Int nPoints = positions.get().length();
    geometry::Curve curve;
    curve.setColor(color);
    curve.setPositionData(positions);
    curve.setWidthData(widths);

    geometry::Vec2fArray strokeVertices;
    core::Array<geometry::Vec4f> pointVertices;
    core::FloatArray pointInstData;
    core::Array<geometry::Vec4f> lineVertices;
    core::FloatArray lineInstData;

    float pointHalfSize = 5.f;
    float lineHalfSize = 2.f;

    double maxAngle = 0.05;
    int minQuads = 1;
    int maxQuads = 64;
    if (requestedTesselationMode_ <= 2) {
        if (requestedTesselationMode_ == 0) {
            maxQuads = 1;
        }
        else if (requestedTesselationMode_ == 1) {
            minQuads = 10;
            maxQuads = 10;
        }
        geometry::CurveSamplingParameters samplingParams = {};
        samplingParams.setMaxAngle(maxAngle * 0.5); // matches triangulate()
        samplingParams.setMinIntraSegmentSamples(minQuads - 1);
        samplingParams.setMaxIntraSegmentSamples(maxQuads - 1);
        core::Array<geometry::CurveSample> samples;
        curve.sampleRange(samplingParams, samples);
        for (const geometry::CurveSample& s : samples) {
            geometry::Vec2d p0 = s.leftPoint();
            strokeVertices.emplaceLast(geometry::Vec2f(p0));
            geometry::Vec2d p1 = s.rightPoint();
            strokeVertices.emplaceLast(geometry::Vec2f(p1));
            geometry::Vec2f p = geometry::Vec2f(s.position());
            geometry::Vec2f n = geometry::Vec2f(s.normal());
            // clang-format off
            lineVertices.emplaceLast(p.x(), p.y(), -lineHalfSize * n.x(), -lineHalfSize * n.y());
            lineVertices.emplaceLast(p.x(), p.y(),  lineHalfSize * n.x(),  lineHalfSize * n.y());
            // clang-format on
        }
    }
    else {
        geometry::Vec2dArray triangles = curve.triangulate(maxAngle, 1, 64);
        for (const geometry::Vec2d& p : triangles) {
            strokeVertices.emplaceLast(geometry::Vec2f(p));
        }
    }

    engine->updateBufferData(
        r.strokeGeometry_->vertexBuffer(0), //
        std::move(strokeVertices));
    engine->updateBufferData(
        r.strokeGeometry_->vertexBuffer(1), //
        core::Array<float>({color.r(), color.g(), color.b(), color.a()}));

    lineInstData.extend({0.f, 0.f, 1.f, 0.02f, 0.64f, 1.0f, 1.f});

    engine->updateBufferData(
        r.dispLineGeometry_->vertexBuffer(0), std::move(lineVertices));
    engine->updateBufferData(
        r.dispLineGeometry_->vertexBuffer(1), std::move(lineInstData));

    // clang-format off
    pointVertices.extend({
        {0, 0, -pointHalfSize, -pointHalfSize},
        {0, 0, -pointHalfSize,  pointHalfSize},
        {0, 0,  pointHalfSize, -pointHalfSize},
        {0, 0,  pointHalfSize,  pointHalfSize} });
    // clang-format on

    const geometry::Vec2dArray& d = curve.positionData();
    Int numPoints = core::int_cast<GLsizei>(d.length());
    const float dl = 1.f / numPoints;
    for (Int j = 0; j < numPoints; ++j) {
        geometry::Vec2d dp = d[j];
        float l = j * dl;
        pointInstData.extend(
            {static_cast<float>(dp.x()),
             static_cast<float>(dp.y()),
             0.f,
             (l > 0.5f ? 2 * (1.f - l) : 1.f),
             0.f,
             (l < 0.5f ? 2 * l : 1.f),
             1.f});
    }
    r.numPoints = numPoints;

    engine->updateBufferData(
        r.pointsGeometry_->vertexBuffer(0), std::move(pointVertices));
    engine->updateBufferData(
        r.pointsGeometry_->vertexBuffer(1), std::move(pointInstData));
}

void Canvas::destroyEdgeGraphics_(EdgeGraphics& r) {
    r.strokeGeometry_.reset();
    r.pointsGeometry_.reset();
    r.dispLineGeometry_.reset();
    r.inited_ = false;
}

void Canvas::startCurve_(const geometry::Vec2d& p, double width) {
    if (!workspace_ || !workspace_->document()) {
        return;
    }

    namespace ss = dom::strings;

    // XXX CLEAN
    static core::StringId Draw_Curve("Draw Curve");
    core::History* history = workspace_->history();
    drawCurveUndoGroup_ = history->createUndoGroup(Draw_Curve);

    drawCurveUndoGroup_->undone().connect([this](core::UndoGroup*, bool /*isAbort*/) {
        // isAbort should be true since we have no sub-group
        isSketching_ = false;
        drawCurveUndoGroup_ = nullptr;
    });

    workspace::Element* wVgc = workspace_->vgcElement();
    dom::Element* dVgc = wVgc->domElement();

    dom::Element* v0 = dom::Element::create(dVgc, ss::vertex);
    dom::Element* v1 = dom::Element::create(dVgc, ss::vertex);
    dom::Element* edge = dom::Element::create(dVgc, ss::edge);

    v0->setAttribute(ss::position, p);
    v1->setAttribute(ss::position, p);

    edge->setAttribute(ss::positions, geometry::Vec2dArray());
    edge->setAttribute(ss::widths, core::DoubleArray());
    edge->setAttribute(ss::color, currentColor_);
    edge->setAttribute(ss::startvertex, v0->getPathFromId());
    edge->setAttribute(ss::endvertex, v1->getPathFromId());

    endVertex_ = v1;
    edge_ = edge;

    continueCurve_(p, width);
}

void Canvas::continueCurve_(const geometry::Vec2d& p, double width) {
    if (!workspace_ || !workspace_->document()) {
        return;
    }

    namespace ss = dom::strings;

    if (edge_) {
        points_.append(p);
        widths_.append(width);

        endVertex_->setAttribute(ss::position, p);

        edge_->setAttribute(ss::positions, points_);
        edge_->setAttribute(ss::widths, widths_);

        //VGC_DEBUG_TMP("workspace_->sync()");
        workspace_->sync();
    }
}

// Reimplementation of Widget virtual methods

bool Canvas::onMouseMove(MouseEvent* event) {
    if (!mousePressed_) {
        return false;
    }

    // Note: event.button() is always NoButton for move events. This is why
    // we use the variable isPanning_ and isSketching_ to remember the current
    // mouse action. In the future, we'll abstract this mechanism in a separate
    // class.

    geometry::Vec2f mousePosf = event->position();
    geometry::Vec2d mousePos = geometry::Vec2d(mousePosf.x(), mousePosf.y());
    if (isSketching_) {
        // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
        // and should be cached), but let's keep it like this for now for testing.
        geometry::Vec2d viewCoords = mousePos;
        geometry::Vec2d worldCoords =
            camera_.viewMatrix().inverted().transformPointAffine(viewCoords);
        continueCurve_(worldCoords, width_(event));
        return true;
    }
    else if (isPanning_) {
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

    if (isSketching_ || isPanning_ || isRotating_ || isZooming_) {
        return true;
    }

    geometry::Vec2f mousePosf = event->position();
    geometry::Vec2d mousePos = geometry::Vec2d(mousePosf.x(), mousePosf.y());
    if (event->modifierKeys().isEmpty() && event->button() == MouseButton::Left) {
        isSketching_ = true;
        // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
        // and should be cached), but let's keep it like this for now for testing.
        geometry::Vec2d viewCoords = mousePos;
        geometry::Vec2d worldCoords =
            camera_.viewMatrix().inverted().transformPointAffine(viewCoords);
        startCurve_(worldCoords, width_(event));
        return true;
    }
    else if (
        event->modifierKeys() == ModifierKey::Alt
        && event->button() == MouseButton::Left) {
        isRotating_ = true;
        mousePosAtPress_ = mousePos;
        cameraAtPress_ = camera_;
        return true;
    }
    else if (
        event->modifierKeys() == ModifierKey::Alt
        && event->button() == MouseButton::Middle) {
        isPanning_ = true;
        mousePosAtPress_ = mousePos;
        cameraAtPress_ = camera_;
        return true;
    }
    else if (
        event->modifierKeys() == ModifierKey::Alt
        && event->button() == MouseButton::Right) {
        isZooming_ = true;
        mousePosAtPress_ = mousePos;
        cameraAtPress_ = camera_;
        return true;
    }

    return false;
}

bool Canvas::onMouseRelease(MouseEvent* event) {
    if (!mousePressed_ || mouseButtonAtPress_ != event->button()) {
        return false;
    }

    isSketching_ = false;
    isRotating_ = false;
    isPanning_ = false;
    isZooming_ = false;

    if (drawCurveUndoGroup_) {
        drawCurveUndoGroup_->close();
        drawCurveUndoGroup_ = nullptr;
    }
    endVertex_ = nullptr;
    edge_ = nullptr;
    points_.clear();
    widths_.clear();

    mousePressed_ = false;

    return true;
}

bool Canvas::onMouseEnter() {
    cursorChanger_.set(crossCursor());
    return false;
}

bool Canvas::onMouseLeave() {
    cursorChanger_.clear();
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
    graphics::RasterizerStateCreateInfo createInfo = {};
    fillRS_ = engine->createRasterizerState(createInfo);
    createInfo.setFillMode(graphics::FillMode::Wireframe);
    wireframeRS_ = engine->createRasterizerState(createInfo);
    bgGeometry_ =
        engine->createDynamicTriangleStripView(graphics::BuiltinGeometryLayout::XYRGB);
    reload_ = true;
}

void Canvas::onPaintDraw(graphics::Engine* engine, PaintOptions /*options*/) {

    namespace gs = graphics::strings;

    updateEdgeGraphics_(engine);

    drawTask_.start();

    auto modifiedParameters = graphics::PipelineParameter::RasterizerState;
    engine->pushPipelineParameters(modifiedParameters);

    engine->setProgram(graphics::BuiltinProgram::Simple);

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
    geometry::Mat4d cameraView = camera_.viewMatrix();
    geometry::Mat4f cameraViewf(
        static_cast<float>(cameraView(0, 0)),
        static_cast<float>(cameraView(0, 1)),
        static_cast<float>(cameraView(0, 2)),
        static_cast<float>(cameraView(0, 3)),
        static_cast<float>(cameraView(1, 0)),
        static_cast<float>(cameraView(1, 1)),
        static_cast<float>(cameraView(1, 2)),
        static_cast<float>(cameraView(1, 3)),
        static_cast<float>(cameraView(2, 0)),
        static_cast<float>(cameraView(2, 1)),
        static_cast<float>(cameraView(2, 2)),
        static_cast<float>(cameraView(2, 3)),
        static_cast<float>(cameraView(3, 0)),
        static_cast<float>(cameraView(3, 1)),
        static_cast<float>(cameraView(3, 2)),
        static_cast<float>(cameraView(3, 3)));
    engine->pushViewMatrix(vm * cameraViewf);

    // Draw triangles

    for (EdgeGraphics& r : edgeGraphics_) {
        engine->draw(r.strokeGeometry_, -1, 0);
    }

    engine->setProgram(graphics::BuiltinProgram::SreenSpaceDisplacement);

    // Draw control points
    if (showControlPoints_) {
        for (EdgeGraphics& r : edgeGraphics_) {
            engine->draw(r.dispLineGeometry_, -1, 0);
            engine->draw(r.pointsGeometry_, -1, r.numPoints);
        }
    }

    engine->popViewMatrix();
    engine->popPipelineParameters(modifiedParameters);

    drawTask_.stop();
}

void Canvas::onPaintDestroy(graphics::Engine*) {
    bgGeometry_.reset();
    for (EdgeGraphics& r : edgeGraphics_) {
        destroyEdgeGraphics_(r);
    }
    fillRS_.reset();
    wireframeRS_.reset();
}

} // namespace vgc::ui
