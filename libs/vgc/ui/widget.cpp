// Copyright 2021 The VGC Developers
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

#include <vgc/ui/widget.h>

#include <vgc/core/colors.h>
#include <vgc/core/io.h>
#include <vgc/core/paths.h>

#include <vgc/style/strings.h>

#include <vgc/graphics/richtext.h>
#include <vgc/graphics/strings.h>

#include <vgc/ui/action.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/margins.h>
#include <vgc/ui/mouseevent.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Widget::Widget(CreateKey key)
    : StylableObject(key)
    , children_(WidgetList::create(this))
    , actions_(ActionList::create(this)) {

    addStyleClass(strings::Widget);
    children_->childAdded().connect(onWidgetAddedSlot_());
    children_->childRemoved().connect(onWidgetRemovedSlot_());
    actions_->childAdded().connect(onActionAddedSlot_());
    actions_->childRemoved().connect(onActionRemovedSlot_());
}

void Widget::onDestroyed() {
    // Auto-reconnect hover chain
    Widget* p = hoverChainParent_;
    Widget* c = hoverChainChild_;
    if (p) {
        p->hoverChainChild_ = c;
    }
    if (c) {
        c->hoverChainParent_ = p;
    }
    // Reset values to improve debuggability
    children_ = nullptr;
    actions_ = nullptr;
    mouseCaptor_ = nullptr;
    hoverChainParent_ = nullptr;
    hoverChainChild_ = nullptr;
    isHovered_ = false;
    isHoverLocked_ = false;
    pressedButtons_.clear();
    computedVisibility_ = false;
    keyboardCaptor_ = nullptr;
    // Call parent destructor
    Object::onDestroyed();
}

WidgetPtr Widget::create() {
    return core::createObject<Widget>();
}

namespace {

bool checkCanReparent_(Widget* parent, Widget* child, bool simulate = false) {
    if (parent && parent->isDescendantObjectOf(child)) {
        if (simulate) {
            return false;
        }
        else {
            throw ChildCycleError(parent, child);
        }
    }
    return true;
}

} // namespace

void Widget::addChild(Widget* child) {
    insertChild(nullptr, child);
}

void Widget::insertChild(Widget* nextSibling, Widget* child) {

    // Check whether reparenting is possible
    checkCanReparent_(this, child);

    // Inform onWidgetRemoved_() and onWidgetAdded_() whether the widget is
    // reparented within the same tree, so that they can be optimized
    Widget* rootBeforeReparent = child->root();
    Widget* rootAfterReparent = root();
    if (rootBeforeReparent == rootAfterReparent) {
        child->isReparentingWithinSameTree_ = true;
    }

    // Perform the reparenting
    children_->insert(nextSibling, child);

    // Restore data members
    child->isReparentingWithinSameTree_ = false;
}

void Widget::insertChild(Int i, Widget* child) {
    if (i < 0 || i > numChildren()) {
        throw core::IndexError(core::format(
            "Cannot insert child widget at index {} (numChildren() == {}).",
            i,
            numChildren()));
    }
    Widget* nextSibling = firstChild();
    for (; i > 0; --i) {
        nextSibling = nextSibling->nextSibling();
    }
    insertChild(nextSibling, child);
}

bool Widget::canReparent(Widget* newParent) {
    const bool simulate = true;
    return checkCanReparent_(newParent, this, simulate);
}

void Widget::reparent(Widget* newParent) {
    if (newParent) {
        newParent->addChild(this);
    }
    else {
        // Make the widget a root widget.
        // This will destroy the widget if no ObjPtr manages it.
        // XXX have a dedicated widget function for that, e.g. Widget::removeChild()?
        removeObjectFromParent_();
    }
}

namespace {

bool checkCanReplace_(Widget* oldWidget, Widget* newWidget, bool simulate = false) {
    if (!oldWidget) {
        if (simulate) {
            return false;
        }
        else {
            throw core::NullError();
        }
    }

    if (oldWidget == newWidget) {
        return true;
    }

    Widget* oldWidgetParent = oldWidget->parent();
    if (oldWidgetParent) {
        return checkCanReparent_(oldWidgetParent, newWidget, simulate);
    }
    else {
        return true;
    }
}

} // namespace

bool Widget::canReplace(Widget* replacedWidget) {
    const bool simulate = true;
    return checkCanReplace_(replacedWidget, this, simulate);
}

void Widget::replace(Widget* replacedWidget) {
    checkCanReplace_(replacedWidget, this);
    if (this == replacedWidget) {
        // nothing to do
        return;
    }
    Widget* parent = replacedWidget->parent();
    Widget* nextSibling = replacedWidget->nextSibling();

    // Inform onWidgetRemoved_() and onWidgetAdded_() whether the widget is
    // reparented within the same tree, so that they can be optimized
    Widget* rootBeforeReplace = root();
    Widget* rootAfterReplace = replacedWidget->root();
    if (rootBeforeReplace == rootAfterReplace) {
        isReparentingWithinSameTree_ = true;
    }

    // Remove `this` from its current parent. We need to do this before
    // destroying replacedWidget, because `this` might be a descendant of
    // replacedWidget.
    core::ObjectPtr self = removeObjectFromParent_();

    // Destroy replacedWidget. We need to do this before inserting `this` at
    // its new location, in case the new parent supports at most one child.
    replacedWidget->destroyObject_();

    // Insert at new location
    if (parent) {
        parent->children_->insert(nextSibling, this);
    }
    else {
        // nothing to do
    }

    // Restore data members
    isReparentingWithinSameTree_ = false;
}

Widget* Widget::root() const {
    const Widget* res = this;
    const Widget* w = this;
    while (w) {
        res = w;
        w = w->parent();
    }
    return const_cast<Widget*>(res);
}

OverlayArea* Widget::topmostOverlayArea() const {
    const OverlayArea* res = nullptr;
    const Widget* w = this;
    while (w) {
        const OverlayArea* oa = dynamic_cast<const OverlayArea*>(w);
        if (oa) {
            res = oa;
        }
        w = w->parent();
    }
    return const_cast<OverlayArea*>(res);
}

namespace {

geometry::Vec2f positionInRoot(const Widget* widget, const Widget** outRoot) {
    geometry::Vec2f pos = widget->position();
    const Widget* root = const_cast<Widget*>(widget);
    const Widget* w = widget->parent();
    while (w) {
        root = w;
        pos += w->position();
        w = w->parent();
    }
    if (outRoot) {
        *outRoot = root;
    }
    return pos;
}

} // namespace

geometry::Vec2f Widget::mapTo(Widget* other, const geometry::Vec2f& position) const {

    if (!other) {
        throw core::NullError();
    }

    // fast path
    if (other->parent() == this) {
        return position - other->position();
    }

    // XXX could use any common ancestor
    const Widget* thisRoot = nullptr;
    geometry::Vec2f thisPosInRoot = positionInRoot(this, &thisRoot);
    const Widget* otherRoot = nullptr;
    geometry::Vec2f otherPosInRoot = positionInRoot(other, &otherRoot);

    if (thisRoot != otherRoot) {
        throw core::LogicError(
            "Cannot map a position between two widget coordinate systems if the widgets "
            "don't have the same root.");
    }

    return position + thisPosInRoot - otherPosInRoot;
}

geometry::Rect2f Widget::mapTo(Widget* other, const geometry::Rect2f& rect) const {
    geometry::Rect2f res = rect;
    res.setPosition(mapTo(other, rect.position()));
    return res;
}

// Note: by design, when margins are expressed in percentage, they are
// relative to the size of this widget, not the parent widget. This makes
// it easy for designers to provide equal values for margins and padding.
// If/when designers want to create spacing between widgets relative to the
// size of parent, they can always choose a combination of padding and gap.
//
Margins Widget::margin() const {
    geometry::Vec2f s = size();
    return Margins(
        detail::getLengthOrPercentageInPx(this, style::strings::margin_top, s[1]),
        detail::getLengthOrPercentageInPx(this, style::strings::margin_right, s[0]),
        detail::getLengthOrPercentageInPx(this, style::strings::margin_bottom, s[1]),
        detail::getLengthOrPercentageInPx(this, style::strings::margin_left, s[0]));
}

Margins Widget::padding() const {
    geometry::Vec2f s = size();
    return Margins(
        detail::getLengthOrPercentageInPx(this, style::strings::padding_top, s[1]),
        detail::getLengthOrPercentageInPx(this, style::strings::padding_right, s[0]),
        detail::getLengthOrPercentageInPx(this, style::strings::padding_bottom, s[1]),
        detail::getLengthOrPercentageInPx(this, style::strings::padding_left, s[0]));
}

Margins Widget::border() const {
    return Margins(detail::getLengthInPx(this, style::strings::border_width));
}

