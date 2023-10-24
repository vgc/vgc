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

#include <vgc/tools/paintbucket.h>

#include <vgc/core/history.h>
#include <vgc/dom/strings.h>
#include <vgc/tools/logcategories.h>
#include <vgc/ui/column.h>
#include <vgc/vacomplex/operations.h>
#include <vgc/workspace/element.h>

namespace vgc::tools {

PaintBucket::PaintBucket(CreateKey key)
    : CanvasTool(key) {
}

PaintBucketPtr PaintBucket::create() {
    return core::createObject<PaintBucket>();
}

void PaintBucket::setColor(const core::Color& color) {
    color_ = color;
    if (hasFaceCandidate_()) {
        isFaceCandidateGraphicsDirty_ = true;
        requestRepaint();
    }
}

ui::WidgetPtr PaintBucket::doCreateOptionsWidget() const {
    ui::WidgetPtr res = ui::Column::create();
    return res;
}

void PaintBucket::onMouseHover(ui::MouseHoverEvent* event) {

    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        clearFaceCandidate_();
        return;
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
        isFaceCandidateGraphicsDirty_ = true;
        requestRepaint();
    }
}

bool PaintBucket::onMouseMove(ui::MouseMoveEvent*) {
    return false;
}

bool PaintBucket::onMousePress(ui::MousePressEvent* event) {

    ui::ModifierKeys keys = event->modifierKeys();
    ui::MouseButton button = event->button();

    if (keys == ui::ModifierKey::None && button == ui::MouseButton::Left
        && hasFaceCandidate_()) {

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

        // Create the face and move it just below its boundary
        vacomplex::KeyFace* face =
            vacomplex::ops::createKeyFace(faceCandidateCycles_, parentGroup, nullptr);
        vacomplex::ops::moveBelowBoundary(face);

        // Set the color of the face
        workspace::VacElement* workspaceFace = workspace_->findVacElement(face);
        dom::Element* domFace = workspaceFace ? workspaceFace->domElement() : nullptr;
        if (domFace) {
            domFace->setAttribute(dom::strings::color, color());
        }
        workspace_->sync();

        // Close the undo group
        if (undoGroup) {
            undoGroup->close();
        }

        clearFaceCandidate_();
        return true;
    }

    return false;
}

bool PaintBucket::onMouseRelease(ui::MouseReleaseEvent* /*event*/) {
    // TODO
    return false;
}

void PaintBucket::onMouseEnter() {
}

void PaintBucket::onMouseLeave() {
    clearFaceCandidate_();
}

void PaintBucket::onPaintCreate(graphics::Engine* engine) {
    using Layout = graphics::BuiltinGeometryLayout;
    SuperClass::onPaintCreate(engine);
    faceCandidateFillGeometry_ = engine->createDynamicTriangleListView(Layout::XY_iRGBA);
}

void PaintBucket::onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) {
    SuperClass::onPaintDraw(engine, options);

    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    if (hasFaceCandidate_() && faceCandidateFillGeometry_) {
        core::Color c = color();
        if (isFaceCandidateGraphicsDirty_) {
            engine->updateBufferData(
                faceCandidateFillGeometry_->vertexBuffer(0), //
                faceCandidateTriangles_);
            engine->updateBufferData(
                faceCandidateFillGeometry_->vertexBuffer(1), //
                core::Array<float>({c.r(), c.g(), c.b(), 1}));
            isFaceCandidateGraphicsDirty_ = false;
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

void PaintBucket::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    faceCandidateFillGeometry_.reset();
}

void PaintBucket::clearFaceCandidate_() {
    if (hasFaceCandidate_()) {
        faceCandidateTriangles_.clear();
        faceCandidateCycles_.clear();
        requestRepaint();
    }
}

void PaintBucket::updateFaceCandidate_(const geometry::Vec2d& worldPosition) {

    // Fast return if no workspace
    workspace::Workspace* workspace = this->workspace();
    if (!workspace) {
        clearFaceCandidate_();
        return;
    }

    // Compute face candidate at given world position
    faceCandidateCycles_ = vacomplex::detail::computeKeyFaceCandidateAt(
        worldPosition, workspace->vac()->rootGroup(), faceCandidateTriangles_);

    // Clear face candidate if document or workspace changes
    workspace->document()->changed().connect(clearFaceCandidateSlot_());
    canvas()->workspaceReplaced().connect(clearFaceCandidateSlot_());
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

} // namespace vgc::tools
