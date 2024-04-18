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

#include <vgc/ui/menu.h>

#include <vgc/core/array.h>
#include <vgc/style/strings.h>
#include <vgc/ui/dropdownbutton.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

namespace {

/*
FlexDirection orthoDir(FlexDirection dir) {
    switch (dir) {
    case FlexDirection::Row:
    case FlexDirection::RowReverse:
        return FlexDirection::Column;
    case FlexDirection::Column:
    case FlexDirection::ColumnReverse:
    default:
        return FlexDirection::Row;
    }
}
*/

namespace commands {

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    open,
    "ui.menu.open",
    "Open Menu")

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    exit,
    "ui.menu.exit",
    "Exit Menu",
    Shortcut(Key::Escape));

} // namespace commands

} // namespace

MenuItem::MenuItem(Widget* widget) noexcept
    : widget_(widget) {
}

MenuItem::MenuItem(Action* action, Button* button) noexcept
    : action_(action)
    , widget_(button) {
}

MenuItem::MenuItem(Action* action, Button* button, Menu* menu) noexcept
    : action_(action)
    , widget_(button)
    , menu_(menu) {
}

Menu::Menu(CreateKey key, std::string_view title)
    : Flex(key, FlexDirection::Column, FlexWrap::NoWrap) {

    addStyleClass(strings::Menu);
    setFocusStrength(FocusStrength::Low);

    action_ = createTriggerAction(commands::open(), title);
    action_->isMenu_ = true;
    action_->triggered().connect(onSelfActionTriggeredSlot_());

    Action* exitAction = createTriggerAction(commands::exit());
    exitAction->triggered().connect(exitSlot_());
}

MenuPtr Menu::create() {
    return core::createObject<Menu>("");
}

MenuPtr Menu::create(std::string_view text) {
    return core::createObject<Menu>(text);
}

void Menu::setTitle(std::string_view title) {
    action_->setText(title);
    notifyChanged(false);
}

void Menu::addSeparatorAt(Int index) {
    Widget* separator = createChildAt<Widget>(index);
    separator->addStyleClass(strings::separator);
    items_.insert(index, MenuItem(separator));
    notifyChanged(true);
}

void Menu::addItemAt(Int index, Action* action) {
    Button* button = createChildAt<Button>(index, action);
    button->addStyleClass(strings::button);
    button->setTooltipEnabled(false);
    items_.insert(index, MenuItem(action, button));
    onItemAdded_(items_[index]);
    notifyChanged(true);
}

void Menu::addItemAt(Int index, Menu* menu) {
    Action* action = menu->menuAction();
    DropdownButton* button = createChildAt<DropdownButton>(index, action);
    button->addStyleClass(strings::button);
    button->setTooltipEnabled(false);
    items_.insert(index, MenuItem(action, button, menu));
    onItemAdded_(items_[index]);
    notifyChanged(true);
}

Menu* Menu::createSubMenuAt(Int index, std::string_view title) {
    MenuSharedPtr menu = Menu::create(title);
    addItemAt(index, menu.get());
    return menu.get();
}

void Menu::clearItems() {
    while (firstChild()) {
        firstChild()->destroy();
    }
    items_.clear();
    notifyChanged(true);
}

bool Menu::isOpenAsPopup() const {
    return dynamic_cast<OverlayArea*>(parent());
}

void Menu::open(Widget* from) {
    if (!parent() && !isOpenAsPopup()) {
        if (openAsPopup_(from)) {
            setFocus(FocusReason::Menu);
            popupOpened().emit();
        }
    }
}

void Menu::closeSubMenu() {
    if (Menu* subMenu = subMenuPopup()) {
        subMenu->close();
    }
}

void Menu::setShortcutTrackEnabled(bool enabled) {
    if (isShortcutTrackEnabled_ != enabled) {
        isShortcutTrackEnabled_ = enabled;
        for (const MenuItem& item : items_) {
            Button* button = item.button();
            if (button) {
                button->setShortcutVisible(enabled);
            }
        }
        requestGeometryUpdate();
    }
}

