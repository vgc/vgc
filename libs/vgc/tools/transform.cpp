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

#include <vgc/tools/transform.h>

#include <set>

#include <QBitmap>
#include <QCursor>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

#include <vgc/core/paths.h>
#include <vgc/graphics/detail/shapeutil.h>
#include <vgc/graphics/strings.h>
#include <vgc/vacomplex/complex.h>
#include <vgc/vacomplex/detail/operationsimpl.h>
#include <vgc/workspace/colors.h>

namespace vgc::tools {

namespace {

namespace commands {

using ui::Key;
using ui::ModifierKey;
using ui::MouseButton;
using ui::Shortcut;

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    translateLeftSmallStep,
    "tools.transform.translateLeftSmallStep",
    "Transform Box: Translate Left (Small Step)",
    Key::Left);

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    translateRightSmallStep,
    "tools.transform.translateRightSmallStep",
    "Transform Box: Translate Right (Small Step)",
    Key::Right);

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    translateUpSmallStep,
    "tools.transform.translateUpSmallStep",
    "Transform Box: Translate Up (Small Step)",
    Key::Up);

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    translateDownSmallStep,
    "tools.transform.translateDownSmallStep",
    "Transform Box: Translate Down (Small Step)",
    Key::Down);

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    translateLeftBigStep,
    "tools.transform.translateLeftBigStep",
    "Transform Box: Translate Left (Big Step)",
    Shortcut(ModifierKey::Shift, Key::Left));

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    translateRightBigStep,
    "tools.transform.translateRightBigStep",
    "Transform Box: Translate Right (Big Step)",
    Shortcut(ModifierKey::Shift, Key::Right));

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    translateUpBigStep,
    "tools.transform.translateUpBigStep",
    "Transform Box: Translate Up (Big Step)",
    Shortcut(ModifierKey::Shift, Key::Up));

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    translateDownBigStep,
    "tools.transform.translateDownBigStep",
    "Transform Box: Translate Down (Big Step)",
    Shortcut(ModifierKey::Shift, Key::Down));

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    scaleDrag,
    "tools.transform.scaleDrag",
    "Transform Box: Scale Drag",
    Shortcut(MouseButton::Left))

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    rotateDrag,
    "tools.transform.rotateDrag",
    "Transform Box: Rotate Drag",
    Shortcut(ModifierKey::Alt, MouseButton::Left))

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    scaleDragWithPivot,
    "tools.transform.scaleDragWithPivot",
    "Transform Box: Scale Drag With Pivot",
    Shortcut(ModifierKey::Alt, MouseButton::Left))

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    rotateDragWithPivot,
    "tools.transform.rotateDragWithPivot",
    "Transform Box: Rotate Drag With Pivot",
    Shortcut(MouseButton::Left))

} // namespace commands

// Returns the file path of a cursor svg file.
std::string cursorSvgPath_(const std::string& name) {
    return core::resourcePath("tools/cursors/" + name);
}

void drawScalingCursor(QPainter& painter, double angle) {
    painter.translate(16, 16);
    qreal angleDeg = angle / core::pi * 180.0;
    painter.rotate(angleDeg);
    QString fpath(cursorSvgPath_("scaling.svg").c_str());
    QSvgRenderer svgRenderer(fpath);
    svgRenderer.render(&painter, QRectF(-16, -16, 32, 32));
}

void drawRotationCursor(QPainter& painter, double angle) {
    painter.translate(16, 16);
    qreal angleDeg = angle / core::pi * 180.0;
    painter.rotate(angleDeg + 90.0);
    QString fpath(cursorSvgPath_("rotation.svg").c_str());
    QSvgRenderer svgRenderer(fpath);
    svgRenderer.render(&painter, QRectF(-16, -16, 32, 32));
}

constexpr Int cursorCount = 128;

template<typename DrawFn>
std::array<QCursor, cursorCount> createRotatedCursors(DrawFn drawFn) {
    std::array<QCursor, cursorCount> res;
    for (Int i = 0; i < cursorCount; ++i) {
        double angle = static_cast<double>(i) / cursorCount * core::pi * 2.0;

        // Draw bitmap
        QPixmap image(32, 32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setPen(QPen(Qt::red, 1.0));
        painter.setRenderHint(QPainter::RenderHint::Antialiasing, true);
        //painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);

        drawFn(painter, angle);

        // Draw mask
        QBitmap mask(32, 32);
        mask.fill(Qt::color0);
        QPainter maskPainter(&mask);
        maskPainter.setPen(QPen(Qt::color1, 1.0));
        drawFn(maskPainter, angle);

        //image.setMask(mask);

#ifndef VGC_CORE_OS_WINDOWS
        // Make the cursor color XOR'd on Windows, black on other platforms. Ideally,
        // we'd prefer XOR'd on all platforms, but it's only supported on Windows.
        // See Qt doc for QCursor(const QBitmap &bitmap, const QBitmap &mask).
        drawFn(maskPainter, angle);
#endif

        // Create cursor
        res[i] = QCursor(image);
    }
    return res;
}

QCursor scalingCursor(double angle) {
    static std::array<QCursor, cursorCount> cursors =
        createRotatedCursors(drawScalingCursor);
    Int idx = static_cast<Int>(std::round(angle * cursorCount / (2 * core::pi)));
    idx = (idx % cursorCount + cursorCount) % cursorCount;
    return cursors[idx];
}

QCursor rotationCursor(double angle) {
    static std::array<QCursor, cursorCount> cursors =
        createRotatedCursors(drawRotationCursor);
    Int idx = static_cast<Int>(std::round(angle * cursorCount / (2 * core::pi)));
    idx = (idx % cursorCount + cursorCount) % cursorCount;
    return cursors[idx];
}

} // namespace

namespace detail {

VGC_DECLARE_OBJECT(TransformDragAction);

class TransformDragAction : public ui::Action {
private:
    VGC_OBJECT(TransformDragAction, ui::Action)

protected:
    /// This is an implementation details.
    /// Please use `TransformDragAction::create()` instead.
    ///
    TransformDragAction(
        CreateKey key,
        TransformBox* box,
        core::StringId commandId,
        TransformDragActionType transformType,
        Int manipIndex,
        bool usePivot)

        : ui::Action(key, commandId)
        , box_(box)
        , transformType_(transformType)
        , manipIndex_(manipIndex)
        , usePivot_(usePivot) {
    }

public:
    /// Creates a `TransformDragAction`.
    ///
    static TransformDragActionPtr create(
        TransformBox* box,
        core::StringId commandId,
        TransformDragActionType transformType,
        Int manipIndex,
        bool usePivot) {

        return core::createObject<TransformDragAction>(
            box, commandId, transformType, manipIndex, usePivot);
    }

public:
    TransformDragActionType transformType() const {
        return transformType_;
    }

