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

#include <vgc/ui/action.h>
#include <vgc/ui/strings.h>

namespace vgc {
namespace ui {

Widget::Widget() :
    StylableObject(),
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
    children_->childAdded().connect(onWidgetAdded_());
    children_->childRemoved().connect(onWidgetRemoved_());
}

void Widget::onDestroyed()
{
    if (lastPaintEngine_)
    {
        // XXX what to do ?
    }
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

void Widget::setGeometry(const geometry::Vec2f& position, const geometry::Vec2f& size)
{
    geometry::Vec2f oldSize = size_;
    position_ = position;
    size_ = size;
    updateChildrenGeometry();
    if (!oldSize.allNear(size_, 1e-6f)) {
        onResize();
    }
}

PreferredSize Widget::preferredWidth() const
{
    return style(strings::preferred_width).to<PreferredSize>();
}

float Widget::stretchWidth() const
{
    return style(strings::stretch_width).toFloat();
}

float Widget::shrinkWidth() const
{
    return style(strings::shrink_width).toFloat();
}

PreferredSize Widget::preferredHeight() const
{
    return style(strings::preferred_height).to<PreferredSize>();
}

float Widget::stretchHeight() const
{
    return style(strings::stretch_height).toFloat();
}

float Widget::shrinkHeight() const
{
    return style(strings::shrink_height).toFloat();
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
        widget->repaintRequested().emit();
        widget = widget->parent();
    }
}

void Widget::paint(graphics::Engine* engine)
{
    if (engine != lastPaintEngine_) {
        if (lastPaintEngine_) {
            releaseEngine_();
        }
        lastPaintEngine_ = engine;
        engine->aboutToBeDestroyed().connect(
            onEngineAboutToBeDestroyed());
        onPaintCreate(engine);
    }
    onPaintDraw(engine);
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
        geometry::Mat4f m = engine->viewMatrix();
        geometry::Vec2f pos = widget->position();
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
        geometry::Vec2f eventPos = event->pos();
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
        widget->focusRequested().emit();
        widget = widget->parent();
    }
}

