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
#include <vgc/geometry/mat4.h>
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
    width,
    "tools.sculpt.width",
    "Sculpt Width",
    Shortcut(ModifierKey::Alt, MouseButton::Left))

VGC_UI_DEFINE_MOUSE_DRAG_COMMAND( //
    editRadius,
    "tools.sculpt.editRadius",
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
    SculptGrabAction(CreateKey key)
        : ui::Action(key, commands::grab()) {
    }

public:
    /// Creates a `SculptGrabAction`.
    ///
    static SculptGrabActionPtr create() {
        return core::createObject<SculptGrabAction>();
    }

public:
    void onMouseDragStart(ui::MouseEvent* event) override {
        if (auto tool = tool_.lock()) {
            cursorPositionAtPress_ = event->position();
            edgeId_ = tool->candidateId();
        }
        else {
            edgeId_ = -1;
        }
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        if (edgeId_ == -1) {
            return;
        }

        auto tool = tool_.lock();
        if (!tool) {
            return;
        }
        auto context = tool->contextLock();
        if (!context) {
            return;
        }
        auto workspace = context.workspace();
        auto canvas = context.canvas();

        cursorPosition_ = event->position();

        geometry::Mat3d inverseViewMatrix = canvas->camera().viewMatrix().inverse();

        float pixelSize = static_cast<float>(
            (inverseViewMatrix.transformAffine(geometry::Vec2d(0, 1))
             - inverseViewMatrix.transformAffine(geometry::Vec2d(0, 0)))
                .length());

        geometry::Vec2d cursorPositionInWorkspace =
            inverseViewMatrix.transformAffine(geometry::Vec2d(cursorPosition_));
        geometry::Vec2d cursorPositionInWorkspaceAtPress =
            inverseViewMatrix.transformAffine(geometry::Vec2d(cursorPositionAtPress_));

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
                vacomplex::KeyEdgeData& data = ke->data();
                if (started_) {
                    data = oldData_;
                    editStroke_->copyAssign(oldData_.stroke());
                }
                else {
                    oldData_ = data;
                    editStroke_ = oldData_.stroke()->clone();
                    started_ = true;
                }
                geometry::Vec2d grabbedPoint = editStroke_->sculptGrab(
                    cursorPositionInWorkspaceAtPress,
                    cursorPositionInWorkspace,
                    options::sculptRadius()->value(),
                    1,
                    pixelSize,
                    ke->isClosed());
                data.setStroke(editStroke_.get());
                tool->setActionCircleCenter(grabbedPoint);
                tool->setActionCircleEnabled(true);
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            canAmendUndoGroup_ |= undoGroup->close(amend);
        }
    }

    void onMouseDragConfirm(ui::MouseEvent* event) override {
        VGC_UNUSED(event);
        if (edgeId_ == -1) {
            return;
        }
        if (auto tool = tool_.lock()) {
            tool->setActionCircleEnabled(false);
        }
        reset_();
    }

    void onMouseDragCancel(ui::MouseEvent* event) override {
        VGC_UNUSED(event);
        if (edgeId_ == -1) {
            return;
        }
        auto tool = tool_.lock();
        if (!tool) {
            return;
        }
        auto context = tool->contextLock();
        if (!context) {
            return;
        }
        auto workspace = context.workspace();

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                ke->data() = oldData_;
            }
        }

        tool->setActionCircleEnabled(false);
        reset_();
    }

    void reset_() {
        canAmendUndoGroup_ = false;
        started_ = false;
        edgeId_ = -1;
    }

public:
    SculptWeakPtr tool_;
    bool canAmendUndoGroup_ = false;
    bool started_ = false;
    core::Id edgeId_ = -1;
    vacomplex::KeyEdgeData oldData_;
    std::unique_ptr<geometry::AbstractStroke2d> editStroke_;
    geometry::Vec2f cursorPositionAtPress_;
    geometry::Vec2f cursorPosition_;
    geometry::Vec2d grabbedPoint_;

    core::StringId actionName() const {
        static core::StringId actionName_("Sculpt Grab");
        return actionName_;
    }
};

VGC_DECLARE_OBJECT(SculptWidthAction);

