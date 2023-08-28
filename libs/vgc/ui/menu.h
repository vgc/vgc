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

#ifndef VGC_UI_MENU_H
#define VGC_UI_MENU_H

#include <variant>

#include <vgc/core/object.h>
#include <vgc/graphics/richtext.h>
#include <vgc/ui/action.h>
#include <vgc/ui/api.h>
#include <vgc/ui/flex.h>
#include <vgc/ui/menubutton.h>
#include <vgc/ui/popuplayer.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Menu);
VGC_DECLARE_OBJECT(MenuButton);
VGC_DECLARE_OBJECT(MenuLayout);

/// \class vgc::ui::MenuItem
/// \brief A menu item.
///
class MenuItem {
protected:
    friend Menu;

    constexpr MenuItem() noexcept = default;
    MenuItem(Action* action, MenuButton* button) noexcept;
    MenuItem(Action* action, MenuButton* button, Menu* menu) noexcept;

public:
    bool isAction() const {
        return !!action_;
    }

    bool isMenu() const {
        return menu_;
    }

    bool isSeparator() const {
        return !action_;
    }

    Action* action() const {
        return action_;
    }

    Menu* menu() const {
        return menu_;
    }

    MenuButton* button() const {
        return button_;
    }

    Widget* widget() const {
        return button_;
    }

private:
    Action* action_ = nullptr;
    MenuButton* button_ = nullptr;
    Menu* menu_ = nullptr;

    // could add custom widget
};

/// \class vgc::ui::Menu
/// \brief A menu widget.
///
class VGC_UI_API Menu : public Flex {
private:
    VGC_OBJECT(Menu, Flex)

protected:
    /// This is an implementation details. Please use
    /// Menu::create(title) instead.
    ///
    Menu(CreateKey, std::string_view title);

public:
    /// Creates a `Menu`.
    ///
    static MenuPtr create();

    /// Creates a `Menu` with the given title.
    ///
    static MenuPtr create(std::string_view = "");

    /// Returns the action corresponding to this menu. This is an action that,
    /// when triggered, opens the menu.
    ///
    Action* menuAction() const {
        return action_;
    }

    /// Returns the title of this menu. This title is for example visible in
    /// the parent menu when this menu is a submenu of another menu.
    ///
    ///  This is equivalent to `menuAction()->text()`.
    ///
    std::string_view title() const {
        return action_->text();
    }

    /// Set the title of this menu.
    ///
    void setTitle(std::string_view title);

    /// Adds a separator item to this menu. This can be used to create visual
    /// grouping inside the menu.
    ///
    void addSeparator();

    /// Adds an action item to this menu.
    ///
    void addItem(Action* action);

    /// Adds a menu item to this menu.
    ///
    /// Note that the given `menu` does not become a child widget of this `menu`.
    ///
    void addItem(Menu* menu);

    /// Creates a new `Menu` with the given `title`, set it as child widget of
    /// this menu, and add it as a menu item to this menu.
    ///
    Menu* createSubMenu(std::string_view title);

    /// Returns the list of `MenuItem` in this menu.
    ///
    const core::Array<MenuItem>& items() const {
        return items_;
    }

    /// Returns whether this menu is open. A menu is considered open either
    /// when it is open as popup (see `isOpenAsPopup()`), or when it is not
    /// open as popup but visible anyway, as any regular widget can be (see
    /// `isVisible()`).
    ///
    bool isOpen() const {
        return isPopupEnabled_ ? isOpenAsPopup_ : isVisible();
    }

    /// Returns whether this menu is open as a popup.
    ///
    bool isOpenAsPopup() const {
        return isOpenAsPopup_;
    }

    /// Opens the menu either in-place with `show()` if `isPopupEnabled()` is
    /// false or as a popup in a popup-layer under the top-most overlay of
    /// `from`.
    ///
    /// Returns true on success, false otherwise.
    ///
    /// If `from` is null, the top-most overlay of the menu itself is used.
    ///
    /// Opening an already open menu triggers both `closed()` then `opened()`.
    ///
    /// If it is opened as a popup, it is aligned for a MenuButton but centered for
    /// any other widget. The behavior is customizable via the overridable
    /// `computePopupPosition()`.
    ///
    bool open(Widget* from);

    /// Closes this menu. This is the reverse operation of `open()`.
    ///
    bool close();

    /// If this menu has an open submenu, then closes this submenu.
    ///
    bool closeSubMenu();

    /// Returns whether this menu can be open as a popup.
    ///
    bool isPopupEnabled() const {
        return isPopupEnabled_;
    }

