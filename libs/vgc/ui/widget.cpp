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

#include <vgc/graphics/richtext.h>
#include <vgc/graphics/strings.h>

#include <vgc/ui/action.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/strings.h>

namespace vgc::ui {

Widget::Widget()
    : StylableObject()
    , children_(WidgetList::create(this))
    , actions_(ActionList::create(this)) {

    addStyleClass(strings::Widget);
    children_->childAdded().connect(onWidgetAddedSlot_());
    children_->childRemoved().connect(onWidgetRemovedSlot_());
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
    focus_ = nullptr;
    keyboardCaptor_ = nullptr;
    // Call parent destructor
    Object::onDestroyed();
}

WidgetPtr Widget::create() {
    return WidgetPtr(new Widget());
}

namespace {

bool checkCanReparent_(Widget* parent, Widget* child, bool simulate = false) {
    if (parent && parent->isDescendantObject(child)) {
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
    checkCanReparent_(this, child);
    children_->append(child);
}

void Widget::insertChild(Widget* position, Widget* child) {
    checkCanReparent_(this, child);
    children_->insert(position, child);
}

void Widget::insertChild(Int i, Widget* child) {
    checkCanReparent_(this, child);
    children_->insert(i, child);
}

bool Widget::canReparent(Widget* newParent) {
    const bool simulate = true;
    return checkCanReparent_(newParent, this, simulate);
}

void Widget::reparent(Widget* newParent) {
    checkCanReparent_(newParent, this);
    newParent->children_->append(this);
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

bool Widget::canReplace(Widget* oldWidget) {
    const bool simulate = true;
    return checkCanReplace_(oldWidget, this, simulate);
}

void Widget::replace(Widget* oldWidget) {
    checkCanReplace_(oldWidget, this);
    if (this == oldWidget) {
        // nothing to do
        return;
    }
    // Note: this Widget might be a descendant of oldWidget, so we need
    // remove it from parent before destroying the old Widget.
    Widget* parent = oldWidget->parent();
    Widget* nextSibling = oldWidget->nextSibling();
    core::ObjectPtr self = removeObjectFromParent_();
    oldWidget->destroyObject_();
    if (parent) {
        parent->children_->insert(nextSibling, this);
    }
    else {
        // nothing to do
    }
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
            "Cannot map a position between two widget coordinate systems if the widgets"
            "don't have the same root.");
    }

    return position + thisPosInRoot - otherPosInRoot;
}

geometry::Rect2f Widget::mapTo(Widget* other, const geometry::Rect2f& rect) const {
    geometry::Rect2f ret = rect;
    ret.setPosition(mapTo(other, rect.position()));
    return ret;
}

void Widget::updateGeometry(
    const geometry::Vec2f& position,
    const geometry::Vec2f& size) {

    position_ = position;
    bool resized = false;
    if (!size_.allNear(size, 1e-6f)) {
        size_ = size;
        resized = true;
    }
    if (isGeometryUpdateRequested_ || resized) {
        isGeometryUpdateRequested_ = false;
        updateChildrenGeometry();
        if (isGeometryUpdateRequested_) {
            VGC_WARNING(
                LogVgcUi,
                "A geometry update has been requested during a geometry update.");
        }
    }
    if (resized) {
        onResize();
    }
}

void Widget::updateGeometry(const geometry::Vec2f& position) {
    position_ = position;
    updateGeometry();
}

void Widget::updateGeometry() {
    if (isGeometryUpdateRequested_) {
        isGeometryUpdateRequested_ = false;
        updateChildrenGeometry();
        if (isGeometryUpdateRequested_) {
            VGC_WARNING(
                LogVgcUi,
                "A geometry update has been requested during a geometry update.");
        }
    }
}

PreferredSize Widget::preferredWidth() const {
    return style(strings::preferred_width).to<PreferredSize>();
}

float Widget::horizontalStretch() const {
    return style(strings::horizontal_stretch).toFloat();
}

float Widget::horizontalShrink() const {
    return style(strings::horizontal_shrink).toFloat();
}

PreferredSize Widget::preferredHeight() const {
    return style(strings::preferred_height).to<PreferredSize>();
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
    onPaintDraw(engine, options);
}

void Widget::onPaintCreate(graphics::Engine* engine) {
    for (Widget* child : children()) {
        child->onPaintCreate(engine);
    }
}

void Widget::onPaintPrepare(graphics::Engine* engine, PaintOptions options) {
    for (Widget* widget : children()) {
        widget->preparePaint(engine, options);
    }
}

void Widget::onPaintDraw(graphics::Engine* engine, PaintOptions options) {
    for (Widget* widget : children()) {
        if (!widget->isVisible()) {
            continue;
        }
        engine->pushViewMatrix();
        geometry::Mat4f m = engine->viewMatrix();
        geometry::Vec2f pos = widget->position();
        m.translate(pos[0], pos[1]); // TODO: Mat4f.translate(const Vec2f&)
        engine->setViewMatrix(m);
        widget->paint(engine, options);
        engine->popViewMatrix();
    }
}

void Widget::onPaintDestroy(graphics::Engine* engine) {
    for (Widget* child : children()) {
        child->onPaintDestroy(engine);
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

bool Widget::mouseMove(MouseEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "mouseMove() can only be called on a root widget.");
        return false;
    }
    mouseMove_(event);
    return event->isHandled();
}

bool Widget::mousePress(MouseEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "mousePress() can only be called on a root widget.");
        return false;
    }
    mousePress_(event);
    return event->isHandled();
}

