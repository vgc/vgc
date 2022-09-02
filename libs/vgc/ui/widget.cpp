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
#include <vgc/ui/strings.h>

namespace vgc::ui {

Widget::Widget()
    : StylableObject()
    , children_(WidgetList::create(this))
    , actions_(ActionList::create(this)) {

    addStyleClass(strings::Widget);
    children_->childAdded().connect(onWidgetAdded_());
    children_->childRemoved().connect(onWidgetRemoved_());
}

void Widget::onDestroyed() {
    if (lastPaintEngine_) {
        // XXX what to do ?
    }
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

void Widget::insertChildAt(Int i, Widget* child) {
    checkCanReparent_(this, child);
    children_->insertAt(i, child);
}

bool Widget::canReparent(Widget* newParent) {
    const bool simulate = true;
    return checkCanReparent_(newParent, this, simulate);
}

void Widget::reparent(Widget* newParent) {
    checkCanReparent_(newParent, this);
    newParent->children_->append(this);
    appendObjectToParent_(newParent);
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
        parent->children_->insert(this, nextSibling);
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

    // XXX could use any common ancestor
    const Widget* thisRoot = nullptr;
    geometry::Vec2f thisPosInRoot = positionInRoot(this, &thisRoot);
    const Widget* otherRoot = nullptr;
    geometry::Vec2f otherPosInRoot = positionInRoot(other, &otherRoot);

    if (thisRoot != otherRoot) {
        throw core::LogicError(
            "Cannot map a position between two widget coordinate systems if the widgets"
            "don't have the same root");
    }

    return position + thisPosInRoot - otherPosInRoot;
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
            // isGeometryUpdateRequested_
            // && isRepaintRequested_
            // && !isPreferredSizeComputed_
            break;
        }
        widget->isPreferredSizeComputed_ = false;
        widget = widget->parent();
    }
}

void Widget::onResize() {
}

void Widget::requestRepaint() {
    Widget* widget = this;
    while (widget && !widget->isRepaintRequested_) {
        widget->isRepaintRequested_ = true;
        Widget* parent = widget->parent();
        if (!parent) {
            widget->repaintRequested().emit();
        }
        widget = parent;
    }
}

void Widget::preparePaint(graphics::Engine* engine, PaintOptions options) {
    prePaintUpdateGeometry_();
    prePaintUpdateEngine_(engine);
    onPaintPrepare(engine, options);
}

void Widget::paint(graphics::Engine* engine, PaintOptions options) {
    prePaintUpdateGeometry_();
    prePaintUpdateEngine_(engine);
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

bool Widget::onMouseMove(MouseEvent* event) {

    // If we are in the middle of a press-move-release sequence, then we
    // automatically forward the move event to the pressed child. We also delay
    // emitting the leave event until the mouse is released (see implementation
    // of onMouseRelease).
    //
    if (mousePressedChild_) {
        event->setPosition(event->position() - mousePressedChild_->position());
        return mousePressedChild_->onMouseMove(event);
    }

    // Otherwise, we iterate over all child widgets. Note that we iterate in
    // reverse order, so that widgets drawn last receive the event first. Also
    // note that for now, widget are always "opaque for mouse events", that is,
    // if a widget A is on top of a sibling widget B, then the widget B doesn't
    // receive the mouse event.
    //
    for (Widget* child = lastChild(); //
         child != nullptr;            //
         child = child->previousSibling()) {

        if (child->geometry().contains(event->position())) {
            if (mouseEnteredChild_ != child) {
                if (mouseEnteredChild_) {
                    mouseEnteredChild_->setHovered(false);
                }
                child->setHovered(true);
                mouseEnteredChild_ = child;
            }
            event->setPosition(event->position() - child->position());
            return child->onMouseMove(event);
        }
    }

    // Emit leave event if we're not anymore in previously entered widget.
    //
    if (mouseEnteredChild_) {
        mouseEnteredChild_->setHovered(false);
        mouseEnteredChild_ = nullptr;
    }

    return false;

    // Note: if in the future we allow non-rectangle or rotated widgets, we
    // could replace `if (child->geometry().contains(event->position()))` by a
    // more generic approach. For example, a `boundingGeometry()` method
    // complemented by a virtual `bool isUnderMouse(const Vec2f& p)` method.
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

bool Widget::onMousePress(MouseEvent* event) {

    // Note: we don't handle multiple clicks, that is, a left-button-pressed
    // must be released before we issue a right-button-press
    if (!mousePressedChild_) {
        if (mouseEnteredChild_) {
            mousePressedChild_ = mouseEnteredChild_;
            event->setPosition(event->position() - mousePressedChild_->position());

            if (mousePressedChild_->focusPolicy().has(FocusPolicy::Click)) {
                mousePressedChild_->setFocus(FocusReason::Mouse);
            }
            else {
                clearNonStickyNonChildFocus_(this, mousePressedChild_);
            }
            return mousePressedChild_->onMousePress(event);
        }
        else {
            clearNonStickyNonChildFocus_(this, nullptr);
        }
    }

    return false;
}

bool Widget::onMouseRelease(MouseEvent* event) {
    if (mousePressedChild_) {
        geometry::Vec2f eventPos = event->position();
        event->setPosition(eventPos - mousePressedChild_->position());
        bool accepted = mousePressedChild_->onMouseRelease(event);

        // Emit the mouse leave event now if the mouse exited the widget during
        // the press-move-release sequence.
        if (!mousePressedChild_->geometry().contains(eventPos)) {
            if (mouseEnteredChild_ != mousePressedChild_) {
                throw core::LogicError("mouseEnteredChild_ != mousePressedChild_");
            }
            mouseEnteredChild_->setHovered(false);
            mouseEnteredChild_ = nullptr;
        }
        mousePressedChild_ = nullptr;
        return accepted;
    }
    return false;
}

bool Widget::onMouseEnter() {
    return false;
}

bool Widget::onMouseLeave() {
    if (mouseEnteredChild_) {
        bool handled = mouseEnteredChild_->setHovered(false);
        mouseEnteredChild_ = nullptr;
        return handled;
    }
    return false;
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

Action* Widget::createAction() {
    ActionPtr action = Action::create();
    actions_->append(action.get());
    return action.get();
}

Action* Widget::createAction(const Shortcut& shortcut) {
    ActionPtr action = Action::create(shortcut);
    actions_->append(action.get());
    return action.get();
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
    // nothing
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

void Widget::onStyleChanged() {
    requestGeometryUpdate();
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

void Widget::releaseEngine_() {
    onPaintDestroy(lastPaintEngine_);
    lastPaintEngine_->aboutToBeDestroyed().disconnect(onEngineAboutToBeDestroyed());
    lastPaintEngine_ = nullptr;
}

void Widget::setEngine_(graphics::Engine* engine) {
    if (lastPaintEngine_) {
        releaseEngine_();
    }
    lastPaintEngine_ = engine;
    engine->aboutToBeDestroyed().connect(onEngineAboutToBeDestroyed());
}

void Widget::prePaintUpdateEngine_(graphics::Engine* engine) {
    if (engine != lastPaintEngine_) {
        setEngine_(engine);
        onPaintCreate(engine);
    }
}

} // namespace vgc::ui