geometry::Rect2f Widget::contentRect() const {
    geometry::Rect2f res = rect() - border() - padding();
    if (res.xMin() > res.xMax()) {
        float x = 0.5f * (res.xMin() + res.xMax());
        res.setXMin(x);
        res.setXMax(x);
    }
    if (res.yMin() > res.yMax()) {
        float y = 0.5f * (res.yMin() + res.yMax());
        res.setYMin(y);
        res.setYMax(y);
    }
    return res;
}

void Widget::updateGeometry(
    const geometry::Vec2f& position,
    const geometry::Vec2f& size) {

    WidgetPtr self(this);

    position_ = position;
    bool resized = false;
    if (!size_.allNear(size, 1e-6f)) {
        size_ = size;
        resized = true;
    }
    bool updated = false;
    if (isGeometryUpdateRequested_ || resized) {
        updateGeometry_();
        updated = true;
    }
    if (self.isAlive() && resized) {
        onResize();
    }

    if (self.isAlive() && updated && !parent()) {
        updateHoverChain();
    }
}

void Widget::updateGeometry(const geometry::Vec2f& position) {
    position_ = position;
    updateGeometry();
}

void Widget::updateGeometry() {

    WidgetPtr self(this);

    bool updated = false;
    if (isGeometryUpdateRequested_) {
        updateGeometry_();
        updated = true;
    }

    if (self.isAlive() && updated && !parent()) {
        updateHoverChain();
    }
}

void Widget::setClippingEnabled(bool isClippingEnabled) {
    if (isClippingEnabled_ != isClippingEnabled) {
        isClippingEnabled_ = isClippingEnabled;
        requestRepaint();
    }
}

style::LengthOrPercentageOrAuto Widget::preferredWidth() const {
    return style(strings::preferred_width).to<style::LengthOrPercentageOrAuto>();
}

float Widget::horizontalStretch() const {
    return std::abs(style(strings::horizontal_stretch).toFloat());
}

float Widget::horizontalShrink() const {
    return std::abs(style(strings::horizontal_shrink).toFloat());
}

style::LengthOrPercentageOrAuto Widget::preferredHeight() const {
    return style(strings::preferred_height).to<style::LengthOrPercentageOrAuto>();
}

float Widget::preferredWidthForHeight(float) const {
    return preferredSize()[0];
}

float Widget::preferredHeightForWidth(float) const {
    return preferredSize()[1];
}

float Widget::verticalStretch() const {
    return style(strings::vertical_stretch).toFloat();
}

float Widget::verticalShrink() const {
    return style(strings::vertical_shrink).toFloat();
}

void Widget::requestGeometryUpdate() {
    Widget* widget = this;
    while (widget) {
        Widget* parent = widget->parent();
        // (not isPreferredSizeComputed_) => isGeometryUpdateRequested_
        // (not isGeometryUpdateRequested_) => isPreferredSizeComputed_
        // isGeometryUpdateRequested_ => isRepaintRequested_
        if (!widget->isGeometryUpdateRequested_) {
            widget->isGeometryUpdateRequested_ = true;
            if (!parent) {
                widget->geometryUpdateRequested().emit();
            }
            // repaint request
            if (!widget->isRepaintRequested_) {
                widget->isRepaintRequested_ = true;

                if (!parent) {
                    widget->repaintRequested().emit();
                }
            }
        }
        else if (!widget->isPreferredSizeComputed_) {
            if (!isRepaintRequested_) {
                VGC_ERROR(
                    LogVgcUi,
                    "Widget seems to have been repainted before its geometry was "
                    "updated.");
            }
            // isGeometryUpdateRequested_
            // && isRepaintRequested_
            // && !isPreferredSizeComputed_
            break;
        }
        widget->isPreferredSizeComputed_ = false;
        // don't forward to parent if child is not visible
        widget = widget->computedVisibility_ ? parent : nullptr;
    }
}

void Widget::onResize() {
    backgroundChanged_ = true;
}

void Widget::requestRepaint() {
    if (isRepaintRequested_) {
        return;
    }

    Widget* widget = this;
    do {
        widget->isRepaintRequested_ = true;
        Widget* parent = widget->parent();
        if (!parent) {
            widget->repaintRequested().emit();
        }
        // don't forward to parent if child is not visible
        widget = widget->computedVisibility_ ? parent : nullptr;

    } while (widget && !widget->isRepaintRequested_);
}

void Widget::preparePaint(graphics::Engine* engine, PaintOptions options) {
    prePaintUpdateGeometry_();
    prePaintUpdateEngine_(engine);
    onPaintPrepare(engine, options);
    for (Widget* widget : children()) {
        widget->preparePaint(engine, options);
    }
}

void Widget::paint(graphics::Engine* engine, PaintOptions options) {
    if (!isVisible()) {
        return;
    }
    prePaintUpdateGeometry_();
    prePaintUpdateEngine_(engine);
    if (isGeometryUpdateRequested_) {
        VGC_WARNING(
            LogVgcUi,
            "A child widget geometry was not updated by its parent before draw.");
        updateGeometry();
    }
    isRepaintRequested_ = false;

    if (isClippingEnabled()) {
        geometry::Rect2f scissorRect = mapTo(root(), rect()); // XXX or contentRect?
        scissorRect.setSize(scissorRect.size());
        scissorRect.intersectWith(engine->scissorRect());
        if (!scissorRect.isDegenerate()) {
            engine->pushScissorRect(scissorRect);
            onPaintDraw(engine, options);
            engine->popScissorRect();
        }
    }
    else {
        onPaintDraw(engine, options);
    }
}

void Widget::onPaintCreate(graphics::Engine* engine) {
    auto layout = graphics::BuiltinGeometryLayout::XYRGB;
    triangles_ = engine->createDynamicTriangleListView(layout);
}

void Widget::onPaintPrepare(graphics::Engine* /*engine*/, PaintOptions /*options*/) {
}

void Widget::onPaintDraw(graphics::Engine* engine, PaintOptions options) {
    paintBackground(engine, options);
    paintChildren(engine, options);
}

void Widget::onPaintDestroy(graphics::Engine* /*engine*/) {
    triangles_.reset();
}

void Widget::paintBackground(graphics::Engine* engine, PaintOptions /*options*/) {
    if (backgroundColor_.a() > 0) {
        if (backgroundChanged_) {
            backgroundChanged_ = false;
            core::FloatArray a;
            detail::insertRect(a, styleMetrics(), backgroundColor_, rect(), borderRadii_);
            engine->updateVertexBufferData(triangles_, std::move(a));
        }
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(triangles_);
    }
}

void Widget::paintChildren(graphics::Engine* engine, PaintOptions options) {
    for (Widget* widget : children()) {
        if (!widget->isVisible()) {
            continue;
        }
        engine->pushViewMatrix();
        geometry::Mat4f m = engine->viewMatrix();
        m.translate(widget->position());
        engine->setViewMatrix(m);
        widget->paint(engine, options);
        engine->popViewMatrix();
    }
}

void Widget::startMouseCapture() {
    // TODO: after we implement WidgetTree, make it safer
    // by listening to mouseCaptor deletion or change of tree
    Widget* r = root();
    if (r->mouseCaptor_ && r->mouseCaptor_ != this) {
        r->mouseCaptor_->stopMouseCapture();
    }
    r->mouseCaptor_ = this;
    r->mouseCaptureStarted().emit();
}

void Widget::stopMouseCapture() {
    Widget* r = root();
    if (r->mouseCaptor_ == this) {
        r->mouseCaptor_ = nullptr;
        r->mouseCaptureStopped().emit();
    }
}

void Widget::startKeyboardCapture() {
    // TODO: after we implement WidgetTree, make it safer
    // by listening to keyboardCaptor deletion or change of tree
    Widget* r = root();
    if (r->keyboardCaptor_ && r->keyboardCaptor_ != this) {
        r->keyboardCaptor_->stopKeyboardCapture();
    }
    r->keyboardCaptor_ = this;
    r->keyboardCaptureStarted().emit();
}

void Widget::stopKeyboardCapture() {
    Widget* r = root();
    if (r->keyboardCaptor_ == this) {
        r->keyboardCaptor_ = nullptr;
        r->keyboardCaptureStopped().emit();
    }
}

