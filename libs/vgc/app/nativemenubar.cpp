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

NativeMenuBar::NativeMenuBar(CreateKey key, ui::Menu* menu)
    : core::Object(key)
    , menu_(menu) {

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
    return core::createObject<NativeMenuBar>(menu);
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
    registerMenuBar_(menu_, qMenuBar_);
    populateMenuBar_(menu_, qMenuBar_);
}

void NativeMenuBar::registerMenuBar_(ui::Menu* menu, QMenuBar* /*qMenu*/) {
    menu->changed().connect(onMenuChangedSlot_());
    ui::userShortcuts()->changed().connect(onShortcutsChangedSlot_());
}

void NativeMenuBar::registerMenu_(ui::Menu* menu, QMenu* qMenu) {

    // Add to the registry.
    auto it = qMenuMap_.find(menu);
    bool alreadyRegistered = it != qMenuMap_.end();
    if (alreadyRegistered) {
        VGC_WARNING(
            LogVgcApp,
            "Attempting to register menu '{}' that is already registered",
            menu->title());
        QMenu* oldQMenu = it->second;
        qMenuMapInv_.erase(oldQMenu);
        delete oldQMenu;
    }
    qMenuMap_[menu] = qMenu;
    qMenuMapInv_[qMenu] = menu;

    // Listen to changes VGC-side.
    if (!alreadyRegistered) {
        menu->aboutToBeDestroyed().connect(onMenuDestroyedSlot_());
        menu->changed().connect(onMenuChangedSlot_());
    }

    // Listen to changes Qt-side.
    // We use qMenuBar_ as context since `this` owns it so is guaranteed to outlive it.
    QObject* context = qMenuBar_;
    QObject::connect(qMenu, &QObject::destroyed, context, [this](QObject* obj) {
        this->onQMenuDestroyed_(obj);
    });
}

void NativeMenuBar::registerAction_(ui::Action* action, QAction* qAction) {

    // Add to the registry.
    auto it = qActionMap_.find(action);
    bool alreadyRegistered = it != qActionMap_.end();
    if (alreadyRegistered) {
        VGC_WARNING(
            LogVgcApp,
            "Re-registering action '{}' that was already registered",
            action->name());
        QAction* oldQAction = it->second;
        qActionMapInv_.erase(oldQAction);
        delete oldQAction;
    }
    qActionMap_[action] = qAction;
    qActionMapInv_[qAction] = action;

    // Listen to changes VGC-side.
    if (!alreadyRegistered) {
        action->aboutToBeDestroyed().connect(onActionDestroyedSlot_());
        action->propertiesChanged().connect(onActionChangedSlot_());
        action->enabledChanged().connect(onActionChangedSlot_());
        action->checkStateChanged().connect(onActionChangedSlot_());
    }

    // Listen to changes Qt-side.
    // We use qMenuBar_ as context since `this` owns it so is guaranteed to outlive it.
    QObject* context = qMenuBar_;
    QObject::connect(qAction, &QObject::destroyed, context, [this](QObject* obj) {
        this->onQActionDestroyed_(obj);
    });
    QObject::connect(qAction, &QAction::triggered, context, [this, qAction]() {
        this->onQActionTriggered_(qAction);
    });
}

void NativeMenuBar::populateMenuBar_(ui::Menu* menu, QMenuBar* qMenuBar) {
    if (!qMenuBar->actions().isEmpty()) {
        VGC_WARNING(LogVgcApp, "Populating a non-empty native menu bar");
    }
    for (const ui::MenuItem& item : menu->items()) {
        if (item.isMenu()) {
            ui::Menu* subMenu = item.menu();
            QMenu* qSubMenu = qMenuBar->addMenu(ui::toQt(subMenu->title()));
            registerMenu_(subMenu, qSubMenu);
            populateMenu_(subMenu, qSubMenu);
        }
    }
}

void NativeMenuBar::populateMenu_(ui::Menu* menu, QMenu* qMenu) {

    if (!qMenu->actions().isEmpty()) {
        VGC_WARNING(LogVgcApp, "Populating a non-empty native menu");
    }
    for (const ui::MenuItem& item : menu->items()) {
        if (item.isMenu()) {
            ui::Menu* subMenu = item.menu();
            QMenu* qSubMenu = qMenu->addMenu(ui::toQt(subMenu->title()));
            registerMenu_(subMenu, qSubMenu);
            populateMenu_(subMenu, qSubMenu);
        }
        else if (item.isAction()) {
            ui::ActionPtr action = item.action();
            QAction* qAction = qMenu->addAction(ui::toQt(action->text()));
            registerAction_(action.get(), qAction);
            updateAction_(action.get(), qAction);
        }
        else if (item.isSeparator()) {
            qMenu->addSeparator();
        }
    }
}

void NativeMenuBar::updateAction_(ui::Action* action, QAction* qAction) {
    qAction->setText(ui::toQt(action->text()));
    qAction->setShortcutContext(Qt::ApplicationShortcut); // XXX required?
    qAction->setEnabled(action->isEnabled());
    updateShortcuts_(action, qAction);
    // TODO: update check state and check mode
}

void NativeMenuBar::updateShortcuts_(ui::Action* action, QAction* qAction) {
    QList<QKeySequence> qShortcuts;
    for (const ui::Shortcut& shortcut : action->userShortcuts()) {
        Qt::Key key = static_cast<Qt::Key>(shortcut.key());
        Qt::KeyboardModifiers keyboardModifiers = toQt(shortcut.modifierKeys());
        qShortcuts.append(keyboardModifiers | key);
    }
    qAction->setShortcuts(qShortcuts);
}

