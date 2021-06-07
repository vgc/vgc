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

#include <vgc/ui/action.h>
#include <vgc/ui/strings.h>

namespace vgc {
namespace ui {

Widget::Widget() :
    Object(),
    children_(WidgetList::create(this)),
    actions_(ActionList::create(this)),
    preferredSize_(0.0f, 0.0f),
    isPreferredSizeComputed_(false),
    position_(0.0f, 0.0f),
    size_(0.0f, 0.0f),
    mousePressedChild_(nullptr),
    mouseEnteredChild_(nullptr),
    isTreeActive_(false),
    focus_(nullptr)
{

}

WidgetPtr Widget::create()
{
    return WidgetPtr(new Widget());
}

namespace {
bool checkCanReparent_(Widget* parent, Widget* child, bool simulate = false)
{
    if (parent && parent->isDescendantObject(child)) {
        if (simulate) return false;
        else throw ChildCycleError(parent, child);
    }
    return true;
}
} // namespace

void Widget::addChild(Widget* child)
{
    checkCanReparent_(this, child);
    children_->append(child);
}

bool Widget::canReparent(Widget* newParent)
{
    const bool simulate = true;
    return checkCanReparent_(newParent, this, simulate);
}

void Widget::reparent(Widget* newParent)
{
    checkCanReparent_(newParent, this);
    newParent->children_->append(this);
    appendObjectToParent_(newParent);
}

namespace {
bool checkCanReplace_(Widget* oldWidget, Widget* newWidget, bool simulate = false)
{
    if (!oldWidget) {
        if (simulate) return false;
        else throw core::NullError();
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

bool Widget::canReplace(Widget* oldWidget)
{
    const bool simulate = true;
    return checkCanReplace_(oldWidget, this, simulate);
}

void Widget::replace(Widget* oldWidget)
{
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
        parent->children_->insert(this, nextSibling);
    }
    else {
        // nothing to do
    }
}

Widget* Widget::root() const
{
    const Widget* res = this;
    const Widget* w = this;
    while (w) {
        res = w;
        w = w->parent();
    }
    return const_cast<Widget*>(res);
}

void Widget::setGeometry(const core::Vec2f& position, const core::Vec2f& size)
{
    core::Vec2f oldSize = size_;
    position_ = position;
    size_ = size;
    updateChildrenGeometry();
    if (!oldSize.allNear(size_, 1e-6f)) {
        onResize();
    }
}

PreferredSize Widget::preferredWidth() const
{
    return style(strings::preferred_width).preferredSize();
}

float Widget::stretchWidth() const
{
    return style(strings::stretch_width).number();
}

float Widget::shrinkWidth() const
{
    return style(strings::shrink_width).number();
}

PreferredSize Widget::preferredHeight() const
{
    return style(strings::preferred_height).preferredSize();
}

float Widget::stretchHeight() const
{
    return style(strings::stretch_height).number();
}

float Widget::shrinkHeight() const
{
    return style(strings::shrink_height).number();
}

void Widget::updateGeometry()
{
    Widget* root = this;
    Widget* w = this;
    while (w) {
        w->isPreferredSizeComputed_ = false;
        root = w;
        w = w->parent();
    }
    root->updateChildrenGeometry();
    repaint();
}

void Widget::onResize()
{

}

void Widget::repaint()
{
    Widget* widget = this;
    while (widget) {
        widget->repaintRequested();
        widget = widget->parent();
    }
}

void Widget::onPaintCreate(graphics::Engine* engine)
{
    for (Widget* child : children()) {
        child->onPaintCreate(engine);
    }
}

void Widget::onPaintDraw(graphics::Engine* engine)
{
    for (Widget* widget : children()) {
        engine->pushViewMatrix();
        core::Mat4f m = engine->viewMatrix();
        core::Vec2f pos = widget->position();
        m.translate(pos[0], pos[1]); // TODO: Mat4f.translate(const Vec2f&)
        engine->setViewMatrix(m);
        widget->onPaintDraw(engine);
        engine->popViewMatrix();
    }
}

void Widget::onPaintDestroy(graphics::Engine* engine)
{
    for (Widget* child : children()) {
        child->onPaintDestroy(engine);
    }
}

bool Widget::onMouseMove(MouseEvent* event)
{
    // If we are in the middle of a press-move-release sequence, then we
    // automatically forward the move event to the pressed child. We also delay
    // emitting the leave event until the mouse is released (see implementation
    // of onMouseRelease).
    //
    if (mousePressedChild_) {
        event->setPos(event->pos() - mousePressedChild_->position());
        return mousePressedChild_->onMouseMove(event);
    }

    // Otherwise, we iterate over all child widgets. Note that we iterate in
    // reverse order, so that widgets drawn last receive the event first. Also
    // note that for now, widget are always "opaque for mouse events", that is,
    // if a widget A is on top of a sibling widget B, then the widget B doesn't
    // receive the mouse event.
    //
    float x = event->x();
    float y = event->y();
    for (Widget* child = lastChild(); child != nullptr; child = child->previousSibling()) {
        float cx = child->x();
        float cy = child->y();
        float cw = child->width();
        float ch = child->height();
        if (cx <= x && x <= cx + cw && cy <= y && y <= cy + ch) {
            if (mouseEnteredChild_ != child) {
                if (mouseEnteredChild_) {
                    mouseEnteredChild_->onMouseLeave();
                }
                child->onMouseEnter();
                mouseEnteredChild_ = child;
            }
            event->setPos(event->pos() - child->position());
            return child->onMouseMove(event);
        }
    }

    // Emit leave event if we're not anymore in previously entered widget.
    //
    if (mouseEnteredChild_) {
        mouseEnteredChild_->onMouseLeave();
        mouseEnteredChild_ = nullptr;
    }

    return false;    

    // TODO: We could (should?) factorize the code:
    //
    //   if (cx <= x && x <= cx + cw && cy <= y && y <= cy + ch) {
    //       ...
    //   }
    //
    // into something along the lines of:
    //
    //   if (child->rect().contains(event->pos())) {
    //       ...
    //   }
    //
    // However, what if we allow rotated widgets? Or if the widget isn't shaped
    // as a rectangle, for example a circle? Perhaps a more generic approach
    // would be, instead of a "rect()" method, to have a boundingRect() method,
    // complemented by a virtual bool isUnderMouse(const Vec2f& p) method, so
    // that we can hangle non-rectangle or non-axis-aligned widgets.
}

bool Widget::onMousePress(MouseEvent* event)
{
    // Note: we don't handle multiple clicks, that is, a left-button-pressed
    // must be released before we issue a right-button-press
    if (mouseEnteredChild_ && !mousePressedChild_) {
        mousePressedChild_ = mouseEnteredChild_;
        event->setPos(event->pos() - mousePressedChild_->position());
        return mousePressedChild_->onMousePress(event);
    }
    return false;
}

bool Widget::onMouseRelease(MouseEvent* event)
{
    if (mousePressedChild_) {
        core::Vec2f eventPos = event->pos();
        event->setPos(eventPos - mousePressedChild_->position());
        bool accepted = mousePressedChild_->onMouseRelease(event);

        // Emit the mouse leave event now if the mouse exited the widget during
        // the press-move-release sequence.
        float x = eventPos[0];
        float y = eventPos[1];
        float cx = mousePressedChild_->x();
        float cy = mousePressedChild_->y();
        float cw = mousePressedChild_->width();
        float ch = mousePressedChild_->height();
        if (!(cx <= x && x <= cx + cw && cy <= y && y <= cy + ch)) {
            if (mouseEnteredChild_ != mousePressedChild_) {
                throw core::LogicError("mouseEnteredChild_ != mousePressedChild_");
            }
            mouseEnteredChild_->onMouseLeave();
            mouseEnteredChild_ = nullptr;
        }
        mousePressedChild_ = nullptr;
        return accepted;
    }
    return false;
}

bool Widget::onMouseEnter()
{
    return false;
}

bool Widget::onMouseLeave()
{
    if (mouseEnteredChild_) {
        mouseEnteredChild_->onMouseLeave();
        mouseEnteredChild_ = nullptr;
    }
    return true;
}

void Widget::setTreeActive(bool active)
{
    Widget* r = root();
    if (r->isTreeActive_ != active) {
        r->isTreeActive_ = active;
        Widget* f = focusedWidget();
        if (f) {
            if (active) {
                f->onFocusIn();
            }
            else {
                f->onFocusOut();
            }
        }
    }
}

void Widget::setFocus()
{
    if (!isFocusedWidget()) {
        clearFocus();
        Widget* widget = this;
        Widget* focus = this;
        while (widget) {
            widget->focus_ = focus;
            focus = widget;
            widget = widget->parent();
        }
        if (isTreeActive()) {
            onFocusIn();
        }
    }
    Widget* widget = this;
    while (widget) {
        widget->focusRequested();
        widget = widget->parent();
    }
}

void Widget::clearFocus()
{
    Widget* w = focusedWidget();
    if (w) {
        if (isTreeActive()) {
            onFocusOut();
        }
        while (w) {
            w->focus_ = nullptr;
            w = w->parent();
        }
    }
}

Widget* Widget::focusedWidget() const
{
    Widget* res = root()->focus_;
    while (res != nullptr) {
        if (res->isFocusedWidget()) {
            return res;
        }
        else {
            res = res->focus_;
        }
    }
    return res;
}

bool Widget::onFocusIn()
{
    return false;
}

bool Widget::onFocusOut()
{
    return false;
}

bool Widget::onKeyPress(QKeyEvent* event)
{
    Widget* fc = focusedChild();
    if (fc) {
        return fc->onKeyPress(event);
    }
    else {
        return false;
    }
}

bool Widget::onKeyRelease(QKeyEvent* event)
{
    Widget* fc = focusedChild();
    if (fc) {
        return fc->onKeyRelease(event);
    }
    else {
        return false;
    }
}

void Widget::addClass(core::StringId class_) {
    classes_.add(class_);
    onClassesChanged_();
}

void Widget::removeClass(core::StringId class_) {
    classes_.remove(class_);
    onClassesChanged_();
}

void Widget::toggleClass(core::StringId class_) {
    classes_.toggle(class_);
    onClassesChanged_();
}

StyleValue Widget::style(core::StringId property) const
{
    return style_.computedValue(property, this);
}

Action* Widget::createAction(const Shortcut& shortcut)
{
    ActionPtr action = Action::create(shortcut);
    actions_->append(action.get());
    return action.get();
}

core::Vec2f Widget::computePreferredSize() const
{
    // TODO: convert units if any.
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    return core::Vec2f(w.type() == auto_ ? 0 : w.value(),
                       h.type() == auto_ ? 0 : h.value());
}

void Widget::updateChildrenGeometry()
{
    // nothing
}

void Widget::onClassesChanged_()
{
    style_ = styleSheet()->computeStyle(this);
    // TODO: re-render
}

} // namespace ui
} // namespace vgc