    Int manipIndex() const {
        return manipIndex_;
    }

    void onMouseDragStart(ui::MouseEvent* event) override {
        box_->isTransformActionOngoing_ = true;
        box_->transformActionMatrix_ = geometry::Mat3d::identity;

        canvas::Canvas* canvas = nullptr;
        workspace::Workspace* workspace = nullptr;
        if (!getPointers_(canvas, workspace)) {
            return;
        }

        // Prepare transformer
        transformer_.setElements(workspace, box_->elementIds_);

        const geometry::Mat4d& cameraMatrix = canvas->camera().viewMatrix();
        geometry::Mat4d invCameraMatrix = cameraMatrix.inverted();

        geometry::Vec2d cursorPositionInCanvas(event->position());
        geometry::Vec2d cursorPosition =
            invCameraMatrix.transformPoint(cursorPositionInCanvas);

        // Retrieve positions in workspace of:
        // - pivot point
        // - manip point
        // - opposite manip point
        //
        geometry::Vec2d pivotPosition = box_->pivotPoint_;
        auto getManipPosition = [](geometry::Rect2d rect,
                                   geometry::Vec2d cursorPosition,
                                   Int index) -> geometry::Vec2d {
            Int cornerIndex = index / 2;
            geometry::Vec2d res = rect.corner(cornerIndex);
            if (index % 2 == 1) {
                geometry::Vec2d otherCorner = rect.corner((cornerIndex + 1) % 4);
                geometry::Vec2d dir = (otherCorner - res).normalized();
                res += dir * dir.dot(cursorPosition - res);
            }
            return res;
        };
        originalManipPoint_ =
            getManipPosition(box_->boundingBox_, cursorPosition, manipIndex_);
        oppositeManipPoint_ =
            getManipPosition(box_->boundingBox_, cursorPosition, (manipIndex_ + 4) % 8);

        if (usePivot_) {
            // use center for now
            oppositeManipPoint_ = pivotPosition;
        }
        else {
            getManipPosition(box_->boundingBox_, cursorPosition, (manipIndex_ + 4) % 8);
        }

        // Compute delta in canvas space between cursor and manipulation point.
        cursorManipDelta_ =
            cameraMatrix.transformPoint(originalManipPoint_) - cursorPositionInCanvas;

        geometry::Vec2d oppositeManipPointInCanvas =
            cameraMatrix.transformPoint(oppositeManipPoint_);
        cursorManipAngleStart_ =
            (cursorPositionInCanvas - oppositeManipPointInCanvas).angle();
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        canvas::Canvas* canvas = nullptr;
        workspace::Workspace* workspace = nullptr;
        if (!getPointers_(canvas, workspace)) {
            return;
        }

        if (transformer_.workspace() != workspace) {
            return;
        }

        core::History* history = workspace->history();
        if (!undoGroup_ && history) {
            // Open history group
            core::StringId groupId = command()->id();
            undoGroup_ = history->createUndoGroup(groupId);
        }

        geometry::Mat3d transform = geometry::Mat3d::identity;

        const geometry::Mat4d& cameraMatrix = canvas->camera().viewMatrix();
        geometry::Mat4d invCameraMatrix = cameraMatrix.inverted();
        geometry::Vec2d cursorPositionInCanvas(event->position());

        switch (transformType_) {
        case detail::TransformDragActionType::Scale: {
            geometry::Vec2d cursorPosition = invCameraMatrix.transformPoint(
                cursorPositionInCanvas + cursorManipDelta_);
            transform.translate(oppositeManipPoint_);
            geometry::Vec2d d0 = (originalManipPoint_ - oppositeManipPoint_);
            geometry::Vec2d d1 = (cursorPosition - oppositeManipPoint_);
            double sx = std::abs(d0.x()) < std::abs(d1.x() * core::epsilon)
                            ? 1.0
                            : d1.x() / d0.x();
            double sy = std::abs(d0.y()) < std::abs(d1.y() * core::epsilon)
                            ? 1.0
                            : d1.y() / d0.y();
            if (manipIndex_ % 2 == 0) {
                transform.scale(sx, sy);
            }
            else if (manipIndex_ == 1 || manipIndex_ == 5) {
                transform.scale(1.0, sy);
            }
            else {
                transform.scale(sx, 1.0);
            }
            transform.translate(-oppositeManipPoint_);
            break;
        }
        case detail::TransformDragActionType::Rotate: {
            transform.translate(oppositeManipPoint_);

            geometry::Vec2d oppositeManipPointInCanvas =
                cameraMatrix.transformPoint(oppositeManipPoint_);
            double cursorManipAngleNow =
                (cursorPositionInCanvas - oppositeManipPointInCanvas).angle();

            double angle = cursorManipAngleNow - cursorManipAngleStart_;

            box_->cursorChanger_.set(rotationCursor(cursorManipAngleNow));

            transform.rotate(angle);
            transform.translate(-oppositeManipPoint_);
            break;
        }
        }

        if (!draggedOnce_) {
            transformer_.startDragTransform();
        }
        transformer_.updateDragTransform(transform);

        box_->transformActionMatrix_ = transform;

        draggedOnce_ = true;
    }

    void onMouseDragConfirm(ui::MouseEvent* /*event*/) override {
        box_->isTransformActionOngoing_ = false;
        box_->isBoundingBoxDirty_ = true;

        workspace::Workspace* workspace = transformer_.workspace();
        if (draggedOnce_ && workspace) {
            core::History* history = workspace->history();
            if (!undoGroup_ && history) {
                // Open history group
                core::StringId groupId = command()->id();
                undoGroup_ = history->createUndoGroup(groupId);
            }
            // Finalize Op
            transformer_.finalizeDragTransform();
            // Close history group
            if (undoGroup_) {
                undoGroup_->close();
                undoGroup_ = nullptr;
            }
        }
        reset_();
    }

    void onMouseDragCancel(ui::MouseEvent* /*event*/) override {
        box_->isTransformActionOngoing_ = false;
        box_->isBoundingBoxDirty_ = true;

        workspace::Workspace* workspace = transformer_.workspace();
        if (draggedOnce_ && workspace) {
            transformer_.cancelDragTransform();

            core::History* history = workspace->history();
            if (history && undoGroup_) {
                // TODO: have abort() in undoGroup.
                // TODO: use UndoGroupPtr ?
                history->abort();
                undoGroup_ = nullptr;
            }
        }
        reset_();
    }

