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
        clearPaintCandidate_();
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
    bool hadPaintCandidate = hasPaintCandidate_();
    updateFaceCandidate_(worldCoords);
    bool hasPaintCandidate = hasPaintCandidate_();

    // Determine whether the paint candidate changed. For now, we just assume
    // it always changes, unless there was no candidate before and there is
    // still no candidate now.
    //
    bool paintCandidateChanged = hasPaintCandidate || hadPaintCandidate;

    // Request a repaint if the candidate changed.
    //
    if (paintCandidateChanged) {
        requestRepaint();
        return true;
    }
    else {
        return false;
    }
}

bool PaintBucketTool::onMousePress(MouseEvent* /*event*/) {
    // TODO

    /*
    if (isBucketPainting_ && event->button() == MouseButton::Left
        && !paintCandidateCycles_.isEmpty()) {

        core::History* history = workspace()->history();
        static core::StringId Paint_Face("Paint Face");
        core::UndoGroup* ug = nullptr;
        if (history) {
            ug = history->createUndoGroup(Paint_Face);
        }

        vacomplex::Group* group =
            paintCandidateCycles_[0].halfedges().first().edge()->parentGroup();
        vacomplex::KeyFace* kf = topology::ops::createKeyFace(
            paintCandidateCycles_, group, group->firstChild());
        workspace::VacElement* element = workspace()->findVacElement(kf);
        element->domElement()->setAttribute(dom::strings::color, paintColor_);

        workspace()->sync();
        if (ug) {
            ug->close();
        }

        clearPaintCandidate_();
        event->stopPropagation();
    }
*/

    /*
    else if (event->button() == MouseButton::Right) {
        isBucketPainting_ = true;
        doBucketPaintTest_(mousePos);
        return true;
    }
*/

    return false;
}

bool PaintBucketTool::onMouseRelease(MouseEvent* /*event*/) {
    // TODO
    return false;
}

void PaintBucketTool::onPaintCreate(graphics::Engine* engine) {
    using Layout = graphics::BuiltinGeometryLayout;
    SuperClass::onPaintCreate(engine);
    paintCandidateFillGeometry_ = engine->createDynamicTriangleListView(Layout::XY_iRGBA);
}

void PaintBucketTool::onPaintDraw(graphics::Engine* engine, PaintOptions options) {
    SuperClass::onPaintDraw(engine, options);

    ui::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    if (hasPaintCandidate_() && paintCandidateFillGeometry_) {
        if (!paintCandidatePendingTriangles_.isEmpty()) {
            engine->updateBufferData(
                paintCandidateFillGeometry_->vertexBuffer(0), //
                paintCandidatePendingTriangles_);
            engine->updateBufferData(
                paintCandidateFillGeometry_->vertexBuffer(1), //
                core::Array<float>({1, 0, 0, 1}));
        }

        // TODO: setting up the view matrix should be done by Canvas.
        engine->pushProgram(graphics::BuiltinProgram::SimplePreview);
        geometry::Mat4f vm = engine->viewMatrix();
        geometry::Mat4f cameraViewf(canvas->camera().viewMatrix());
        engine->pushViewMatrix(vm * cameraViewf);
        engine->draw(paintCandidateFillGeometry_);
        engine->popViewMatrix();
        engine->popProgram();
    }
}

void PaintBucketTool::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
    paintCandidateFillGeometry_.reset();
}

void PaintBucketTool::clearPaintCandidate_() {
    if (!paintCandidateCycles_.isEmpty()) {
        paintCandidatePendingTriangles_.clear();
        paintCandidateCycles_.clear();
        requestRepaint();
    }
}

void PaintBucketTool::updateFaceCandidate_(const geometry::Vec2d& worldPosition) {
    paintCandidateCycles_ = topology::detail::computeKeyFaceCandidateAt(
        worldPosition, workspace()->vac()->rootGroup(), paintCandidatePendingTriangles_);
}
/*
// temporary method to test color change of element
void Canvas::onColorChanged_(const core::Color& color) {
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
void Canvas::onDocumentChanged_(const dom::Diff& diff) {
    clearPaintCandidate_();
}
*/

} // namespace vgc::ui
