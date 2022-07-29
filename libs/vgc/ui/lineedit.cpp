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

#include <QClipboard>
#include <QGuiApplication>
#include <QKeyEvent>

#include <vgc/core/array.h>
#include <vgc/core/colors.h>
#include <vgc/core/performancelog.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/internal/paintutil.h>

namespace vgc::ui {

LineEdit::LineEdit(std::string_view text) :
    Widget(),
    richText_(graphics::RichText::create()),
    reload_(true),
    isHovered_(false),
    isMousePressed_(false)
{
    addStyleClass(strings::LineEdit);
    setText(text);
    richText_->setParentStylableObject(this);
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
    if (text != richText_->text()) {
        richText_->setText(text);
        reload_ = true;
        repaint();
    }
}

style::StylableObject* LineEdit::firstChildStylableObject() const
{
    return richText_.get();
}

style::StylableObject* LineEdit::lastChildStylableObject() const
{
    return richText_.get();
}

void LineEdit::onResize()
{
    // Compute contentRect
    // TODO: move to Widget::contentRect()
    float paddingLeft = internal::getLength(this, strings::padding_left);
    float paddingRight = internal::getLength(this, strings::padding_right);
    float paddingTop = internal::getLength(this, strings::padding_top);
    float paddingBottom = internal::getLength(this, strings::padding_bottom);
    geometry::Rect2f r = rect();
    geometry::Vec2f pMinOffset(paddingLeft, paddingTop);
    geometry::Vec2f pMaxOffset(paddingRight, paddingBottom);
    geometry::Rect2f contentRect(r.pMin() + pMinOffset, r.pMax() - pMaxOffset);

    // Set appropriate size for the RichText
    richText_->setRect(contentRect);

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

        // Draw background
        core::Color backgroundColor = internal::getColor(this, isHovered_ ?
                        strings::background_color_on_hover :
                        strings::background_color);
        float borderRadius = internal::getLength(this, strings::border_radius);
#ifdef VGC_QOPENGL_EXPERIMENT
        static core::Stopwatch sw = {};
        auto t = sw.elapsed() * 50.f;
        backgroundColor = core::Color::hsl(t, 0.6f, 0.3f);
#endif
        internal::insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);

        // Draw text
        richText_->fill(a);

        // Load triangles data
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
        geometry::Vec2f mousePosition = event->pos();
        geometry::Vec2f mouseOffset = richText_->rect().pMin();

        Int bytePosition = richText_->bytePosition(mousePosition - mouseOffset);
        richText_->setSelectionEndBytePosition(bytePosition);

        isMousePressed_ = true;
        reload_ = true;
        setFocus();
        repaint();
        return true;
    }
    return true;
}

bool LineEdit::onMousePress(MouseEvent* event)
{
    geometry::Vec2f mousePosition = event->pos();
    geometry::Vec2f mouseOffset = richText_->rect().pMin();

    Int bytePosition = richText_->bytePosition(mousePosition - mouseOffset);
    richText_->setSelectionBeginBytePosition(bytePosition);
    richText_->setSelectionEndBytePosition(bytePosition);

    isMousePressed_ = true;
    reload_ = true;
    setFocus();
    repaint();
    return true;
}

namespace {

void copyToClipboard_(std::string_view text, QClipboard::Mode mode = QClipboard::Clipboard)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    size_t size = core::clamp(text.size(), 0, core::tmax<int>);
    QString qtext = QString::fromUtf8(text.data(), static_cast<int>(size));
    clipboard->setText(qtext, mode);
}

} // namespace

bool LineEdit::onMouseRelease(MouseEvent* /*event*/)
{
    static bool supportsSelection = QGuiApplication::clipboard()->supportsSelection();
    isMousePressed_ = false;
    if (supportsSelection && richText_->hasSelection()) {
        copyToClipboard_(richText_->selectedTextView(), QClipboard::Selection);
    }
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
    richText_->setSelectionVisible(true);
    richText_->setCursorVisible(true);
    reload_ = true;
    repaint();
    return true;
}