namespace {

// crossOffsets[0]: Offset to apply to the cross position if placed after
// crossOffsets[1]: Offset to apply to the cross position if placed before
//
// These are used to perfectly align the first item of a submenu with the item
// of the parent menu that opened the submenu, by taking into account padding
// and border of the menu.
//
// Example: DropDirection = Horizontal
//
// If placed "after" in the cross dir (i.e., top-aligned with the anchor):
//
//           main dir
// o----------------------------->
// |
// |                  +----------+ ^ crossOffsets[0] (negative in this case)
// |    +------------+|          | ^
// |    |   anchor   ||   drop   |
// |    +------------+|          |
// |                  |          |
// | cross dir        |          |
// V                  +----------+
//
// If placed "before" in the cross dir (i.e., bottom-aligned with the anchor):
//
//           main dir
// o----------------------------->
// |
// |                  +----------+
// |                  |          |
// |                  |          |
// |    +------------+|          |
// |    |   anchor   ||   drop   |
// |    +------------+|          | v
// |                  +----------+ v crossOffsets[1] (positive in this case)
// | cross dir
// V
//
void placeMenuFit(
    geometry::Rect2f& menuRect,
    const geometry::Vec2f& crossOffsets,
    const geometry::Rect2f& areaRect,
    const geometry::Rect2f& anchorRect,
    int dropDir) {

    const geometry::Vec2f menuSize = menuRect.size();
    const int crossDir = dropDir ? 0 : 1;

    const geometry::Vec2f areaMin = areaRect.pMin();
    const geometry::Vec2f areaMax = areaRect.pMax();
    const geometry::Vec2f anchorMin = anchorRect.pMin();
    const geometry::Vec2f anchorMax = anchorRect.pMax();
    geometry::Vec2f resultPos;

    // Determine whether to place to menu "after" or "before"
    // in the main direction.
    //
    if (anchorMax[dropDir] + menuSize[dropDir] <= areaMax[dropDir]) {
        // Enough space after, so place it after
        resultPos[dropDir] = anchorMax[dropDir];
    }
    else {
        // Place either after or before, whichever has more space
        float spaceAfter = areaMax[dropDir] - anchorMax[dropDir];
        float spaceBefore = anchorMin[dropDir] - areaMin[dropDir];
        if (spaceAfter >= spaceBefore) {
            resultPos[dropDir] = anchorMax[dropDir];
        }
        else {
            resultPos[dropDir] = anchorMin[dropDir] - menuSize[dropDir];
        }
    }

    // Determine whether to place to menu "after" (min-aligned) or "before"
    // (max-aligned) in the cross direction.
    //
    const float areaCrossMin = areaMin[crossDir];
    const float areaCrossMax = areaMax[crossDir];
    const float crossSize = menuSize[crossDir];
    const float minIfAfter = anchorMin[crossDir] + crossOffsets[0];
    const float maxIfAfter = minIfAfter + crossSize;
    const float maxIfBefore = anchorMax[crossDir] + crossOffsets[1];
    const float minIfBefore = maxIfBefore - crossSize;

    if (minIfAfter >= areaCrossMin && maxIfAfter <= areaCrossMax) {
        // Enough space after, so place it after
        resultPos[crossDir] = minIfAfter;
    }
    else if (minIfBefore >= areaCrossMin && maxIfBefore <= areaCrossMax) {
        // Enough space before, so place it before
        resultPos[crossDir] = minIfBefore;
    }
    else {
        const float areaCrossSize = areaCrossMax - areaCrossMin;
        if (crossSize < areaCrossSize) {
            // Enough total space: align the menu with the area border
            // that was otherwise cropping the menu
            if (minIfAfter < areaCrossMin) {
                resultPos[crossDir] = areaCrossMin;
            }
            else {
                resultPos[crossDir] = areaCrossMax - crossSize;
            }
        }
        else {
            // Not enough space to fit the menu in the area.
            // So we min-align it (i.e., prefer cropping the "end" of the menu).
            resultPos[crossDir] = areaCrossMin;
        }
    }

    menuRect.setPosition(resultPos);
}

DropDirection getDropDirection(Menu* parentMenu, DropdownButton* button) {
    if (parentMenu) {
        if (parentMenu->isRow()) {
            return DropDirection::Vertical;
        }
        else {
            return DropDirection::Horizontal;
        }
    }
    else if (button) {
        return button->dropDirection();
    }
    else {
        return DropDirection::Horizontal;
    }
}

Menu* getMenuFromItem(Widget* item) {
    return item ? dynamic_cast<Menu*>(item->parent()) : nullptr;
}

} // namespace

