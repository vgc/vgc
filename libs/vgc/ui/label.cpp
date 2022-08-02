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

#include <vgc/ui/label.h>

#include <vgc/core/array.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/internal/paintutil.h>

namespace vgc {
namespace ui {

Label::Label() :
    Widget(),
    text_(""),
    reload_(true)
{
    addStyleClass(strings::Label);
}

Label::Label(const std::string& text) :
    Widget(),
    text_(text),
    reload_(true)
{

}

/* static */
LabelPtr Label::create()
{
    return LabelPtr(new Label());
}

/* static */
LabelPtr Label::create(const std::string& text)
{
    return LabelPtr(new Label(text));
}

void Label::setText(const std::string& text)
{
    if (text_ != text) {
        text_ = text;
        reload_ = true;
        repaint();
    }
}

void Label::onPaintCreate(graphics::Engine* engine)
{
    triangles_ = engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void Label::onPaintDraw(graphics::Engine* engine, PaintFlags /*flags*/)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a = {};
        core::Color textColor = internal::getColor(this, strings::text_color);
        graphics::TextProperties textProperties(
                    graphics::TextHorizontalAlign::Center,
                    graphics::TextVerticalAlign::Middle);
        graphics::TextCursor textCursor;
        bool hinting = style(strings::pixel_hinting) == strings::normal;
        internal::insertText(a, textColor, 0, 0, width(), height(), 0, 0, 0, 0, text_, textProperties, textCursor, hinting);
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_, -1, 0);
}

void Label::onPaintDestroy(graphics::Engine*)
{
    triangles_.reset();
}

} // namespace ui
} // namespace vgc
