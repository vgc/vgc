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

#include <vgc/ui/paintbuckettool.h>

#include <vgc/ui/column.h>

namespace vgc::ui {

PaintBucketTool::PaintBucketTool()
    : CanvasTool() {
}

PaintBucketToolPtr PaintBucketTool::create() {
    return PaintBucketToolPtr(new PaintBucketTool());
}

ui::WidgetPtr PaintBucketTool::createOptionsWidget() const {
    ui::WidgetPtr res = ui::Column::create();
    return res;
}

bool PaintBucketTool::onMouseMove(MouseEvent* event) {

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        clearFaceCandidate_();
        return false;
    }

    // Convert mouse position from view to world coords.
    // TODO: Have a helper function in Canvas for this.
    //
    geometry::Vec2f mousePosf = event->position();
    geometry::Vec2d mousePos = geometry::Vec2d(mousePosf.x(), mousePosf.y());
    geometry::Vec2d viewCoords = mousePos;
    geometry::Vec2d worldCoords =
        canvas->camera().viewMatrix().inverted().transformPointAffine(viewCoords);

    // Compute the key face candidate for the current mouse position.
    //
    bool hadFaceCandidate = hasFaceCandidate_();
    updateFaceCandidate_(worldCoords);
    bool hasFaceCandidate = hasFaceCandidate_();

    // Determine whether the face candidate changed. For now, we just assume
    // it always changes, unless there was no candidate before and there is
    // still no candidate now.
    //
    bool faceCandidateChanged = hasFaceCandidate || hadFaceCandidate;

    // Request a repaint if the face candidate changed.
    //
    if (faceCandidateChanged) {
        requestRepaint();
        return true;
    }
    else {
        return false;
    }
}

bool PaintBucketTool::onMousePress(MouseEvent* event) {

    if (event->button() == MouseButton::Left && hasFaceCandidate_()) {

        // Get workspace and history
        workspace::Workspace* workspace_ = workspace();
        if (!workspace_) {
            VGC_WARNING(
                LogVgcToolsPaintBucket, "Workspace not found: cannot create face.");
            return false;
        }
        core::History* history = workspace_->history();

        // Open undo group if history is enabled
        static core::StringId operationName("Create Face with Paint Bucket");
        core::UndoGroup* undoGroup = nullptr;
        if (history) {
            undoGroup = history->createUndoGroup(operationName);
        }

        // Find the parent group under which to create the new face.
        // Note: we know that paintCandidateCycles_ is non-empty.
        //
        const vacomplex::KeyCycle& anyCycle = faceCandidateCycles_[0];
        if (!anyCycle.isValid()) {
            // This shouldn't happen since computeKeyFaceCandidateAt() is not
            // supposed to return invalid cycles, but we double-check anyway.
            VGC_WARNING(LogVgcToolsPaintBucket, "Invalid cycle: cannot create face.");
            clearFaceCandidate_();
            return false;
        }
        vacomplex::Cell* anyCell = anyCycle.steinerVertex();
        if (!anyCell) {
            VGC_ASSERT(!anyCycle.halfedges().isEmpty());
            anyCell = anyCycle.halfedges()[0].edge();
        }
        VGC_ASSERT(anyCell);
        vacomplex::Group* parentGroup = anyCell->parentGroup();

        // Create the face
        vacomplex::KeyFace* kf = topology::ops::createKeyFace(
            faceCandidateCycles_, parentGroup, parentGroup->firstChild());
        workspace::VacElement* element = workspace()->findVacElement(kf);
        core::Color faceColor(1, 0, 0); // TODO: use current tool color
        element->domElement()->setAttribute(dom::strings::color, faceColor);

        // Close the undo group. This requires syncing the DOM from the VAC.
        workspace_->sync();
        if (undoGroup) {
            undoGroup->close();
        }

        clearFaceCandidate_();
        return true;
    }

    return false;
}

bool PaintBucketTool::onMouseRelease(MouseEvent* /*event*/) {
    // TODO
    return false;
}

void PaintBucketTool::onPaintCreate(graphics::Engine* engine) {
    using Layout = graphics::BuiltinGeometryLayout;
    SuperClass::onPaintCreate(engine);
    faceCandidateFillGeometry_ = engine->createDynamicTriangleListView(Layout::XY_iRGBA);
}

void PaintBucketTool::onPaintDraw(graphics::Engine* engine, PaintOptions options) {
    SuperClass::onPaintDraw(engine, options);

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    // TODO: use current tool color?
    core::Color facePreviewColor(1, 0, 0);

    if (hasFaceCandidate_() && faceCandidateFillGeometry_) {
        if (!faceCandidatePendingTriangles_.isEmpty()) {
            engine->updateBufferData(
                faceCandidateFillGeometry_->vertexBuffer(0), //
                faceCandidatePendingTriangles_);
            engine->updateBufferData(
                faceCandidateFillGeometry_->vertexBuffer(1), //
                core::Array<float>(
                    {facePreviewColor.r(),
                     facePreviewColor.g(),
                     facePreviewColor.b(),
                     1}));
        }

        // TODO: setting up the view matrix should be done by Canvas.
        engine->pushProgram(graphics::BuiltinProgram::SimplePreview);
        geometry::Mat4f vm = engine->viewMatrix();
        geometry::Mat4f cameraViewf(canvas->camera().viewMatrix());
        engine->pushViewMatrix(vm * cameraViewf);
        engine->draw(faceCandidateFillGeometry_);
        engine->popViewMatrix();
        engine->popProgram();
    }
}

void PaintBucketTool::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    faceCandidateFillGeometry_.reset();
}

void PaintBucketTool::clearFaceCandidate_() {
    if (!faceCandidateCycles_.isEmpty()) {
        faceCandidatePendingTriangles_.clear();
        faceCandidateCycles_.clear();
        requestRepaint();
    }
}

void PaintBucketTool::updateFaceCandidate_(const geometry::Vec2d& worldPosition) {
    faceCandidateCycles_ = topology::detail::computeKeyFaceCandidateAt(
        worldPosition, workspace()->vac()->rootGroup(), faceCandidatePendingTriangles_);
}

/*
// temporary method to test color change of element
void PaintBucketTool::onColorChanged_(const core::Color& color) {
    //paintColor_ = color;
    workspace::Element* element = selectedElement_();
    core::History* history = workspace()->history();
    if (element && element->domElement()) {
        core::UndoGroup* ug = nullptr;
        static core::StringId Change_Color("Change Color");
        if (history) {
            ug = history->createUndoGroup(Change_Color);
        }
        element->domElement()->setAttribute(dom::strings::color, color);
        if (ug) {
            ug->close(
                canMergeColorChange_ && ug->parent()
                && ug->parent()->name() == Change_Color);
            canMergeColorChange_ = true;
        }
        workspace()->sync();
    }
}
*/

/*
void PaintBucketTool::onDocumentChanged_(const dom::Diff& diff) {
    clearPaintCandidate_();
}
*/

} // namespace vgc::ui
