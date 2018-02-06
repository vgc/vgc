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

#ifndef VGC_WIDGETS_CONSOLE_H
#define VGC_WIDGETS_CONSOLE_H

#include <vgc/widgets/api.h>
#include <QPlainTextEdit>

namespace vgc {

namespace core { class PythonInterpreter; }

namespace widgets {

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

protected:
    void paintEvent(QPaintEvent*) override;

private:
    core::PythonInterpreter* interpreter_;

    // Handling key presses
    void inputMethodEvent(QInputMethodEvent*);
    QVariant inputMethodQuery(Qt::InputMethodQuery) const;
    void keyPressEvent(QKeyEvent* e) override;

    int currentLineNumber_() const;

    // Sorted list of 0-indexed line numbers where code block starts.
    std::vector<int> codeBlocks_;
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_CONSOLE_H
