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

#include <vgc/ui/label.h>

#include <vgc/core/paths.h>
#include <vgc/core/floatarray.h>
#include <vgc/graphics/font.h>

namespace vgc {
namespace ui {

Label::Label() :
    Widget(),
    text_(""),
    trianglesId_(-1),
    reload_(true)
{

}

Label::Label(const std::string& text) :
    Widget(),
    text_(text),
    trianglesId_(-1),
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
    trianglesId_ = engine->createTriangles();
}

void Label::onPaintDraw(graphics::Engine* engine)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a;

        // Convert text to triangles
        if (text_.length() > 0) {
            // TODO: we should a more persistent font library rather than creating
            // a new one each time
            std::string facePath = core::resourcePath("graphics/fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf");
            graphics::FontLibraryPtr fontLibrary = graphics::FontLibrary::create();
            graphics::FontFace* fontFace = fontLibrary->addFace(facePath); // XXX can this be nullptr?
            core::DoubleArray b;
            for (const graphics::FontItem& shape : fontFace->shape(text_)) {
                float xoffset = shape.offset()[0];
                float yoffset = shape.offset()[1];
                b.clear();
                shape.glyph()->outline().stroke(b, 3);
                for (Int i = 0; 2*i+1 < b.length(); ++i) {
                    a.insert(a.end(), {
                        xoffset + static_cast<float>(b[2*i]),
                        yoffset + static_cast<float>(b[2*i+1]),
                        1, 0, 0});
                }
            }
        }
        // Load triangles
        engine->loadTriangles(trianglesId_, a.data(), a.length());
    }
    engine->clear(core::Color(0, 0.345, 0.353));
    engine->drawTriangles(trianglesId_);
}

void Label::onPaintDestroy(graphics::Engine* engine)
{
    engine->destroyTriangles(trianglesId_);
}

} // namespace ui
} // namespace vgc