bool Widget::mouseRelease(MouseEvent* event) {
    if (!isRoot()) {
        VGC_WARNING(LogVgcUi, "mouseRelease() can only be called on a root widget.");
        return false;
    }
    mouseRelease_(event);
    return event->isHandled();
}

void Widget::preMouseMove(MouseEvent* /*event*/) {
    // no-op
}

void Widget::preMousePress(MouseEvent* /*event*/) {
    // no-op
}

void Widget::preMouseRelease(MouseEvent* /*event*/) {
    // no-op
}

bool Widget::onMouseMove(MouseEvent* /*event*/) {
    return false;
}

bool Widget::onMousePress(MouseEvent* /*event*/) {
    return false;
}

bool Widget::onMouseRelease(MouseEvent* /*event*/) {
    return false;
}

bool Widget::checkAlreadyHovered_() {
    if (!isHovered_) {
        WidgetPtr thisPtr = this;
        VGC_WARNING(
            LogVgcUi,
            "Widget should have been hovered prior to receiving a mouse event.");
        setHovered(true);
        if (!thisPtr.isAlive()) {
            return false;
        }
    }
    return true;
}

void Widget::mouseMove_(MouseEvent* event) {

    geometry::Vec2f eventPos = event->position();

    if (!checkAlreadyHovered_()) {
        return;
    }

    // Prepare against death of `this`
    WidgetPtr thisPtr = this;

    // Get or update hover-chain child.
    Widget* hcChild = hoverChainChild();
    const bool hasNoHoverLockedChild = !hcChild || !hcChild->isHoverLocked();
    if (isChildHoverEnabled_ && hasNoHoverLockedChild) {
        hcChild = computeHoverChainChild(eventPos);
        if (!setHoverChainChild(hcChild)) {
            // Exceptionnal things happened. Event can be considered handled.
            hcChild = nullptr;
            event->handled_ = true;
        }
        // Check for deaths.
        if (!thisPtr.isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
    }

    // User-defined capture phase handler.
    preMouseMove(event);
    if (!thisPtr.isAlive()) {
        // Widget got killed. Event can be considered handled.
        event->handled_ = true;
        return;
    }

    // Handle stop propagation.
    if (event->isStopPropagationRequested()) {
        return;
    }

    // Get final hover-chain child (possibly changed in preMouseMove).
    hcChild = hoverChainChild();

    // Call hover-chain child's handler.
    if (hcChild) {
        // Prepare against widget killers.
        WidgetPtr hcChildPtr = hcChild;
        // Call handler.
        event->setPosition(mapTo(hcChild, eventPos));
        hcChild->mouseMove_(event);
        // Check for deaths.
        if (!thisPtr.isAlive() || !hcChildPtr.isAlive()) {
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
        if (!thisPtr.isAlive()) {
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

namespace {

// Clears focus if:
// - there is a focused widget in the widget tree of parent, and
// - the focused widget is not parent nor the given child or any of its descendant, and
// - the focused widget isn't sticky
//
// parent must be non-null, but child can be null.
//
void clearNonStickyNonChildFocus_(Widget* parent, Widget* child) {
    if (parent->root()->hasFocusedWidget()) {
        bool childHasFocusedWidget = child && child->hasFocusedWidget();
        if (!parent->isFocusedWidget() && !childHasFocusedWidget) {
            Widget* focusedWidget = parent->focusedWidget();
            if (!focusedWidget->focusPolicy().has(FocusPolicy::Sticky)) {
                parent->clearFocus(FocusReason::Mouse);
            }
        }
    }
}

} // namespace

void Widget::mousePress_(MouseEvent* event) {

    const geometry::Vec2f eventPos = event->position();
    const bool otherWasPressed = !pressedButtons_.isEmpty();

    if (!checkAlreadyHovered_()) {
        return;
    }

    // User-defined capture phase handler.
    WidgetPtr thisPtr = this;
    preMousePress(event);
    if (!thisPtr.isAlive()) {
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

    // Apply focus policy.
    if (!otherWasPressed) {
        if (focusPolicy().has(FocusPolicy::Click)) {
            setFocus(FocusReason::Mouse);
        }
        else {
            // XXX Probably buggy, see other call below.
            //     If a parent has focus, a click on its child will clear it because
            //     the child itself calls clearNonStickyNonChildFocus_(this, nullptr);
            //     Shouldn't we do it only based on the chain end ?
            clearNonStickyNonChildFocus_(this, hcChild);
        }
        // Check for deaths.
        if (!thisPtr.isAlive()) {
            // Widget got killed. Event can be considered handled.
            event->handled_ = true;
            return;
        }
    }

    // Call hover-chain child's handler.
    if (hcChild) {
        // Prepare against widget killers.
        WidgetPtr hcChildPtr = hcChild;
        // Call handler.
        event->setPosition(mapTo(hcChild, eventPos));
        hcChild->mousePress_(event);
        // Check for deaths.
        if (!thisPtr.isAlive() || !hcChildPtr.isAlive()) {
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
        // User-defined bubble phase handler.
        event->handled_ |= onMousePress(event);
        if (!thisPtr.isAlive()) {
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

void Widget::mouseRelease_(MouseEvent* event) {

    geometry::Vec2f eventPos = event->position();

    if (!checkAlreadyHovered_()) {
        return;
    }

    // User-defined capture phase handler.
    WidgetPtr thisPtr = this;
    preMouseRelease(event);
    if (!thisPtr.isAlive()) {
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
        if (!thisPtr.isAlive() || !hcChildPtr.isAlive()) {
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
        if (!thisPtr.isAlive()) {
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

    // We update the hover-unlocked part of the hover-chain now if we are hover-locked
    // but our child is not.
    if (isHoverLocked() && isChildHoverEnabled_) {
        Widget* hcParent = this;
        hcChild = hoverChainChild();
        const bool hasNoHoverLockedChild = !hcChild || !hcChild->isHoverLocked();
        if (hasNoHoverLockedChild) {
            geometry::Vec2f relPos = eventPos;
            while (hcParent) {
                hcChild = computeHoverChainChild(relPos);
                if (!hcParent->setHoverChainChild(hcChild)) {
                    // Exceptionnal things happened. Event can be considered handled.
                    event->handled_ = true;
                    break;
                }
                relPos = hcParent->mapTo(hcChild, relPos);
                hcParent = hcChild;
            }
            // Check for deaths.
            if (!thisPtr.isAlive()) {
                // Widget got killed. Event can be considered handled.
                event->handled_ = true;
                return;
            }
        }
    }
}

Widget* Widget::computeHoverChainChild(const geometry::Vec2f& position) {

    // Return null if child hovering is disabled.
    if (!isChildHoverEnabled_) {
        return nullptr;
    }

    // We iterate over all child widgets in reverse order, so that widgets drawn
    // last receive the event first. Also note that for now, widget are always
    // "opaque for mouse events", that is, if a widget A is on top of a sibling
    // widget B, then the widget B doesn't receive the mouse event.
    //
    for (Widget* child = lastChild(); //
         child != nullptr;            //
         child = child->previousSibling()) {

        if (!child->isVisible()) {
            continue;
        }

        // Note: if in the future we allow non-rectangle or rotated widgets, we
        // could replace `if (child->geometry().contains(event->position()))` by a
        // more generic approach. For example, a `boundingGeometry()` method
        // complemented by a virtual `bool isUnderMouse(const Vec2f& p)` method.
        if (child->geometry().contains(position)) {
            return child;
        }
    }

    return nullptr;
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

        // From here, let's be carefull about dangling pointers.
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
        if (!thisPtr.isAlive()) {
            return true;
        }

        // We could fake releasing buttons here with pressedButtons_ if desired.
        /**/

        // Notify leave
        mouseLeave_();

        if (!thisPtr.isAlive()) {
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
        return thisPtr.isAlive();
    }

    WidgetPtr newChildPtr = newHoverChainChild;

    // Unhover child's children
    if (hoverChainChild_) {
        hoverChainChild_->setHovered(false);
        if (!thisPtr.isAlive()) {
            return false;
        }
        VGC_ASSERT(hoverChainChild_ == nullptr);
        if (!newChildPtr.isAlive()) {
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
    return thisPtr.isAlive() && newChildPtr.isAlive()
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

bool Widget::mouseEnter_() {
    isHovered_ = true;
    return onMouseEnter();
}

bool Widget::mouseLeave_() {
    return onMouseLeave();
}

bool Widget::onMouseEnter() {
    return false;
}

bool Widget::onMouseLeave() {
    return false;
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
    if (!isFocusedWidget()) {
        clearFocus(reason);
        Widget* widget = this;
        Widget* focus = this;
        while (widget) {
            widget->focus_ = focus;
            focus = widget;
            widget = widget->parent();
        }
        if (isTreeActive()) {
            onFocusIn(reason);
        }
    }
    Widget* widget = this;
    while (widget) {
        widget->focusSet().emit(reason);
        widget = widget->parent();
    }
}

void Widget::clearFocus(FocusReason reason) {
    Widget* oldFocusedWidget = focusedWidget();
    Widget* ancestor = oldFocusedWidget;
    while (ancestor) {
        ancestor->focus_ = nullptr;
        ancestor = ancestor->parent();
    }
    if (oldFocusedWidget && isTreeActive()) {
        oldFocusedWidget->onFocusOut(reason);
    }
    Widget* widget = this;
    while (widget) {
        widget->focusCleared().emit(reason);
        widget = widget->parent();
    }
}

// Class invariant: for any widget w, if w->focus_ is non-null then:
// 1. w->focus_->focus_ is also non-null, and
// 2. w->focus_ points to either w or a child of w

Widget* Widget::focusedWidget() const {
    Widget* res = root()->focus_;
    if (res) {                       // If this widget tree has a focused widget,
        while (res->focus_ != res) { // then while res is not the focused widget,
            res = res->focus_;       // keep iterating down the tree
        }
        return res;
    }
    else {
        return nullptr;
    }
}

bool Widget::onFocusIn(FocusReason) {
    return false;
}

bool Widget::onFocusOut(FocusReason) {
    return false;
}

bool Widget::onKeyPress(QKeyEvent* event) {
    Widget* fc = focusedChild();
    if (fc) {
        return fc->onKeyPress(event);
    }
    else {
        return false;
    }
}

bool Widget::onKeyRelease(QKeyEvent* event) {
    Widget* fc = focusedChild();
    if (fc) {
        return fc->onKeyRelease(event);
    }
    else {
        return false;
    }
}

geometry::Vec2f Widget::computePreferredSize() const {
    // TODO: convert units if any.
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    return geometry::Vec2f(
        w.type() == auto_ ? 0 : w.value(), h.type() == auto_ ? 0 : h.value());
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

using style::StyleTokenIterator;
using style::StyleTokenType;
using style::StyleValue;
using style::StyleValueType;

StyleValue parseStyleNumber(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Number && begin + 1 == end) {
        return StyleValue::number(begin->toFloat());
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue parseStyleLength(StyleTokenIterator begin, StyleTokenIterator end) {
    // For now, we only support a unique Dimension token with a "dp" unit
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (
        begin->type == StyleTokenType::Dimension //
        && begin->codePointsValue == "dp"        //
        && begin + 1 == end) {

        return StyleValue::number(begin->toFloat());
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue parseStylePreferredSize(StyleTokenIterator begin, StyleTokenIterator end) {
    // For now, we only support 'auto' or a unique Dimension token with a "dp" unit
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (
        begin->type == StyleTokenType::Identifier             //
        && begin->codePointsValue == graphics::strings::auto_ //
        && begin + 1 == end) {

        return StyleValue::custom(PreferredSize(PreferredSizeType::Auto));
    }
    else if (
        begin->type == StyleTokenType::Dimension //
        && begin->codePointsValue == "dp"        //
        && begin + 1 == end) {

        return StyleValue::custom(PreferredSize(PreferredSizeType::Dp, begin->toFloat()));
    }
    else {
        return StyleValue::invalid();
    }
}

// clang-format off

style::StylePropertySpecTablePtr createGlobalStylePropertySpecTable_() {

    using namespace strings;

    auto autosize    = StyleValue::custom(PreferredSize(PreferredSizeType::Auto));
    auto zero        = StyleValue::number(0.0f);
    auto one         = StyleValue::number(1.0f);

    // Start with the same specs as RichTextSpan
    auto table = std::make_shared<style::StylePropertySpecTable>();
    *table.get() = *graphics::RichTextSpan::stylePropertySpecs();

    // Insert additional specs
    // Reference: https://www.w3.org/TR/CSS21/propidx.html

    table->insert(preferred_height,     autosize, false, &parseStylePreferredSize);
    table->insert(preferred_width,      autosize, false, &parseStylePreferredSize);
    table->insert(column_gap,           zero,     false, &parseStyleLength);
    table->insert(row_gap,              zero,     false, &parseStyleLength);
    table->insert(grid_auto_columns,    autosize, false, &parseStylePreferredSize);
    table->insert(grid_auto_rows,       autosize, false, &parseStylePreferredSize);
    table->insert(horizontal_stretch,   one,      false, &parseStyleNumber);
    table->insert(horizontal_shrink,    one,      false, &parseStyleNumber);
    table->insert(vertical_stretch,     one,      false, &parseStyleNumber);
    table->insert(vertical_shrink,      one,      false, &parseStyleNumber);

    return table;
}

// clang-format on

const style::StylePropertySpecTablePtr& stylePropertySpecTable_() {
    static style::StylePropertySpecTablePtr table = createGlobalStylePropertySpecTable_();
    return table;
}

style::StyleSheetPtr createGlobalStyleSheet_() {
    std::string path = core::resourcePath("ui/stylesheets/default.vgcss");
    std::string s = core::readFile(path);
    return style::StyleSheet::create(stylePropertySpecTable_(), s);
}

} // namespace

const style::StyleSheet* Widget::defaultStyleSheet() const {
    static style::StyleSheetPtr s = createGlobalStyleSheet_();
    return s.get();
}

style::StylableObject* Widget::parentStylableObject() const {
    return static_cast<style::StylableObject*>(parent());
}

style::StylableObject* Widget::firstChildStylableObject() const {
    return static_cast<style::StylableObject*>(firstChild());
}

style::StylableObject* Widget::lastChildStylableObject() const {
    return static_cast<style::StylableObject*>(lastChild());
}

style::StylableObject* Widget::previousSiblingStylableObject() const {
    return static_cast<style::StylableObject*>(previousSibling());
}

style::StylableObject* Widget::nextSiblingStylableObject() const {
    return static_cast<style::StylableObject*>(nextSibling());
}

void Widget::onChildRemoved(Object* child) {
    if (child == children_) {
        children_ = nullptr;
    }
    else if (child == actions_) {
        actions_ = nullptr;
    }
}

void Widget::onStyleChanged() {
    requestGeometryUpdate();
}

void Widget::onWidgetAdded_(Widget* widget, bool wasOnlyReordered) {
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
}

void Widget::onWidgetRemoved_(Widget* widget) {
    onWidgetRemoved(widget);
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
}

void Widget::prePaintUpdateEngine_(graphics::Engine* engine) {
    if (engine != lastPaintEngine_) {
        setEngine_(engine);
        onPaintCreate(engine);
    }
}

} // namespace vgc::ui