class SculptWidthAction : public ui::Action {
private:
    VGC_OBJECT(SculptWidthAction, ui::Action)

protected:
    /// This is an implementation details.
    /// Please use `SculptWidthAction::create()` instead.
    ///
    SculptWidthAction(CreateKey key)
        : ui::Action(key, commands::width()) {
    }

public:
    /// Creates a `SculptWidthAction`.
    ///
    static SculptWidthActionPtr create() {
        return core::createObject<SculptWidthAction>();
    }

public:
    void onMouseDragStart(ui::MouseEvent* event) override {
        cursorPositionAtPress_ = event->position();
        if (auto tool = tool_.lock()) {
            edgeId_ = tool->candidateId();
        }
        else {
            edgeId_ = -1;
        }
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        if (edgeId_ == -1) {
            return;
        }

        auto tool = tool_.lock();
        if (!tool) {
            return;
        }
        auto context = tool->contextLock();
        if (!context) {
            return;
        }
        auto workspace = context.workspace();
        auto canvas = context.canvas();

        cursorPosition_ = event->position();

        geometry::Mat3d inverseViewMatrix = canvas->camera().viewMatrix().inverse();

        float pixelSize = static_cast<float>(
            (inverseViewMatrix.transformAffine(geometry::Vec2d(0, 1))
             - inverseViewMatrix.transformAffine(geometry::Vec2d(0, 0)))
                .length());

        geometry::Vec2d cursorPositionInWorkspaceAtPress =
            inverseViewMatrix.transformAffine(geometry::Vec2d(cursorPositionAtPress_));

        geometry::Vec2f deltaCursor = cursorPosition_ - cursorPositionAtPress_;
        double sinAngle = deltaCursor.normalized().det(geometry::Vec2f(1, 0));
        double delta = (cursorPosition_ - cursorPositionAtPress_).x();
        delta *= (1 + sinAngle);
        delta *= pixelSize;

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
                vacomplex::KeyEdgeData& data = ke->data();
                if (started_) {
                    data = oldData_;
                    editStroke_->copyAssign(oldData_.stroke());
                }
                else {
                    oldData_ = data;
                    editStroke_ = oldData_.stroke()->clone();
                    started_ = true;
                }
                geometry::Vec2d closestPoint = editStroke_->sculptWidth(
                    cursorPositionInWorkspaceAtPress,
                    delta,
                    options::sculptRadius()->value(),
                    pixelSize,
                    ke->isClosed());
                data.setStroke(editStroke_.get());
                tool->setActionCircleCenter(closestPoint);
                tool->setActionCircleEnabled(true);
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            canAmendUndoGroup_ |= undoGroup->close(amend);
        }
    }

    void onMouseDragConfirm(ui::MouseEvent* event) override {
        VGC_UNUSED(event);
        if (edgeId_ == -1) {
            return;
        }

        if (auto tool = tool_.lock()) {
            tool->setActionCircleEnabled(false);
        }

        reset_();
    }

    void onMouseDragCancel(ui::MouseEvent* event) override {
        VGC_UNUSED(event);
        if (edgeId_ == -1) {
            return;
        }

        auto tool = tool_.lock();
        if (!tool) {
            return;
        }
        auto context = tool->contextLock();
        if (!context) {
            return;
        }
        auto workspace = context.workspace();

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                ke->data() = oldData_;
            }
        }

        tool->setActionCircleEnabled(false);
        reset_();
    }

    void reset_() {
        canAmendUndoGroup_ = false;
        started_ = false;
        edgeId_ = -1;
    }