bool Widget::mouseMove(MouseMoveEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "mouseMove() can only be called on a root widget.");
        return false;
    }
    lastMousePosition_ = event->position();
    lastModifierKeys_ = event->modifierKeys();
    lastTimestamp_ = event->timestamp();
    WidgetPtr thisPtr = this;

    // We first check whether this MouseMove can be handled by a registered action.
    //
    bool handled = handleMouseMoveActions_(event);

    // Otherwise, propagate the mouse move through the hover chain.
    //
    if (!handled) {

        // Update the hover-chain.
        //
        // This is the most typical scenario when we want to update
        // the hover-chain: the mouse juste moved, so we need to update
        // what's under the mouse, then call the onMouseMove() callbacks.
        //
        // Note that if handleMouseMoveActions_() returns true, then
        // we should not call updateHoverChain() in this
        // function, since it was already taken care of.
        //
        updateHoverChain();

        // Call the preMouseMove() and onMouseMove() callbacks.
        //
        if (isAlive()) {
            mouseMove_(event);
            handled = event->isHandled();
        }

        // TODO: update the hover-chain again if ForceUnlock was used. For now
        // we don't do it since none of our current widgets use ForceUnlock,
        // and it would be quite time-consuming to call updateHoverChain()
        // twice on each mouse move.
    }
    return handled;
}

bool Widget::mousePress(MousePressEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "mousePress() can only be called on a root widget.");
        return false;
    }
    lastMousePosition_ = event->position();
    lastModifierKeys_ = event->modifierKeys();
    lastTimestamp_ = event->timestamp();

    WidgetPtr thisPtr = this;

    // We first check whether this MousePress can be handled by a registered action.
    //
    bool handled = handleMousePressActions_(event);

    // Otherwise, propagate the mouse press through the hover chain.
    //
    if (!handled) {
        const bool isFirstPressedButton = pressedButtons_.isEmpty();

        isFocusSetOrCleared_ = false;
        mousePress_(event);
        handled = event->isHandled();

        // Update the hover-chain.
        //
        // Typically, we do not need/want to update the hover chain on
        // mousePress(), since a mousePress() is only supposed to perform an
        // action based on the hover state, not change the hover state itself.
        //
        // Also, if a mouse press action modifies the geometry (for example, a
        // widget moves or is deleted), then updateGeometry() will already take
        // care of calling updateHoverChain().
        //
        // However, the onMousePress() callbacks have the option to ForceUnlock
        // the hover-chain, which while rare and discouraged means that we do
        // need to recompute which widget is hovered.
        //
        updateHoverChain();

        // Update focus using the FocusPolicy, unless the focus was explicitly
        // set or cleared during the handling of the event, in which case
        // such explicit handling takes priority.
        //
        if (isFirstPressedButton && !isFocusSetOrCleared_) {
            Widget* clickedWidget = this;
            Widget* child = hoverChainChild();
            while (child) {
                clickedWidget = child;
                child = child->hoverChainChild();
            }
            updateFocusOnClick_(clickedWidget);
        }
    }

    return handled;
}

bool Widget::mouseRelease(MouseReleaseEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "mouseRelease() can only be called on a root widget.");
        return false;
    }
    lastMousePosition_ = event->position();
    lastModifierKeys_ = event->modifierKeys();
    lastTimestamp_ = event->timestamp();
    WidgetPtr thisPtr = this;

    // We first check whether this MouseRelease can be handled by a registered action.
    //
    bool handled = handleMouseReleaseActions_(event);

    // Otherwise, propagate the mouse release through the hover chain.
    //
    if (!handled) {
        mouseRelease_(event);
        handled = event->isHandled();

        // Update the hover-chain.
        //
        // It is important to update the hover-chain after calling
        // mouseRelease_(), since the default behavior is to unlock the
        // hover-chain (that was previously locked by mousePress_()), and
        // therefore we need to update which widget is now hovered.
        //
        updateHoverChain();
    }
    return handled;
}

bool Widget::mouseScroll(ScrollEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "mouseScroll() can only be called on a root widget.");
        return false;
    }
    lastMousePosition_ = event->position();
    lastModifierKeys_ = event->modifierKeys();
    lastTimestamp_ = event->timestamp();

    WidgetPtr thisPtr = this;
    mouseScroll_(event);
    return event->isHandled();

    // Note: there is no need to explicitly update the hover-chain in this
    // function. If the position of widgets changed as a result of the scroll
    // (which is typical), then the hover-chain will already be indirectly
    // updated as a result of that change.
}

void Widget::preMouseMove(MouseMoveEvent* /*event*/) {
    // no-op
}

void Widget::preMousePress(MousePressEvent* /*event*/) {
    // no-op
}

void Widget::preMouseRelease(MouseReleaseEvent* /*event*/) {
    // no-op
}

bool Widget::onMouseMove(MouseMoveEvent* /*event*/) {
    return false;
}

bool Widget::onMousePress(MousePressEvent* /*event*/) {
    return false;
}

bool Widget::onMouseRelease(MouseReleaseEvent* /*event*/) {
    return false;
}

bool Widget::onMouseScroll(ScrollEvent* /*event*/) {
    return false;
}

void Widget::updateFocusOnClick_(Widget* clickedWidget) {

    // If the clicked widget or any of its ancestors is visible and has a Click
    // focus policy, then we give the focus to the deepmost one.
    //
    while (clickedWidget) {
        if (clickedWidget->focusPolicy().has(FocusPolicy::Click)
            && clickedWidget->isVisible()) {

            clickedWidget->setFocus(FocusReason::Mouse);
            return;
        }
        clickedWidget = clickedWidget->parent();
    }

    // If neither the clicked widget nor any or its ancestors has a
    // Click focus policy, then we should behave as if we clicked on
    // an empty space, so we clear the focus of the current focused
    // widget if its focus strength is Low or Medium.
    //
    WidgetPtr focusedWidget_ = focusedWidget();
    if (focusedWidget_ && focusedWidget_->focusStrength() != FocusStrength::High) {
        focusedWidget_->clearFocus(FocusReason::Mouse);
    }
}

void Widget::appendToPendingMouseActionEvents_(MouseEvent* event) {
    if (pendingMouseClickAction_ || pendingMouseDragAction_) {
        // Note: the widget is set to a non-null value when delivering the
        // event, see maybeStartPendingMouseAction_().
        Widget* widget = nullptr;
        pendingMouseActionEvents_.append(MouseActionEvent::create(event, widget));
    }
}

void Widget::mapMouseActionPosition_(MouseEvent* event, Widget* widget) {
    geometry::Vec2f position = mapTo(widget, event->position());
    event->setPosition(position);
}

namespace {

// Time elapsed from press after which the action becomes a drag.
// TODO: make it a user preference. Allow setting it in px?
inline constexpr double dragTimeThreshold = 0.5; // in seconds
inline constexpr style::Length dragDeltaThreshold(5, style::LengthUnit::Dp);

} // namespace

// TODO: Allow starting actions on mouse release (even when there is no
// conflict), based on user preference (could be a property of Shortcut).
//
// Indeed, in some cases, it may be preferrable to always triggers a MouseClick
// on mouse release, even if there is no MouseDrag with the same shortcut.
//
// Also, in some cases, it may be preferrable to start a MouseDrag on
// button/key release, so that the drag action can be performed without having
// to hold the button/key. We would then wait a second button/key release to
// end the action. The shortcut to end the action could be different than the
// one used to start the action.
//
void Widget::maybeStartPendingMouseAction_() {

    // We cannot start an action if there is no event to handle.
    //
    if (pendingMouseActionEvents_.isEmpty()) {
        return;
    }

    // If both a ClickAction and DragAction are available for the same
    // shortcut, it's a conflict. We resolve the conflict by waiting until the
    // mouse moved enough, or enough time passed, or the mouse button was
    // released.
    //
    if (pendingMouseClickAction_ && pendingMouseDragAction_) {
        if (pendingMouseActionEvents_.length() >= 2) {
            MouseEvent* pressEvent = pendingMouseActionEvents_.first().get();
            MouseEvent* moveEvent = pendingMouseActionEvents_.last().get();
            double deltaTime = moveEvent->timestamp() - pressEvent->timestamp();
            float deltaInPx = (moveEvent->position() - pressEvent->position()).length();
            float deltaThresholdInPx = dragDeltaThreshold.toPx(styleMetrics());
            if (deltaInPx > deltaThresholdInPx || deltaTime > dragTimeThreshold) {

                // => Conflict resolved: it's a drag action.
                pendingMouseClickWidget_ = nullptr;
                pendingMouseClickAction_ = nullptr;
            }
        }
    }

    // Trigger or Start the action if there is no conflict.
    //
    if (pendingMouseClickAction_ && !pendingMouseDragAction_) {

        // Clear pending state.
        //
        MouseActionEventPtr event = pendingMouseActionEvents_.first();
        WidgetPtr clickedWidget = pendingMouseClickWidget_;
        ActionPtr clickedAction = pendingMouseClickAction_;
        pendingMouseActionEvents_.clear();
        pendingMouseClickWidget_ = nullptr;
        pendingMouseClickAction_ = nullptr;

        // Trigger the MouseClick.
        //
        updateFocusOnClick_(clickedWidget.get());
        event->setWidget(pendingMouseClickWidget_.get());
        mapMouseActionPosition_(event.get(), clickedWidget.get());
        clickedAction->onMouseClick(event.get());

        // XXX Call updateHoverChain() here? Indeed, a previous mouse
        //     move might have restrainted from updating the hover
        //     state due to waiting for conflicts to be resolved.
    }
    else if (pendingMouseDragAction_ && !pendingMouseClickAction_) {

        // Set pending state as current state.
        //
        currentMouseDragWidget_ = pendingMouseDragWidget_;
        currentMouseDragAction_ = pendingMouseDragAction_;

        // Clear pending state.
        //
        core::Array<MouseActionEventPtr> events = std::move(pendingMouseActionEvents_);
        pendingMouseActionEvents_.clear();
        pendingMouseDragWidget_ = nullptr;
        pendingMouseDragAction_ = nullptr;

        // Start the MouseDrag.
        //
        updateFocusOnClick_(currentMouseDragWidget_.get());
        bool isStart = true;
        for (const MouseActionEventPtr& event : events) {
            event->setWidget(currentMouseDragWidget_.get());
            mapMouseActionPosition_(event.get(), currentMouseDragWidget_.get());
            if (isStart) {
                currentMouseDragAction_->onMouseDragStart(event.get());
                isStart = false;
            }
            else {
                // TODO: only give the last mouse move if
                // action->isMouseMoveCompressionEnabled() is true
                currentMouseDragAction_->onMouseDragMove(event.get());
            }
        }
    }
}