bool LineEdit::onFocusOut()
{
    // Focus out can occur in two different situations:
    // 1. Another widget in the same window gained the focus. In this
    //    case, we want to clear the selection and make the cursor invisible.
    // 2. Another window gained the focus. In this case, we keep the
    //    selection, and simply make the cursor invisible.
    if (!isFocusedWidget()) {
        richText_->clearSelection();
    }
    richText_->setCursorVisible(false);
    reload_ = true;
    repaint();
    return true;
}

bool LineEdit::onKeyPress(QKeyEvent* event)
{
    int key = event->key();
    bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
        graphics::TextBoundaryType boundaryType = ctrl
                ? graphics::TextBoundaryType::Word
                : graphics::TextBoundaryType::Grapheme;
        if (key == Qt::Key_Delete) {
            richText_->deleteNext(boundaryType);
        }
        else { // backspace
            richText_->deletePrevious(boundaryType);
        }
        reload_ = true;
        repaint();
        return true;
    }
    else if (key == Qt::Key_Home) {
        Int p1 = richText_->cursorBytePosition();
        Int home = 0;
        if (p1 != home) {
            richText_->setCursorBytePosition(home);
            reload_ = true;
            repaint();
        }
        return true;
    }
    else if (key == Qt::Key_End) {
        Int p1 = richText_->cursorBytePosition();
        Int end = core::int_cast<Int>(text().size());
        if (p1 != end) {
            richText_->setCursorBytePosition(end);
            reload_ = true;
            repaint();
        }
        return true;
    }
    else if (key == Qt::Key_Left || key == Qt::Key_Right) {
        Int p1 = richText_->cursorBytePosition();
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
            richText_->setCursorBytePosition(it.position());
            reload_ = true;
            repaint();
        }
        return true;
    }
    else if (ctrl && key == Qt::Key_X) {
        if (richText_->hasSelection()) {
            copyToClipboard_(richText_->selectedTextView());
            richText_->deleteSelectedText();
            reload_ = true;
            repaint();
        }
        return true;
    }
    else if (ctrl && key == Qt::Key_C) {
        if (richText_->hasSelection()) {
            copyToClipboard_(richText_->selectedTextView());
        }
        return true;
    }
    else if (ctrl && key == Qt::Key_V) {
        bool needsRepaint = false;
        if (richText_->hasSelection()) {
            richText_->deleteSelectedText();
            needsRepaint = true;
        }
        QClipboard* clipboard = QGuiApplication::clipboard();
        std::string t = clipboard->text().toStdString();
        if (!t.empty()) {
            size_t p = core::int_cast<size_t>(richText_->cursorBytePosition());
            std::string newText;
            newText.reserve(text().size() + t.size());
            newText.append(text(), 0, p);
            newText.append(t);
            newText.append(text(), p);
            richText_->setText(newText);
            richText_->setCursorBytePosition(p + t.size());
            needsRepaint = true;
        }
        if (needsRepaint) {
            reload_ = true;
            repaint();
        }
        return true;
    }
    else if (ctrl && key == Qt::Key_A) {
        richText_->selectAll();
        reload_ = true;
        repaint();
        return true;
    }
    else if (!ctrl) {
        std::string t = event->text().toStdString();
        if (!t.empty()) {
            size_t p = core::int_cast<size_t>(richText_->cursorBytePosition());
            std::string newText;
            newText.reserve(text().size() + t.size());
            newText.append(text(), 0, p);
            newText.append(t);
            newText.append(text(), p);
            richText_->setText(newText);
            richText_->setCursorBytePosition(p + t.size());
            reload_ = true;
            repaint();
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
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
    float paddingLeft = internal::getLength(this, strings::padding_left);
    float paddingTop = internal::getLength(this, strings::padding_top);
    geometry::Vec2f mouseOffset(paddingLeft, paddingTop);
    Int oldBytePosition = richText_->cursorBytePosition();
    richText_->setCursorFromMousePosition(mousePosition - mouseOffset);
    Int newBytePosition = richText_->cursorBytePosition();
    if (oldBytePosition != newBytePosition) {
        reload_ = true;
        repaint();
    }
}

} // namespace vgc::ui
