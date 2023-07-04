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

#include <vgc/tools/sculpt.h>

#include <vgc/canvas/canvas.h>
#include <vgc/geometry/mat4d.h>
#include <vgc/geometry/mat4f.h>
#include <vgc/graphics/detail/shapeutil.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/column.h>
#include <vgc/ui/numbersettingedit.h>
#include <vgc/workspace/colors.h>

namespace vgc::tools {

namespace {

namespace options {

ui::NumberSetting* sculptRadius() {
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::session(), "tools.sculpt.radius", "Sculpt Radius", 20, 0, 1000);
    return setting.get();
}

} // namespace options

namespace commands {

using ui::ModifierKey;
using ui::MouseButton;
using ui::Shortcut;

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    grab,
    "tools.sculpt.grab",
    "Sculpt Grab",
    MouseButton::Left)

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    smooth,
    "tools.sculpt.smooth",
    "Sculpt Smooth",
    Shortcut(ModifierKey::Shift, MouseButton::Left))

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    radius,
    "tools.sculpt.radius",
    "Sculpt Radius",
    Shortcut(ModifierKey::Alt, MouseButton::Left))

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    editSculptRadius,
    "tools.sculpt.editSculptRadius",
    "Edit Sculpt Radius",
    Shortcut(ModifierKey::Ctrl, MouseButton::Left))

} // namespace commands

VGC_DECLARE_OBJECT(SculptGrabAction);

class SculptGrabAction : public ui::Action {
private:
    VGC_OBJECT(SculptGrabAction, ui::Action)

protected:
    /// This is an implementation details.
    /// Please use `SculptGrabAction::create()` instead.
    ///
    SculptGrabAction()
        : ui::Action(commands::grab) {
    }

public:
    /// Creates a `SculptGrabAction`.
    ///
    static SculptGrabActionPtr create() {
        return SculptGrabActionPtr(new SculptGrabAction());
    }

public:
    void onMouseDragStart(ui::MouseEvent* event) override {
        cursorPositionAtPress_ = event->position();
        edgeId_ = tool_->candidateId();
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        cursorPosition_ = event->position();

        geometry::Mat4d inverseViewMatrix = canvas->camera().viewMatrix().inverted();

        float pixelSize = static_cast<float>(
            (inverseViewMatrix.transformPointAffine(geometry::Vec2d(0, 1))
             - inverseViewMatrix.transformPointAffine(geometry::Vec2d(0, 0)))
                .length());

        geometry::Vec2d cursorPositionInWorkspace =
            inverseViewMatrix.transformPointAffine(geometry::Vec2d(cursorPosition_));
        geometry::Vec2d cursorPositionInWorkspaceAtPress =
            inverseViewMatrix.transformPointAffine(
                geometry::Vec2d(cursorPositionAtPress_));

        // Open history group
        core::UndoGroup* undoGroup = nullptr;
        core::History* history = workspace->history();
        if (history) {
            undoGroup = history->createUndoGroup(actionName());
        }

        // Translate Vertices
        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    if (started_) {
                        geometry->resetEdit();
                    }
                    else {
                        geometry->startEdit();
                        started_ = true;
                    }
                    geometry::Vec2d grabbedPoint = geometry->sculptGrab(
                        cursorPositionInWorkspaceAtPress,
                        cursorPositionInWorkspace,
                        options::sculptRadius()->value(),
                        1,
                        pixelSize,
                        ke->isClosed());
                    tool_->setActionCircleCenter(grabbedPoint);
                    tool_->setActionCircleEnabled(true);
                }
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            undoGroup->close(amend);
            canAmendUndoGroup_ = true;
        }
    }

    void onMouseDragConfirm(ui::MouseEvent* /*event*/) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        // Open history group
        core::UndoGroup* undoGroup = nullptr;
        core::History* history = workspace->history();
        if (history) {
            undoGroup = history->createUndoGroup(actionName());
        }

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    geometry->finishEdit();
                }
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            undoGroup->close(amend);
            canAmendUndoGroup_ = true;
        }

        tool_->setActionCircleEnabled(false);
        reset_();
    }

    void onMouseDragCancel(ui::MouseEvent* /*event*/) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    geometry->resetEdit();
                    geometry->finishEdit();
                }
            }
        }

        tool_->setActionCircleEnabled(false);
        reset_();
    }

    void reset_() {
        canAmendUndoGroup_ = false;
        started_ = false;
        edgeId_ = -1;
    }