    void reset_() {
        draggedOnce_ = false;
        undoGroup_ = nullptr;
        transformer_.clear();
    }

public:
    TransformBox* const box_;
    const TransformDragActionType transformType_;
    const Int manipIndex_;
    const bool usePivot_;

    bool draggedOnce_ = false;
    core::UndoGroup* undoGroup_ = nullptr;

    geometry::Vec2d cursorManipDelta_;
    double cursorManipAngleStart_;
    geometry::Vec2d originalManipPoint_;
    geometry::Vec2d oppositeManipPoint_;

    detail::TopologyAwareTransformer transformer_;

    bool getPointers_(canvas::Canvas*& canvas, workspace::Workspace*& workspace) {
        if (!box_->canvasTool_) {
            return false;
        }
        canvas = box_->canvasTool_->canvas();
        if (!canvas) {
            return false;
        }
        workspace = box_->workspace_.getIfAlive();
        if (!workspace) {
            return false;
        }
        return true;
    }
};

TopologyAwareTransformer::~TopologyAwareTransformer() {
    if (isDragTransforming_) {
        cancelDragTransform();
    }
}

void TopologyAwareTransformer::setElements(
    const workspace::WorkspacePtr& workspace,
    const core::Array<core::Id>& elementIds) {

    if (isDragTransforming_) {
        cancelDragTransform();
    }

    vertices_.clear();
    edges_.clear();
    edgesToSnap_.clear();

    workspace_ = workspace;
    if (!workspace_.isAlive()) {
        return;
    }

    // Only key vertices and edges have intrinsic spatial data amongst
    // vac cells, so we identify those first.
    //
    std::set<vacomplex::KeyVertex*> vertices;
    std::set<vacomplex::KeyEdge*> edges;
    auto insertCell = [&](vacomplex::Cell* cell) {
        switch (cell->cellType()) {
        case vacomplex::CellType::KeyVertex: {
            vertices.insert(cell->toKeyVertexUnchecked());
            break;
        }
        case vacomplex::CellType::KeyEdge: {
            edges.insert(cell->toKeyEdgeUnchecked());
            break;
        }
        default:
            break;
        }
    };

    for (core::Id id : elementIds) {
        workspace::Element* element = workspace_->find(id);
        if (!element) {
            continue;
        }
        vacomplex::Node* node = element->vacNode();
        if (!node || !node->isCell()) {
            continue;
        }
        vacomplex::Cell* cell = node->toCellUnchecked();
        insertCell(cell);
        for (vacomplex::Cell* boundaryCell : cell->boundary()) {
            insertCell(boundaryCell);
        }
    }

    // Every edge connected to translated vertices has to be either
    // partially modified (snapped) or translated (both vertices are
    // translated).
    std::set<vacomplex::KeyEdge*> edgesToSnap;
    for (vacomplex::KeyVertex* kv : vertices) {
        for (vacomplex::Cell* cell : kv->star()) {
            if (cell->cellType() == vacomplex::CellType::KeyEdge) {
                vacomplex::KeyEdge* ke = cell->toKeyEdgeUnchecked();
                if (edges.count(ke) == 0) {
                    edgesToSnap.insert(ke);
                }
            }
        }
    }
    // Now transfer edges of affectedEdges that have both end vertices
    // in `vertices` to `edges`.
    for (auto it = edgesToSnap.begin(); it != edgesToSnap.end();) {
        vacomplex::KeyEdge* ke = *it;
        // It is guaranteed that these edges have start and end vertices,
        // otherwise they would not be in any vertex star.
        Int n = vertices.count(ke->startVertex()) + vertices.count(ke->endVertex());
        if (n != 1) {
            edges.insert(ke);
            it = edgesToSnap.erase(it);
        }
        else {
            ++it;
        }
    }

    // Save original intrinsic geometry data for translation
    for (vacomplex::KeyVertex* kv : vertices) {
        workspace::Element* element = workspace_->findVacElement(kv->id());
        if (element) {
            vertices_.append({element->id(), kv->position()});
        }
    }
    for (vacomplex::KeyEdge* ke : edges) {
        workspace::Element* element = workspace_->findVacElement(ke->id());
        if (element) {
            edges_.append({element->id()});
        }
    }
    for (vacomplex::KeyEdge* ke : edgesToSnap) {
        workspace::Element* element = workspace_->findVacElement(ke->id());
        if (element) {
            edgesToSnap_.append({element->id()});
        }
    }
}

void TopologyAwareTransformer::clear() {
    if (isDragTransforming_) {
        cancelDragTransform();
    }

    vertices_.clear();
    edges_.clear();
    edgesToSnap_.clear();
    workspace_ = nullptr;
}

namespace {

class MultiComplexMainOperation {
public:
    void addComplex(vacomplex::Complex* complex) {
        bool found = false;
        for (const vacomplex::detail::Operations& op : ops_) {
            if (op.complex() == complex) {
                found = true;
                break;
            }
        }
        if (!found) {
            ops_.emplaceLast(complex);
        }
    }

    void finish() {
        ops_.clear();
    }

private:
    core::Array<vacomplex::detail::Operations> ops_;
};

} // namespace

void TopologyAwareTransformer::transform(const geometry::Mat3d& transform) {

    MultiComplexMainOperation mainOp = {};

    // TODO: take group transformations into account.

    if (!workspace_.isAlive()) {
        return;
    }

    // Vertices
    for (const KeyVertexTransformData& td : vertices_) {
        vacomplex::KeyVertex* kv = findKeyVertex_(td.elementId);
        if (kv) {
            mainOp.addComplex(kv->complex());
            vacomplex::ops::setKeyVertexPosition(
                kv, transform.transformPoint(kv->position()));
        }
    }

    // Edges
    for (const KeyEdgeTransformData& td : edges_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            mainOp.addComplex(ke->complex());
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                // TODO: take layer transformations into account.
                data->transform(transform);
            }
        }
    }

    // Edges to snap
    for (const KeyEdgeTransformData& td : edgesToSnap_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            mainOp.addComplex(ke->complex());
            ke->snapGeometry();
        }
    }
}

