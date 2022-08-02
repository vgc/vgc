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
    mouseButton_(MouseButton::None),
    numLeftMouseButtonClicks_(0),
    mouseSelectionMarkers_(graphics::TextBoundaryMarker::Grapheme),
    mouseSelectionInitialPair_(0, 0)
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
    triangles_ = engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void LineEdit::onPaintDraw(graphics::Engine* engine, PaintOptions /*options*/)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a;

        // Draw background
        core::Color backgroundColor = internal::getColor(this, isHovered_ ?
                        strings::background_color_on_hover :
                        strings::background_color);
        float borderRadius = internal::getLength(this, strings::border_radius);
#if defined(VGC_QOPENGL_EXPERIMENT) || 1
        static core::Stopwatch sw = {};
        auto t = sw.elapsed() * 50.f;
        backgroundColor = core::Color::hsl(t, 0.6f, 0.3f);
#endif
        internal::insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);

        // Draw text
        richText_->fill(a);

        // Load triangles data
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_, -1, 0);
}

void LineEdit::onPaintDestroy(graphics::Engine*)
{
    triangles_.reset();
}

void LineEdit::extendSelection_(const geometry::Vec2f& point)
{
    Int position = richText_->positionFromPoint(point, mouseSelectionMarkers_);
    Int beginPosition;
    Int endPosition;
    if (position < mouseSelectionInitialPair_.first) {
        beginPosition = mouseSelectionInitialPair_.second;
        endPosition = position;
    }
    else if (position < mouseSelectionInitialPair_.second) {
        beginPosition = mouseSelectionInitialPair_.first;
        endPosition = mouseSelectionInitialPair_.second;
    }
    else {
        beginPosition = mouseSelectionInitialPair_.first;
        endPosition = position;
    }
    richText_->setSelectionStart(beginPosition);
    richText_->setSelectionEnd(endPosition);
}

void LineEdit::resetSelectionInitialPair_()
{
    Int position = richText_->selectionStart();
    mouseSelectionMarkers_ = graphics::TextBoundaryMarker::Grapheme;
    mouseSelectionInitialPair_ = std::make_pair(position, position);
}

bool LineEdit::onMouseMove(MouseEvent* event)
{
    if (mouseButton_ == MouseButton::Left) {
        geometry::Vec2f mousePosition = event->position();
        geometry::Vec2f mouseOffset = richText_->rect().pMin();
        geometry::Vec2f point = mousePosition - mouseOffset;
        extendSelection_(point);
        reload_ = true;
        repaint();
    }
    return true;
}
namespace MouseButton_ {
enum {
    None = core::toUnderlying(MouseButton::None)
};
}

bool LineEdit::onMousePress(MouseEvent* event)
{
    // Only support one mouse button at a time
    if (mouseButton_ != MouseButton::None) {
        return false;
    }
    mouseButton_ = event->button();

    const bool left = mouseButton_ == MouseButton::Left;
    const bool right = mouseButton_ == MouseButton::Right;
    const bool middle = mouseButton_ == MouseButton::Middle;
    const bool shift = event->modifierKeys().has(ModifierKey::Shift);

    // Handle double/triple left click
    geometry::Vec2f mousePosition = event->position();
    if (left) {
        if (numLeftMouseButtonClicks_ > 0
            && leftMouseButtonStopwatch_.elapsedMilliseconds() < 500
            && (mousePosition - mousePositionOnPress_).length() < 5) {

            ++numLeftMouseButtonClicks_;
        }
        else {
            numLeftMouseButtonClicks_ = 1;
        }
        leftMouseButtonStopwatch_.restart();
    }
    else {
        numLeftMouseButtonClicks_ = 0;
    }
    mousePositionOnPress_ = mousePosition;

    // Change cursor position on press of any of the 3 standard mouse buttons
    if (left || right || middle) {
        geometry::Vec2f mouseOffset = richText_->rect().pMin();
        geometry::Vec2f point = mousePosition - mouseOffset;
        if (left && shift) {
            extendSelection_(point);
        }
        else {
            // On multiple left clicks, cycle between set cursor / select word / select line
            Int mod = numLeftMouseButtonClicks_ % 3;
            if (numLeftMouseButtonClicks_ < 2 || mod == 1) {
                mouseSelectionMarkers_ = graphics::TextBoundaryMarker::Grapheme;
                Int position = richText_->positionFromPoint(point, mouseSelectionMarkers_);
                mouseSelectionInitialPair_ = {position, position};
            }
            else {
                mouseSelectionMarkers_ = mod ? graphics::TextBoundaryMarker::Word
                                             : graphics::TextBoundaryMarker::MandatoryLineBreak;
                mouseSelectionInitialPair_ =
                    richText_->positionPairFromPoint(point, mouseSelectionMarkers_);
            }
            richText_->setSelectionStart(mouseSelectionInitialPair_.first);
            richText_->setSelectionEnd(mouseSelectionInitialPair_.second);
        }

        // Middle-button paste on supported platforms (e.g., X11)
        if (middle) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            std::string t = clipboard->text(QClipboard::Selection).toStdString();
            richText_->insertText(t);
            resetSelectionInitialPair_();
        }
    }

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