public:
    SculptWeakPtr tool_;
    bool canAmendUndoGroup_ = false;
    bool started_ = false;
    core::Id edgeId_ = -1;
    vacomplex::KeyEdgeData oldData_;
    std::unique_ptr<geometry::AbstractStroke2d> editStroke_;
    geometry::Vec2f cursorPositionAtPress_;
    geometry::Vec2f cursorPosition_;
    geometry::Vec2d grabbedPoint_;

    core::StringId actionName() const {
        static core::StringId actionName_("Sculpt Width");
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
    SculptSmoothAction(CreateKey key)
        : ui::Action(key, commands::smooth()) {
    }

public:
    /// Creates a `SculptSmoothAction`.
    ///
    static SculptSmoothActionPtr create() {
        return core::createObject<SculptSmoothAction>();
    }

public:
    void onMouseDragStart(ui::MouseEvent* event) override {
        cursorPositionAtLastSmooth_ = event->position();
        if (auto tool = tool_.lock()) {
            edgeId_ = tool->candidateId();
        }
        else {
            edgeId_ = -1;
        }
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        if (edgeId_ == -1) {
            return;
        }

        auto tool = tool_.lock();
        if (!tool) {
            return;
        }
        auto context = tool->contextLock();
        if (!context) {
            return;
        }
        auto workspace = context.workspace();
        auto canvas = context.canvas();

        cursorPosition_ = event->position();

        geometry::Mat3d inverseViewMatrix = canvas->camera().viewMatrix().inverse();

        float pixelSize = static_cast<float>(
            (inverseViewMatrix.transformAffine(geometry::Vec2d(0, 1))
             - inverseViewMatrix.transformAffine(geometry::Vec2d(0, 0)))
                .length());

        // for now, smooth once in the middle of the cursor displacement
        geometry::Vec2d positionInWorkspace = inverseViewMatrix.transformAffine(
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
                vacomplex::KeyEdgeData& data = ke->data();
                if (!started_) {
                    oldData_ = data;
                    editStroke_ = oldData_.stroke()->clone();
                    started_ = true;
                }
                cursorPositionAtLastSmooth_ = cursorPosition_;
                double radius = options::sculptRadius()->value();
                double strength = 0.4;
                geometry::Vec2d smoothedPoint = editStroke_->sculptSmooth(
                    positionInWorkspace,
                    radius,
                    (std::min)(1.0, disp / radius) * strength,
                    pixelSize,
                    ke->isClosed());
                data.setStroke(editStroke_.get());
                tool->setActionCircleCenter(smoothedPoint);
                tool->setActionCircleEnabled(true);
            }
        }

        // Close operation
        if (undoGroup) {
            bool amend = canAmendUndoGroup_ && undoGroup->parent()
                         && undoGroup->parent()->name() == actionName();
            canAmendUndoGroup_ |= undoGroup->close(amend);
        }
    }

    void onMouseDragConfirm(ui::MouseEvent* event) override {
        VGC_UNUSED(event);
        if (edgeId_ == -1) {
            return;
        }

        if (auto tool = tool_.lock()) {
            tool->setActionCircleEnabled(false);
        }
        reset_();
    }

    void onMouseDragCancel(ui::MouseEvent* event) override {
        VGC_UNUSED(event);
        if (edgeId_ == -1) {
            return;
        }
        auto tool = tool_.lock();
        if (!tool) {
            return;
        }
        auto context = tool->contextLock();
        if (!context) {
            return;
        }
        auto workspace = context.workspace();

        workspace::Element* element = workspace->find(edgeId_);
        if (element && element->vacNode() && element->vacNode()->isCell()) {
            vacomplex::KeyEdge* ke = element->vacNode()->toCellUnchecked()->toKeyEdge();
            if (ke) {
                ke->data() = oldData_;
            }
        }

        tool->setActionCircleEnabled(false);
        reset_();
    }

    void reset_() {
        canAmendUndoGroup_ = false;
        started_ = false;
        edgeId_ = -1;
    }

public:
    SculptWeakPtr tool_;
    bool canAmendUndoGroup_ = false;
    bool started_ = false;
    core::Id edgeId_ = -1;
    vacomplex::KeyEdgeData oldData_;
    std::unique_ptr<geometry::AbstractStroke2d> editStroke_;
    geometry::Vec2f cursorPositionAtLastSmooth_;
    geometry::Vec2f cursorPosition_;
    geometry::Vec2d grabbedPoint_;

    core::StringId actionName() const {
        static core::StringId actionName_("Sculpt Smooth");
        return actionName_;
    }
};

VGC_DECLARE_OBJECT(SculptEditRadiusAction);

class SculptEditRadiusAction : public ui::Action {
private:
    VGC_OBJECT(SculptEditRadiusAction, ui::Action)

protected:
    /// This is an implementation details.
    /// Please use `SculptEditRadiusAction::create()` instead.
    ///
    SculptEditRadiusAction(CreateKey key)
        : ui::Action(key, commands::editRadius()) {
    }

public:
    /// Creates a `SculptEditRadiusAction`.
    ///
    static SculptEditRadiusActionPtr create() {
        return core::createObject<SculptEditRadiusAction>();
    }

public:
    void onMouseDragStart(ui::MouseEvent* event) override {
        cursorPositionAtPress_ = event->position();
        oldRadius_ = options::sculptRadius()->value();
    }

