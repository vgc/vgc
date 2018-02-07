// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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

#include <vgc/widgets/console.h>

#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>

#include <vgc/core/algorithm.h>
#include <vgc/core/math.h>
#include <vgc/core/python.h>

// Notes:
//
// [1]
//
// Handling of dead keys (e.g., '^' + 'e' => ê) or more complex input methods
// (Chinese, etc.) is tricky. The current method seems to work for dead keys
// and compose, but may not work for more complex input method. See:
//   https://stackoverflow.com/questions/28793356/qt-and-dead-keys-in-a-custom-widget
//   http://www.kdab.com/qt-input-method-depth/
//
// [2]
//
// Here is a simple code editor example we took inspiration from:
//
//   http://doc.qt.io/qt-5.6/qtwidgets-widgets-codeeditor-example.html
//
// We also used inpiration from QtCreator text editor, which is based on the
// same idea:
//
//   https://github.com/qt-creator/qt-creator/blob/master/src/plugins/texteditor/texteditor.h

namespace {
bool isTextInsertionOrDeletion_(QKeyEvent* e) {
    return !e->text().isEmpty();
}
}

namespace {
int lineNumber_(const QTextCursor& cursor) {
    return cursor.blockNumber();
}
}

namespace vgc {
namespace widgets {

Console::Console(
    core::PythonInterpreter* interpreter,
    QWidget* parent) :

