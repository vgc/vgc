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
#include <vgc/interpreter/interpreter.h>
#include <QKeyEvent>

namespace vgc {
namespace widgets {

Console::Console(
    interpreter::Interpreter* interpreter,
    QWidget* parent) :

    QTextEdit(parent),
    interpreter_(interpreter)
{

}

Console::~Console()
{

}

void Console::keyPressEvent(QKeyEvent* e)
{
    switch (e->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        onEnterKeyPress_();
    }

    QTextEdit::keyPressEvent(e);
}

QString Console::currentLine_() const
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    return cursor.selectedText();
}

void Console::onEnterKeyPress_()
{
    QString command = currentLine_();
    interpreter()->run(qPrintable(command));
}

} // namespace widgets
} // namespace vgc
