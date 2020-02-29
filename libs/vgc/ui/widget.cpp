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

Widget::Widget(const ConstructorKey&) :
    Object(core::Object::ConstructorKey()),
    width_(0.0f),
    height_(0.0f)
{

}

/* static */
WidgetSharedPtr Widget::create()
{
    return std::make_shared<Widget>(ConstructorKey());
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
