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

#include <vgc/ui/colorpalette.h>

#include <vgc/core/colors.h>

namespace vgc {
namespace ui {

ColorPalette::ColorPalette(const ConstructorKey&) :
    Widget(Widget::ConstructorKey()),
    currentColor_(core::colors::blue) // tmp color for testing. TODO: black
{

}

/* static */
ColorPaletteSharedPtr ColorPalette::create()
{
    return std::make_shared<ColorPalette>(ConstructorKey());
}

void ColorPalette::initialize(graphics::Engine* /*engine*/)
{

}

void ColorPalette::resize(graphics::Engine* /*engine*/, Int /*w*/, Int /*h*/)
{

}

void ColorPalette::paint(graphics::Engine* engine)
{
    engine->clear(currentColor());
}

void ColorPalette::cleanup(graphics::Engine* /*engine*/)
{

}

bool ColorPalette::mousePressEvent(MouseEvent* /*event*/)
{
    setCurrentColor(
        currentColor() == core::colors::blue ?
        core::colors::red :
        core::colors::blue);
    repaint();
    return true;
}

} // namespace ui
} // namespace vgc
