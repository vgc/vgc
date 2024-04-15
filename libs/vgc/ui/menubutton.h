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

#ifndef VGC_UI_MENUBUTTON_H
#define VGC_UI_MENUBUTTON_H

#include <string>
#include <string_view>

#include <vgc/core/innercore.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/richtext.h>
#include <vgc/ui/action.h>
#include <vgc/ui/api.h>
#include <vgc/ui/button.h>
#include <vgc/ui/flex.h>
#include <vgc/ui/iconwidget.h>
#include <vgc/ui/label.h>
#include <vgc/ui/margins.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Menu);
VGC_DECLARE_OBJECT(MenuButton);

/// \enum vgc::ui::MenuDropDirection
/// \brief The direction in which a dropdown menu should appear.
///
enum class MenuDropDirection {
    Horizontal,
    Vertical,
};

/// \class vgc::ui::MenuButton
/// \brief A button with a special layout for Menus.
///
class VGC_UI_API MenuButton : public Button {
private:
    VGC_OBJECT(MenuButton, Button)
    friend Menu;

protected:
    MenuButton(CreateKey, Action* action, FlexDirection layoutDirection);

public:
    /// Creates an MenuButton with the given `action`.
    ///
    static MenuButtonPtr
    create(Action* action, FlexDirection layoutDirection = FlexDirection::Column);

    void setMenuDropDirection(MenuDropDirection direction) {
        menuDropDirection_ = direction;
    }

    MenuDropDirection menuDropDirection() const {
        return menuDropDirection_;
    }

    Menu* parentMenu() const {
        return parentMenu_;
    };

    Menu* popupMenu() const {
        return popupMenu_;
    };

    void closePopupMenu();

    VGC_SIGNAL(menuPopupOpened);
    VGC_SIGNAL(menuPopupClosed, (bool, recursive));

protected:
    void onParentWidgetChanged(Widget* newParent) override;

private:
    MenuDropDirection menuDropDirection_ = MenuDropDirection::Horizontal;
    Menu* parentMenu_ = nullptr;
    Menu* popupMenu_ = nullptr;

    // The menu calls this when it opens as a popup.
    void onMenuPopupOpened_(Menu* menu);

    void onMenuPopupClosed_(bool recursive);
    VGC_SLOT(onMenuPopupClosedSlot_, onMenuPopupClosed_);
};

} // namespace vgc::ui

#endif // VGC_UI_MENUBUTTON_H