// XXX Should we call onMouseDragCancel(), or is it too risky?
//
void Widget::cancelActionsFromDeadWidgets_() {
    if (currentMouseDragAction_ && !currentMouseDragWidget_.isAlive()) {
        currentMouseDragWidget_ = nullptr;
        currentMouseDragAction_ = nullptr;
    }
    if (pendingMouseDragAction_ && !pendingMouseDragWidget_.isAlive()) {
        pendingMouseDragWidget_ = nullptr;
        pendingMouseDragAction_ = nullptr;
    }
    if (pendingMouseClickAction_ && !pendingMouseClickWidget_.isAlive()) {
        pendingMouseClickWidget_ = nullptr;
        pendingMouseClickAction_ = nullptr;
    }
}

namespace {

struct CollectActionsParams {
    Shortcut shortcut = {};
    Widget* widget = {};
    CommandType commandType = {};
};

// Sets the pending mouse widget/action to the first action
// that has a shortcut matching the given shortcut.
//
void collectActions(
    const CollectActionsParams& params,
    WidgetPtr& pendingWidget_,
    ActionPtr& pendingAction_) {

    for (Action* action : params.widget->actions()) {
        if (action->type() == params.commandType) {
            for (const Shortcut& shortcut : action->userShortcuts()) {
                if (shortcut == params.shortcut) {
                    pendingWidget_ = params.widget;
                    pendingAction_ = action;
                    return;
                }
            }
        }
    }
}

} // namespace

// Check for registered actions whose shortcut match the pressed mouse button
// and modifiers. Returns true if an action has been handled this way, which
// means that the mouse press shouldn't be propagated through the widget
// hierarchy via onMousePress().
//
bool Widget::handleMousePressActions_(MouseEvent* event) {

    cancelActionsFromDeadWidgets_();

    // Ignore mouse press if there is a pending or current mouse action.
    //
    // TODO: support sub-actions, for example, right-clicking in the middle of
    // a left-drag to do something special. We could implement this by calling
    // an onMousePress() handler on the action itself.
    //
    if (pendingMouseClickAction_ || pendingMouseDragAction_ || currentMouseDragAction_) {
        return true;
    }

    // Collect possible mouse actions corresponding to this mouse press.
    //
    // If there are two conflicting MouseClick actions (or two conflicting
    // MouseDrag actions), then we prioritize the inner-most in widget depth,
    // then the first in action list order.
    //
    CollectActionsParams params;
    params.shortcut.setMouseButton(event->button());
    params.shortcut.setModifierKeys(event->modifierKeys());
    for (Widget* widget = this; widget; widget = widget->hoverChainChild()) {
        params.widget = widget;

        // Collect MouseClick actions
        params.commandType = CommandType::MouseClick;
        collectActions(params, pendingMouseClickWidget_, pendingMouseClickAction_);

        // Collect MouseDrag actions
        params.commandType = CommandType::MouseDrag;
        collectActions(params, pendingMouseDragWidget_, pendingMouseDragAction_);
    }

    if (pendingMouseClickAction_ || pendingMouseDragAction_) {

        // If there is a pending action, start it now or defer start in case of
        // conflicts. We return true anyway so that the rest of the application
        // behaves as if the action was already in progress.
        //
        mouseActionButton_ = event->button();
        pendingMouseActionEvents_.clear();
        appendToPendingMouseActionEvents_(event);
        maybeStartPendingMouseAction_();
        return true;
    }
    else {
        // If there is no pending action, this means that the event is not
        // handled via registered actions, so we need to propagate the
        // MousePress through the widget hierarchy for raw handling without
        // actions.
        //
        return false;
    }
}

bool Widget::handleMouseMoveActions_(MouseEvent* event) {

    cancelActionsFromDeadWidgets_();

    if (currentMouseDragAction_) {

        // If there is a current mouse drag action, redirect the mouse move to this action.
        //
        mapMouseActionPosition_(event, currentMouseDragWidget_.get());
        currentMouseDragAction_->onMouseDragMove(event);
        return true;
    }
    else if (pendingMouseClickAction_ || pendingMouseDragAction_) {

        // Resolve conflicts.
        //
        appendToPendingMouseActionEvents_(event);
        maybeStartPendingMouseAction_();
        return true;
    }
    else {
        return false;
    }
}

bool Widget::handleMouseReleaseActions_(MouseEvent* event) {

    // Note: if there is a current or pending action, then we return true even
    // if the released button is not the same as the action's button, to
    // prevent any further processing while an action is current or pending.

    cancelActionsFromDeadWidgets_();

    bool handled = false;
    bool actionEnded = false;

    if (currentMouseDragAction_) {

        // Confirm the action if the button matches.
        //
        if (mouseActionButton_ == event->button()) {
            mapMouseActionPosition_(event, currentMouseDragWidget_.get());
            currentMouseDragAction_->onMouseDragConfirm(event);
            currentMouseDragWidget_ = nullptr;
            currentMouseDragAction_ = nullptr;
            actionEnded = true;
        }
        handled = true;
    }
    else if (pendingMouseClickAction_ || pendingMouseDragAction_) {

        // If there is both a pending click and a pending drag, then this resolves
        // the conflict: it's a click, not a drag.
        //
        // If there is no pending click action, this means that there was a potential
        // MouseDrag that could have happened, but it's too late now: we don't
        // want to initiate a MouseDrag on MouseRelease, so we just discard it.
        //
        if (mouseActionButton_ == event->button()) {
            pendingMouseDragWidget_ = nullptr;
            pendingMouseDragAction_ = nullptr;
            maybeStartPendingMouseAction_(); // trigger pending click if any
            actionEnded = true;
        }
        handled = true;
    }

    if (actionEnded) {
        mouseActionButton_ = MouseButton::None;
        unlockHover_();
        updateHoverChain();
    }
    return handled;
}

bool Widget::checkAlreadyHovered_() {
    if (!isHovered_) {
        WidgetPtr thisPtr = this;
        VGC_WARNING(
            LogVgcUi,
            "Widget should have been hovered prior to receiving a mouse event.");
        setHovered(true);
        if (!isAlive()) {
            return false;
        }
    }
    return true;
}