    QPlainTextEdit(parent),
    interpreter_(interpreter)
{    
    codeBlocks_.push_back(0);

    // Handling of dead keys. See [1].
    setAttribute(Qt::WA_InputMethodEnabled, true);

    // Set background color.
    // Note: In Qt, the background color in text edits is called "Base".
    QColor backgroundColor(255, 255, 255);
    QPalette p = palette();
    p.setColor(QPalette::Base, backgroundColor);
    setPalette(p);
}

Console::~Console()
{

}

// The following implementation is inspired from:
// 1. Qt's implementation of QPlainTextEdit::paintEvent()
// 2. QtCreator's implementation of TextEditor::paintEvent()
// 3. Code Editor Example in Qt documentation
//
void Console::paintEvent(QPaintEvent* event)
{
    QPainter painter(viewport());

    // Get paint context. This provides cursor position and selections
    QAbstractTextDocumentLayout::PaintContext context = getPaintContext();

    // Get area to be repainted
    QRect eventRect = event->rect();
    int eventTop = eventRect.top();
    int eventBottom = eventRect.bottom();

    // Get viewport rectangle, to avoid painting anything outside of it
    QRect viewportRect = viewport()->rect();
    int viewportHeight = viewportRect.height();

    // Paint console background
    double backgroundMaxWidth = document()->documentLayout()->documentSize().width();
    painter.fillRect(eventRect, palette().base());

    // Whether to draw code block separators and their pen style
    QPen codeBlockSeparatorsPen(codeBlockSeparatorsColor_);

    // Whether to draw the cursor.
    bool isEditable = !isReadOnly();
    bool isTextSelectableByKeyboard = textInteractionFlags() & Qt::TextSelectableByKeyboard;
    bool drawCursor = isEditable || isTextSelectableByKeyboard;
    int cursorPosition = context.cursorPosition;

    // Loop through all visible lines.
    //
    // Note: in a QPlainTextEdit, each QTextDocument line consists of one
    // QTextBlock. However, due to text wrapping, one QTextDocument line may
    // be displayed as several rows.
    //
    QPointF offset = contentOffset();
    QTextBlock block = firstVisibleBlock();
    int lineNumber = block.blockNumber();
    int codeBlockIndex = -1;
    while (block.isValid()) {

        // Get basic block geometry
        QRectF blockRect = blockBoundingRect(block).translated(offset);
        double blockTop = blockRect.top();
        double blockHeight = blockRect.height();
        double blockBottom = blockTop + blockHeight;
        double blockWidth = blockRect.width();
        double blockLeft = blockRect.left();

        // Ignore block if it is invisible
        if (!block.isVisible()) {
            offset.ry() += blockHeight;
            block = block.next();
            ++lineNumber;
            continue;
        }

        // Paint block if it is within area to be repainted.
        // We use "blockTop - 1" instead of simply "blockTop" to account for
        // the code block separators which are drawn 1px higher than the block.
        if (blockBottom >= eventTop && blockTop - 1 <= eventBottom) {

            // Paint block background. This is for the rare case where a block
            // have a different background than the general console background.
            QTextBlockFormat blockFormat = block.blockFormat();
            QBrush backgroundBrush = blockFormat.background();
            if (backgroundBrush != Qt::NoBrush) {
                QRectF backgroundRect = blockRect;
                backgroundRect.setWidth(std::max(blockWidth, backgroundMaxWidth));
                painter.fillRect(backgroundRect, backgroundBrush);
            }

            // Paint separation between code blocks. We simply draw a line on
            // top of the first QTextBlock of the code block, except for the
            // very first QTextBlock.
            if (showCodeBlockSeparators_ && lineNumber > 0) {

                // Find code block corresponding to this line number. More specifically,
                // we are looking for the value codeBlockIndex such that:
                // codeBlocks_[codeBlockIndex] <= lineNumber < codeBlocks_[codeBlockIndex + 1]
                if (codeBlockIndex == -1) {
                    // The first time, we use a binary search
                    codeBlockIndex = core::upper_bound(codeBlocks_, lineNumber) - 1;
                }
                else {
                    // The subsequent times, we simply advance one by one
                    while ((int) codeBlocks_.size() > codeBlockIndex + 1
                           && codeBlocks_[codeBlockIndex + 1] <= lineNumber)
                    {
                        ++codeBlockIndex;
                    }
                }

                // Draw if text block is the first text block of its code block
                if (codeBlocks_[codeBlockIndex] == lineNumber) {
                    double y = blockTop - 1;
                    double x1 = blockLeft;
                    double x2 = x1 + std::max(blockWidth, backgroundMaxWidth);
                    QLineF line(x1, y, x2, y);
                    painter.save();
                    painter.setPen(codeBlockSeparatorsPen);
                    painter.drawLine(line);
                    painter.restore();
                }
            }

            // Determine per-block selection from global document selection
            QVector<QTextLayout::FormatRange> selections;
            int blockPosition = block.position();
            int blockLength = block.length();
            Q_FOREACH (const QAbstractTextDocumentLayout::Selection& selection, context.selections) {
                int selectionStart = selection.cursor.selectionStart() - blockPosition;
                int selectionEnd = selection.cursor.selectionEnd() - blockPosition;
                if (selectionStart < blockLength
                    && selectionEnd > 0
                    && selectionEnd > selectionStart)
                {
                    QTextLayout::FormatRange formatRange;
                    formatRange.start = selectionStart;
                    formatRange.length = selectionEnd - selectionStart;
                    formatRange.format = selection.format;
                    selections.append(formatRange);
                }
                // Note: in Qt 5.6 implementation of QPlainTextEdit::paintEvent(),
                // there is additional code here to support
                // QTextFormat::FullWidthSelection, which we don't support.
            }

            // Determine whether the cursor belong to this block
            bool isCursorInBlock = cursorPosition >= blockPosition
                                   && cursorPosition < blockPosition + blockLength;

            // Determine whether we should draw the cursor in the current loop
            // iteration, and whether to draw it as selection or as line
            bool drawCursorNow = drawCursor && isCursorInBlock;
            bool drawCursorAsSelection = drawCursorNow
                                         && overwriteMode()
                                         && cursorPosition < blockPosition + blockLength - 1;
            bool drawCursorAsLine = drawCursorNow && !drawCursorAsSelection;

            // Add cursor as selection
            if (drawCursorAsSelection) {
                QTextLayout::FormatRange formatRange;
                formatRange.start = cursorPosition - blockPosition;
                formatRange.length = 1;
                formatRange.format.setForeground(palette().base());
                formatRange.format.setBackground(palette().text());
                selections.append(formatRange);
            }

            // Paint selection + text
            QTextLayout* layout = block.layout();
            if (block.isVisible() && blockBottom >= eventTop) {
                layout->draw(&painter, offset, selections, eventRect);
            }

            // Paint cursor
            if (drawCursorAsLine) {
                int cursorPositionInBlock = cursorPosition - blockPosition;
                layout->drawCursor(&painter, offset, cursorPositionInBlock, cursorWidth());
                // Note: in Qt 5.6 implementation of QPlainTextEdit::paintEvent(),
                // there is additional code here to do something different when
                // cursorPosition < -1 && !layout->preeditAreaText().isEmpty().
                // I didn't understand what this code was for, therefore I chose
                // to omit this part of the implementation.
            }

        }

        // Iterate, stopping at last visible block
        offset.ry() += blockHeight;
        if (offset.y() > viewportHeight)
            break;
        block = block.next();
        ++lineNumber;
    }
}

// Handling of dead keys. See [1].
void Console::inputMethodEvent(QInputMethodEvent* event)
{
    if (!event->commitString().isEmpty()) {
        QKeyEvent keyEvent(
            QEvent::KeyPress, 0, Qt::NoModifier, event->commitString());
        keyPressEvent(&keyEvent);
    }
    event->accept();
}

// Handling of dead keys. See [1].
QVariant Console::inputMethodQuery(Qt::InputMethodQuery) const
{
    return QVariant();
}

// XXX TODO ignore backspace if beginning of code block
//
// XXX TODO ignore deletion if selection or part of selection
// is out of code block.

void Console::keyPressEvent(QKeyEvent* e)
{
    if (isTextInsertionOrDeletion_(e)) {

        // Prevent inserting or deleting text before last code block
        // XXX This seems to also prevent copying via Ctrl+C, which
        // generates e->text() == "\u0003".
        if (currentLineNumber_() < codeBlocks_.back()) {
            e->accept();
        }

        // Process last code block on Ctrl + Enter
        else if ((e->text() == "\r") &&
                 (e->modifiers() & Qt::CTRL))
        {
            // Move cursor's anchor+position to beginning of code block
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::StartOfLine);
            while (lineNumber_(cursor) > codeBlocks_.back()) {
                cursor.movePosition(QTextCursor::Up);
            }

            // Move cursor's position to end of code block, which
            // happens to be the end of the document
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

            // Get code block as string. We need to replace all paragraph
            // separators by line breaks otherwise it's not legal python code
            // and PyRun_String errors out.
            //
            // From Qt doc:
            // "If the selection obtained from an editor spans a line break,
            // the text will contain a Unicode U+2029 paragraph separator
            // character instead of a newline \n character. Use
            // QString::replace() to replace these characters with newlines."
            //
            QString codeBlock = cursor.selectedText();
            codeBlock.replace("\u2029", "\n");

            // Deselect
            cursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);

            // Clear selection and set text cursor to end of document
            cursor.clearSelection();
            setTextCursor(cursor);

            // Insert a new line by processing Ctrl+Enter as if it was a
            // regular Enter (at least under my Linux environment, Ctrl+Enter
            // does not insert anything in a QTextEdit, reason why we clear the
            // modifiers)
            e->setModifiers(Qt::NoModifier);
            QPlainTextEdit::keyPressEvent(e);

            // Interpret python code
            interpreter()->run(qUtf8Printable(codeBlock));

            // Update code blocks
            codeBlocks_.push_back(currentLineNumber_());
        }

        // Normal insertion/deletion of character, including newlines
        else {
            QPlainTextEdit::keyPressEvent(e);
        }
    }
    else {
        // Anything which is not an insertion or deletion, such as:
        // - Key modifiers
        // - Navigation (arrows, home, end, page up/down, etc.)
        // - Complex input methods (dead key, Chinese character composition, etc.)
        QPlainTextEdit::keyPressEvent(e);
    }
}

int Console::currentLineNumber_() const
{
    return lineNumber_(textCursor());
}

} // namespace widgets
} // namespace vgc
