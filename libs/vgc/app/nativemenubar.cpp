// Copyright 2022 The VGC Developers
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

#include <vgc/app/nativemenubar.h>

#include <vgc/app/logcategories.h>
#include <vgc/ui/qtutil.h>

#if VGC_APP_MAY_HAVE_NATIVE_MENU_BAR
#    include <QAction>
#    include <QMenu>
#    include <QMenuBar>
#endif

namespace vgc::app {

NativeMenuBar* NativeMenuBar::nativeMenuBar_ = nullptr;

NativeMenuBar::NativeMenuBar(ui::Menu* menu)
    : menu_(menu) {

    if (nativeMenuBar_) {
        VGC_WARNING(
            LogVgcApp,
            "Instanciating another NativeMenuBar: it will have no effect (there can only "
            "be one native menu bar)");
        return;
    }
    else {
        nativeMenuBar_ = this;
    }

#if VGC_APP_MAY_HAVE_NATIVE_MENU_BAR
    convertToNativeMenuBar_();
#endif
}

NativeMenuBarPtr NativeMenuBar::create(ui::Menu* menu) {
    return NativeMenuBarPtr(new NativeMenuBar(menu));
}

void NativeMenuBar::onDestroyed() {
    if (nativeMenuBar_ == this) {
        nativeMenuBar_ = nullptr;
    }
#if VGC_APP_MAY_HAVE_NATIVE_MENU_BAR
    if (qMenuBar_) {
        delete qMenuBar_;
        qMenuBar_ = nullptr;
    }
#endif
}

#if VGC_APP_MAY_HAVE_NATIVE_MENU_BAR

void NativeMenuBar::onMenuChanged_() {
    core::Object* obj = emitter();
    ui::Menu* menu = dynamic_cast<ui::Menu*>(obj);
    ui::Menu* menuBar = menu_;
    if (!menu) {
        return;
    }
    if (menu == menuBar) {
        qMenuBar_->clear(); // XXX cleanup maps and disconnect before doing this?
        doPopulateNativeMenuBar_(menu, qMenuBar_);
        return;
    }
    auto it = qMenuMap_.find(menu);
    if (it != qMenuMap_.end()) {
        QMenu* qMenu = it->second;
        qMenu->clear(); // XXX cleanup maps and connections before doing this?
        doPopulateNativeMenu_(menu, qMenu);
        return;
    }
}

void NativeMenuBar::onActionChanged_() {
    core::Object* obj = emitter();
    ui::Action* action = dynamic_cast<ui::Action*>(obj);
    if (!action) {
        return;
    }
    auto it = qActionMap_.find(action);
    if (it != qActionMap_.end()) {
        QAction* qAction = it->second;
        updateNativeAction_(action, qAction);
    }
}

void NativeMenuBar::populateNativeMenu_(ui::Menu* menu, QMenu* qMenu) {
    bool alreadyConnected = qMenuMap_.find(menu) != qMenuMap_.end();
    qMenuMap_[menu] = qMenu;
    qMenuMapInv_[qMenu] = menu;
    if (!alreadyConnected) {
        menu->changed().connect(onMenuChangedSlot_());
    }
    QObject::connect(qMenu, &QObject::destroyed, [this](QObject* obj) {
        auto it = this->qMenuMapInv_.find(dynamic_cast<QMenu*>(obj));
        if (it != this->qMenuMapInv_.end()) {
            it->second->disconnect(this);
            this->qMenuMap_.erase(it->second);
            this->qMenuMapInv_.erase(it);
        }
    });
    doPopulateNativeMenu_(menu, qMenu);
}

void NativeMenuBar::doPopulateNativeMenu_(ui::Menu* menu, QMenu* qMenu) {
    // Note: we know that if we're here, the qMenu is empty: we either just
    // created it or cleared it.
    if (!qMenu->isEmpty()) {
        throw vgc::core::LogicError("Attempting to populate a non-empty QMenu.");
    }
    for (const ui::MenuItem& item : menu->items()) {
        if (item.isMenu()) {
            ui::Menu* subMenu = item.menu();
            QMenu* qSubMenu = qMenu->addMenu(ui::toQt(subMenu->title()));
            populateNativeMenu_(subMenu, qSubMenu);
        }
        else if (item.isAction()) {
            ui::ActionPtr action = item.action();
            QAction* qAction = qMenu->addAction(ui::toQt(action->text()));
            bool alreadyConnected = qActionMap_.find(action.get()) != qActionMap_.end();
            qActionMap_[action.get()] = qAction;
            qActionMapInv_[qAction] = action.get();
            if (!alreadyConnected) {
                action->propertiesChanged().connect(onActionChangedSlot_());
                action->enabledChanged().connect(onActionChangedSlot_());
                action->checkStateChanged().connect(onActionChangedSlot_());
                updateNativeAction_(action.get(), qAction);
            }
            QObject::connect(qAction, &QObject::destroyed, [this](QObject* obj) {
                auto it = this->qActionMapInv_.find(dynamic_cast<QAction*>(obj));
                if (it != this->qActionMapInv_.end()) {
                    it->second->disconnect(this);
                    this->qActionMap_.erase(it->second);
                    this->qActionMapInv_.erase(it);
                }
            });
            QObject::connect(
                qAction, &QAction::triggered, [action]() { action->trigger(); });
        }
    }
}

void NativeMenuBar::updateNativeAction_(ui::Action* action, QAction* qAction) {
    QList<QKeySequence> qShortcuts;
    for (const ui::Shortcut& shortcut : action->userShortcuts()) {
        Qt::Key key = static_cast<Qt::Key>(shortcut.key());
        Qt::KeyboardModifiers keyboardModifiers = toQt(shortcut.modifierKeys());
        qShortcuts.append(keyboardModifiers | key);
    }
    qAction->setText(ui::toQt(action->text()));
    qAction->setShortcuts(qShortcuts);
    qAction->setEnabled(action->isEnabled());
    // TODO: update check state and check mode
}

void NativeMenuBar::populateNativeMenuBar_(ui::Menu* menu, QMenuBar* qMenu) {
    menu->changed().connect(onMenuChangedSlot_());
    ui::userShortcuts()->changed().connect(onMenuChangedSlot_());
    doPopulateNativeMenuBar_(menu, qMenu);
}

void NativeMenuBar::doPopulateNativeMenuBar_(ui::Menu* menu, QMenuBar* qMenu) {
    for (const ui::MenuItem& item : menu->items()) {
        if (item.isMenu()) {
            ui::Menu* subMenu = item.menu();
            QMenu* qSubMenu = qMenu->addMenu(ui::toQt(subMenu->title()));
            populateNativeMenu_(subMenu, qSubMenu);
        }
    }
}

void NativeMenuBar::convertToNativeMenuBar_() {

    // Check whether we need a native menubar. Typically, this is always
    // false on Windows, always true on macOS, and on Linux, it depends on
    // the desktop environment. It would be nice to be able to check this
    // without instanciating a QMenuBar, but it doesn't seem possible.
    //
    qMenuBar_ = new QMenuBar();
    if (!qMenuBar_->isNativeMenuBar()) {
        delete qMenuBar_;
        qMenuBar_ = nullptr;
        return;
    }

    menu_->hide();
    populateNativeMenuBar_(menu_, qMenuBar_);
}

#endif // VGC_APP_MAY_HAVE_NATIVE_MENU_BAR

} // namespace vgc::app