public:
    Sculpt* tool_ = nullptr;
    bool canAmendUndoGroup_ = false;
    bool started_ = false;
    core::Id edgeId_ = -1;
    geometry::Vec2f cursorPositionAtPress_;
    geometry::Vec2f cursorPosition_;
    geometry::Vec2d grabbedPoint_;

    core::StringId actionName() const {
        static core::StringId actionName_("Sculpt Grab");
        return actionName_;
    }
};

VGC_DECLARE_OBJECT(SculptRadiusAction);

class SculptRadiusAction : public ui::Action {
private:
    VGC_OBJECT(SculptRadiusAction, ui::Action)

protected:
    /// This is an implementation details.
    /// Please use `SculptRadiusAction::create()` instead.
    ///
    SculptRadiusAction()
        : ui::Action(commands::radius) {
    }

public:
    /// Creates a `SculptRadiusAction`.
    ///
    static SculptRadiusActionPtr create() {
        return SculptRadiusActionPtr(new SculptRadiusAction());
    }

public:
    void onMouseDragStart(ui::MouseEvent* event) override {
        cursorPositionAtPress_ = event->position();
        edgeId_ = tool_->candidateId();
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        cursorPosition_ = event->position();

        geometry::Mat4d inverseViewMatrix = canvas->camera().viewMatrix().inverted();

        float pixelSize = static_cast<float>(
            (inverseViewMatrix.transformPointAffine(geometry::Vec2d(0, 1))
             - inverseViewMatrix.transformPointAffine(geometry::Vec2d(0, 0)))
                .length());

        geometry::Vec2d cursorPositionInWorkspaceAtPress =
            inverseViewMatrix.transformPointAffine(
                geometry::Vec2d(cursorPositionAtPress_));

        geometry::Vec2f deltaCursor = cursorPosition_ - cursorPositionAtPress_;
        double sinAngle = deltaCursor.normalized().det(geometry::Vec2f(1, 0));
        double delta = (cursorPosition_ - cursorPositionAtPress_).x();
        delta *= (1 + sinAngle);

        // Open history group
        core::UndoGroup* undoGroup = nullptr;
        core::History* history = workspace->history();
        if (history) {
            undoGroup = history->createUndoGroup(actionName());
        }

        // Translate Vertices
        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    if (started_) {
                        geometry->resetEdit();
                    }
                    else {
                        geometry->startEdit();
                        started_ = true;
                    }
                    geometry::Vec2d closestPoint = geometry->sculptRadius(
                        cursorPositionInWorkspaceAtPress,
                        delta,
                        options::sculptRadius()->value(),
                        pixelSize,
                        ke->isClosed());
                    tool_->setActionCircleCenter(closestPoint);
                    tool_->setActionCircleEnabled(true);
                }
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            undoGroup->close(amend);
            canAmendUndoGroup_ = true;
        }
    }

    void onMouseDragConfirm(ui::MouseEvent* /*event*/) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        // Open history group
        core::UndoGroup* undoGroup = nullptr;
        core::History* history = workspace->history();
        if (history) {
            undoGroup = history->createUndoGroup(actionName());
        }

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    geometry->finishEdit();
                }
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            undoGroup->close(amend);
            canAmendUndoGroup_ = true;
        }

        tool_->setActionCircleEnabled(false);
        reset_();
    }

    void onMouseDragCancel(ui::MouseEvent* /*event*/) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    geometry->resetEdit();
                    geometry->finishEdit();
                }
            }
        }

        tool_->setActionCircleEnabled(false);
        reset_();
    }

    void reset_() {
        canAmendUndoGroup_ = false;
        started_ = false;
        edgeId_ = -1;
    }