void Widget::clearFocus()
{
    Widget* w = focusedWidget();
    if (w) {
        if (isTreeActive()) {
            w->onFocusOut();
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

Action* Widget::createAction(const Shortcut& shortcut)
{
    ActionPtr action = Action::create(shortcut);
    actions_->append(action.get());
    return action.get();
}

geometry::Vec2f Widget::computePreferredSize() const
{
    // TODO: convert units if any.
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    return geometry::Vec2f(w.type() == auto_ ? 0 : w.value(),
                           h.type() == auto_ ? 0 : h.value());
}

void Widget::updateChildrenGeometry()
{
    // nothing
}

namespace {

using style::StyleValue;
using style::StyleValueType;
using style::StyleTokenIterator;
using style::StyleTokenType;

StyleValue parseStyleColor(StyleTokenIterator begin, StyleTokenIterator end)
{
    try {
        std::string str(begin->begin, (end-1)->end);
        core::Color color = core::parse<core::Color>(str);
        return StyleValue::custom(color);
    } catch (const core::ParseError&) {
        return StyleValue::invalid();
    } catch (const core::RangeError&) {
        return StyleValue::invalid();
    }
}

StyleValue parseStyleLength(StyleTokenIterator begin, StyleTokenIterator end)
{
    // For now, we only support a unique Dimension token with a "dp" unit
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Dimension &&
             begin->codePointsValue == "dp" &&
             begin + 1 == end) {
        return StyleValue::number(begin->toFloat());
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue parseStyleNumber(StyleTokenIterator begin, StyleTokenIterator end)
{
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Number &&
             begin + 1 == end) {
        return StyleValue::number(begin->toFloat());
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue parseStylePreferredSize(StyleTokenIterator begin, StyleTokenIterator end)
{
    // For now, we only support 'auto' or a unique Dimension token with a "dp" unit
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Identifier &&
             begin->codePointsValue == strings::auto_ &&
             begin + 1 == end) {
        return StyleValue::custom(PreferredSize(PreferredSizeType::Auto));
    }
    else if (begin->type == StyleTokenType::Dimension &&
             begin->codePointsValue == "dp" &&
             begin + 1 == end) {
        return StyleValue::custom(PreferredSize(PreferredSizeType::Dp, begin->toFloat()));
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue parsePixelHinting(StyleTokenIterator begin, StyleTokenIterator end)
{
    StyleValue res = style::parseStyleDefault(begin, end);
    if (res.type() == StyleValueType::Identifier &&
            (res == strings::off || res == strings::normal)) {
        res = StyleValue::invalid();
    }
    return res;
}

style::StylePropertySpecTablePtr createGlobalStylePropertySpecTable_()
{
    // For reference: https://www.w3.org/TR/CSS21/propidx.html
    auto black       = StyleValue::custom(core::colors::black);
    auto transparent = StyleValue::custom(core::colors::transparent);
    auto zero        = StyleValue::number(0.0f);
    auto one         = StyleValue::number(1.0f);
    auto autosize    = StyleValue::custom(PreferredSize(PreferredSizeType::Auto));
    auto normal      = StyleValue::identifier(strings::normal);

    auto table = std::make_shared<style::StylePropertySpecTable>();
    table->insert("background-color",          transparent, false, &parseStyleColor);
    table->insert("background-color",          transparent, false, &parseStyleColor);
    table->insert("background-color-on-hover", transparent, false, &parseStyleColor);
    table->insert("border-radius",             zero,        false, &parseStyleLength);
    table->insert("margin-bottom",             zero,        false, &parseStyleLength);
    table->insert("margin-left",               zero,        false, &parseStyleLength);
    table->insert("margin-right",              zero,        false, &parseStyleLength);
    table->insert("margin-top",                zero,        false, &parseStyleLength);
    table->insert("padding-bottom",            zero,        false, &parseStyleLength);
    table->insert("padding-left",              zero,        false, &parseStyleLength);
    table->insert("padding-right",             zero,        false, &parseStyleLength);
    table->insert("padding-top",               zero,        false, &parseStyleLength);
    table->insert("pixel-hinting",             normal,      false, &parsePixelHinting);
    table->insert("preferred-height",          autosize,    false, &parseStylePreferredSize);
    table->insert("preferred-width",           autosize,    false, &parseStylePreferredSize);
    table->insert("shrink-height",             one,         false, &parseStyleNumber);
    table->insert("shrink-width",              one,         false, &parseStyleNumber);
    table->insert("stretch-height",            one,         false, &parseStyleNumber);
    table->insert("stretch-width",             one,         false, &parseStyleNumber);
    table->insert("text-color",                black,       true,  &parseStyleColor);

    return table;
}

const style::StylePropertySpecTablePtr& stylePropertySpecTable_()
{
    static style::StylePropertySpecTablePtr table = createGlobalStylePropertySpecTable_();
    return table;
}

style::StyleSheetPtr createGlobalStyleSheet_() {
    std::string path = core::resourcePath("ui/stylesheets/default.vgcss");
    std::string s = core::readFile(path);
    return style::StyleSheet::create(stylePropertySpecTable_(), s);
}

} // namespace

const style::StyleSheet* Widget::defaultStyleSheet() const
{
    static style::StyleSheetPtr s = createGlobalStyleSheet_();
    return s.get();
}

style::StylableObject* Widget::parentStylableObject() const
{
    return static_cast<style::StylableObject*>(parent());
}

style::StylableObject* Widget::firstChildStylableObject() const
{
    return static_cast<style::StylableObject*>(firstChild());
}

style::StylableObject* Widget::lastChildStylableObject() const
{
    return static_cast<style::StylableObject*>(lastChild());
}

style::StylableObject* Widget::previousSiblingStylableObject() const
{
    return static_cast<style::StylableObject*>(previousSibling());
}

style::StylableObject* Widget::nextSiblingStylableObject() const
{
    return static_cast<style::StylableObject*>(nextSibling());
}

void Widget::releaseEngine_()
{
    onPaintDestroy(lastPaintEngine_);
    lastPaintEngine_->aboutToBeDestroyed().disconnect(
        onEngineAboutToBeDestroyed());
    lastPaintEngine_ = nullptr;
}

} // namespace ui
} // namespace vgc