void copyToX11SelectionClipboard_(graphics::RichText* richText)
{
    static bool supportsSelection = QGuiApplication::clipboard()->supportsSelection();
    if (supportsSelection && richText->hasSelection()) {
        copyToClipboard_(richText->selectedTextView(), QClipboard::Selection);
    }
}

} // namespace

bool LineEdit::onMouseRelease(MouseEvent* event)
{
    // Only support one mouse button at a time
    if (mouseButton_ != event->button()) {
        return false;
    }

    if (mouseButton_ == MouseButton::Left) {
        copyToX11SelectionClipboard_(richText_.get());
    }

    mouseButton_ = MouseButton::None;
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
    using Op = graphics::RichTextMoveOperation;

    const int key = event->key();
    const bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);

    bool handled = true;
    bool needsRepaint = true;
    bool isMoveOperation = false;

    if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
        if (key == Qt::Key_Delete) {
            richText_->deleteFromCursor(ctrl ? Op::NextWord : Op::NextCharacter);
        }
        else { // backspace
            richText_->deleteFromCursor(ctrl ? Op::PreviousWord : Op::PreviousCharacter);
        }
    }
    else if (key == Qt::Key_Home) {
        richText_->moveCursor(ctrl ? Op::StartOfText : Op::StartOfLine, shift);
        isMoveOperation = true;
    }
    else if (key == Qt::Key_End) {
        richText_->moveCursor(ctrl ? Op::EndOfText : Op::EndOfLine, shift);
        isMoveOperation = true;
    }
    else if (key == Qt::Key_Left) {
        if (richText_->hasSelection() && !shift) {
            richText_->moveCursor(Op::LeftOfSelection);
        }
        else {
            richText_->moveCursor(ctrl ? Op::LeftOneWord : Op::LeftOneCharacter, shift);
        }
        isMoveOperation = true;
    }
    else if (key == Qt::Key_Right) {
        if (richText_->hasSelection() && !shift) {
            richText_->moveCursor(Op::RightOfSelection);
        }
        else {
            richText_->moveCursor(ctrl ? Op::RightOneWord : Op::RightOneCharacter, shift);
        }
        isMoveOperation = true;
    }
    else if (ctrl && key == Qt::Key_X) {
        if (richText_->hasSelection()) {
            copyToClipboard_(richText_->selectedTextView());
            richText_->deleteSelectedText();
        }
        else {
            needsRepaint = false;
        }
    }
    else if (ctrl && key == Qt::Key_C) {
        if (richText_->hasSelection()) {
            copyToClipboard_(richText_->selectedTextView());
        }
        needsRepaint = false;
    }
    else if (ctrl && key == Qt::Key_V) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        std::string t = clipboard->text().toStdString();
        richText_->insertText(t);
    }
    else if (ctrl && key == Qt::Key_A) {
        richText_->selectAll();
    }
    else if (key == Qt::Key_Escape) {
        handled = false;
    }
    else if (key == Qt::Key_Tab) {
        handled = false;
    }
    else if (!ctrl) {
        std::string t = event->text().toStdString();
        bool isControlCharacter = (t.size() == 1) && (t[0] >= 0) && (t[0] < 32);
        if (!t.empty() && !isControlCharacter) {
            richText_->insertText(t);
        }
        else {
            handled = false;
        }
    }
    else {
        handled = false;
    }

    // X11 selection clipboard
    if (shift && isMoveOperation) {
        copyToX11SelectionClipboard_(richText_.get());
    }

    if (handled && needsRepaint) {
        resetSelectionInitialPair_();
        reload_ = true;
        repaint();
    }
    return handled;
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

} // namespace vgc::ui
