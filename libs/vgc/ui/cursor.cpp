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

#include <vgc/ui/cursor.h>

#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>

#include <vgc/core/colors.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/qtutil.h>

namespace vgc::ui {

namespace {

class CursorStack {
public:
    static CursorStack* instance() {
        static CursorStack* instance_ = new CursorStack();
        return instance_;
    }

    Int push(const QCursor& cursor) {
        Int id = stack_.isEmpty() ? 0 : stack_.last().id + 1;
        stack_.append({cursor, id});
        if (stack_.size() == 1) {
            QGuiApplication::setOverrideCursor(cursor);
        }
        else {
            QGuiApplication::changeOverrideCursor(cursor);
        }
        return id;
    }

    void pop(Int id) {
        auto found = stack_.search([=](const Item& item) { return item.id == id; });
        if (found) {
            bool wasTopmostCursor = (found == stack_.end() - 1);
            stack_.erase(found);
            if (stack_.size() == 0) {
                QGuiApplication::restoreOverrideCursor();
            }
            else if (wasTopmostCursor) {
                QGuiApplication::changeOverrideCursor(stack_.last().cursor);
            }
            else {
                // nothing to do if we removed a cursor in the middle of the stack
            }
        }
        else {
            VGC_WARNING(
                LogVgcUi,
                "Attempting to pop cursor index {} which is not in the cursor stack.",
                id);
        }
    }

private:
    struct Item {
        QCursor cursor;
        Int id;
    };
    core::Array<Item> stack_;

    CursorStack() {
    }
};

} // namespace

Int pushCursor(const QCursor& cursor) {
    return CursorStack::instance()->push(cursor);
}

void popCursor(Int id) {
    CursorStack::instance()->pop(id);
}

void CursorChanger::set(const QCursor& cursor) {
    // Note: pushing before popping is more efficient since it may perform
    // a single call to QGuiApplication::changeOverrideCursor(), see the
    // "nothing to do" branch in the implementation of CursorChanger::pop().
    Int oldId = id_;
    id_ = pushCursor(cursor);
    if (oldId != -1) {
        popCursor(oldId);
    }
}

void CursorChanger::clear() {
    if (id_ != -1) {
        popCursor(id_);
        id_ = -1;
    }
}

geometry::Vec2f globalCursorPosition() {
    QPoint globalPos = QCursor::pos();
    return fromQtf(globalPos);
}

void setGlobalCursorPosition(const geometry::Vec2f& position) {
    // TODO: use iround<int>() instead (not implemented as of writing this).
    QCursor::setPos(core::ifloor<int>(position.x()), core::ifloor<int>(position.y()));
}

core::Color colorUnderCursor() {
    if (!qApp) {
        return core::colors::black;
    }
    QPoint globalPos = QCursor::pos();
    QScreen* screen = qApp->screenAt(globalPos);
    if (!screen) {
        return core::colors::black;
    }
    QPoint screenPos = globalPos - screen->geometry().topLeft();
    QPixmap pixmap = screen->grabWindow(0, screenPos.x(), screenPos.y(), 1, 1);
    QImage image = pixmap.toImage();
    QColor qcolor = image.pixelColor(0, 0);
    return fromQt(qcolor);
}

} // namespace vgc::ui