public:
    Sculpt* tool_ = nullptr;
    bool canAmendUndoGroup_ = false;
    bool started_ = false;
    core::Id edgeId_ = -1;
    geometry::Vec2f cursorPositionAtPress_;
    geometry::Vec2f cursorPosition_;
    geometry::Vec2d grabbedPoint_;

    core::StringId actionName() const {
        static core::StringId actionName_("Sculpt Grab");
        return actionName_;
    }
};

VGC_DECLARE_OBJECT(SculptSmoothAction);

class SculptSmoothAction : public ui::Action {
private:
    VGC_OBJECT(SculptSmoothAction, ui::Action)

protected:
    /// This is an implementation details.
    /// Please use `SculptSmoothAction::create()` instead.
    ///
    SculptSmoothAction()
        : ui::Action(commands::smooth) {
    }

public:
    /// Creates a `SculptSmoothAction`.
    ///
    static SculptSmoothActionPtr create() {
        return SculptSmoothActionPtr(new SculptSmoothAction());
    }

public:
    void onMouseDragStart(ui::MouseEvent* event) override {
        cursorPositionAtLastSmooth_ = event->position();
        edgeId_ = tool_->candidateId();
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        cursorPosition_ = event->position();

        geometry::Mat4d inverseViewMatrix = canvas->camera().viewMatrix().inverted();

        float pixelSize = static_cast<float>(
            (inverseViewMatrix.transformPointAffine(geometry::Vec2d(0, 1))
             - inverseViewMatrix.transformPointAffine(geometry::Vec2d(0, 0)))
                .length());

        // for now, smooth once in the middle of the cursor displacement
        geometry::Vec2d positionInWorkspace = inverseViewMatrix.transformPointAffine(
            geometry::Vec2d(0.5 * (cursorPosition_ + cursorPositionAtLastSmooth_)));
        double disp = (cursorPosition_ - cursorPositionAtLastSmooth_).length()
                      / canvas->camera().zoom();

        // Open history group
        core::UndoGroup* undoGroup = nullptr;
        core::History* history = workspace->history();
        if (history) {
            undoGroup = history->createUndoGroup(actionName());
        }

        // Translate Vertices
        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    if (!started_) {
                        geometry->startEdit();
                        started_ = true;
                    }
                    cursorPositionAtLastSmooth_ = cursorPosition_;
                    double radius = options::sculptRadius()->value();
                    double strength = 0.4;
                    geometry::Vec2d smoothedPoint = geometry->sculptSmooth(
                        positionInWorkspace,
                        radius,
                        std::min(1.0, disp / radius) * strength,
                        pixelSize,
                        ke->isClosed());
                    tool_->setActionCircleCenter(smoothedPoint);
                    tool_->setActionCircleEnabled(true);
                }
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            undoGroup->close(amend);
            canAmendUndoGroup_ = true;
        }
    }

    void onMouseDragConfirm(ui::MouseEvent* /*event*/) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        // Open history group
        core::UndoGroup* undoGroup = nullptr;
        core::History* history = workspace->history();
        if (history) {
            undoGroup = history->createUndoGroup(actionName());
        }

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    geometry->finishEdit();
                }
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            undoGroup->close(amend);
            canAmendUndoGroup_ = true;
        }

        tool_->setActionCircleEnabled(false);
        reset_();
    }

    void onMouseDragCancel(ui::MouseEvent* /*event*/) override {
        if (edgeId_ == -1) {
            return;
        }

        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                vacomplex::KeyEdgeGeometry* geometry = ke->geometry();
                if (geometry) {
                    geometry->resetEdit();
                    geometry->finishEdit();
                }
            }
        }

        tool_->setActionCircleEnabled(false);
        reset_();
    }

    void reset_() {
        canAmendUndoGroup_ = false;
        started_ = false;
        edgeId_ = -1;
    }

