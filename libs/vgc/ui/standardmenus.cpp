// Copyright 2023 The VGC Developers
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

#include <vgc/ui/standardmenus.h>

#include <vgc/ui/menu.h>

namespace vgc::ui {

StandardMenus::StandardMenus(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {
}

StandardMenusSharedPtr StandardMenus::create(const ui::ModuleContext& context) {
    return core::createObject<StandardMenus>(context);
}

namespace {

void transferMenu(const MenuWeakPtr& menu_, Menu& newMenuBar) {
    if (auto menu = menu_.lock()) {
        newMenuBar.addChild(menu.get()); // Add the menu itself as a (hidden) child widget
        newMenuBar.addItem(menu.get());  // Add the "open submenu" action as a menu item
        // TODO: specify where to insert it
    }
}

} // namespace

void StandardMenus::setMenuBar(MenuWeakPtr menuBar) {
    if (auto newMenuBar = menuBar.lock()) {
        transferMenu(fileMenu_, *newMenuBar);
        transferMenu(editMenu_, *newMenuBar);
    }
    menuBar_ = menuBar;
}

namespace {

void createMenu(MenuWeakPtr& menu, const MenuWeakPtr& menuBar, std::string_view name) {
    if (auto alreadyExists = menu.lock()) {
        return;
    }
    else if (auto lMenuBar = menuBar.lock()) {
        menu = lMenuBar->createSubMenu(name);
    }
}

} // namespace

void StandardMenus::createFileMenu() {
    createMenu(fileMenu_, menuBar_, "File");
}

MenuWeakPtr StandardMenus::getOrCreateFileMenu() {
    createFileMenu();
    return fileMenu();
}

void StandardMenus::createEditMenu() {
    createMenu(editMenu_, menuBar_, "Edit");
}

MenuWeakPtr StandardMenus::getOrCreateEditMenu() {
    createEditMenu();
    return editMenu();
}

void StandardMenus::createViewMenu() {
    createMenu(fileMenu_, menuBar_, "View");
}

MenuWeakPtr StandardMenus::getOrCreateViewMenu() {
    createViewMenu();
    return viewMenu();
}

} // namespace vgc::ui
