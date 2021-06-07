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

#include <vgc/ui/lineedit.h>

#include <QKeyEvent>

#include <vgc/core/colors.h>
#include <vgc/core/floatarray.h>
#include <vgc/ui/strings.h>
#include <vgc/ui/style.h>

#include <vgc/ui/internal/paintutil.h>

namespace vgc {
namespace ui {

LineEdit::LineEdit(const std::string& text) :
    Widget(),
    text_(text),
    trianglesId_(-1),
    reload_(true),
    isHovered_(false)
{
    addClass(strings::LineEdit);
}

LineEditPtr LineEdit::create()
{
    return LineEditPtr(new LineEdit(""));
}

LineEditPtr LineEdit::create(const std::string& text)
{
    return LineEditPtr(new LineEdit(text));
}

void LineEdit::setText(const std::string& text)
{
    if (text_ != text) {
        text_ = text;
        reload_ = true;
        repaint();
    }
}

void LineEdit::onResize()
{
    reload_ = true;
}

void LineEdit::onPaintCreate(graphics::Engine* engine)
{
    trianglesId_ = engine->createTriangles();
}

void LineEdit::onPaintDraw(graphics::Engine* engine)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a;
        core::Color backgroundColor = internal::getColor(this, isHovered_ ?
                        strings::background_color_on_hover :
                        strings::background_color);
        core::Color textColor = internal::getColor(this, strings::text_color);
        if (hasFocus()) {
            textColor = core::colors::green;
        }
        float borderRadius = internal::getLength(this, strings::border_radius);
        graphics::TextProperties textProperties(
                    graphics::TextHorizontalAlign::Left,
                    graphics::TextVerticalAlign::Middle);
        bool hinting = style(strings::pixel_hinting) == strings::normal;
        internal::insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);
        internal::insertText(a, textColor, 0, 0, width(), height(), text_, textProperties, hinting);
        engine->loadTriangles(trianglesId_, a.data(), a.length());
    }
    engine->drawTriangles(trianglesId_);
}

void LineEdit::onPaintDestroy(graphics::Engine* engine)
{
    engine->destroyTriangles(trianglesId_);
}

bool LineEdit::onMouseMove(MouseEvent* /*event*/)
{
    return true;
}

bool LineEdit::onMousePress(MouseEvent* /*event*/)
{
    setFocus();
    return true;
}

bool LineEdit::onMouseRelease(MouseEvent* /*event*/)
{
    return true;
}

bool LineEdit::onMouseEnter()
{
    /*
    isHovered_ = true;
    reload_ = true;
    repaint();
    */
    return true;
}

bool LineEdit::onMouseLeave()
{
    /*
    isHovered_ = false;
    reload_ = true;
    repaint();
    */
    return true;
}

bool LineEdit::onFocusIn()
{
    reload_ = true;
    repaint();
    return true;
}

bool LineEdit::onFocusOut()
{
    reload_ = true;
    repaint();
    return true;
}

bool LineEdit::onKeyPress(QKeyEvent* event)
{
    std::string t = event->text().toStdString();
    if (!t.empty()) {
        setText(text() + t);
        return true;
    }
    else {
        return false;
    }
}

core::Vec2f LineEdit::computePreferredSize() const
{
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    core::Vec2f res(0, 0);
    if (w.type() == auto_) {
        res[0] = 100;
        // TODO: compute appropriate width based on text length
    }
    else {
        res[0] = w.value();
    }
    if (h.type() == auto_) {
        res[1] = 26;
        // TODO: compute appropriate height based on font size?
    }
    else {
        res[1] = h.value();
    }
    return res;
}

} // namespace ui
} // namespace vgc
