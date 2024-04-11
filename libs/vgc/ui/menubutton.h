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

    MenuDropDirection menuDropDirection() const {
        return menuDropDirection_;
    }

    geometry::Vec2f preferredIconSize() const {
        return iconWidget()->preferredSize();
    }

    geometry::Vec2f preferredTextSize() const {
        return textLabel()->preferredSize();
    }

    geometry::Vec2f preferredShortcutSize() const {
        return shortcutLabel()->preferredSize();
    }

    geometry::Vec2f preferredArrowSize() const {
        // XXX todo
        return isActionEnabled() ? geometry::Vec2f(10.f, 10.f) : geometry::Vec2f();
    }

    /// Returns the icon size overrides.
    ///
    const geometry::Vec2f& iconSizeOverrides() const {
        return iconSizeOverrides_;
    }

    /// Sets the icon size overrides.
    /// A component value < 0.f means it is automatic (uses preferred-size).
    /// Any component value at 0.f makes the icon invisible.
    ///
    /// No geometry update request is made.
    ///
    void setIconSizeOverrides(float x, float y) {
        iconSizeOverrides_ = geometry::Vec2f(x, y);
    }

    /// Returns the shortcut size overrides.
    ///
    const geometry::Vec2f& shortcutSizeOverrides() const {
        return shortcutSizeOverrides_;
    }

    /// Sets the shortcut size overrides.
    /// A component value < 0.f means it is automatic (uses preferred-size).
    /// Any component value at 0.f makes the shortcut invisible.
    ///
    /// No geometry update request is made.
    ///
    void setShortcutSizeOverrides(float x, float y) {
        shortcutSizeOverrides_ = geometry::Vec2f(x, y);
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
    // Reimplementation of Widget virtual methods.
    void onParentWidgetChanged(Widget* newParent) override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;

private:
    // Style
    geometry::Vec2f iconSizeOverrides_ = {-1.f, -1.f};
    geometry::Vec2f textSizeOverrides_ = {-1.f, -1.f};
    geometry::Vec2f shortcutSizeOverrides_ = {-1.f, -1.f};
    float arrowSizeOverride_ = 0.f;
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
