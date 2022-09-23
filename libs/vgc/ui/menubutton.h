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
#include <vgc/ui/flex.h>
#include <vgc/ui/label.h>
#include <vgc/ui/margins.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ActionButton);
VGC_DECLARE_OBJECT(Menu);
VGC_DECLARE_OBJECT(MenuButton);

/// \class vgc::ui::ActionButton
/// \brief A clickable widget that represents an action and triggers it on click/hover.
///
/// It destroys itself whenever its action gets destroyed.
///
class VGC_UI_API ActionButton : public Flex {
private:
    VGC_OBJECT(ActionButton, Flex)

protected:
    ActionButton(Action* action, FlexDirection layoutDirection);

public:
    /// Creates an MenuButton with the given `action`.
    ///
    static ActionButtonPtr
    create(Action* action, FlexDirection layoutDirection = FlexDirection::Column);

    Action* action() const {
        return action_;
    }

    bool isDisabled() const {
        return action_ ? action_->isDisabled() : true;
    }

    bool isMenu() const {
        return action_ ? action_->isMenu() : false;
    }

    bool click() {
        return tryClick_();
    }

    /// This signal is emitted when the button is clicked and is not disabled.
    ///
    VGC_SIGNAL(clicked);

protected:
    // Reimplementation of Object virtual methods
    void onDestroyed() override;

    // Reimplementation of Widget virtual methods
    void onWidgetRemoved(Widget* child) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    bool onMousePress(MouseEvent* event) override;
    void onResize() override;
    void onStyleChanged() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

    virtual void onClicked();

    Widget* iconWidget() const {
        return iconWidget_;
    }

    Label* textLabel() const {
        return textLabel_;
    }

    Label* shortcutLabel() const {
        return shortcutLabel_;
    }

    void setActive(bool isActive);

private:
    // Components
    Action* action_ = nullptr;
    Widget* iconWidget_ = nullptr;
    Label* textLabel_ = nullptr;
    Label* shortcutLabel_ = nullptr;

    // Style
    bool isActive_ = false;

    // Background
    graphics::GeometryViewPtr triangles_;
    bool reload_ = true;

    // Behavior
    bool tryClick_();
    void onActionChanged_();

    VGC_SLOT(onActionChangedSlot_, onActionChanged_);
    VGC_SLOT(onActionAboutToBeDestroyed_, destroy);
};

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
class VGC_UI_API MenuButton : public ActionButton {
private:
    VGC_OBJECT(MenuButton, ActionButton)
    friend Menu;

protected:
    MenuButton(Action* action, FlexDirection layoutDirection);

public:
    /// Creates an MenuButton with the given `action`.
    ///
    static MenuButtonPtr
    create(Action* action, FlexDirection layoutDirection = FlexDirection::Column);

    MenuDropDirection menuDropDir() const {
        return menuDropDir_;
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
        return isDisabled() ? geometry::Vec2f() : geometry::Vec2f(10.f, 10.f);
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

    Menu* menuPopup() const {
        return menuPopup_;
    };

    bool closeMenuPopup();

    VGC_SIGNAL(menuPopupOpened);
    VGC_SIGNAL(menuPopupClosed, (Action*, triggeredAction));

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
    MenuDropDirection menuDropDir_ = MenuDropDirection::Horizontal;
    Menu* parentMenu_ = nullptr;
    Menu* menuPopup_ = nullptr;

    // The menu calls this when it opens as a popup.
    void onMenuPopupOpened_(Menu* menu);

    void onMenuPopupClosed_(Action* triggeredAction);
    VGC_SLOT(onMenuPopupClosedSlot_, onMenuPopupClosed_);
};

} // namespace vgc::ui

#endif // VGC_UI_MENUBUTTON_H
