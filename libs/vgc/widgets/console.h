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

#ifndef VGC_WIDGETS_CONSOLE_H
#define VGC_WIDGETS_CONSOLE_H

#include <QPlainTextEdit>
#include <vgc/core/python.h>
#include <vgc/widgets/api.h>

namespace vgc::widgets {

class ConsoleMargin;

/// \class vgc::core::Console
/// \brief GUI around the Python interpreter
///
class VGC_WIDGETS_API Console : public QPlainTextEdit {
private:
    Q_OBJECT

public:
    /// Constructs a Console.
    ///
    Console(core::PythonInterpreter* interpreter, QWidget* parent = nullptr);

    /// Destructs a Console.
    ///
    ~Console();

    /// Returns the underlying PythonInterpreter.
    ///
    core::PythonInterpreter* interpreter() const {
        return interpreter_;
    }

    /// Returns whether to show code block separators.
    ///
    bool showCodeBlockSeparators() const {
        return showCodeBlockSeparators_;
    }

    /// Sets whether to show code block separators.
    ///
    void showCodeBlockSeparators(bool value) {
        showCodeBlockSeparators_ = value;
    }

    /// Returns the color of code block separators.
    ///
    QColor codeBlockSeparatorsColor() const {
        return codeBlockSeparatorsColor_;
    }

    /// Sets whether to show code block separators.
    ///
    void setCodeBlockSeparatorsColor(const QColor& color) {
        codeBlockSeparatorsColor_ = color;
    }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void updateConsoleMargin_(const QRect&, int);

private:
    core::PythonInterpreter* interpreter_;

    // Handling key presses
    QVariant inputMethodQuery(Qt::InputMethodQuery) const override;
    void inputMethodEvent(QInputMethodEvent*) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;
    void dropEvent(QDropEvent* e) override;

    void beginReadOnlyProtection_(QMouseEvent* e);
    void beginReadOnlyProtection_(const QTextCursor&);
    void endProtectPreviousBlocks_();
    int currentLineNumber_() const;

    // Code blocks. This is a sorted list of 0-indexed
    // line numbers where code blocks start.
    std::vector<int> codeBlocks_;

    // Console margin
    friend class ConsoleMargin;
    ConsoleMargin* consoleMargin_;
    int consoleMarginWidth_;
    void setupConsoleMargin_();
    void consoleMarginPaintEvent_(QPaintEvent* event);
    void computeConsoleMarginWidth_();

    // Code block separators
    bool showCodeBlockSeparators_ = false;
    QColor codeBlockSeparatorsColor_ = QColor(190, 190, 190);

    // Interpreter prompt
    QString primaryPromptString_ = QString(">>>");
    QString secondaryPromptString_ = QString("...");
};

/// \class ConsoleMargin
/// \brief The margin area of a Console
///
/// This widget represents the margin area of a Console, typically drawn on the
/// left of the Console, and displaying the interpreter prompt.
///
/// Normally, you should not create a ConsoleMargin yourself, since it is
/// automatically created and managed by its associated Console. The reason
/// this class is public is to allow users to style it using Qt stylesheets.
/// Ideally, we would be better to keep this class internal, and allow styling
/// via Console::margin. However, it was unclear how to achieve this in the
/// given time constraints, which is why we adopted this simpler solution.
///
class VGC_WIDGETS_API ConsoleMargin : public QWidget {
private:
    Q_OBJECT

public:
    /// Constructs a ConsoleMargin.
    ///
    ConsoleMargin(vgc::widgets::Console* console);

    /// Destructs this ConsoleMargin.
    ///
    ~ConsoleMargin();

    /// Reimplements sizeHint().
    ///
    QSize sizeHint() const override;

protected:
    /// Reimplements paintEvent().
    ///
    void paintEvent(QPaintEvent* event) override;

private:
    Console* console_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_CONSOLE_H
