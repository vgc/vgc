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
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Menu);

/// \class vgc::ui::MenuItem
/// \brief Represents one action or separator in a menu.
///
/// A `Menu` contains a list of items, each represented by one instance of
/// this `MenuItem` class.
///
/// The items in a menu can be queried via `Menu::items()`.
///
/// Each item can either be:
///
/// 1. An action that performs an actual operation (e.g., "Edit > Copy").
///    In this case, `isAction()` is `true` and `isMenu()` is `false`.
///
/// 2. An action whose role is to open a sub-menu of this menu.
///    In this case, both `isAction()` and `isMenu()` are `true`.
///
/// 3. A visual separator between actions. In this case, both `isAction()`
///    and `isMenu()` are false, and `isSeparator()` is false.
///
/// For more information on how to add and remove menu items in a menu,
/// see the documentation of `Menu`.
///
class MenuItem {
protected:
    friend Menu;

    constexpr MenuItem() noexcept = default;
    MenuItem(Widget* separator) noexcept;
    MenuItem(Action* action, MenuButton* button) noexcept;
    MenuItem(Action* action, MenuButton* button, Menu* menu) noexcept;

public:
    /// Returns whether this item has a non-null `action()`, whose role could
    /// either be to perform an actual operation or simply open a sub-menu.
    ///
    /// This is equivalent to `action() != nullptr`.
    ///
    /// \sa `isMenu()`, `isSeparator()`, `action()`.
    ///
    bool isAction() const {
        return action_ != nullptr;
    }

    /// Returns whether this item has a non-null `menu()`. If
    /// this is `true`, then `action()` is also non-null.
    ///
    /// This is equivalent to `menu() != nullptr`.
    ///
    /// \sa `isAction()`, `isSeparator()`, `menu()`.
    ///
    bool isMenu() const {
        return menu_ != nullptr;
    }

    /// Returns whether this item is a visual separation between actions.
    ///
    /// This is equivalent to `action() == nullptr`.
    ///
    /// \sa `isAction()`, `isSeparator()`.
    ///
    bool isSeparator() const {
        return action_ == nullptr;
    }

    /// Returns the action, if any, that is represented by this item. The role
    /// of the action could either be to perform an actual operation or simply
    /// open a sub-menu.
    ///
    /// \sa `isAction()`.
    ///
    Action* action() const {
        return action_;
    }

    /// Returns the sub-menu, if any, that is represented by this item.
    ///
    /// If the returned `Menu` is non-null, then the function `action()`
    /// returns a non-null `Action`, whose role is to open the sub-menu.
    ///
    /// \sa `isMenu()`.
    ///
    Menu* menu() const {
        return menu_;
    }

    /// Returns the `MenuButton` widget, if any, that is used to visually show
    /// this `MenuItem` in the widget tree.
    ///
    /// This is typically non-null if `isAction()` is true, but we do not provide
    /// this guarantee and therefore the return value must always be checked. For example,
    /// in future version of the library, we could add support for custom menu widgets,
    /// which a represented differently than a `MenuButton`.
    ///
    MenuButton* button() const {
        if (isAction()) {
            return static_cast<MenuButton*>(widget_);
        }
        else {
            return nullptr;
        }
    }

    /// Returns the `Widget`, if any, that is used to visually show
    /// the `MenuItem` in the widget tree.
    ///
    Widget* widget() const {
        return widget_;
    }

private:
    Action* action_ = nullptr;
    Widget* widget_ = nullptr;
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
    static MenuPtr create(std::string_view title);

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

    /// Adds a separator item to this menu at the given `index`. This can be
    /// used to create visual grouping inside the menu.
    ///
    void addSeparatorAt(Int index);

    /// Adds a separator item at the end of this menu. This can be used to
    /// create visual grouping inside the menu.
    ///
    void addSeparator() {
        addSeparatorAt(numItems());
    }

    /// Adds an action item to this menu at the given `index`.
    ///
    void addItemAt(Int index, Action* action);

    /// Adds an action item at the end of this menu.
    ///
    void addItem(Action* action) {
        addItemAt(numItems(), action);
    }

    /// Adds a menu item to this menu at the given `index`.
    ///
    /// Note that the given `menu` does not become a child widget of this `menu`.
    ///
    void addItemAt(Int index, Menu* menu);

    /// Adds a menu item at the end of this menu.
    ///
    /// Note that the given `menu` does not become a child widget of this `menu`.
    ///
    void addItem(Menu* menu) {
        addItemAt(numItems(), menu);
    }

    /// Creates a new `Menu` with the given `title`, sets it as child widget of
    /// this menu, and adds it as a menu item of this menu at the given `index`.
    ///
    Menu* createSubMenuAt(Int index, std::string_view title);

    /// Creates a new `Menu` with the given `title`, sets it as child widget of
    /// this menu, and adds it as a menu item at the end of this menu.
    ///
    Menu* createSubMenu(std::string_view title) {
        return createSubMenuAt(numItems(), title);
    }

    /// Returns the list of `MenuItem` in this menu.
    ///
    const core::Array<MenuItem>& items() const {
        return items_;
    }

    /// Returns the number of items in this menu.
    ///
    Int numItems() const {
        return items().length();
    }

    /// Removes all the items on this menu.
    ///
    void clearItems();

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

    /// If this menu has an open submenu, then closes this submenu.
    ///
    void closeSubMenu();

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
    void onWidgetRemoved(Widget* widget) override;
    void preMouseMove(MouseMoveEvent* event) override;
    void preMousePress(MousePressEvent* event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onVisible() override;
    void onHidden() override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;

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

    bool openAsPopup_(Widget* from);

    void onClosed() override;
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
