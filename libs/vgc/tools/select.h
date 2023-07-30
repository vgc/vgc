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

#ifndef VGC_TOOLS_SELECT_H
#define VGC_TOOLS_SELECT_H

#include <vgc/canvas/canvastool.h>
#include <vgc/core/array.h>
#include <vgc/core/id.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/tools/api.h>
#include <vgc/tools/transform.h>
#include <vgc/vacomplex/edgegeometry.h>

namespace vgc::tools {

VGC_DECLARE_OBJECT(Select);

/// \class vgc::tools::SelectTool
/// \brief A CanvasTool that implements selecting strokes.
///
class VGC_TOOLS_API Select : public canvas::CanvasTool {
private:
    VGC_OBJECT(Select, canvas::CanvasTool)

protected:
    /// This is an implementation details.
    /// Please use `SketchTool::create()` instead.
    ///
    Select(CreateKey);

public:
    /// Creates a `SelectTool`.
    ///
    static SelectPtr create();

protected:
    // Reimplementation of CanvasTool virtual methods
    ui::WidgetPtr createOptionsWidget() const override;

    // Reimplementation of Widget virtual methods
    bool onMouseMove(ui::MouseMoveEvent* event) override;
    bool onMousePress(ui::MousePressEvent* event) override;
    bool onMouseRelease(ui::MouseReleaseEvent* event) override;

    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    void updateChildrenGeometry() override;

private:
    enum class SelectionMode {
        New,
        Add,
        Remove,
        Toggle
    };
    enum class DragAction {
        Select,
        TranslateSelection,
        TranslateCandidate,
    };

    core::Array<canvas::SelectionCandidate> candidates_;
    core::Array<core::Id> rectCandidates_;
    core::Array<core::Id> selectionAtPress_;
    geometry::Vec2f cursorPositionAtPress_;
    geometry::Vec2f cursorPosition_;
    double timeAtPress_ = 0;
    bool isInAction_ = false;
    bool isDragging_ = false;
    bool canAmendUndoGroup_ = false;
    DragAction dragAction_ = {};
    SelectionMode selectionMode_ = {};
    bool isAlternativeMode_ = false;
    core::Id lastSelectedId_ = -1;
    core::Id lastDeselectedId_ = -1;
    graphics::GeometryViewPtr selectionRectangleGeometry_;
    geometry::Vec2d deltaInWorkspace_ = {};
    TransformBoxPtr transformBox_ = {};

    // drag-move data
    struct KeyVertexDragData {
        core::Id elementId;
        geometry::Vec2d position;
    };
    struct KeyEdgeDragData {
        core::Id elementId;
        bool isUniformTranslation;
        mutable bool isEditStarted = false;
    };

    core::Array<KeyVertexDragData> draggedVertices_;
    core::Array<KeyEdgeDragData> draggedEdges_;

    // assumes workspace is not null
    void initializeDragMoveData_(
        workspace::Workspace* workspace,
        const core::Array<core::Id>& elementsIds);

    // assumes workspace is not null
    void updateDragMovedElements_(
        workspace::Workspace* workspace,
        const geometry::Vec2d& translationInWorkspace);

    // assumes workspace is not null
    void finalizeDragMovedElements_(workspace::Workspace* workspace);

    void resetActionState_();

    canvas::Canvas* connectedCanvas_ = nullptr;
    void disconnectCanvas_();

    void onCanvasChanged_();
    VGC_SLOT(onCanvasChangedSlot_, onCanvasChanged_)

    void onCanvasAboutToBeDestroyed_();
    VGC_SLOT(onCanvasAboutToBeDestroyedSlot_, onCanvasAboutToBeDestroyed_)

    void onSelectionChanged_();
    VGC_SLOT(onSelectionChangedSlot_, onSelectionChanged_)

    void onShowTransformBoxChanged_();
    VGC_SLOT(onShowTransformBoxChangedSlot_, onShowTransformBoxChanged_)

    void updateTransformBoxElements_();

    void onGlue_();
    VGC_SLOT(onGlueSlot_, onGlue_)

    void onUnglue_();
    VGC_SLOT(onUnglueSlot_, onUnglue_)
};

} // namespace vgc::tools

#endif // VGC_TOOLS_SELECT_H