geometry::Vec2f Menu::computePopupPosition(Widget* opener, Widget* area) {

    DropdownButton* button = dynamic_cast<DropdownButton*>(opener);
    Menu* parentMenu = getMenuFromItem(opener);

    DropDirection dropDir = getDropDirection(parentMenu, button);
    int dropDirIndex = (dropDir == DropDirection::Horizontal) ? 0 : 1;

    geometry::Rect2f areaRect = area->rect();
    geometry::Rect2f anchorRect = opener->mapTo(area, opener->rect());

    geometry::Rect2f menuRect(geometry::Vec2f(), preferredSize());

    // Ensures that anchorRect is a subset of areaRect. Note that this is not
    // the same as computing the intersection between anchorRect and areaRect
    // in case where the intersection is empty.
    //
    anchorRect = areaRect.clamp(anchorRect);

    Margins paddingAndBorder = padding() + border();
    geometry::Vec2f crossOffsets;
    if (dropDir == DropDirection::Horizontal) {
        crossOffsets = {-paddingAndBorder.top(), paddingAndBorder.bottom()};
    }
    else {
        crossOffsets = {-paddingAndBorder.left(), paddingAndBorder.right()};
    }

    placeMenuFit(menuRect, crossOffsets, areaRect, anchorRect, dropDirIndex);

    return menuRect.position();
}

void Menu::notifyChanged(bool geometryChanged) {
    if (geometryChanged) {
        requestGeometryUpdate();
    }
    changed().emit();
}

void Menu::removeItem(Widget* widget) {
    bool changed_ = false;
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        const MenuItem& item = *it;
        if (item.widget() == widget) {
            changed_ = true;
            preItemRemoved_(item);
            items_.erase(it);
            break;
        }
    }
    if (changed_) {
        changed().emit();
    }
}

void Menu::onItemAdded_(const MenuItem& item) {
    Button* button = item.button();
    if (button) {
        //button->setDirection(orthoDir(direction()));
        button->setDirection(FlexDirection::Row);
        button->setShortcutVisible(isShortcutTrackEnabled_);
    }
    item.action()->triggered().connect(onItemActionTriggeredSlot_());
    item.action()->aboutToBeDestroyed().connect(onItemActionAboutToBeDestroyedSlot_());
}

void Menu::preItemRemoved_(const MenuItem& item) {
    Button* button = item.button();
    DropdownButton* dropdownButton = dynamic_cast<DropdownButton*>(button);
    if (dropdownButton) {
        dropdownButton->closePopupMenu();
    }
    Action* action = item.action();
    if (action) {
        action->triggered().disconnect(onItemActionTriggeredSlot_());
    }
}