    void onMouseDragMove(ui::MouseEvent* event) override {
        auto tool = tool_.lock();
        if (!tool) {
            return;
        }
        auto context = tool->contextLock();
        if (!context) {
            return;
        }
        auto canvas = context.canvas();
        double zoom = canvas->camera().zoom();
        double dx = event->position().x() - cursorPositionAtPress_.x();
        double newRadius = (std::max)(0.0, oldRadius_ + dx / zoom);
        options::sculptRadius()->setValue(newRadius);
        tool->dirtyActionCircle();
    }

    void onMouseDragConfirm(ui::MouseEvent* event) override {
        VGC_UNUSED(event);
    }

    void onMouseDragCancel(ui::MouseEvent* event) override {
        VGC_UNUSED(event);
        options::sculptRadius()->setValue(oldRadius_);
        if (auto tool = tool_.lock()) {
            tool->dirtyActionCircle();
        }
    }

public:
    SculptWeakPtr tool_;
    geometry::Vec2f cursorPositionAtPress_;
    double oldRadius_ = 0;
};

} // namespace

Sculpt::Sculpt(CreateKey key)
    : CanvasTool(key) {

    auto grabAction = createAction<SculptGrabAction>();
    grabAction->tool_ = this;
    auto widthAction = createAction<SculptWidthAction>();
    widthAction->tool_ = this;
    auto smoothAction = createAction<SculptSmoothAction>();
    smoothAction->tool_ = this;
    auto editRadiusAction = createAction<SculptEditRadiusAction>();
    editRadiusAction->tool_ = this;
}

SculptPtr Sculpt::create() {
    return core::createObject<Sculpt>();
}

ui::WidgetPtr Sculpt::doCreateOptionsWidget() const {
    ui::WidgetPtr res = ui::Column::create();
    res->createChild<ui::NumberSettingEdit>(options::sculptRadius());
    return res;
}

void Sculpt::onMouseHover(ui::MouseHoverEvent* event) {

    auto context = contextLock();
    if (!context) {
        candidateId_ = -1;
        return;
    }
    auto workspace = context.workspace();
    auto canvas = context.canvas();

    cursorPosition_ = event->position();

    // compute candidate
    core::Id candidateId = -1;
    double minDistance = {};
    geometry::Vec2d closestPoint = {};

    geometry::Vec2d viewCursor(event->position());
    geometry::Vec2d worldCursor =
        canvas->camera().viewMatrix().inverse().transformAffine(viewCursor);

    double worldSculptRadius =
        options::sculptRadius()->value(); // / canvas->camera().zoom();
    minDistance = worldSculptRadius;

    workspace->visitDepthFirst(
        [](workspace::Element*, Int) { return true; },
        [&, worldCursor](workspace::Element* e, Int depth) {
            VGC_UNUSED(depth);
            if (!e) {
                return;
            }
            if (e->isVacElement()) {
                vacomplex::Node* node = e->toVacElement()->vacNode();
                if (node && node->isCell()) {
                    vacomplex::KeyEdge* ke = node->toCellUnchecked()->toKeyEdge();
                    if (ke) {
                        const geometry::StrokeSampling2d& sampling = ke->strokeSampling();
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

    auto context = contextLock();
    if (!context) {
        return;
    }
    auto canvas = context.canvas();

    using geometry::Mat3d;
    using geometry::Mat4f;
    using geometry::Vec2d;
    using geometry::Vec2f;

    Mat3d canvasView = canvas->camera().viewMatrix();
    Mat3d canvasViewInverse = canvasView.inverse();

    Mat3d translation = Mat3d::identity;
    if (isActionCircleEnabled_) {
        translation.translate(actionCircleCenter_);
    }
    else if (candidateId_ != -1) {
        translation.translate(candidateClosestPoint_);
    }
    else {
        Vec2d pos(cursorPosition_);
        translation.translate(canvasViewInverse.transformAffine(pos));
    }

    Mat4f scaling = Mat4f::identity;
    scaling.scale(static_cast<float>(options::sculptRadius()->value()));

    engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);

    Mat4f currentView = engine->viewMatrix();
    Mat4f view = currentView * Mat4f::fromTransform(canvasView * translation);

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