void NativeMenuBar::onMenuDestroyed_(core::Object* obj) {
    ui::Menu* menu = dynamic_cast<ui::Menu*>(obj);
    if (!menu) {
        return;
    }
    auto it = qMenuMap_.find(menu);
    if (it != qMenuMap_.end()) {
        QObject* qMenu = it->second;
        qMenuMap_.erase(it);
        qMenuMapInv_.erase(qMenu);
        delete qMenu;
    }
}

void NativeMenuBar::onActionDestroyed_(core::Object* obj) {
    ui::Action* action = dynamic_cast<ui::Action*>(obj);
    if (!action) {
        return;
    }
    auto it = qActionMap_.find(action);
    if (it != qActionMap_.end()) {
        QAction* qAction = it->second;
        qActionMap_.erase(it);
        qActionMapInv_.erase(qAction);
        delete qAction;
    }
}

namespace {

// We use this function as a replacement for QMenu[Bar]::clear() which leaks:
// it does not cause deletion of its submenus (in both Qt 5 and 6 as of 2023).
//
// See:
//
// - QMenu::addMenu(title) {
//       ...
//       QMenu *menu = new QMenu(title, this); // => the submenu is a child object of the menu
//       addAction(menu->menuAction());        // => this doesn't change ownership
//       ...
//   }
//
//   https://github.com/qt/qtbase/blob/ea25b3962b90154f8c6eba0951ee1c58fe873139/src/widgets/widgets/qmenu.cpp#L1883
//
//   => the submenu is a child object of the menu
//
// - QMenu::QMenu(title, parent) { ... d->init(); ... }
//   QMenuPrivate::init() {
//       ...
//       menuAction = new QAction(q); // => menuAction is a child object of submenu
//       ...
//   }
//
//   https://github.com/qt/qtbase/blob/ea25b3962b90154f8c6eba0951ee1c58fe873139/src/widgets/widgets/qmenu.cpp#L1755
//   https://github.com/qt/qtbase/blob/ea25b3962b90154f8c6eba0951ee1c58fe873139/src/widgets/widgets/qmenu.cpp#L158
//
//
// - QMenu::clear() { // pseudo-code below
//       for (action : actions) {
//           removeAction(action);
//           if (action->parent() == this) { // NOT TRUE for a submenu action:
//               delete action;              // neither the submenu or its menuAction is deleted
//           }
//       }
//   }
//
//   https://github.com/qt/qtbase/blob/ea25b3962b90154f8c6eba0951ee1c58fe873139/src/widgets/widgets/qmenu.cpp#L2218
//
void clearQMenu(QWidget* menu) {
    QList<QAction*> actions = menu->actions();
    for (QAction* action : actions) {
        menu->removeAction(action);
        if (action->menu()) {
            clearQMenu(action->menu());
            delete action->menu();
        }
        else if (action->parent() == menu) {
            delete action;
        }
    }
}

} // namespace

void NativeMenuBar::onMenuChanged_() {
    core::Object* obj = emitter();
    ui::Menu* menu = dynamic_cast<ui::Menu*>(obj);
    if (!menu) {
        return;
    }

    ui::Menu* menuBar = menu_;
    if (menu == menuBar) {
        clearQMenu(qMenuBar_);
        populateMenuBar_(menu, qMenuBar_);
        return;
    }

    auto it = qMenuMap_.find(menu);
    if (it != qMenuMap_.end()) {
        QMenu* qMenu = it->second;
        clearQMenu(qMenu);
        populateMenu_(menu, qMenu);
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
        updateAction_(action, qAction);
    }
}

void NativeMenuBar::onShortcutsChanged_() {
    for (auto pair : qActionMap_) {
        updateShortcuts_(pair.first, pair.second);
    }
}

// Note: when QObject::destroyed() is emitted, we are already in the destructor
// of the QObject, so we cannot static_cast or dynamic_cast to QAction or
// QMenu. This is why we store QObject pointers in the maps, rather than
// pointers to the subclasses.

void NativeMenuBar::onQMenuDestroyed_(QObject* obj) {
    auto it = qMenuMapInv_.find(obj);
    if (it != qMenuMapInv_.end()) {
        ui::Menu* menu = it->second;
        qMenuMap_.erase(menu);
        qMenuMapInv_.erase(it);
        menu->disconnect(this);
    }
}

void NativeMenuBar::onQActionDestroyed_(QObject* obj) {
    auto it = qActionMapInv_.find(obj);
    if (it != qActionMapInv_.end()) {
        ui::Action* action = it->second;
        qActionMap_.erase(action);
        qActionMapInv_.erase(it);
        action->disconnect(this);
    }
}

void NativeMenuBar::onQActionTriggered_(QAction* qAction) {
    auto it = qActionMapInv_.find(qAction);
    if (it != qActionMapInv_.end()) {
        ui::Action* action = it->second;
        action->trigger();
    }
    else {
        VGC_WARNING(
            LogVgcApp,
            "Native menu bar action '{}' triggered without being registered.",
            qAction->text().toStdString());
    }
}

#endif // VGC_APP_MAY_HAVE_NATIVE_MENU_BAR

} // namespace vgc::app