void Menu::setupWidthOverrides_() const {

    // This logic is currently disabled because we removed the ability of now-deleted
    // MenuButton class to override children sizes (it did not implement all style
    // rules and made styling difficult). We still keep the code below as
    // comments in case we re-implement it later more generically, for example
    // by adding the ability to setup size overrides to any widget. Or with
    // or more advance grid styling ability to align multiple buttons, etc.
    //
    //    const core::Array<MenuItem>& items = items_;
    //    if (isShortcutTrackEnabled_) {
    //        float maxShortcutWidth = 0.f;
    //        for (const MenuItem& item : items) {
    //            MenuButton* button = item.button();
    //            if (button) {
    //                maxShortcutWidth =
    //                    (std::max)(maxShortcutWidth, button->preferredShortcutSize().x());
    //            }
    //        }
    //        for (const MenuItem& item : items) {
    //            MenuButton* button = item.button();
    //            if (button) {
    //                button->setShortcutSizeOverrides(maxShortcutWidth, -1.0f);
    //            }
    //        }
    //    }
}

bool Menu::openAsPopup_(Widget* from) {

    DropdownButton* button = dynamic_cast<DropdownButton*>(from);
    Menu* parentMenu = getMenuFromItem(from);

    // Find the OverlayArea where to place the popup.
    //
    OverlayArea* area = nullptr;
    if (parentMenu && parentMenu->isOpenAsPopup()) {
        area = dynamic_cast<OverlayArea*>(parentMenu->parent());
    }
    if (!area) {
        area = from ? from->topmostOverlayArea() : topmostOverlayArea();
    }
    if (!area) {
        VGC_WARNING(
            LogVgcUi,
            "Menu couldn't be opened as a popup because the initiator widget has no "
            "top-most overlay area.")
        return false;
    }

    // Place the popup in the overlay area.
    //
    // Note: we need to add the menu as overlay before computing its preferred
    // size and position, since these may depend on style attributes, which
    // depend on the location of the menu in the widget tree.
    //
    area->addWeakModalOverlay(this);
    if (parentMenu && !parentMenu->isOpenAsPopup()) {
        area->addPassthrough(this, parentMenu);
    }
    geometry::Vec2f pos(0, 0);
    geometry::Vec2f size = preferredSize();
    updateGeometry(pos, size);
    pos = computePopupPosition(from, area);
    updateGeometry(pos, size);

    // Let the button know.
    if (button) {
        button->onMenuPopupOpened_(this);
    }

    return true;
}

void Menu::onClosed() {
    close_();
}

void Menu::close_(bool recursivelyCloseParentPopupMenus) {

    // Remove focus if any. This must be done first while
    // the menu is still in the widget tree.
    clearFocus(FocusReason::Menu);

    // Recursively close all submenus.
    closeSubMenu();

    if (isOpenAsPopup()) {
        // Close this menu
        MenuSharedPtr keepAlive = this;
        reparent(nullptr);

        // Emit signal, and (maybe) recursively close parent popup menus.
        popupClosed().emit(recursivelyCloseParentPopupMenus);
    }
    else {
        hide();
    }
}

void Menu::exit_() {
    if (isOpenAsPopup()) {
        bool recursivelyCloseParentPopupMenus = true;
        close_(recursivelyCloseParentPopupMenus);
    }
    else {
        clearFocus(FocusReason::Menu);
        closeSubMenu();
    }
}

void Menu::onSelfActionTriggered_(Widget* from) {

    Menu* parentMenu = getMenuFromItem(from);

    if (parentMenu && !parentMenu->isOpenAsPopup() && isOpenAsPopup()) {
        parentMenu->exit_();
        // Example:
        // - Clicking on the 'Menubar > File' button when the 'File' menu
        //   is already open exits the 'Menubar' (clears focus + closes 'File' menu).
    }
    else {
        open(from);
        // Examples:
        // - Clicking on the 'Menubar > File' button when the 'File' menu
        //   is not already open opens the 'File' menu.
        // - Clicking on the 'Menubar > File > More' button when the 'More' menu
        //   is not already open opens the 'More' menu.
        // - Clicking on the 'Menubar > File > More' button when the 'More' menu
        //   is already open keeps the 'More' menu open.
        // - Clicking on a ComboBox (no parent menu) opens its menu
    }
}

