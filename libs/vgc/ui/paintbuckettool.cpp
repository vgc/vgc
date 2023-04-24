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

bool PaintBucketTool::onMouseMove(MouseEvent* /*event*/) {
    // TODO
    /*
    else if (isBucketPainting_) {
        doBucketPaintTest_(mousePos);
        return true;
    }
    */
    return false;
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
    SuperClass::onPaintCreate(engine);

    /*
    paintCandidateFillGeometry_ =
        engine->createDynamicTriangleListView(BuiltinGeometryLayout::XY_iRGBA);
    */
}

void PaintBucketTool::onPaintDraw(graphics::Engine* engine, PaintOptions options) {
    SuperClass::onPaintDraw(engine, options);

    /*
    if (hasPaintCandidate_ && paintCandidateFillGeometry_) {
        if (!paintCandidatePendingTriangles_.isEmpty()) {
            engine->updateBufferData(
                paintCandidateFillGeometry_->vertexBuffer(0), //
                paintCandidatePendingTriangles_);
            engine->updateBufferData(
                paintCandidateFillGeometry_->vertexBuffer(1), //
                core::Array<float>({1, 0, 0, 1}));
        }
        engine->setProgram(graphics::BuiltinProgram::SimplePreview);
        engine->draw(paintCandidateFillGeometry_);
    }
*/
}

void PaintBucketTool::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);

    /*
    paintCandidateFillGeometry_.reset();
    */
}

/*
void Canvas::clearPaintCandidate_() {
    if (!paintCandidateCycles_.isEmpty()) {
        paintCandidatePendingTriangles_.clear();
        hasPaintCandidate_ = false;
        paintCandidateCycles_.clear();
        requestRepaint();
    }
}
*/

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

/*
void Canvas::doBucketPaintTest_(const geometry::Vec2d& mousePos) {
    clearSelection_();
    geometry::Vec2d worldCoords =
        camera_.viewMatrix().inverted().transformPointAffine(mousePos);
    paintCandidateCycles_ = topology::detail::computeKeyFaceCandidateAt(
        worldCoords, workspace()->vac()->rootGroup(), paintCandidatePendingTriangles_);
    bool hadPaintCandidate = hasPaintCandidate_;
    hasPaintCandidate_ = !paintCandidateCycles_.isEmpty();
    if (hasPaintCandidate_ || hadPaintCandidate) {
        requestRepaint();
    }
}
*/

} // namespace vgc::ui
