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

#ifndef VGC_UI_STANDARDMENUS_H
#define VGC_UI_STANDARDMENUS_H

#include <vgc/ui/api.h>
#include <vgc/ui/module.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Menu);
VGC_DECLARE_OBJECT(StandardMenus);

/// \class vgc::ui::StandardMenus
/// \brief A module to access and modify standard menus (File, Edit, etc.).
///
class VGC_UI_API StandardMenus : public ui::Module {
private:
    VGC_OBJECT(StandardMenus, ui::Module)

protected:
    StandardMenus(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `StandardMenus` module.
    ///
    static StandardMenusSharedPtr create(const ui::ModuleContext& context);

    /// Sets the menu bar where the standard menus are located.
    ///
    /// This should typically only be called once at application startup
    /// by the `Application` object just after creating the "main window".
    ///
    /// Plugins should typically never call this method.
    ///
    void setMenuBar(MenuWeakPtr menuBar);

    /// Returns the menu bar where the standard menus are located.
    ///
    MenuWeakPtr menuBar() const {
        return menuBar_;
    }

    /// Creates the File menu.
    ///
    /// Does nothing if `menuBar()` is null or `fileMenu()` is non-null.
    ///
    void createFileMenu();

    /// Returns the File menu, if any.
    ///
    MenuWeakPtr fileMenu() {
        return fileMenu_;
    }

    /// Returns the existing File menu, if any, otherwise creates it.
    ///
    MenuWeakPtr getOrCreateFileMenu();

    /// Creates the Edit Menu.
    ///
    /// Does nothing if `menuBar()` is null or `editMenu()` is non-null.
    ///
    void createEditMenu();

    /// Returns the Edit menu, if any.
    ///
    MenuWeakPtr editMenu() {
        return editMenu_;
    }

    /// Returns the existing Edit menu, if any, otherwise creates it.
    ///
    MenuWeakPtr getOrCreateEditMenu();

    /// Creates the View Menu.
    ///
    /// Does nothing if `menuBar()` is null or `viewMenu()` is non-null.
    ///
    void createViewMenu();

    /// Returns the View menu, if any.
    ///
    MenuWeakPtr viewMenu() {
        return editMenu_;
    }

    /// Returns the existing View menu, if any, otherwise creates it.
    ///
    MenuWeakPtr getOrCreateViewMenu();

private:
    MenuWeakPtr menuBar_;
    MenuWeakPtr fileMenu_;
    MenuWeakPtr editMenu_;
    MenuWeakPtr viewMenu_;
};

} // namespace vgc::ui

#endif // VGC_UI_STANDARDMENUS_H