void TopologyAwareTransformer::transform(const geometry::Vec2d& translation) {

    MultiComplexMainOperation mainOp = {};

    // TODO: take group transformations into account.

    if (!workspace_.isAlive()) {
        return;
    }

    // Vertices
    for (const KeyVertexTransformData& td : vertices_) {
        vacomplex::KeyVertex* kv = findKeyVertex_(td.elementId);
        if (kv) {
            mainOp.addComplex(kv->complex());
            vacomplex::ops::setKeyVertexPosition(kv, kv->position() + translation);
        }
    }

    // Edges
    for (const KeyEdgeTransformData& td : edges_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            mainOp.addComplex(ke->complex());
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                // TODO: take layer transformations into account.
                data->translate(translation);
            }
        }
    }

    // Edges to snap
    for (const KeyEdgeTransformData& td : edgesToSnap_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            mainOp.addComplex(ke->complex());
            ke->snapGeometry();
        }
    }
}

void TopologyAwareTransformer::startDragTransform() {
    if (!workspace_.isAlive() || isDragTransforming_) {
        return;
    }
    isDragTransforming_ = true;

    // Vertices
    for (KeyVertexTransformData& td : vertices_) {
        vacomplex::KeyVertex* kv = findKeyVertex_(td.elementId);
        if (kv) {
            td.originalPosition = kv->position();
        }
    }

    // Edges
    for (KeyEdgeTransformData& td : edges_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                td.oldData = data->clone();
            }
        }
    }

    // Edges to snap
    for (KeyEdgeTransformData& td : edgesToSnap_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                td.oldData = data->clone();
            }
        }
    }
}

void TopologyAwareTransformer::updateDragTransform(const geometry::Mat3d& transform) {
    if (!workspace_.isAlive() || !isDragTransforming_) {
        return;
    }

    MultiComplexMainOperation mainOp = {};

    // TODO: take group transformations into account.

    // Vertices
    for (const KeyVertexTransformData& td : vertices_) {
        vacomplex::KeyVertex* kv = findKeyVertex_(td.elementId);
        if (kv) {
            mainOp.addComplex(kv->complex());
            vacomplex::ops::setKeyVertexPosition(
                kv, transform.transformPoint(td.originalPosition));
        }
    }

    // Edges
    for (const KeyEdgeTransformData& td : edges_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            mainOp.addComplex(ke->complex());
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                *data = *td.oldData;
                data->transform(transform);
            }
        }
    }

    // Edges to snap
    for (const KeyEdgeTransformData& td : edgesToSnap_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            mainOp.addComplex(ke->complex());
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                *data = *td.oldData;
                ke->snapGeometry();
            }
        }
    }
}

void TopologyAwareTransformer::updateDragTransform(const geometry::Vec2d& translation) {
    if (!workspace_.isAlive() || !isDragTransforming_) {
        return;
    }

    MultiComplexMainOperation mainOp = {};

    // TODO: take group transformations into account.

    // Vertices
    for (const KeyVertexTransformData& td : vertices_) {
        vacomplex::KeyVertex* kv = findKeyVertex_(td.elementId);
        if (kv) {
            mainOp.addComplex(kv->complex());
            vacomplex::ops::setKeyVertexPosition(kv, td.originalPosition + translation);
        }
    }

    // Edges
    for (const KeyEdgeTransformData& td : edges_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            mainOp.addComplex(ke->complex());
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                *data = *td.oldData;
                data->translate(translation);
            }
        }
    }

    // Edges to snap
    for (const KeyEdgeTransformData& td : edgesToSnap_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            mainOp.addComplex(ke->complex());
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                *data = *td.oldData;
                ke->snapGeometry();
            }
        }
    }
}

void TopologyAwareTransformer::finalizeDragTransform() {
    if (!workspace_.isAlive() || !isDragTransforming_) {
        return;
    }

    // Edges
    for (const KeyEdgeTransformData& td : edges_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                //data->finishEdit();
            }
        }
    }

    // Edges to snap
    for (const KeyEdgeTransformData& td : edgesToSnap_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                //data->finishEdit();
            }
        }
    }

    isDragTransforming_ = false;
}

void TopologyAwareTransformer::cancelDragTransform() {
    if (!workspace_.isAlive() || !isDragTransforming_) {
        return;
    }

    // TODO: take group transformations into account.

    // Vertices
    for (const KeyVertexTransformData& td : vertices_) {
        vacomplex::KeyVertex* kv = findKeyVertex_(td.elementId);
        if (kv) {
            vacomplex::ops::setKeyVertexPosition(kv, td.originalPosition);
        }
    }

    // Edges
    for (const KeyEdgeTransformData& td : edges_) {
        vacomplex::KeyEdge* ke = findKeyEdge_(td.elementId);
        if (ke) {
            vacomplex::KeyEdgeData* data = ke->data();
            if (data) {
                *data = *td.oldData;
            }
        }
    }

    isDragTransforming_ = false;
}

vacomplex::KeyVertex* TopologyAwareTransformer::findKeyVertex_(core::Id id) {
    workspace::Element* element = workspace_->find(id);
    if (element && element->vacNode() && element->vacNode()->isCell()) {
        return element->vacNode()->toCellUnchecked()->toKeyVertex();
    }
    return nullptr;
}

vacomplex::KeyEdge* TopologyAwareTransformer::findKeyEdge_(core::Id id) {
    workspace::Element* element = workspace_->find(id);
    if (element && element->vacNode() && element->vacNode()->isCell()) {
        return element->vacNode()->toCellUnchecked()->toKeyEdge();
    }
    return nullptr;
}

} // namespace detail

namespace {

constexpr bool doHinting = true;
constexpr float thickness = 1.f;
constexpr float squareWidth = 6.f;
constexpr float scaleManipDistance = squareWidth + 2.f;
constexpr float rotateManipDistance = scaleManipDistance + 23.f;
constexpr float sideLengthThreshold = scaleManipDistance * 2.f;

} // namespace

TransformBox::TransformBox(CreateKey key)
    : ui::Widget(key) {

    setFocusPolicy(ui::FocusPolicy::Click);
    setFocusStrength(ui::FocusStrength::High);

    // Enable clipping, so that the box is not drawn outside the canvas.
    //
    setClippingEnabled(true);
    createTranslateStepActions_();
}

TransformBoxPtr TransformBox::create() {
    return core::createObject<TransformBox>();
}

void TransformBox::setElements(const core::Array<core::Id>& elementIds) {
    elementIds_ = elementIds;
    updateFromElements_();
}

void TransformBox::clear() {
    elementIds_.clear();
    hide_();
}

void TransformBox::onParentWidgetChanged(Widget* newParent) {
    canvasTool_ = dynamic_cast<canvas::CanvasTool*>(newParent);
}