void Widget::mouseMove_(MouseMoveEvent* event) {

    geometry::Vec2f eventPos = event->position();

    if (!checkAlreadyHovered_()) {
        return;
    }

    // Prepare against death of `this`
    WidgetPtr thisPtr = this;

    // User-defined capture phase handler.
    preMouseMove(event);
    if (!isAlive()) {
        // Widget got killed. Event can be considered handled.
        event->handled_ = true;
        return;
    }

    // Handle stop propagation.
    if (event->isStopPropagationRequested()) {
        return;
    }

    // Get hover-chain child (possibly changed in preMouseMove).
    Widget* hcChild = hoverChainChild();

    // Call hover-chain child's handler.
    if (hcChild) {
        // Prepare against widget killers.
        WidgetPtr hcChildPtr = hcChild;
        // Call handler.
        event->setPosition(mapTo(hcChild, eventPos));
        hcChild->mouseMove_(event);
        // Check for deaths.
        if (!isAlive() || !hcChild->isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
        // Handle stop propagation.
        if (event->isStopPropagationRequested()) {
            return;
        }
        // Restore event position.
        event->setPosition(eventPos);
    }

    HoverLockPolicy hoverLockPolicy = HoverLockPolicy::Default;

    if (!event->handled_ || handledEventPolicy_ == HandledEventPolicy::Receive) {
        event->setHoverLockPolicy(HoverLockPolicy::Default);
        // User-defined bubble phase handler.
        event->handled_ |= onMouseMove(event);
        if (!isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
        hoverLockPolicy = event->hoverLockPolicy();
    }

    // Update hover-lock state based on the given policy.
    // By default, we keep current state.
    switch (hoverLockPolicy) {
    case HoverLockPolicy::ForceUnlock:
        unlockHover_(); // It also releases pressed buttons.
        break;
    case HoverLockPolicy::ForceLock:
        lockHover_();
        break;
    case HoverLockPolicy::Default:
    default:
        // Keep currrent hover-lock state.
        break;
    }
}

void Widget::mousePress_(MousePressEvent* event) {
    const geometry::Vec2f eventPos = event->position();

    if (!checkAlreadyHovered_()) {
        return;
    }

    // User-defined capture phase handler.
    WidgetPtr thisPtr = this;
    preMousePress(event);
    if (!isAlive()) {
        // Widget got killed. Event can be considered handled.
        event->handled_ = true;
        return;
    }

    // Handle stop propagation.
    if (event->isStopPropagationRequested()) {
        return;
    }

    // Get hover-chain child without update.
    Widget* hcChild = hoverChainChild();

    // Set button as pressed.
    pressedButtons_.set(event->button());

    // Call hover-chain child's handler.
    if (hcChild) {

        // Prepare against widget killers.
        WidgetPtr hcChildPtr = hcChild;

        // Set event position.
        event->setPosition(mapTo(hcChild, eventPos));

        // Recurse
        hcChild->mousePress_(event);

        // Check for deaths.
        if (!isAlive() || !hcChildPtr.isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }

        // Handle stop propagation.
        if (event->isStopPropagationRequested()) {
            return;
        }

        // Restore event position.
        event->setPosition(eventPos);
    }
    else {
        // By default, if no child is hovered (as when clicking on a Flex gap)
        // we don't want to allow children to be hovered until the release.
        // It is to behave as-if the "press-move-release" sequence is captured
        // by an invisible background widget.
        isChildHoverEnabled_ = false;
    }

    HoverLockPolicy hoverLockPolicy = HoverLockPolicy::Default;

    if (!event->handled_ || handledEventPolicy_ == HandledEventPolicy::Receive) {
        event->setHoverLockPolicy(HoverLockPolicy::Default);
        event->handled_ |= onMousePress(event);
        if (!isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
        hoverLockPolicy = event->hoverLockPolicy();
    }

    // Update hover-lock state based on the given policy.
    // By default, we hover-lock the widget to capture the "press-move-release"
    // sequence.
    switch (hoverLockPolicy) {
    case HoverLockPolicy::ForceUnlock:
        unlockHover_(); // It also releases pressed buttons.
        break;
    case HoverLockPolicy::ForceLock:
    case HoverLockPolicy::Default:
    default:
        lockHover_();
        break;
    }
}

void Widget::mouseRelease_(MouseReleaseEvent* event) {

    geometry::Vec2f eventPos = event->position();

    if (!checkAlreadyHovered_()) {
        return;
    }

    // User-defined capture phase handler.
    WidgetPtr thisPtr = this;
    preMouseRelease(event);
    if (!isAlive()) {
        // Widget got killed. Event can be considered handled.
        event->handled_ = true;
        return;
    }

    // Handle stop propagation.
    if (event->isStopPropagationRequested()) {
        return;
    }

    // Get hover-chain child without update.
    Widget* hcChild = hoverChainChild();

    // Set button as not pressed.
    pressedButtons_.unset(event->button());
    const bool otherStillPressed = bool(pressedButtons_);

    // Call hover-chain child's handler.
    if (hcChild) {
        // Prepare against widget killers.
        WidgetPtr hcChildPtr = hcChild;
        // Call handler.
        event->setPosition(mapTo(hcChild, eventPos));
        hcChild->mouseRelease_(event);
        // Check for deaths.
        if (!isAlive() || !hcChild->isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
        // Handle stop propagation.
        if (event->isStopPropagationRequested()) {
            return;
        }
        // Restore event position.
        event->setPosition(eventPos);
    }

    HoverLockPolicy hoverLockPolicy = HoverLockPolicy::Default;

    if (!event->handled_ || handledEventPolicy_ == HandledEventPolicy::Receive) {
        event->setHoverLockPolicy(HoverLockPolicy::Default);
        // User-defined bubble phase handler.
        event->handled_ |= onMouseRelease(event);
        if (!isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
        hoverLockPolicy = event->hoverLockPolicy();
    }

    // Update hover-lock state based on the given policy.
    // By default, we keep the hover locked if buttons are still pressed,
    // otherwise unlock the hover.
    bool shouldLock = false;
    switch (hoverLockPolicy) {
    case HoverLockPolicy::ForceUnlock:
        break;
    case HoverLockPolicy::ForceLock:
        shouldLock = true;
        break;
    case HoverLockPolicy::Default:
    default:
        shouldLock = otherStillPressed;
        break;
    }

    if (shouldLock) {
        lockHover_();
    }
    else {
        unlockHover_(); // It also releases pressed buttons.
    }
}

void Widget::mouseScroll_(ScrollEvent* event) {

    geometry::Vec2f eventPos = event->position();

    if (!checkAlreadyHovered_()) {
        return;
    }

    // User-defined capture phase handler is not implemented yet.

    // Keep weak pointer to `this`.
    WidgetPtr thisPtr = this;

    // Get hover-chain child without update.
    Widget* hcChild = hoverChainChild();

    // Call hover-chain child's handler.
    if (hcChild) {
        // Prepare against widget killers.
        WidgetPtr hcChildPtr = hcChild;
        // Call handler.
        event->setPosition(mapTo(hcChild, eventPos));
        hcChild->mouseScroll_(event);
        // Check for deaths.
        if (!isAlive() || !hcChild->isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
        // Handle stop propagation.
        if (event->isStopPropagationRequested()) {
            return;
        }
        // Restore event position.
        event->setPosition(eventPos);
    }

    // Scroll events cannot be used as action start nor end, thus we
    // do not change the current hover-lock state.

    if (!event->handled_ || handledEventPolicy_ == HandledEventPolicy::Receive) {
        // User-defined bubble phase handler.
        event->handled_ |= onMouseScroll(event);
        if (!isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
    }
}

Widget* Widget::computeHoverChainChild(MouseHoverEvent* event) const {

    // Return null if child hovering is disabled.
    //
    if (!isChildHoverEnabled()) {
        return nullptr;
    }

    // Iterate over child widgets in reverse order, so that widgets drawn last
    // receive the event first.
    //
    geometry::Vec2f pos = event->position();
    for (Widget* child = lastChild(); //
         child != nullptr;            //
         child = child->previousSibling()) {

        event->setPosition(mapTo(child, pos));
        if (child->computeIsHovered(event)) {
            return child;
        }
    }

    return nullptr;
}

bool Widget::computeIsHovered(MouseHoverEvent* event) const {
    return isVisible() && rect().contains(event->position());
}

bool Widget::setHovered(bool hovered) {

    if (isHovered_ == hovered) {
        // Nothing to do.
    }
    else if (hovered) {
        VGC_ASSERT(!hoverChainParent_);

        // Trivial cases
        // -------------
        {
            Widget* p = parent();
            if (!p) {
                // This is the root.
                if (!isHovered_) {
                    mouseEnter_();
                }
            }
            else if (p->isHovered()) {
                return p->setHoverChainChild(this);
            }
        }

        // Generic case
        // ------------
        // Handlers of onMouseEnter and onMouseLeave could
        // modify the hierarchy while we fix the hover chain
        // so let's compute it first.
        core::Array<WidgetPtr> path; // XXX small array would be nice.
        path.append(this);
        {
            Widget* p = parent();
            while (p) {
                path.append(p);
                if (p->isHovered()) {
                    break;
                }
                p = p->parent();
            }
        }
        auto it = path.rbegin();
        WidgetPtr thisPtr = this;
        WidgetPtr currentParentPtr = *(it++);
        Widget* p = currentParentPtr.get();
        Widget* c = nullptr;
        // If first parent is root, it could be not hovered yet.
        if (!p->isHovered_) {
            p->mouseEnter_();
        }

        // From here, let's be careful about dangling pointers.
        bool aborted = false;
        for (; it != path.rend(); ++it) {
            c = it->getIfAlive();
            if (!c) {
                continue;
            }

            if (!p->setHoverChainChild(c)) {
                // Check our pointers to see if we can recover from this failure.
                p = currentParentPtr.getIfAlive();
                c = it->getIfAlive();
                if (!p) {
                    // Let's abort if current parent has died.
                    // Otherwise it could infinite loop..
                    aborted = true;
                    break;
                }
                if (!c) {
                    // Let's skip this child.
                    // XXX Opinion ?
                    continue;
                }
                // Unexpected conflict in setHoverChainChild().
                // Let's abort.
                aborted = true;
                break;
            }

            // Current child is the next parent.
            currentParentPtr = c;
            p = c;
        }
        return !aborted;
    }
    else {
        WidgetPtr thisPtr = this;

        // Unhover chain child
        if (hoverChainChild_) {
            hoverChainChild_->setHovered(false);
        }
        if (!isAlive()) {
            return true;
        }

        // We could fake releasing buttons here with pressedButtons_ if desired.
        /**/

        // Notify leave
        mouseLeave_();

        if (!isAlive()) {
            return true;
        }

        // Unlink from hover-chain
        if (hoverChainParent_) {
            Widget* p = hoverChainParent_;
            p->hoverChainChild_ = nullptr;
            hoverChainParent_ = nullptr;
        }
        isHovered_ = false;
        onUnhover_();
    }

    return true;
}

bool Widget::setHoverChainChild(Widget* newHoverChainChild) {

    if (!isHovered_) {
        VGC_WARNING(
            LogVgcUi,
            "Cannot set the hovered child of a widget that is not itself hovered");
        return false;
    }
    if (hoverChainChild_ == newHoverChainChild) {
        return true;
    }

    WidgetPtr thisPtr = this;
    if (!newHoverChainChild) {
        if (!hoverChainChild_->setHovered(false)) {
            return false;
        }
        return isAlive();
    }

    WidgetPtr newHoverChainChildPtr = newHoverChainChild;

    // Unhover child's children
    if (hoverChainChild_) {
        hoverChainChild_->setHovered(false);
        if (!isAlive()) {
            return false;
        }
        VGC_ASSERT(hoverChainChild_ == nullptr);
        if (!newHoverChainChild->isAlive()) {
            return false;
        }
    }

    // Abort if the new child is already hovered, it would create a cycle.
    if (newHoverChainChild->isHovered_) {
        return false;
    }

    // Link parent and child.
    hoverChainChild_ = newHoverChainChild;
    newHoverChainChild->hoverChainParent_ = this;

    // Notify enter
    hoverChainChild_->mouseEnter_();

    // Returns whether the requested state is set or not.
    return isAlive() && newHoverChainChild->isAlive()
           && hoverChainChild_ == newHoverChainChild;
}

void Widget::onUnhover_() {
    if (isHoverLocked_) {
        isHoverLocked_ = false;
        onHoverUnlocked_();
    }
}

void Widget::lockHover_() {
    Widget* w = this;
    while (w && !w->isHoverLocked()) {
        w->isHoverLocked_ = true;
        w = w->hoverChainParent();
    }
}

void Widget::unlockHover_() {
    Widget* w = this;
    while (w && w->isHoverLocked()) {
        w->isHoverLocked_ = false;
        w->onHoverUnlocked_();
        w = w->hoverChainChild();
    }
}

void Widget::onHoverUnlocked_() {
    isChildHoverEnabled_ = true;
    pressedButtons_.clear();
}

void Widget::setVisibility(Visibility visibility) {
    if (visibility == visibility_) {
        return;
    }
    visibility_ = visibility;
    updateComputedVisibility_();
    Widget* p = parent();
    if (p) {
        requestGeometryUpdate();
    }
}

void Widget::onParentWidgetChanged(Widget*) {
    // no-op
}

void Widget::onWidgetAdded(Widget*, bool) {
    // no-op
}

void Widget::onWidgetRemoved(Widget*) {
    // no-op
}

void Widget::mouseHover_() {

    // New hover data
    Widget* root = this->root();
    MouseButton button = MouseButton::None;
    geometry::Vec2f position = root->mapTo(this, root->lastMousePosition_);
    ModifierKeys modifierKeys = root->lastModifierKeys_;
    double timestamp = root->lastTimestamp_;

    // Determine if we should send the mouse hover.
    //
    // One option is to only send the mouse hover if the position of the mouse
    // or the modifiers have changed. This avoids sending "useless" events, but
    // unfortunately it does not work in all situations: sometimes, the mouse
    // position hasn't changed but widgets would still like to receive a mouse
    // hover because their internal state changed, and therefore something else
    // is now hovered.
    //
    // Therefore, for now, we unconditionally send the mouse hover, but keep
    // the other implementation in case we need it again later.
    //
    // For example, a better approach might be to implement a
    // `requestHoverUpdate()` method, so that widgets could opt-in receiving a
    // mouse hover when they need to recompute their hover state. Otherwise, it
    // would only be sent if the mouse position or modifiers have changed.
    //
    constexpr bool unconditionalMouseHover = true;
    bool doMouseHover = false;
    if (unconditionalMouseHover) {
        doMouseHover = true;
    }
    else if (forceNextMouseHover_) {
        doMouseHover = true;
    }
    else if (
        lastMouseHoverPosition_ != position
        || lastMouseHoverModifierKeys_ != modifierKeys) {

        doMouseHover = true;
    }

    // Do the mouse hover if necessary
    if (doMouseHover) {
        lastMouseHoverPosition_ = position;
        lastMouseHoverModifierKeys_ = modifierKeys;
        MouseHoverEventPtr hoverEvent = MouseHoverEvent::create();
        hoverEvent->setTimestamp(timestamp);
        hoverEvent->setModifierKeys(modifierKeys);
        hoverEvent->setPosition(position); // XXX: what about isTablet?
        hoverEvent->setButton(button);
        onMouseHover(hoverEvent.get());
        forceNextMouseHover_ = false;
    }
}

void Widget::mouseEnter_() {
    WidgetPtr thisPtr = this;
    isHovered_ = true;
    onMouseEnter();
    if (thisPtr.isAlive()) {
        forceNextMouseHover_ = true;
        mouseHover_();
    }
}

void Widget::mouseLeave_() {
    // isHovered_ is set to false in setHovered()
    onMouseLeave();
}

void Widget::onMouseHover(MouseHoverEvent*) {
}

void Widget::onMouseEnter() {
}

void Widget::onMouseLeave() {
}

bool Widget::updateHoverChainChild(MouseHoverEvent* event) {
    Widget* hcChild = computeHoverChainChild(event);
    return setHoverChainChild(hcChild);
}

// TODO: fix recursion discovered with the "opened menu + window resize" crash.
// updateGeometry->updateHoverChain->mapTo->position->updateRootGeometry_->updateGeometry->updateHoverChain
bool Widget::updateHoverChain() {

    // We do not want to update the hover-chain if this widget isn't hovered,
    // or if a MouseDrag action is in progress.
    //
    if (!isAlive() || !isHovered() || currentMouseDragAction()) {
        return false;
    }

    // Useful variables.
    //
    WidgetPtr root = this->root();
    MouseButton button = MouseButton::None;
    geometry::Vec2f position = root->lastMousePosition_;
    ModifierKeys modifiers = root->lastModifierKeys_;
    double timestamp = root->lastTimestamp_;

    // Start by traversing the locked part of the hover-chain.
    //
    // We keep this part of the chain unchanged, that is, we do no call
    // updateHoverChainChild() for these widgets.
    //
    WidgetPtr widget = this;
    WidgetPtr child = hoverChainChild();
    while (child && child->isHoverLocked()) {
        widget = child;
        child = widget->hoverChainChild();
    }

    // Update the remaining (non-locked) part of the hover-chain.
    //
    WidgetPtr leaf = widget;
    bool changed = false;
    while (widget && widget->isChildHoverEnabled_) {
        WidgetPtr oldChild = child;
        geometry::Vec2f relPos = root->mapTo(widget.get(), position);
        MouseHoverEventPtr hoverEvent = MouseHoverEvent::create();
        hoverEvent->setTimestamp(timestamp);
        hoverEvent->setModifierKeys(modifiers);
        hoverEvent->setPosition(relPos); // XXX: what about isTablet?
        hoverEvent->setButton(button);
        bool success = widget->updateHoverChainChild(hoverEvent.get());
        if (!success || !widget->isAlive()) {
            changed = true;
            break;
        }
        leaf = widget;
        child = widget->hoverChainChild();
        if (child != oldChild) {
            changed = true;
        }
        widget = child;
    }

    // Call onMouseHover from leaf to root.
    //
    if (leaf.isAlive()) {
        widget = leaf;
        while (widget) {
            widget->mouseHover_();
            if (widget.get() == this) {
                widget = nullptr;
            }
            else {
                widget = widget->hoverChainParent_;
            }
        }
    }

    return changed;
}

void Widget::onVisible() {
    // no-op
}

void Widget::onHidden() {
    // no-op
}

void Widget::setTreeActive(bool active, FocusReason reason) {
    Widget* r = root();
    if (r->isTreeActive_ != active) {
        r->isTreeActive_ = active;
        Widget* f = focusedWidget();
        if (f) {
            if (active) {
                f->onFocusIn(reason);
            }
            else {
                f->onFocusOut(reason);
            }
        }
    }
}

void Widget::setFocus(FocusReason reason) {

    Widget* root_ = root();
    root_->isFocusSetOrCleared_ = true;
    core::Array<WidgetPtr>& focusStack = root_->focusStack_;
    core::Array<WidgetPtr> oldFocusStack = focusStack;

    // Update focus stack and emit FocusIn and FocusOut signals.
    //
    WidgetPtr oldFocusedWidget = focusedWidget();
    WidgetPtr newFocusedWidget = this;
    if (oldFocusedWidget != newFocusedWidget) {

        // Update focus stack.
        //
        switch (focusStrength()) {
        case FocusStrength::Low:
            // Remove all widgets that are either not alive or with low strength
            focusStack.removeIf([](const WidgetPtr& w) {
                return !w || w->focusStrength() == FocusStrength::Low;
            });
            break;
        case FocusStrength::Medium:
            // Remove all widgets that are either not alive or with low or medium strength
            focusStack.removeIf([](const WidgetPtr& w) {
                return !w || w->focusStrength() != FocusStrength::High;
            });
            break;
        case FocusStrength::High:
            // Remove all widgets
            focusStack.clear();
            break;
        }
        focusStack.append(this);

        // Emit focus in/out events.
        //
        emitFocusInOutEvents_(oldFocusStack, focusStack, reason);
    }

    // Emit focusSet() signal for this widget and all its ancestor, regardless
    // of whether the focused widget changed.
    //
    WidgetPtr widget = newFocusedWidget;
    while (widget) {
        widget->focusSet().emit(reason);
        widget = widget->parent();
    }
}

void Widget::clearFocus(FocusReason reason) {

    Widget* root_ = root();
    root_->isFocusSetOrCleared_ = true;
    core::Array<WidgetPtr>& focusStack = root_->focusStack_;
    core::Array<WidgetPtr> oldFocusStack = focusStack;

    // Update focus stack.
    //
    WidgetPtr oldFocusedWidget = focusedWidget();
    focusStack.removeIf(
        [this](const WidgetPtr& w) { return !w || w->isDescendantOf(this); });
    WidgetPtr newFocusedWidget = focusedWidget();

    // Emit focus in/out events.
    //
    emitFocusInOutEvents_(oldFocusStack, focusStack, reason);

    // Emit focusCleared() signal for this widget and all its ancestor,
    // regardless of whether the focused widget changed.
    //
    WidgetPtr widget = this;
    while (widget) {
        widget->focusCleared().emit(reason);
        widget = widget->parent();
    }
}

core::Array<WidgetPtr> Widget::focusStack() const {
    core::Array<WidgetPtr> res;
    const core::Array<WidgetPtr>& stack = root()->focusStack_;
    res.reserve(stack.length());
    for (const WidgetPtr& widget : stack) {
        if (widget) {
            res.append(widget);
        }
    }
    return res;
}

Widget* Widget::focusedWidget() const {
    const core::Array<WidgetPtr>& focusStack = root()->focusStack_;
    if (focusStack.isEmpty()) {
        return nullptr;
    }
    else {
        return focusStack.last().get();
    }
}

void Widget::onFocusIn(FocusReason) {
}

void Widget::onFocusOut(FocusReason) {
}

void Widget::onFocusStackIn(FocusReason) {
}

void Widget::onFocusStackOut(FocusReason) {
}

void Widget::setTextInputReceiver(bool isTextInputReceiver) {
    if (isTextInputReceiver != isTextInputReceiver_) {
        isTextInputReceiver_ = isTextInputReceiver;
        textInputReceiverChanged().emit(isTextInputReceiver_);
    }
}

namespace {

core::Array<WidgetPtr> getFocusBranch(Widget* root) {
    core::Array<WidgetPtr> res;
    Widget* widget = root->focusedWidget();
    while (widget) {
        res.append(widget);
        widget = widget->parent();
    }
    std::reverse(res.begin(), res.end());
    return res;
}

} // namespace

bool Widget::keyPress(KeyPressEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "keyPress() can only be called on a root widget.");
        return false;
    }
    WidgetPtr thisPtr = this;
    bool isKeyPress = true;
    core::Array<WidgetPtr> focusBranch = getFocusBranch(this);
    if (!focusBranch.isEmpty()) {
        Int index = 0;
        focusBranch[index]->keyEvent_(event, isKeyPress, focusBranch, index);
    }
    return event->isHandled();
}

bool Widget::keyRelease(KeyReleaseEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "keyRelease() can only be called on a root widget.");
        return false;
    }
    WidgetPtr thisPtr = this;
    bool isKeyPress = false;
    core::Array<WidgetPtr> focusBranch = getFocusBranch(this);
    if (!focusBranch.isEmpty()) {
        Int index = 0;
        focusBranch[index]->keyEvent_(event, isKeyPress, focusBranch, index);
    }
    return event->isHandled();
}

void Widget::preKeyPress(KeyPressEvent*) {
    // no-op
}

void Widget::preKeyRelease(KeyReleaseEvent*) {
    // no-op
}

bool Widget::onKeyPress(KeyPressEvent*) {
    return false;
}

bool Widget::onKeyRelease(KeyReleaseEvent*) {
    return false;
}

void Widget::addAction(Action* action) {
    actions_->append(action);
}

void Widget::removeAction(Action* action) {
    actions_->remove(action);
}

void Widget::clearActions() {
    Action* action = actions_->first();
    while (action) {
        removeAction(action);
        action = actions_->first();
    }
}

void Widget::emitFocusInOutEvents_(
    const core::Array<WidgetPtr>& oldStack,
    const core::Array<WidgetPtr>& newStack,
    FocusReason reason) {

    // Compute everything before sending any signal
    // (in case signal handlers change the state)

    WidgetPtr oldFocusedWidget;
    WidgetPtr newFocusedWidget;
    if (isTreeActive()) {
        if (!oldStack.isEmpty()) {
            oldFocusedWidget = oldStack.last();
        }
        if (!newStack.isEmpty()) {
            newFocusedWidget = newStack.last();
        }
    }

    core::Array<WidgetPtr> removed = oldStack;
    for (const WidgetPtr& widget : newStack) {
        removed.removeAll(widget);
    }

    core::Array<WidgetPtr> added = newStack;
    for (const WidgetPtr& widget : oldStack) {
        added.removeAll(widget);
    }

    // Send all signals

    if (oldFocusedWidget != newFocusedWidget) {
        if (oldFocusedWidget) {
            oldFocusedWidget->onFocusOut(reason);
        }
    }

    for (const WidgetPtr& widget : removed) {
        if (widget) {
            widget->onFocusStackOut(reason);
        }
    }

    for (const WidgetPtr& widget : added) {
        if (widget) {
            widget->onFocusStackIn(reason);
        }
    }

    if (oldFocusedWidget != newFocusedWidget) {
        if (newFocusedWidget) {
            newFocusedWidget->onFocusIn(reason);
        }
    }
}

void Widget::keyEvent_(
    PropagatedKeyEvent* event,
    bool isKeyPress,
    const core::Array<WidgetPtr>& focusChain,
    Int index) {

    // User-defined capture phase handler.
    if (isKeyPress) {
        preKeyPress(static_cast<KeyPressEvent*>(event));
    }
    else {
        preKeyRelease(static_cast<KeyReleaseEvent*>(event));
    }
    if (!isAlive()) {
        // Widget got killed. Event can be considered handled.
        event->handled_ = true;
        return;
    }

    // Handle stop propagation.
    if (event->isStopPropagationRequested()) {
        return;
    }

    // Get focused child
    //
    ++index;
    WidgetPtr focusedChild;
    if (index < focusChain.length()) {
        focusedChild = focusChain[index];
    }

    // Call focused child's handler.
    if (focusedChild) {
        // Call handler.
        focusedChild->keyEvent_(event, isKeyPress, focusChain, index);
        // Check for deaths.
        if (!isAlive() || !focusedChild.isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
        // Handle stop propagation.
        if (event->isStopPropagationRequested()) {
            return;
        }
    }

    // Trigger action if it has a matching shortcut
    if (!event->handled_ && isKeyPress) {
        WidgetPtr matchingWidget;
        ActionPtr matchingAction;
        CollectActionsParams params;
        params.shortcut.setKey(event->key());
        params.shortcut.setModifierKeys(event->modifierKeys());
        params.widget = this;
        params.commandType = CommandType::Trigger;
        collectActions(params, matchingWidget, matchingAction);
        if (matchingAction) {
            event->handled_ = true;
            matchingAction->trigger();
            if (!isAlive()) {
                return;
            }
        }
    }

    // User-defined bubble phase handler
    if (!event->handled_ || handledEventPolicy_ == HandledEventPolicy::Receive) {
        event->handled_ |= isKeyPress
                               ? onKeyPress(static_cast<KeyPressEvent*>(event))
                               : onKeyRelease(static_cast<KeyReleaseEvent*>(event));
        if (!isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
    }
}

geometry::Vec2f Widget::computePreferredSize() const {
    float refLength = 0.0f;
    float valueIfAuto = 0.0f;
    style::LengthOrPercentageOrAuto w = preferredWidth();
    style::LengthOrPercentageOrAuto h = preferredHeight();
    return geometry::Vec2f(
        w.toPx(styleMetrics(), refLength, valueIfAuto),
        h.toPx(styleMetrics(), refLength, valueIfAuto));
}

void Widget::updateChildrenGeometry() {
    // No default layout.
    for (auto c : children()) {
        if (c->isGeometryUpdateRequested()) {
            c->updateGeometry();
        }
    }
}

namespace {

style::Value parseStyleNumber(style::TokenIterator begin, style::TokenIterator end) {
    if (begin == end) {
        return style::Value::invalid();
    }
    else if (begin->type() == style::TokenType::Number && begin + 1 == end) {
        return style::Value::number(begin->floatValue());
    }
    else {
        return style::Value::invalid();
    }
}

} // namespace

// clang-format off

void Widget::populateStyleSpecTable(style::SpecTable* table) {

    if (!table->setRegistered(staticClassName())) {
        return;
    }

    graphics::RichTextSpan::populateStyleSpecTable(table);

    using namespace strings;
    using namespace style::literals;
    using style::LengthOrPercentage;
    using style::LengthOrPercentageOrAuto;

    auto auto_lpa = style::Value::custom(LengthOrPercentageOrAuto());
    auto zero_lp =  style::Value::custom(LengthOrPercentage());
    auto huge_lp =  style::Value::custom(LengthOrPercentage(1e30_dp));
    auto one_n =    style::Value::number(1.0f);

    // Reference: https://www.w3.org/TR/CSS21/propidx.html
    table->insert(min_width,          zero_lp,  false, &LengthOrPercentage::parse);
    table->insert(min_height,         zero_lp,  false, &LengthOrPercentage::parse);
    table->insert(max_width,          huge_lp,  false, &LengthOrPercentage::parse);
    table->insert(max_height,         huge_lp,  false, &LengthOrPercentage::parse);
    table->insert(preferred_width,    auto_lpa, false, &LengthOrPercentageOrAuto::parse);
    table->insert(preferred_height,   auto_lpa, false, &LengthOrPercentageOrAuto::parse);
    table->insert(column_gap,         zero_lp,  false, &LengthOrPercentage::parse);
    table->insert(row_gap,            zero_lp,  false, &LengthOrPercentage::parse);
    table->insert(grid_auto_columns,  auto_lpa, false, &LengthOrPercentageOrAuto::parse);
    table->insert(grid_auto_rows,     auto_lpa, false, &LengthOrPercentageOrAuto::parse);
    table->insert(horizontal_stretch, one_n,    false, &parseStyleNumber);
    table->insert(horizontal_shrink,  one_n,    false, &parseStyleNumber);
    table->insert(vertical_stretch,   one_n,    false, &parseStyleNumber);
    table->insert(vertical_shrink,    one_n,    false, &parseStyleNumber);

    SuperClass::populateStyleSpecTable(table);
}

// clang-format on

void Widget::onChildRemoved(Object* child) {
    if (child == children_) {
        children_ = nullptr;
    }
    else if (child == actions_) {
        actions_ = nullptr;
    }
}

void Widget::onStyleChanged() {

    core::Color oldBackgroundColor = backgroundColor_;
    style::BorderRadii oldBorderRadii = borderRadii_;

    backgroundColor_ = detail::getColor(this, graphics::strings::background_color);
    borderRadii_ = style::BorderRadii(this);

    if (oldBackgroundColor != backgroundColor_ || oldBorderRadii != borderRadii_) {
        backgroundChanged_ = true;
    }

    requestGeometryUpdate();
    requestRepaint();

    SuperClass::onStyleChanged();
}

void Widget::onWidgetAdded_(Widget* widget, bool wasOnlyReordered) {
    if (!wasOnlyReordered) {

        // TODO: insert at a more appropriate location rather than at the end.
        // It doesn't matter for now since StylableObject child index doesn't
        // influence style, but it might in the future if/when we implement the
        // CSS nth-child pseudo class, see:
        //
        //  https://developer.mozilla.org/en-US/docs/Web/CSS/:nth-child)
        //
        // One question is where to insert the child widgets relative to the "extra"
        // stylable objects manually inserted? One idea might be to have a concept
        // of layers: child widgets would be on layer 0, and extra child stylable
        // objets would be on different layers (1, 2, ...), so they would have
        // independent indexing. The API would be something like this:
        //
        //     appendChildStylableObject(child, layerIndex).
        //
        appendChildStylableObject(widget);
    }
    onWidgetAdded(widget, wasOnlyReordered);
    if (!wasOnlyReordered) {
        widget->onParentWidgetChanged(this);
    }
    // may call onVisible, and resume pending requests
    widget->updateComputedVisibility_();
    // XXX temporary bug fix, sometimes pending requests are not resent..
    if (widget->isVisible()) {
        widget->resendPendingRequests_();
    }
    if (!widget->isReparentingWithinSameTree_) {
        root()->widgetAddedToTree().emit(widget);
    }
}

void Widget::onWidgetRemoved_(Widget* widget) {
    removeChildStylableObject(widget);
    onWidgetRemoved(widget);
    if (!widget->isReparentingWithinSameTree_) {
        root()->widgetRemovedFromTree().emit(widget);
    }
}

void Widget::onActionAdded_(Action* action, bool wasOnlyReordered) {
    if (!wasOnlyReordered) {
        action->setOwningWidget_(this);
        actionAdded().emit(action);
    }
}

void Widget::onActionRemoved_(Action* action) {
    action->setOwningWidget_(nullptr);
    actionRemoved().emit(action);
}

void Widget::resendPendingRequests_() {
    // transmit pending requests
    Widget* p = parent();
    if (p) {
        if (isGeometryUpdateRequested_) {
            p->requestGeometryUpdate();
        }
        if (isRepaintRequested_) {
            p->requestRepaint();
        }
    }
}

void Widget::updateGeometry_() {
    isGeometryUpdateRequested_ = false;
    isGeometryUpdateOngoing_ = true;
    updateChildrenGeometry();
    isGeometryUpdateOngoing_ = false;
    if (isGeometryUpdateRequested_) {
        VGC_WARNING(
            LogVgcUi, "A geometry update has been requested during a geometry update.");
    }
}

void Widget::prePaintUpdateGeometry_() {
    if (!parent()) {
        // Calling updateRootGeometry_() could indirectly call requestRepaint() from
        // resized children.
        // However we are already painting so we don't want to emit a request from
        // the root now.
        // Setting isRepaintRequested_ to true makes requestRepaint() a no-op for
        // this widget.
        isRepaintRequested_ = true;
        updateGeometry();
    }
}

void Widget::updateComputedVisibility_() {
    const Widget* p = parent();
    if (visibility_ == Visibility::Invisible) {
        setComputedVisibility_(false);
    }
    else if (!p || p->isVisible()) {
        setComputedVisibility_(true);
    }
}

void Widget::setComputedVisibility_(bool isVisible) {

    if (computedVisibility_ == isVisible) {
        return;
    }
    computedVisibility_ = isVisible;

    if (isVisible) {
        // set visible the children which inherit visibility
        for (Widget* w : children()) {
            if (w->visibility() == Visibility::Inherit) {
                w->setComputedVisibility_(true);
            }
        }
        resendPendingRequests_();
        onVisible();
    }
    else {
        for (Widget* w : children()) {
            w->setComputedVisibility_(false);
        }
        onHidden();
    }
}

void Widget::releaseEngine_() {
    onPaintDestroy(lastPaintEngine_);
    lastPaintEngine_->aboutToBeDestroyed().disconnect(onEngineAboutToBeDestroyed_());
    lastPaintEngine_ = nullptr;
}

void Widget::setEngine_(graphics::Engine* engine) {
    if (lastPaintEngine_) {
        releaseEngine_();
    }
    lastPaintEngine_ = engine;
    engine->aboutToBeDestroyed().connect(onEngineAboutToBeDestroyed_());
    onPaintCreate(engine);
}

void Widget::prePaintUpdateEngine_(graphics::Engine* engine) {
    if (engine != lastPaintEngine_) {
        setEngine_(engine);
    }
}

} // namespace vgc::ui