    /// Sets whether this menu can be open as a popup.
    ///
    void setPopupEnabled(bool enabled);

    // temporary, will be a style prop
    // Sets the icon track fixed size.
    // Any value < 0.f makes the size automatic (uses max preferred-size of shortcuts).
    //
    void setIconTrackSize(float size) {
        if (iconTrackSize_ != size) {
            iconTrackSize_ = size;
            requestGeometryUpdate();
        }
    }

    // temporary, will be a style prop
    bool isIconTrackEnabled() const {
        return isIconTrackEnabled_;
    }

    // temporary, will be a style prop
    void setIconTrackEnabled(bool enabled) {
        if (isIconTrackEnabled_ != enabled) {
            isIconTrackEnabled_ = enabled;
            requestGeometryUpdate();
        }
    }

    // temporary, will be a style prop
    // Sets the shortcut track fixed size.
    // Any value < 0.f makes the size automatic (uses max preferred-size of shortcuts).
    //
    void setShortcutTrackSize(float size) {
        if (shortcutTrackSize_ != size) {
            shortcutTrackSize_ = size;
            requestGeometryUpdate();
        }
    }

    // temporary, will be a style prop
    bool isShortcutTrackEnabled() const {
        return isShortcutTrackEnabled_;
    }

    // temporary, will be a style prop
    void setShortcutTrackEnabled(bool enabled);

    VGC_SIGNAL(changed);
    VGC_SIGNAL(popupOpened);
    VGC_SIGNAL(popupClosed, (bool, recursive));

protected:
    // Reimplementation of Widget virtual methods
    void onParentWidgetChanged(Widget* newParent) override;
    void onWidgetRemoved(Widget* widget) override;
    void preMouseMove(MouseMoveEvent* event) override;
    void preMousePress(MousePressEvent* event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onVisible() override;
    void onHidden() override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;
    //

    // Returns a position relative to area.
    // XXX make it virtual and return a global position when possible.
    geometry::Vec2f computePopupPosition(Widget* opener, Widget* area);

    Menu* subMenuPopup() const {
        return subMenuPopup_;
    }

    void notifyChanged(bool geometryChanged);
    void removeItem(Widget* widget);

private:
    Action* action_;
    core::Array<MenuItem> items_;
    //MenuItem* lastHovered_ = nullptr;

    void onItemAdded_(const MenuItem& item);
    void preItemRemoved_(const MenuItem& item);

    // Style
    mutable float iconTrackSize_ = -1.f;
    mutable float shortcutTrackSize_ = -1.f;
    mutable Margins padding_ = {};

    void setupWidthOverrides_() const;

    // Behavior popin/popup
    Widget* host_ = nullptr;
    Menu* subMenuPopup_ = nullptr;
    bool isDeferringOpen_ = true;
    geometry::Vec2f lastHoverPos_ = {};
    bool isFirstMoveSinceEnter_ = true;
    geometry::Rect2f subMenuPopupHitRect_ = {};

    // Handling of PopupLayer
    PopupLayerPtr popupLayer_ = nullptr;
    void createPopupLayer_(OverlayArea* area, Widget* underlyingWidget = nullptr);
    void destroyPopupLayer_();

    bool openAsPopup_(Widget* from);

    bool close_(bool recursive = false);

    void exit_();
    VGC_SLOT(exitSlot_, exit_);

    void onSelfActionTriggered_(Widget* from);
    VGC_SLOT(onSelfActionTriggeredSlot_, onSelfActionTriggered_);

    void onItemActionTriggered_(Widget* from);
    VGC_SLOT(onItemActionTriggeredSlot_, onItemActionTriggered_);

    void onSubMenuPopupOpened_(Menu* subMenu);

    void onSubMenuPopupClosed_(bool recursive);
    VGC_SLOT(onSubMenuPopupClosedSlot_, onSubMenuPopupClosed_);

    void onSubMenuPopupDestroy_();
    VGC_SLOT(onSubMenuPopupDestroySlot_, onSubMenuPopupDestroy_);

    void onItemActionAboutToBeDestroyed_();
    VGC_SLOT(onItemActionAboutToBeDestroyedSlot_, onItemActionAboutToBeDestroyed_);

    // Flags
    mutable bool isIconTrackEnabled_ = true;
    mutable bool isShortcutTrackEnabled_ = true;
    bool isPopupEnabled_ = true;
    bool isOpenAsPopup_ = false;
    //float subMenuOpenDelay_ = 0.f;
};

} // namespace vgc::ui

#endif // VGC_UI_MENU_H