void TransformBox::onResize() {
    SuperClass::onResize();
}

void TransformBox::onMouseHover(ui::MouseHoverEvent* event) {
    if (!isVisible_) {
        return;
    }

    if (isBoundingBoxDirty_ && !isTransformActionOngoing_) {
        updateFromElements_();
        isBoundingBoxDirty_ = false;
    }

    // Recompute which mouse actions are available.
    canvas::Canvas* canvas = canvasTool_ ? canvasTool_->canvas() : nullptr;
    if (canvas) {
        computeHoverData_(canvas);
    }

    if (!canvas || isTooSmallForBox_) {
        clearDragActions_();
        cursorChanger_.clear();
        return;
    }

    geometry::Vec2f p = event->position();

    const std::array<geometry::Vec2f, 4>& c = corners_;
    const std::array<geometry::Vec2f, 4>& s = sideVectors_;
    const std::array<geometry::Vec2f, 4>& cn = cornerNormals_;
    const std::array<geometry::Vec2f, 4>& ct = cornerTangents_;
    const std::array<float, 4>& sl = sideLengths_;

    // Corner-Point vectors.
    std::array<geometry::Vec2f, 4> cp = {(p - c[0]), (p - c[1]), (p - c[2]), (p - c[3])};

    // Corner-Point Distances.
    std::array<float, 4> cpd = {
        cp[0].length(), cp[1].length(), cp[2].length(), cp[3].length()};

    // Side-Point Distances.
    std::array<float, 4> spd;
    for (Int i = 0; i < 4; ++i) {
        float d = cpd[i];
        if (sl[i] > hoverTestEpsilon_) {
            geometry::Vec2f sDir = s[i] / sl[i];
            float projectedOnSide = sDir.dot(cp[i]);
            if (projectedOnSide >= 0 && projectedOnSide <= sl[i]) {
                float orthoDist = std::abs(sDir.det(cp[i]));
                if (orthoDist < d) {
                    d = orthoDist;
                }
            }
            float c1Dist = cpd[(i + 1) % 4];
            if (c1Dist < d) {
                d = c1Dist;
            }
        }
        spd[i] = d;
    }

    // Test Corners for scaling manipulator
    std::array<bool, 4> isInCornerScalingManipRadius;
    for (Int i = 0; i < 4; ++i) {
        bool isInScalingManipRadius = cpd[i] < scaleManipDistance;
        if (isCornerManipulatable_[i] && isInScalingManipRadius) {
            Int manipIndex = i * 2;
            setDragActions_(detail::TransformDragActionType::Scale, manipIndex);
            cursorChanger_.set(scalingCursor(cn[i].angle()));
            // Drag action found.
            return;
        }
        isInCornerScalingManipRadius[i] = isInScalingManipRadius;
    }

    // Test Small Sides for scaling manipulator
    for (Int i = 0; i < 4; ++i) {
        if (!sideIsSmall_[i] || spd[i] >= scaleManipDistance) {
            continue;
        }
        Int i0 = (i - 1 + 4) % 4;
        if (sideIsSmall_[i0]) {
            if (cp[i].dot(ct[i]) < 0) {
                continue;
            }
        }
        Int i1 = (i + 1) % 4;
        if (sideIsSmall_[i1]) {
            if (cp[i1].dot(ct[i1]) > 0) {
                continue;
            }
        }
        Int manipIndex = i * 2 + 1;
        setDragActions_(detail::TransformDragActionType::Scale, manipIndex);
        cursorChanger_.set(scalingCursor(sideScaleDirs_[i].angle()));
        // Drag action found.
        return;
    }

    // Test Non-Small Sides for scaling manipulator
    for (Int i = 0; i < 4; ++i) {
        if (sideIsSmall_[i] || spd[i] >= scaleManipDistance) {
            continue;
        }
        if (cp[i].dot(ct[i]) < 0) {
            continue;
        }
        Int i1 = (i + 1) % 4;
        if (cp[i1].dot(ct[i1]) > 0) {
            continue;
        }
        Int manipIndex = i * 2 + 1;
        setDragActions_(detail::TransformDragActionType::Scale, manipIndex);
        cursorChanger_.set(scalingCursor(sideScaleDirs_[i].angle()));
        // Drag action found.
        return;
    }

    // Test Corners for rotation manipulator
    for (Int i = 0; i < 4; ++i) {
        if (cpd[i] >= rotateManipDistance) {
            continue;
        }
        Int i0 = (i - 1 + 4) % 4;
        geometry::Vec2f cpNormalized = cp[i].normalized();
        float det0 = sideScaleDirs_[i].det(sideScaleDirs_[i0]);
        float det1 = sideScaleDirs_[i].det(cpNormalized);
        float dot0 = sideScaleDirs_[i].dot(sideScaleDirs_[i0]);
        float dot1 = sideScaleDirs_[i].dot(cpNormalized);
        if (std::signbit(det0) == std::signbit(det1) && dot1 >= dot0) {
            Int manipIndex = i * 2;
            setDragActions_(detail::TransformDragActionType::Rotate, manipIndex);
            cursorChanger_.set(rotationCursor(cn[i].angle()));
            // Drag action found.
            return;
        }
    }

    if (dragAction_) {
        removeAction(dragAction_);
        dragAction_ = nullptr;
    }
    if (dragAltAction_) {
        removeAction(dragAltAction_);
        dragAltAction_ = nullptr;
    }
    cursorChanger_.clear();
}

void TransformBox::onMouseLeave() {
    cursorChanger_.clear();
}

