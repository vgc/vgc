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

#include <vgc/core/array.h>
#include <vgc/core/colors.h>
#include <vgc/core/performancelog.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/internal/paintutil.h>

namespace vgc {
namespace ui {

LineEdit::LineEdit(std::string_view text) :
    Widget(),
    text_(""),
    shapedText_(internal::getDefaultSizedFont(), text_),
    textCursor_(false, 0),
    scrollLeft_(0.0f),
    reload_(true),
    isHovered_(false),
    isMousePressed_(false)
{
    addStyleClass(strings::LineEdit);
    setText(text);
}

LineEditPtr LineEdit::create()
{
    return LineEditPtr(new LineEdit(""));
}

LineEditPtr LineEdit::create(std::string_view text)
{
    return LineEditPtr(new LineEdit(text));
}

void LineEdit::setText(std::string_view text)
{
    if (text_ != text) {
        text_ = text;
        shapedText_.setText(text);
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
    triangles_ = engine->createTriangles();
}

void LineEdit::onPaintDraw(graphics::Engine*)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a;
        core::Color backgroundColor = internal::getColor(this, isHovered_ ?
                        strings::background_color_on_hover :
                        strings::background_color);

#ifdef VGC_QOPENGL_EXPERIMENT
        static core::Stopwatch sw = {};
        auto t = sw.elapsed() * 50.f;
        backgroundColor = core::Color::hsl(t, 0.6f, 0.3f);
#endif

        core::Color textColor = internal::getColor(this, strings::text_color);
        float borderRadius = internal::getLength(this, strings::border_radius);
        float paddingLeft = internal::getLength(this, strings::padding_left);
        float paddingRight = internal::getLength(this, strings::padding_right);
        float textWidth = width() - paddingLeft - paddingRight;
        graphics::TextProperties textProperties(
                    graphics::TextHorizontalAlign::Left,
                    graphics::TextVerticalAlign::Middle);
        if (hasFocus()) {
            textCursor_.setVisible(true);
        }
        else {
            textCursor_.setVisible(false);
        }
        updateScroll_(textWidth);
        bool hinting = style(strings::pixel_hinting) == strings::normal;
        internal::insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);
        internal::insertText(a, textColor, 0, 0, width(), height(), paddingLeft, paddingRight, 0, 0, shapedText_, textProperties, textCursor_, hinting, scrollLeft_);
        triangles_->load(a.data(), a.length());
    }
    triangles_->draw();
}

void LineEdit::onPaintDestroy(graphics::Engine*)
{
    triangles_.reset();
}

bool LineEdit::onMouseMove(MouseEvent* event)
{
    if (isMousePressed_) {
        updateBytePosition_(event->pos());
    }
    return true;
}

bool LineEdit::onMousePress(MouseEvent* event)
{
    isMousePressed_ = true;
    setFocus();
    updateBytePosition_(event->pos());
    return true;
}

bool LineEdit::onMouseRelease(MouseEvent* /*event*/)
{
    isMousePressed_ = false;
    return true;
}

bool LineEdit::onMouseEnter()
{
    pushCursor(Qt::IBeamCursor);
    return true;
}

bool LineEdit::onMouseLeave()
{
    popCursor();
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
    int key = event->key();
    if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
        Int p1_ = textCursor_.bytePosition();
        Int p2_ = -1;
        graphics::TextBoundaryType boundaryType =
                (event->modifiers().testFlag(Qt::ControlModifier)) ?
                    graphics::TextBoundaryType::Word :
                    graphics::TextBoundaryType::Grapheme;
        graphics::TextBoundaryIterator it(boundaryType, text());
        it.setPosition(p1_);
        if (key == Qt::Key_Delete) {
            p2_ = it.toNextBoundary();
        }
        else { // Backspace
            p2_ = p1_;
            p1_ = it.toPreviousBoundary();
        }
        if (p1_ != -1 && p2_ != -1) {
            size_t p1 = core::int_cast<size_t>(p1_);
            size_t p2 = core::int_cast<size_t>(p2_);
            std::string newText;
            newText.reserve(text().size() - (p2 - p1));
            newText.append(text(), 0, p1);
            newText.append(text(), p2);
            textCursor_.setBytePosition(p1_);
            setText(newText);
        }
        return true;
    }
    else if (key == Qt::Key_Home) {
        Int p1 = textCursor_.bytePosition();
        Int home = 0;
        if (p1 != home) {
            textCursor_.setBytePosition(home);
            reload_ = true;
            repaint();
        }
        return true;
    }
    else if (key == Qt::Key_End) {
        Int p1 = textCursor_.bytePosition();
        Int end = core::int_cast<Int>(text().size());
        if (p1 != end) {
            textCursor_.setBytePosition(end);
            reload_ = true;
            repaint();
        }
        return true;
    }
    else if (key == Qt::Key_Left || key == Qt::Key_Right) {
        Int p1 = textCursor_.bytePosition();
        Int p2 = -1;
        graphics::TextBoundaryType boundaryType =
                (event->modifiers().testFlag(Qt::ControlModifier)) ?
                    graphics::TextBoundaryType::Word :
                    graphics::TextBoundaryType::Grapheme;
        graphics::TextBoundaryIterator it(boundaryType, text());
        it.setPosition(p1);
        if (key == Qt::Key_Left) {
            p2 = it.toPreviousBoundary();
        }
        else { // Right
            p2 = it.toNextBoundary();
        }
        if (p2 != -1 && p1 != p2) {
            textCursor_.setBytePosition(it.position());
            reload_ = true;
            repaint();
        }
        return true;
    }
    else {
        std::string t = event->text().toStdString();
        if (!t.empty()) {
            size_t p = core::int_cast<size_t>(textCursor_.bytePosition());
            std::string newText;
            newText.reserve(text().size() + t.size());
            newText.append(text(), 0, p);
            newText.append(t);
            newText.append(text(), p);
            textCursor_.setBytePosition(p + t.size());
            setText(newText);
            return true;
        }
        else {
            return false;
        }
    }
}

geometry::Vec2f LineEdit::computePreferredSize() const
{
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    geometry::Vec2f res(0, 0);
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

void LineEdit::updateBytePosition_(const geometry::Vec2f& mousePosition)
{
    Int bytePosition = bytePosition_(mousePosition);
    if (bytePosition != textCursor_.bytePosition()) {
        textCursor_.setBytePosition(bytePosition);
        reload_ = true;
        repaint();
    }
}

Int LineEdit::bytePosition_(const geometry::Vec2f& mousePosition)
{
    float paddingLeft = internal::getLength(this, strings::padding_left);
    float x = mousePosition[0] - paddingLeft + scrollLeft_;
    float y = mousePosition[1];
    return shapedText_.bytePosition(geometry::Vec2f(x, y));
}

void LineEdit::updateScroll_(float textWidth)
{
    float textEndAdvance = shapedText_.advance()[0];
    float currentTextEndPos = textEndAdvance - scrollLeft_;
    if (currentTextEndPos < textWidth && scrollLeft_ > 0) {
        if (textEndAdvance < textWidth) {
            scrollLeft_ = 0;
        }
        else {
            scrollLeft_ = textEndAdvance - textWidth;
        }
    }
    if (textCursor_.isVisible()) {
        float cursorAdvance = shapedText_.advance(textCursor_.bytePosition())[0];
        float currentCursorPos = cursorAdvance - scrollLeft_;
        if (currentCursorPos < 0) {
            scrollLeft_ = cursorAdvance;
        }
        else if (currentCursorPos > textWidth) {
            scrollLeft_ = cursorAdvance - textWidth;
        }
    }
}

} // namespace ui
} // namespace vgc
