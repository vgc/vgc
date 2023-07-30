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

#ifndef VGC_APP_NATIVEMENUBAR_H
#define VGC_APP_NATIVEMENUBAR_H

#include <vgc/app/api.h>
#include <vgc/core/object.h>
#include <vgc/core/os.h>
#include <vgc/ui/menu.h>

#ifdef VGC_CORE_OS_WINDOWS
#    define VGC_APP_MAY_HAVE_NATIVE_MENU_BAR 0
#else
#    define VGC_APP_MAY_HAVE_NATIVE_MENU_BAR 1
#endif

#if VGC_APP_MAY_HAVE_NATIVE_MENU_BAR
class QAction;
class QMenu;
class QMenuBar;
#endif

namespace vgc::app {

VGC_DECLARE_OBJECT(NativeMenuBar);

/// \class vgc::app::NativeMenuBar
/// \brief Creates a native menu bar in sync with a given `Menu`
///
/// Some operating systems, for example macOS, use a shared menu bar for all
/// applications, instead of having each application show a menu bar on top of
/// their window. We call such shared menu bar a "native menu bar".
///
/// Instanciating a `NativeMenuBar` allows you to specify which `Menu` should
/// be used to populate the native menu bar, whenever supported and/or
/// recommended by the operating system. The `NativeMenuBar` stores all
/// information and performs all the logic required to keep in sync the native
/// menu bar with the given `Menu`.
///
/// Whenever a native menu bar is populated, the given `Menu` is automatically
/// hidden, but is kept as part of the widget tree.
///
/// Since there can only be one native menu bar per application, instaciating
/// several `NativeMenuBar` will issue a warning, and only the first instance
/// will have an effect.
///
class VGC_APP_API NativeMenuBar : public core::Object {
    VGC_OBJECT(NativeMenuBar, core::Object)

protected:
    NativeMenuBar(CreateKey, ui::Menu* menu);
    void onDestroyed() override;

public:
    /// Creates a `NativeMenuBar`.
    ///
    static NativeMenuBarPtr create(ui::Menu* menu);

    /// Returns the `ui::Menu` used as reference to populate the native menu bar.
    ///
    ui::Menu* menu() const {
        return menu_;
    }

private:
    static NativeMenuBar* nativeMenuBar_;
    ui::Menu* menu_;

#if VGC_APP_MAY_HAVE_NATIVE_MENU_BAR
    QMenuBar* qMenuBar_ = nullptr;
    std::unordered_map<ui::Menu*, QMenu*> qMenuMap_;
    std::unordered_map<QMenu*, ui::Menu*> qMenuMapInv_;
    std::unordered_map<ui::Action*, QAction*> qActionMap_;
    std::unordered_map<QAction*, ui::Action*> qActionMapInv_;
    void onMenuChanged_();
    void onActionChanged_();
    VGC_SLOT(onMenuChangedSlot_, onMenuChanged_);
    VGC_SLOT(onActionChangedSlot_, onActionChanged_);
    void populateNativeMenu_(ui::Menu* menu, QMenu* qMenu);
    void doPopulateNativeMenu_(ui::Menu* menu, QMenu* qMenu);
    void updateNativeAction_(ui::Action* action, QAction* qAction);
    void populateNativeMenuBar_(ui::Menu* menu, QMenuBar* qMenu);
    void doPopulateNativeMenuBar_(ui::Menu* menu, QMenuBar* qMenu);
    void convertToNativeMenuBar_();
#endif
};

} // namespace vgc::app

#endif // VGC_APP_NATIVEMENUBAR_H