void TransformBox::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);

    using geometry::Vec2f;

    rectangleGeometry_ = graphics::detail::createRectangleWithScreenSpaceThickness(
        engine, geometry::Rect2f(), 2.f, core::Color());

    // TODO: use this when displacement shader has normal matrix
    //cornersGeometry_ = graphics::detail::createScreenSpaceSquare(
    //    engine, geometry::Vec2f(), 2.f, core::Color());
    for (Int i = 0; i < 4; ++i) {
        cornerGeometry_[i] = engine->createDynamicGeometryView(
            graphics::PrimitiveType::TriangleStrip,
            graphics::BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    }

    pivotCircleGeometry_ = graphics::detail::createCircleWithScreenSpaceThickness(
        engine, 1.f, core::Color(), 15);
    pivotCross0Geometry_ = engine->createDynamicGeometryView(
        graphics::PrimitiveType::TriangleStrip,
        graphics::BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    geometry::Vec2fArray cross0Vertices = {
        Vec2f(0, -2),
        Vec2f(-1, 0),
        Vec2f(0, -2),
        Vec2f(1, 0),
        Vec2f(0, 2),
        Vec2f(-1, 0),
        Vec2f(0, 2),
        Vec2f(1, 0),
    };
    engine->updateBufferData(
        pivotCross0Geometry_->vertexBuffer(0), std::move(cross0Vertices));
    pivotCross1Geometry_ = engine->createDynamicGeometryView(
        graphics::PrimitiveType::TriangleStrip,
        graphics::BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
    geometry::Vec2fArray cross1Vertices = {
        Vec2f(-2, 0),
        Vec2f(0, -1),
        Vec2f(-2, 0),
        Vec2f(0, 1),
        Vec2f(2, 0),
        Vec2f(0, -1),
        Vec2f(2, 0),
        Vec2f(0, 1),
    };
    engine->updateBufferData(
        pivotCross1Geometry_->vertexBuffer(0), std::move(cross1Vertices));
}

void TransformBox::onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) {
    if (!isVisible_) {
        return;
    }

    SuperClass::onPaintDraw(engine, options);

    canvas::Canvas* canvas = canvasTool_ ? canvasTool_->canvas() : nullptr;
    if (!canvas) {
        return;
    }

    using namespace graphics;
    namespace gs = graphics::strings;

    using geometry::Vec2d;
    using geometry::Vec2f;

    // Recompute the bounding box whenever necessary.
    //
    if (isBoundingBoxDirty_) {
        updateFromElements_();
        isBoundingBoxDirty_ = false;
    }

    // Let's check if the rect is not too small.
    const geometry::Mat4d& cameraMatrix = canvas->camera().viewMatrix();

    if (!isTransformActionOngoing_) {
        computeHoverData_(canvas);
    }

    if (isTooSmallForBox_) {
        // Don't paint anything.
        return;
    }

    // TODO: also check the bounding-box view when per-element/group transforms are implemented.
    bool hasRotation = (cameraMatrix * geometry::Vec4d(0, 1, 0, 0)).x() != 0
                       || (cameraMatrix * geometry::Vec4d(1, 0, 0, 0)).y() != 0;
    std::array<Vec2f, 4> corners;
    Vec2f pivot;
    if (isTransformActionOngoing_) {
        for (Int i = 0; i < 4; ++i) {
            Vec2d p = transformActionMatrix_.transformPoint(boundingBox_.corner(i));
            corners[i] = Vec2f(cameraMatrix.transformPoint(p));
        }
        {
            Vec2d p = transformActionMatrix_.transformPoint(pivotPoint_);
            pivot = Vec2f(cameraMatrix.transformPoint(p));
        }
        hasRotation |= (transformActionMatrix_ * geometry::Vec3d(0, 1, 0)).x() != 0
                       || (transformActionMatrix_ * geometry::Vec3d(1, 0, 0)).y() != 0;
    }
    else {
        corners = corners_;
        pivot = Vec2f(cameraMatrix.transformPoint(pivotPoint_));
    }

    if (doHinting && !hasRotation) {
        for (Int i = 0; i < 4; ++i) {
            Vec2f& p = corners[i];
            p[0] = std::round(p[0]);
            p[1] = std::round(p[1]);
        }
    }

    const core::Color& color = workspace::colors::selection;

    // This is a manual update of the rectangle to support scaling/persp transforms.
    // TODO: give normal matrix to displacement shader, implement polygon in shapeutil.
    std::array<Vec2f, 4> transformedSideDirections = {
        (corners[1] - corners[0]).normalized(),
        (corners[2] - corners[1]).normalized(),
        (corners[3] - corners[2]).normalized(),
        (corners[0] - corners[3]).normalized(),
    };
    std::array<Vec2f, 4> transformedSideNormals = {
        transformedSideDirections[0].orthogonalized(),
        transformedSideDirections[1].orthogonalized(),
        transformedSideDirections[2].orthogonalized(),
        transformedSideDirections[3].orthogonalized(),
    };
    bool isClockwise = transformedSideDirections[0].det(transformedSideDirections[1]) > 0;
    if (isClockwise) {
        // Keep normals towards the outside of the polygon.
        for (Int i = 0; i < 4; ++i) {
            transformedSideNormals[i] = -transformedSideNormals[i];
        }
    }
    // XYDxDy
    geometry::Vec2fArray rectangleVertices = {
        corners[0], Vec2f(), corners[0], transformedSideNormals[0],
        corners[1], Vec2f(), corners[1], transformedSideNormals[0],
        corners[1], Vec2f(), corners[1], transformedSideNormals[1],
        corners[2], Vec2f(), corners[2], transformedSideNormals[1],
        corners[2], Vec2f(), corners[2], transformedSideNormals[2],
        corners[3], Vec2f(), corners[3], transformedSideNormals[2],
        corners[3], Vec2f(), corners[3], transformedSideNormals[3],
        corners[0], Vec2f(), corners[0], transformedSideNormals[3],
        corners[0], Vec2f(), corners[0], transformedSideNormals[0],
    };
    engine->updateBufferData(
        rectangleGeometry_->vertexBuffer(0), std::move(rectangleVertices));

    graphics::detail::updateScreenSpaceInstance(
        engine, rectangleGeometry_, Vec2f(), thickness, color);

    core::Color white(1, 1, 1, 1);

    for (Int i = 0; i < 4; ++i) {
        Vec2f p = corners[i];
        Vec2f d0 = transformedSideDirections[(i - 1 + 4) % 4];
        Vec2f d1 = transformedSideDirections[i];
        Vec2f n0 = d0 - d1;
        Vec2f n1 = d0 + d1;
        geometry::Vec2fArray cornerSquareVertices = {
            p, Vec2f(), p, n0,      p, Vec2f(), p, n1,      p, Vec2f(),
            p, -n0,     p, Vec2f(), p, -n1,     p, Vec2f(), p, n0};
        engine->updateBufferData(
            cornerGeometry_[i]->vertexBuffer(0), std::move(cornerSquareVertices));

        core::Array<graphics::detail::ScreenSpaceInstanceData> cornerInstancesData;
        cornerInstancesData.resize(2);
        auto& ci0 = cornerInstancesData[0];
        auto& ci1 = cornerInstancesData[1];
        ci0.color = color;
        ci1.color = white;
        ci0.isRotationEnabled = true;
        ci1.isRotationEnabled = true;
        ci0.displacementScale = squareWidth * 0.5f + thickness;
        ci1.displacementScale = squareWidth * 0.5f;
        engine->updateBufferData(
            cornerGeometry_[i]->vertexBuffer(1), std::move(cornerInstancesData));
    }

    graphics::detail::updateCircleWithScreenSpaceThickness(
        engine, pivotCircleGeometry_, thickness, color);

    // Pivot cross will keep screenspace-axis aligned cross for now.
    // XYDxDy
    graphics::detail::updateScreenSpaceInstance(
        engine, pivotCross0Geometry_, Vec2f(), thickness * 0.5, color);
    graphics::detail::updateScreenSpaceInstance(
        engine, pivotCross1Geometry_, Vec2f(), thickness * 0.5, color);
    geometry::Mat4f scaling = geometry::Mat4f::identity;
    scaling.scale(squareWidth);

    geometry::Mat4f currentView(engine->viewMatrix());
    geometry::Mat4f canvasView(canvas->camera().viewMatrix());

    engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
    engine->pushViewMatrix();

    if (doHinting) {
        pivot[0] = std::round(pivot[0] - 0.5f) + 0.5f;
        pivot[1] = std::round(pivot[1] - 0.5f) + 0.5f;
    }

    geometry::Mat4f pivotView = currentView;
    pivotView.translate(Vec2f(pivot));
    pivotView.scale(squareWidth * 0.6f);
    engine->setViewMatrix(pivotView);
    engine->draw(pivotCircleGeometry_);
    engine->draw(pivotCross0Geometry_);
    engine->draw(pivotCross1Geometry_);

    engine->setViewMatrix(currentView);
    engine->draw(rectangleGeometry_);

    for (Int i = 0; i < 4; ++i) {
        engine->drawInstanced(cornerGeometry_[i]);
    }

    engine->popViewMatrix();
}

void TransformBox::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    rectangleGeometry_.reset();
}