void Menu::onItemActionTriggered_(Widget* from) {

    // Detect whether the triggered action opened a new popup menu
    // or was any other type of action.
    //
    Menu* newPopup = nullptr;
    for (const MenuItem& item : items_) {
        DropdownButton* dropdownButton = dynamic_cast<DropdownButton*>(item.button());
        if (item.widget() == from) {
            if (dropdownButton) {
                newPopup = dropdownButton->popupMenu();
            }
        }
        else if (dropdownButton) {
            // close other menus
            dropdownButton->closePopupMenu();
        }
    }

    if (newPopup) {
        // If a new popup menu was opened, then we register it as our
        // subMenuPopup().
        onSubMenuPopupOpened_(newPopup);
    }
    else {
        // Otherwise, this means that an actual action has been perfomed,
        // so if this menu was open as a popup, we can now close it
        // as well as all its parent popup menu.
        exit_();
    }
}

void Menu::onSubMenuPopupOpened_(Menu* subMenu) {

    // Register sub-menu.
    //
    subMenu->popupClosed().connect(onSubMenuPopupClosedSlot_());
    subMenu->aboutToBeDestroyed().connect(onSubMenuPopupDestroySlot_());
    subMenuPopup_ = subMenu;
    subMenuPopupHitRect_ = subMenu->mapTo(this, subMenu->rect());

    // Add margins to popup hit rect when applicable (no overlap with our buttons).
    //
    geometry::Rect2f itemsRect = contentRect();
    const float hitMargin = 5.f;
    Margins hitMargins = {};
    if (subMenuPopupHitRect_.xMin() >= itemsRect.xMax()
        || subMenuPopupHitRect_.xMax() <= itemsRect.xMin()) {
        hitMargins.setTop(hitMargin);
        hitMargins.setBottom(hitMargin);
    }
    if (subMenuPopupHitRect_.yMin() >= itemsRect.yMax()
        || subMenuPopupHitRect_.yMax() <= itemsRect.yMin()) {
        hitMargins.setRight(hitMargin);
        hitMargins.setLeft(hitMargin);
    }
    subMenuPopupHitRect_ = subMenuPopupHitRect_ + hitMargins;
}

void Menu::onSubMenuPopupClosed_(bool recursivelyCloseParentPopupMenus) {
    if (subMenuPopup_ == emitter()) {
        subMenuPopup_->popupClosed().disconnect(onSubMenuPopupClosedSlot_());
        subMenuPopup_->aboutToBeDestroyed().disconnect(onSubMenuPopupDestroySlot_());
        subMenuPopup_ = nullptr;
        if (recursivelyCloseParentPopupMenus && isOpenAsPopup()) {
            close_(recursivelyCloseParentPopupMenus);
        }
    }
}

void Menu::onSubMenuPopupDestroy_() {
    if (subMenuPopup_ == emitter()) {
        subMenuPopup_ = nullptr;
    }
}

void Menu::onItemActionAboutToBeDestroyed_() {
    Action* action = static_cast<Action*>(emitter());
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        MenuItem& item = *it;
        if (item.action_ == action) {
            item.action_ = nullptr;
        }
    }
}

// Reimplementation of Widget virtual methods

void Menu::onWidgetRemoved(Widget* widget) {
    removeItem(widget);
}

