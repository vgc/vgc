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
    Menu(const std::string& title);

public:
    /// Creates a Menu.
    ///
    static MenuPtr create();

    /// Creates a Menu with the given title.
    ///
    static MenuPtr create(const std::string& title = "");

    Action* menuAction() const {
        return action_;
    }

    void setTitle(const std::string& title);

    void addSeparator();

    void addItem(Action* action);

    /// Appends the given `menu` to the menu items.
    ///
    /// `menu` is not reparented.
    ///
    void addItem(Menu* menu);

    /// Creates a child `Menu` with the given `title` and appends it to the menu items.
    ///
    Menu* createSubMenu(const std::string& title);

    bool isOpen() const {
        return isPopupEnabled_ ? isOpenAsPopup_ : isVisible();
    }

    bool isOpenAsPopup() const {
        return isOpenAsPopup_;
    }

    /// Opens the menu either in-place with `show()` if `isPopupEnabled()`
    /// is false or as a popup in a popup-layer under the top-most overlay of `from`.
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

    bool close();
    bool closeSubMenu();

    bool isPopupEnabled() const {
        return isPopupEnabled_;
    }

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
    void setShortcutTrackEnabled(bool enabled) {
        if (isShortcutTrackEnabled_ != enabled) {
            isShortcutTrackEnabled_ = enabled;
            requestGeometryUpdate();
        }
    }

    VGC_SIGNAL(changed);
    VGC_SIGNAL(popupOpened);
    VGC_SIGNAL(popupClosed, (Action*, triggeredAction));

protected:
    // Reimplementation of Widget virtual methods
    void onParentWidgetChanged(Widget* newParent) override;
    void onWidgetRemoved(Widget* widget) override;
    void preMouseMove(MouseEvent* event) override;
    void preMousePress(MouseEvent* event);
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    void onVisible() override;
    void onHidden() override;
    void onResize() override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    //

    // Returns a position relative to area.
    // XXX make it virtual and return a global position when possible.
    geometry::Vec2f computePopupPosition(Widget* opener, Widget* area);

    Menu* subMenuPopup() const {
        return subMenuPopup_;
    }

    void notifyChanged(bool geometryChanged);
    void removeItem(Widget* widget);

    void closeLayer();

private:
    Action* action_;
    core::Array<MenuItem> items_;
    //MenuItem* lastHovered_ = nullptr;

    void onItemAdded_(const MenuItem& item);
    void preItemRemoved_(const MenuItem& item);

    // Background
    graphics::GeometryViewPtr triangles_;

    // Style
    mutable float iconTrackSize_ = -1.f;
    mutable float shortcutTrackSize_ = -1.f;
    mutable Margins padding_ = {};

    void setupWidthOverrides_() const;

    // Behavior popin/popup
    Widget* host_ = nullptr;
    PopupLayerPtr popupLayer_ = nullptr;
    Menu* subMenuPopup_ = nullptr;
    bool isDeferringOpen_ = true;
    geometry::Vec2f lastHoverPos_ = {};
    geometry::Rect2f subMenuPopupRect_ = {};

    bool openAsPopup_(Widget* from);
    bool close_(Action* triggeredAction);

    void onLayerCatch_();
    VGC_SLOT(onLayerCatchSlot_, onLayerCatch_);

    void onSelfActionTriggered_(Widget* from);
    VGC_SLOT(onSelfActionTriggeredSlot_, onSelfActionTriggered_);

    void onItemActionTriggered_(Widget* from);
    VGC_SLOT(onItemActionTriggeredSlot_, onItemActionTriggered_);

    void onSubMenuPopupClosed_(Action* triggeredAction);
    VGC_SLOT(onSubMenuPopupClosedSlot_, onSubMenuPopupClosed_);

    void onSubMenuPopupDestroy_();
    VGC_SLOT(onSubMenuPopupDestroySlot_, onSubMenuPopupDestroy_);

    // Flags
    mutable bool isIconTrackEnabled_ = true;
    mutable bool isShortcutTrackEnabled_ = true;
    bool isPopupEnabled_ = true;
    bool isOpenAsPopup_ = false;
    bool reload_ = true;
    bool isOpeningSubMenuOnHover_ = false;
    //float subMenuOpenDelay_ = 0.f;
};

} // namespace vgc::ui

#endif // VGC_UI_MENU_H