public:
    Sculpt* tool_ = nullptr;
    bool canAmendUndoGroup_ = false;
    bool started_ = false;
    core::Id edgeId_ = -1;
    geometry::Vec2f cursorPositionAtLastSmooth_;
    geometry::Vec2f cursorPosition_;
    geometry::Vec2d grabbedPoint_;

    core::StringId actionName() const {
        static core::StringId actionName_("Sculpt Smooth");
        return actionName_;
    }
};

VGC_DECLARE_OBJECT(EditSculptRadiusAction);

class EditSculptRadiusAction : public ui::Action {
private:
    VGC_OBJECT(EditSculptRadiusAction, ui::Action)

protected:
    /// This is an implementation details.
    /// Please use `EditSculptRadiusAction::create()` instead.
    ///
    EditSculptRadiusAction()
        : ui::Action(commands::editSculptRadius) {
    }

public:
    /// Creates a `EditSculptRadiusAction`.
    ///
    static EditSculptRadiusActionPtr create() {
        return EditSculptRadiusActionPtr(new EditSculptRadiusAction());
    }

public:
    void onMouseDragStart(ui::MouseEvent* event) override {
        cursorPositionAtPress_ = event->position();
        oldRadius_ = options::sculptRadius()->value();
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        canvas::Canvas* canvas = tool_->canvas();
        workspace::Workspace* workspace = tool_->workspace();
        if (!canvas || !workspace) {
            return;
        }
        double zoom = canvas->camera().zoom();
        double dx = event->position().x() - cursorPositionAtPress_.x();
        double newRadius = (std::max)(0.0, oldRadius_ + dx / zoom);
        options::sculptRadius()->setValue(newRadius);
        tool_->dirtyActionCircle();
    }

    void onMouseDragConfirm(ui::MouseEvent* /*event*/) override {
    }

    void onMouseDragCancel(ui::MouseEvent* /*event*/) override {
        options::sculptRadius()->setValue(oldRadius_);
        tool_->dirtyActionCircle();
    }

public:
    Sculpt* tool_ = nullptr;
    geometry::Vec2f cursorPositionAtPress_;
    double oldRadius_ = 0;
};

} // namespace

Sculpt::Sculpt()
    : CanvasTool() {

    auto grabAction = createAction<SculptGrabAction>();
    grabAction->tool_ = this;
    auto radiusAction = createAction<SculptRadiusAction>();
    radiusAction->tool_ = this;
    auto smoothAction = createAction<SculptSmoothAction>();
    smoothAction->tool_ = this;
    auto editSculptRadiusAction = createAction<EditSculptRadiusAction>();
    editSculptRadiusAction->tool_ = this;
}

SculptPtr Sculpt::create() {
    return SculptPtr(new Sculpt());
}

ui::WidgetPtr Sculpt::createOptionsWidget() const {
    ui::WidgetPtr res = ui::Column::create();
    res->createChild<ui::NumberSettingEdit>(options::sculptRadius());
    return res;
}

