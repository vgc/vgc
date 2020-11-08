// Copyright 2020 The VGC Developers
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

namespace vgc {
namespace ui {

Widget::Widget() :
    Object(),
    width_(0.0f),
    height_(0.0f)
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

bool Widget::canReparent(Widget* newParent)
{
    const bool simulate = true;
    return checkCanReparent_(newParent, this, simulate);
}

void Widget::reparent(Widget* newParent)
{
    checkCanReparent_(newParent, this);
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
        parent->insertChildObject_(this, nextSibling);
    }
    else {
        // nothing to do
    }
}

void Widget::resize(float width, float height)
{
    width_ = width;
    height_ = height;
    repaint();
}

void Widget::repaint()
{
    repaintRequested();
}

void Widget::onPaintCreate(graphics::Engine* /*engine*/)
{

}

void Widget::onPaintDraw(graphics::Engine* /*engine*/)
{

}

void Widget::onPaintDestroy(graphics::Engine* /*engine*/)
{

}

bool Widget::onMouseMove(MouseEvent* /*event*/)
{
    return false;
}

bool Widget::onMousePress(MouseEvent* /*event*/)
{
    return false;
}

bool Widget::onMouseRelease(MouseEvent* /*event*/)
{
    return false;
}

bool Widget::onMouseEnter()
{
    return false;
}

bool Widget::onMouseLeave()
{
    return false;
}

} // namespace ui
} // namespace vgc
