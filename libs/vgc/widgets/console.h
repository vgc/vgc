// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc-io/vgc/blob/master/COPYRIGHT
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

#include <vgc/widgets/api.h>
#include <QPlainTextEdit>

namespace vgc {

namespace core { class PythonInterpreter; }

namespace widgets {

namespace internal {
class ConsoleLeftMargin;
}

/// \class vgc::core::Console
/// \brief GUI around the Python interpreter
///
class VGC_WIDGETS_API Console : public QPlainTextEdit
{
    Q_OBJECT

public:
    /// Constructs a Console.
    ///
    Console(core::PythonInterpreter* interpreter,
            QWidget* parent = nullptr);

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
    void updateLeftMargin_(const QRect&, int);

private:
    core::PythonInterpreter* interpreter_;

    // Handling key presses
    void inputMethodEvent(QInputMethodEvent*);
    QVariant inputMethodQuery(Qt::InputMethodQuery) const;
    void keyPressEvent(QKeyEvent* e) override;

    int currentLineNumber_() const;

    // Code blocks. This is a sorted list of 0-indexed
    // line numbers where code blocks start.
    std::vector<int> codeBlocks_;

    // left margin
    friend class internal::ConsoleLeftMargin;
    QWidget* leftMargin_;
    int leftMarginWidth_;
    void setupLeftMargin_();
    void leftMarginPaintEvent_(QPaintEvent* event);
    void computeLeftMarginWidth_();

    // Code block separators
    bool showCodeBlockSeparators_ = false;
    QColor codeBlockSeparatorsColor_ = QColor(190, 190, 190);

    // Interpreter prompt
    QString primaryPromptString_ = QString(">>>");
    QString secondaryPromptString_ = QString("...");

    // Color scheme
    QColor backgroundColor_ = QColor(46, 47, 48);
    QColor marginBackgroundColor_ = QColor(64, 66, 68);
    QColor textColor_ = QColor(214, 208, 157);
    QColor promptColor_ = QColor(190, 192, 194);
    QColor selectionForegroundColor_ = QColor(190, 192, 194);
    QColor selectionBackgroundColor_ = QColor(29, 84, 92);
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_CONSOLE_H