geometry::Vec2f TransformBox::computePreferredSize() const {
    return geometry::Vec2f(0, 0);
}

void TransformBox::computeHoverData_(canvas::Canvas* canvas) {

    using geometry::Vec2f;

    const geometry::Mat4d& cameraMatrix = canvas->camera().viewMatrix();

    // Compute corners_
    for (Int i = 0; i < 4; ++i) {
        Vec2f p(cameraMatrix.transformPoint(boundingBox_.corner(i)));
        corners_[i] = p;
    }

    // Compute sideVectors_ and sideLengths_
    isTooSmallForBox_ = true;
    hoverTestEpsilon_ = 0;
    for (Int i = 0; i < 4; ++i) {
        Vec2f v = corners_[(i + 1) % 4] - corners_[i];
        double l = v.length();
        bool isSmall = l < sideLengthThreshold;
        isTooSmallForBox_ &= isSmall;
        sideVectors_[i] = v;
        sideLengths_[i] = core::narrow_cast<float>(l);
        sideIsSmall_[i] = isSmall;
    }

    hoverTestEpsilon_ =
        10e-6f * (sideLengths_[0] + sideLengths_[1] + sideLengths_[2] + sideLengths_[3]);

    // Compute sideScaleDirs_
    for (Int i = 0; i < 4; ++i) {
        Vec2f a = 0.5 * (corners_[i] + corners_[(i + 1) % 4]);
        Vec2f b = 0.5 * (corners_[(i + 2) % 4] + corners_[(i + 3) % 4]);
        Vec2f c = a - b;
        float l = c.length();
        if (l > hoverTestEpsilon_) {
            sideScaleDirs_[i] = c / l;
        }
        else {
            // If the middle points of this side and the opposite side are too close,
            // we directly orthogonalize the side vector (direction does not matter).
            sideScaleDirs_[i] = sideVectors_[i].orthogonalized() / sideLengths_[i];
        }
    }

    if (isTooSmallForBox_) {
        // Don't update hover data further.
        return;
    }

    // Compute isCornerManipulatable_
    for (Int i = 0; i < 4; ++i) {
        isCornerManipulatable_[i] = !sideIsSmall_[(i - 1 + 4) % 4] && !sideIsSmall_[i];
    }

    // Compute cornerNormals_ and cornerTangents_
    for (Int i = 0; i < 4; ++i) {
        Int i0 = (i - 1 + 4) % 4;
        Int i1 = i;
        if (sideLengths_[i0] <= hoverTestEpsilon_) {
            i0 = (i - 2 + 4) % 4;
        }
        if (sideLengths_[i1] <= hoverTestEpsilon_) {
            i0 = (i + 1) % 4;
        }
        Vec2f t0 = sideVectors_[i0] / sideLengths_[i0];
        Vec2f t1 = sideVectors_[i1] / sideLengths_[i1];
        cornerNormals_[i] = (t0 - t1).normalized();
        cornerTangents_[i] = (t0 + t1).normalized();
    }
}

void TransformBox::onWorkspaceChanged_() {
    if (!isTransformActionOngoing_) {

        // We need to recompute the bounding box whenever the workspace changes
        // and the change is not caused by the TransformBox widget itself.
        //
        // However, we cannot recompute it right now: it might be too early and
        // cause undesired retro-action feedback. For example, if a face with a
        // closed edge as boundary is selected and being dragged, then calling
        // face->boundingBox() now causes the face to update its triangulation
        // based on the old geometry of its closed edge boundary.
        //
        // Therefore, we defer recomputing the bounding box until we actually
        // need to draw it.
        //
        isBoundingBoxDirty_ = true;
    }
}

void TransformBox::updateFromElements_() {

    if (!updateWorkspacePointer_() || elementIds_.isEmpty()) {
        hide_();
    }

    boundingBox_ = geometry::Rect2d::empty;
    pivotPoint_ = {};
    //boundingBoxTransform_ = geometry::Mat3d::identity;

    bool hasContent = false;
    for (core::Id id : elementIds_) {
        workspace::Element* element = workspace_->find(id);
        if (element) {
            // TODO: support layer transforms.
            // should bounding box of elements always be in workspace coords?
            boundingBox_.uniteWith(element->boundingBox());
            hasContent = true;
        }
    }
    // Initialize pivot point to center.
    pivotPoint_ = 0.5 * (boundingBox_.pMin() + boundingBox_.pMax());

    if (hasContent) {
        show_();
    }
    else {
        hide_();
    }
}

void TransformBox::hide_() {
    if (isVisible_) {
        clearDragActions_();
        isVisible_ = false;
        cursorChanger_.clear();
    }
}

void TransformBox::show_() {
    if (!isVisible_) {
        // setup always available actions
        isVisible_ = true;
        requestRepaint();
    }
}