void Sculpt::onMouseHover(ui::MouseHoverEvent* event) {

    cursorPosition_ = event->position();

    canvas::Canvas* canvas = this->canvas();
    workspace::Workspace* workspace = this->workspace();
    if (!canvas || !workspace) {
        candidateId_ = -1;
        return;
    }

    // compute candidate
    core::Id candidateId = -1;
    double minDistance = {};
    geometry::Vec2d closestPoint = {};

    geometry::Vec2d viewCursor(event->position());
    geometry::Vec2d worldCursor =
        canvas->camera().viewMatrix().inverted().transformPointAffine(viewCursor);

    double worldSculptRadius =
        options::sculptRadius()->value(); // / canvas->camera().zoom();
    minDistance = worldSculptRadius;

    workspace->visitDepthFirst(
        [](workspace::Element*, Int) { return true; },
        [&, worldCursor](workspace::Element* e, Int /*depth*/) {
            if (!e) {
                return;
            }
            if (e->isVacElement()) {
                vacomplex::Node* node = e->toVacElement()->vacNode();
                if (node->isCell()) {
                    vacomplex::KeyEdge* ke = node->toCellUnchecked()->toKeyEdge();
                    if (ke) {
                        const vacomplex::EdgeSampling& sampling = ke->sampling();
                        const geometry::StrokeSample2dArray& samples = sampling.samples();
                        geometry::DistanceToCurve dtc =
                            geometry::distanceToCurve(samples, worldCursor);
                        if (dtc.distance() <= minDistance) {
                            candidateId = e->id();
                            minDistance = dtc.distance();
                            closestPoint = samples[dtc.segmentIndex()].position();
                            Int numSegments = samples.length() - 1;
                            if (dtc.segmentParameter() > 0
                                && dtc.segmentIndex() < numSegments) {

                                closestPoint =
                                    closestPoint * (1 - dtc.segmentParameter())
                                    + samples[dtc.segmentIndex() + 1].position()
                                          * dtc.segmentParameter();
                            }
                        }
                    }
                }
            }
        });

    candidateId_ = candidateId;
    candidateClosestPoint_ = closestPoint;

    requestRepaint();
}

void Sculpt::onMouseLeave() {
    requestRepaint();
}

void Sculpt::onResize() {
    SuperClass::onResize();
}

namespace {

constexpr float circleSSThickness = 2.f;
constexpr Int circleNumSides = 127;
constexpr float pointDiskSSRadius = 2.f;
constexpr Int pointDiskNumSides = 127;

} // namespace

void Sculpt::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    using namespace graphics;

    circleGeometry_ = detail::createCircleWithScreenSpaceThickness(
        engine, circleSSThickness, workspace::colors::selection, circleNumSides);
    pointGeometry_ = detail::createScreenSpaceDisk(
        engine,
        geometry::Vec2f(),
        pointDiskSSRadius,
        workspace::colors::selection,
        pointDiskNumSides);
}

void Sculpt::onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) {
    SuperClass::onPaintDraw(engine, options);

    if (!isHovered()) {
        return;
    }

    using namespace graphics;
    namespace gs = graphics::strings;

    canvas::Canvas* canvas = this->canvas();
    if (!canvas) {
        return;
    }

    using geometry::Vec2d;
    using geometry::Vec2f;

    geometry::Mat4d ivm = canvas->camera().viewMatrix().inverted();

    geometry::Mat4f translation = geometry::Mat4f::identity;
    if (isActionCircleEnabled_) {
        geometry::Vec2f pos(actionCircleCenter_);
        translation.translate(pos);
    }
    else if (candidateId_ != -1) {
        geometry::Vec2f pos(candidateClosestPoint_);
        translation.translate(pos);
    }
    else {
        geometry::Vec2f pos(ivm.transformPointAffine(geometry::Vec2d(cursorPosition_)));
        translation.translate(pos);
    }

    geometry::Mat4f scaling = geometry::Mat4f::identity;
    scaling.scale(static_cast<float>(options::sculptRadius()->value()));

    geometry::Mat4f currentView(engine->viewMatrix());
    geometry::Mat4f canvasView(canvas->camera().viewMatrix());

    engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);

    geometry::Mat4f view = currentView * canvasView * translation;
    engine->pushViewMatrix(view);
    engine->draw(pointGeometry_);

    engine->setViewMatrix(view * scaling);
    engine->draw(circleGeometry_);

    engine->popViewMatrix();
}

void Sculpt::onPaintDestroy(graphics::Engine* engine) {
    SuperClass::onPaintDestroy(engine);
}

} // namespace vgc::tools
