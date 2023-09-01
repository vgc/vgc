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

#ifndef VGC_TOOLS_TRANSFORM_H
#define VGC_TOOLS_TRANSFORM_H

#include <array>
#include <string_view>

#include <vgc/canvas/canvas.h>
#include <vgc/canvas/canvastool.h>
#include <vgc/core/array.h>
#include <vgc/core/id.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/tools/api.h>
#include <vgc/ui/cursor.h>
#include <vgc/vacomplex/keyedgedata.h>
#include <vgc/workspace/workspace.h>

namespace vgc::tools {

namespace detail {

class TransformDragAction;

enum class TransformDragActionType {
    //Translate,
    Scale,
    Rotate
};

enum class TranslateStepDirection {
    Left,
    Right,
    Up,
    Down
};

struct VGC_TOOLS_API KeyVertexTransformData {
    core::Id elementId;
    geometry::Vec2d originalPosition;
};

struct VGC_TOOLS_API KeyEdgeTransformData {
    core::Id elementId;
    std::unique_ptr<vacomplex::KeyEdgeData> oldData;
};

class VGC_TOOLS_API TopologyAwareTransformer {
public:
    TopologyAwareTransformer() noexcept = default;
    ~TopologyAwareTransformer();

    TopologyAwareTransformer(const TopologyAwareTransformer&) = delete;
    TopologyAwareTransformer& operator=(const TopologyAwareTransformer&) = delete;

    void setElements(
        const workspace::WorkspacePtr& workspace,
        const core::Array<core::Id>& elementIds);

    void clear();

    workspace::Workspace* workspace() const {
        return workspace_.getIfAlive();
    }

    void transform(const geometry::Mat3d& transform);
    void transform(const geometry::Vec2d& translation);

    void startDragTransform();
    void updateDragTransform(const geometry::Mat3d& transform);
    void updateDragTransform(const geometry::Vec2d& translation);
    void finalizeDragTransform();
    void cancelDragTransform();

private:
    workspace::WorkspacePtr workspace_;
    core::Array<KeyVertexTransformData> vertices_;
    core::Array<KeyEdgeTransformData> edges_;
    core::Array<KeyEdgeTransformData> edgesToSnap_;

    bool isDragTransforming_ = false;

    vacomplex::KeyVertex* findKeyVertex_(core::Id id);
    vacomplex::KeyEdge* findKeyEdge_(core::Id id);
};

} // namespace detail

VGC_DECLARE_OBJECT(TransformBox);

/// \class vgc::tools::TransformBox
/// \brief A widget for a transform box.
///
class VGC_TOOLS_API TransformBox : public ui::Widget {
private:
    VGC_OBJECT(TransformBox, ui::Widget)

protected:
    /// This is an implementation details. Please use
    /// TransformBox::create() instead.
    ///
    TransformBox(CreateKey);

public:
    /// Creates a TransformBox.
    ///
    static TransformBoxPtr create();

    const core::Array<core::Id>& elements() const {
        return elementIds_;
    }

    void setElements(const core::Array<core::Id>& elementIds);

    void clear();

protected:
    // Reimplementation of Widget virtual methods
    void onParentWidgetChanged(ui::Widget* newParent) override;
    void onResize() override;
    void onMouseHover(ui::MouseHoverEvent* event) override;
    void onMouseLeave() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    geometry::Vec2f computePreferredSize() const override;

private:
    friend detail::TransformDragAction;

    canvas::CanvasTool* canvasTool_ = {};
    // we assume that the workspace will not change.
    // if we support that later, we could use a signal/slot.
    workspace::WorkspacePtr workspace_ = {};

    core::Array<core::Id> elementIds_;

    geometry::Rect2d boundingBox_;
    geometry::Vec2d pivotPoint_;
    //geometry::Mat3d boundingSpaceMatrix_;

    std::array<geometry::Vec2f, 4> corners_;
    std::array<geometry::Vec2f, 4> sideVectors_;
    std::array<geometry::Vec2f, 4> sideScaleDirs_;
    std::array<geometry::Vec2f, 4> cornerNormals_;
    std::array<geometry::Vec2f, 4> cornerTangents_;
    std::array<float, 4> sideLengths_;
    std::array<bool, 4> isCornerManipulatable_;
    std::array<bool, 4> sideIsSmall_;
    float hoverTestEpsilon_;

    bool isTooSmallForBox_ = false;

    bool isBoundingBoxDirty_ = true;
    void computeHoverData_(canvas::Canvas* canvas);

    detail::TransformDragAction* dragAction_ = nullptr;
    detail::TransformDragAction* dragAltAction_ = nullptr;
    bool isTransformActionOngoing_ = false;
    geometry::Mat3d transformActionMatrix_;

    graphics::GeometryViewPtr rectangleGeometry_;
    // TODO: use this when displacement shader uses normal matrix
    //graphics::GeometryViewPtr cornersGeometry_;
    std::array<graphics::GeometryViewPtr, 4> cornerGeometry_;
    graphics::GeometryViewPtr pivotCircleGeometry_;
    graphics::GeometryViewPtr pivotCross0Geometry_;
    graphics::GeometryViewPtr pivotCross1Geometry_;

    ui::CursorChanger cursorChanger_;

    void onWorkspaceChanged_();
    VGC_SLOT(onWorkspaceChangedSlot_, onWorkspaceChanged_);

    void updateFromElements_();

    bool isVisible_ = false;
    void hide_();
    void show_();

    bool updateWorkspacePointer_();

    void setDragActions_(detail::TransformDragActionType transformType, Int manipIndex);
    void clearDragActions_();

    void createTranslateStepActions_();

    void onTranslateLeftSmallStep_();
    VGC_SLOT(onTranslateLeftSmallStepSlot_, onTranslateLeftSmallStep_);
    void onTranslateRightSmallStep_();
    VGC_SLOT(onTranslateRightSmallStepSlot_, onTranslateRightSmallStep_);
    void onTranslateUpSmallStep_();
    VGC_SLOT(onTranslateUpSmallStepSlot_, onTranslateUpSmallStep_);
    void onTranslateDownSmallStep_();
    VGC_SLOT(onTranslateDownSmallStepSlot_, onTranslateDownSmallStep_);
    void onTranslateLeftBigStep_();
    VGC_SLOT(onTranslateLeftBigStepSlot_, onTranslateLeftBigStep_);
    void onTranslateRightBigStep_();
    VGC_SLOT(onTranslateRightBigStepSlot_, onTranslateRightBigStep_);
    void onTranslateUpBigStep_();
    VGC_SLOT(onTranslateUpBigStepSlot_, onTranslateUpBigStep_);
    void onTranslateDownBigStep_();
    VGC_SLOT(onTranslateDownBigStepSlot_, onTranslateDownBigStep_);

    void onTranslate_(detail::TranslateStepDirection direction, double size);
};

} // namespace vgc::tools

#endif // VGC_TOOLS_TRANSFORM_H