void Menu::preMouseMove(MouseMoveEvent* event) {
    Widget* hcc = hoverChainChild();
    if (hcc && hcc->parent() != this) {
        return;
    }

    geometry::Vec2f newHoverPos = event->position();
    if (isFirstMoveSinceEnter_) {
        // Hover discontinuity (leave/enter).
        lastHoverPos_ = newHoverPos;
    }

    geometry::Vec2f delta = newHoverPos - lastHoverPos_;
    float sqDist = delta.squaredLength();
    bool moved = sqDist > 15.f; // ~4 pixels

    const bool hasOpenSubmenuPopup = subMenuPopup_;

    // A menu is either docked (menu bar) or popup (dropdown).
    // A drop-down menu always opens sub-menus on hover.
    // A menu bar opens its sub-menus on hover only if one is already open.
    const bool isOpenAsPopup = this->isOpenAsPopup();
    const bool shouldOpenSubmenuOnHover = isOpenAsPopup || hasOpenSubmenuPopup;
    const bool shouldProtectOpenSubMenu = isOpenAsPopup;

    Button* button = dynamic_cast<Button*>(hcc);
    Action* action = button ? button->action() : nullptr;
    const bool isHccMenu = action && action->isMenu_ && action->isEnabled();

    bool doNothing = false;
    if (shouldProtectOpenSubMenu && subMenuPopup_ && !isFirstMoveSinceEnter_) {
        if (!moved) {
            // Move was too small, let's keep the sub-menu open.
            doNothing = true;
        }
        else {
            geometry::Vec2f origin = lastHoverPos_;
            geometry::Vec2f dir = newHoverPos - origin;
            dir.normalize();
            // Assumes pos is out of the popup rect.
            int hitPlaneDim = -1;
            float hitDist = 0.f;
            for (int i = 0; i < 2; ++i) {
                float c = dir[i];
                // Check parallelism.
                if (c == 0.f) {
                    continue;
                }
                float o = origin[i];
                // Find candidate plane.
                float plane = subMenuPopupHitRect_.pMin()[i];
                if (newHoverPos[i] > plane) {
                    plane = subMenuPopupHitRect_.pMax()[i];
                    if (newHoverPos[i] < plane) {
                        continue;
                    }
                }
                // Calculate dist.
                float d = (plane - o) / c;
                if (d > hitDist) {
                    hitPlaneDim = i;
                    hitDist = d;
                }
            }
            if (hitPlaneDim >= 0) {
                int hitCrossDim = hitPlaneDim ? 0 : 1;
                float vHit = origin[hitCrossDim] + hitDist * dir[hitCrossDim];
                geometry::Vec2f hitBounds = geometry::Vec2f(
                    subMenuPopupHitRect_.pMin()[hitCrossDim],
                    subMenuPopupHitRect_.pMax()[hitCrossDim]);
                // If hit, let's do nothing on hover.
                if (vHit >= hitBounds[0] && vHit <= hitBounds[1]) {
                    doNothing = true;
                }
            }
        }
    }

    if (hcc && !doNothing) {
        // We have no pointer to our current active button atm.
        // But we can check if the hovered button's open popup
        // menu is our open sub-menu.
        DropdownButton* dropdownButton = dynamic_cast<DropdownButton*>(button);
        Menu* hccMenuPopup = dropdownButton ? dropdownButton->popupMenu() : nullptr;
        if (!subMenuPopup_ || subMenuPopup_ != hccMenuPopup) {
            if (isOpenAsPopup || isHccMenu) {
                closeSubMenu();
            }
            if (isHccMenu && shouldOpenSubmenuOnHover) {
                button->click(newHoverPos);
                // Update move origin now.
                lastHoverPos_ = newHoverPos;
            }
        }
    }

    if (moved) {
        lastHoverPos_ = newHoverPos;
    }
    isFirstMoveSinceEnter_ = false;
}

bool Menu::onMousePress(MousePressEvent*) {
    // Clicking on empty space of a docked menu (e.g., the menubar) that
    // has a submenu opened should clear focus and close the submenu.
    if (!isOpenAsPopup() && subMenuPopup_) {
        exit_();
        return true;
    }
    return false;
}

void Menu::onMouseEnter() {
    isFirstMoveSinceEnter_ = true;
}

void Menu::onMouseLeave() {
}

void Menu::onVisible() {
}

void Menu::onHidden() {
    closeSubMenu();
}

geometry::Vec2f Menu::computePreferredSize() const {
    setupWidthOverrides_();
    geometry::Vec2f ret = Flex::computePreferredSize();
    return ret;
}

void Menu::updateChildrenGeometry() {
    setupWidthOverrides_();
    Flex::updateChildrenGeometry();
}

} // namespace vgc::ui
