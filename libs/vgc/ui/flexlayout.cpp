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

#include <vgc/ui/flexlayout.h>

namespace vgc {
namespace ui {

FlexLayout::FlexLayout(
        FlexDirection direction,
        FlexWrap wrap) :
    Widget(),
    direction_(direction),
    wrap_(wrap)
{

}

FlexLayoutPtr FlexLayout::create(
        FlexDirection direction,
        FlexWrap wrap)
{
    return FlexLayoutPtr(new FlexLayout(direction, wrap));
}

void FlexLayout::setDirection(FlexDirection direction)
{
    direction_ = direction;
    updateGeometry_();
}

void FlexLayout::setWrap(FlexWrap wrap)
{
    wrap_ = wrap;
    updateGeometry_();
}

void FlexLayout::onResize()
{
    updateGeometry_();
}

void FlexLayout::onChildAdded(Object*)
{
    updateGeometry_();
}

void FlexLayout::onChildRemoved(Object*)
{
    updateGeometry_();
}

void FlexLayout::updateGeometry_()
{
    // TODO: take into account children "preferred" size, stretch/shrink factors,
    // margin/padding, gap, and allow to wrap, align, etc.
    Int numChildren = 0;
    for (Widget* child : children()) {
        (void)child; // Unused
        numChildren += 1;
    }
    if (numChildren > 0) {
        bool isRow = (direction_ == FlexDirection::Row) ||
                     (direction_ == FlexDirection::RowReverse);
        bool isReverse =
                (direction_ == FlexDirection::RowReverse) ||
                (direction_ == FlexDirection::ColumnReverse);
        float s1, s2, sc1, sc2; // 1 = main axis; 2 = other axis; s = size; sc = size of child
        if (isRow) {
            s1 = width();
            s2 = height();
        }
        else {
            s1 = height();
            s2 = width();
        }
        sc1 = s1 / numChildren;
        sc2 = s2;
        Int childIndex = 0;
        Widget* child = isReverse ? lastChild() : firstChild();
        while (child) {
            if (isRow) {
                child->move(childIndex * sc1, 0);
                child->resize(sc1, sc2);
            }
            else {
                child->move(0, childIndex * sc1);
                child->resize(sc2, sc1);
            }
            child = isReverse ? child->previousSibling() : child->nextSibling();
            childIndex += 1;
        }
    }
}

} // namespace ui
} // namespace vgc