bool TransformBox::updateWorkspacePointer_() {
    workspace::Workspace* oldWorkspace = workspace_.getIfAlive();

    workspace::Workspace* newWorkspace = canvasTool_ ? canvasTool_->workspace() : nullptr;
    if (oldWorkspace != newWorkspace) {
        if (oldWorkspace != nullptr) {
            oldWorkspace->disconnect(this);
        }
        if (newWorkspace != nullptr) {
            newWorkspace->changed().connect(this->onWorkspaceChangedSlot_());
        }
        workspace_ = newWorkspace;
    }
    else {
        // sets it to nullptr if it was no longer alive.
        workspace_ = oldWorkspace;
    }
    return newWorkspace != nullptr;
}

namespace {

template<typename TSlot>
ui::Action*
createTriggerActionWithSlot(ui::Widget* parent, core::StringId commandId, TSlot slot) {
    ui::Action* action = parent->createTriggerAction(commandId);
    action->triggered().connect(slot);
    return action;
}

} // namespace

void TransformBox::setDragActions_(
    detail::TransformDragActionType transformType,
    Int manipIndex) {

    if (dragAction_
        && (dragAction_->transformType() != transformType
            || dragAction_->manipIndex() != manipIndex)) {
        clearDragActions_();
    }
    if (!dragAction_) {
        std::array<core::StringId, 2> commands;
        switch (transformType) {
        case detail::TransformDragActionType::Scale:
            commands = {commands::scaleDrag(), commands::scaleDragWithPivot()};
            break;
        case detail::TransformDragActionType::Rotate:
            commands = {commands::rotateDrag(), commands::rotateDragWithPivot()};
            break;
        }

        dragAction_ = createAction<detail::TransformDragAction>(
            this, commands[0], transformType, manipIndex, false);
        dragAltAction_ = createAction<detail::TransformDragAction>(
            this, commands[1], transformType, manipIndex, true);
    }
}

void TransformBox::clearDragActions_() {
    if (dragAction_) {
        removeAction(dragAction_);
        dragAction_ = nullptr;
    }
    if (dragAltAction_) {
        removeAction(dragAltAction_);
        dragAltAction_ = nullptr;
    }
}

void TransformBox::createTranslateStepActions_() {
    createTriggerActionWithSlot(
        this, commands::translateLeftSmallStep(), onTranslateLeftSmallStepSlot_());
    createTriggerActionWithSlot(
        this, commands::translateRightSmallStep(), onTranslateRightSmallStepSlot_());
    createTriggerActionWithSlot(
        this, commands::translateUpSmallStep(), onTranslateUpSmallStepSlot_());
    createTriggerActionWithSlot(
        this, commands::translateDownSmallStep(), onTranslateDownSmallStepSlot_());
    createTriggerActionWithSlot(
        this, commands::translateLeftBigStep(), onTranslateLeftBigStepSlot_());
    createTriggerActionWithSlot(
        this, commands::translateRightBigStep(), onTranslateRightBigStepSlot_());
    createTriggerActionWithSlot(
        this, commands::translateUpBigStep(), onTranslateUpBigStepSlot_());
    createTriggerActionWithSlot(
        this, commands::translateDownBigStep(), onTranslateDownBigStepSlot_());
}

constexpr double smallTranslateStep = 1.0;
constexpr double bigTranslateStep = 20.0;

void TransformBox::onTranslateLeftSmallStep_() {
    onTranslate_(detail::TranslateStepDirection::Left, smallTranslateStep);
}

void TransformBox::onTranslateRightSmallStep_() {
    onTranslate_(detail::TranslateStepDirection::Right, smallTranslateStep);
}

void TransformBox::onTranslateUpSmallStep_() {
    onTranslate_(detail::TranslateStepDirection::Up, smallTranslateStep);
}

void TransformBox::onTranslateDownSmallStep_() {
    onTranslate_(detail::TranslateStepDirection::Down, smallTranslateStep);
}

void TransformBox::onTranslateLeftBigStep_() {
    onTranslate_(detail::TranslateStepDirection::Left, bigTranslateStep);
}

void TransformBox::onTranslateRightBigStep_() {
    onTranslate_(detail::TranslateStepDirection::Right, bigTranslateStep);
}

void TransformBox::onTranslateUpBigStep_() {
    onTranslate_(detail::TranslateStepDirection::Up, bigTranslateStep);
}

void TransformBox::onTranslateDownBigStep_() {
    onTranslate_(detail::TranslateStepDirection::Down, bigTranslateStep);
}

void TransformBox::onTranslate_(detail::TranslateStepDirection direction, double size) {
    if (!updateWorkspacePointer_()) {
        return;
    }

    canvas::Canvas* canvas = canvasTool_->canvas();
    if (!canvas) {
        return;
    }

    ui::Action* action = dynamic_cast<ui::Action*>(core::detail::currentEmitter());
    if (!action) {
        return;
    }

    core::StringId groupId = action->commandId();

    // Open history group
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = workspace_->history();
    if (history) {
        undoGroup = history->createUndoGroup(groupId);
    }

    // Do operation
    geometry::Vec2d unitDelta = {};
    switch (direction) {
    case detail::TranslateStepDirection::Left: {
        unitDelta.setX(-1);
        break;
    }
    case detail::TranslateStepDirection::Right: {
        unitDelta.setX(1);
        break;
    }
    case detail::TranslateStepDirection::Up: {
        unitDelta.setY(-1);
        break;
    }
    case detail::TranslateStepDirection::Down: {
        unitDelta.setY(1);
        break;
    }
    }

    const geometry::Mat4d& cameraMatrix = canvas->camera().viewMatrix();
    geometry::Mat4d invCameraMatrix = cameraMatrix.inverted();
    geometry::Vec2d p0 = pivotPoint_; // or center
    geometry::Vec2d p0c = cameraMatrix.transformPoint(p0);
    geometry::Vec2d p1c = cameraMatrix.transformPoint(p0 + unitDelta);
    geometry::Vec2d dir = (p1c - p0c).normalized();
    geometry::Vec2d p1 = invCameraMatrix.transformPoint(p0c + dir * size);
    geometry::Vec2d delta = p1 - p0;

    detail::TopologyAwareTransformer transformer;
    transformer.setElements(workspace_, elementIds_);
    transformer.transform(delta);

    // Close history group
    if (undoGroup) {
        bool amend = undoGroup->parent() && undoGroup->parent()->name() == groupId;
        undoGroup->close(amend);
    }
}

} // namespace vgc::tools
