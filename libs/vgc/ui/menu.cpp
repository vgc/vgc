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
#include <vgc/graphics/strings.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/menubutton.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/popuplayer.h>
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

} // namespace

MenuItem::MenuItem(Action* action, MenuButton* button) noexcept
    : action_(action)
    , button_(button) {
}

MenuItem::MenuItem(Action* action, MenuButton* button, Menu* menu) noexcept
    : action_(action)
    , button_(button)
    , menu_(menu) {
}

Menu::Menu(const std::string& title)
    : Flex(FlexDirection::Column, FlexWrap::NoWrap) {

    addStyleClass(strings::Menu);
    action_ = createAction(title);
    action_->isMenu_ = true;
    action_->triggered().connect(onSelfActionTriggeredSlot_());
}

MenuPtr Menu::create() {
    return MenuPtr(new Menu(""));
}

MenuPtr Menu::create(const std::string& text) {
    return MenuPtr(new Menu(text));
}

void Menu::setTitle(const std::string& title) {
    action_->setText(title);
    notifyChanged(false);
}

void Menu::addSeparator() {
    items_.emplaceLast(MenuItem()); // XXX todo: separators
    notifyChanged(true);
}

void Menu::addItem(Action* action) {
    MenuButton* menuButton = createChild<MenuButton>(action);
    items_.emplaceLast(MenuItem(action, menuButton));
    onItemAdded_(items_.last());
    notifyChanged(true);
}

void Menu::addItem(Menu* menu) {
    Action* action = menu->menuAction();
    MenuButton* menuButton = createChild<MenuButton>(action);
    items_.emplaceLast(MenuItem(action, menuButton, menu));
    onItemAdded_(items_.last());
    notifyChanged(true);
}

Menu* Menu::createSubMenu(const std::string& title) {
    Menu* menu = createChild<Menu>(title);
    menu->hide();
    addItem(menu);
    return menu;
}

bool Menu::open(Widget* from) {
    close_(nullptr);
    if (isPopupEnabled_) {
        Widget* host = parent();
        if (!openAsPopup_(from)) {
            return false;
        }
        host_ = host;
        isOpenAsPopup_ = true;
        show();
        popupOpened().emit();
    }
    else if (!isVisible()) {
        show();
    }
    return true;
}

bool Menu::close() {
    return close_(nullptr);
}

bool Menu::closeSubMenu() {
    Menu* subMenu = subMenuPopup();
    if (subMenu) {
        return subMenu->close();
    }
    return false;
}

void Menu::setPopupEnabled(bool enabled) {
    if (isPopupEnabled_ == enabled) {
        return;
    }
    close_(nullptr);
    isPopupEnabled_ = enabled;
}

namespace detail {

bool placeMenuFit(
    geometry::Rect2f& menuRect,
    const geometry::Vec2f& fixedCrossAlignShifts,
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

    if (anchorMax[dropDir] + menuSize[dropDir] <= areaMax[dropDir]) {
        // Enough space on the Right/Bottom
        resultPos[dropDir] = anchorMax[dropDir];
    }
    else if (anchorMin[dropDir] - menuSize[dropDir] >= areaMin[dropDir]) {
        // Enough space on the Left/Top
        resultPos[dropDir] = anchorMin[dropDir] - menuSize[dropDir];
    }
    else {
        return false;
    }

    float delta = menuSize[crossDir];
    if (anchorMin[crossDir] + delta + fixedCrossAlignShifts[1] <= areaMax[crossDir]) {
        // Enough space Bottom/Right
        resultPos[crossDir] = anchorMin[crossDir] + fixedCrossAlignShifts[1];
    }
    else if (
        anchorMax[crossDir] - delta - fixedCrossAlignShifts[0] >= areaMin[crossDir]) {
        // Enough space Top/Left
        resultPos[crossDir] = anchorMax[crossDir] - delta - fixedCrossAlignShifts[0];
    }
    else {
        return false;
    }

    menuRect.setPosition(resultPos);
    return true;
}

} // namespace detail

geometry::Vec2f Menu::computePopupPosition(Widget* opener, Widget* area) {

    MenuButton* button = dynamic_cast<MenuButton*>(opener);
    Menu* parentMenu = button ? button->parentMenu() : nullptr;

    MenuDropDirection dropDir =
        button ? button->menuDropDir() : MenuDropDirection::Horizontal;
    if (parentMenu) {
        switch (parentMenu->direction()) {
        case FlexDirection::Row:
        case FlexDirection::RowReverse:
            dropDir = MenuDropDirection::Vertical;
            break;
        case FlexDirection::Column:
        case FlexDirection::ColumnReverse:
            dropDir = MenuDropDirection::Horizontal;
            break;
        default:
            break;
        }
    }

    geometry::Rect2f anchorRect = opener->mapTo(area, opener->rect());
    geometry::Rect2f areaRect = area->rect();
    geometry::Rect2f menuRect(geometry::Vec2f(), preferredSize());

    const bool preferLeftRight = (dropDir == MenuDropDirection::Horizontal);
    const int sideDir = preferLeftRight ? 0 : 1;
    const int crossDir = preferLeftRight ? 1 : 0;

    std::array<geometry::Vec2f, 2> fixedShifts = {
        geometry::Vec2f{-padding_.bottom(), -padding_.top()},
        geometry::Vec2f{-padding_.right(), -padding_.left()}};

    float crossShift = anchorRect.size()[sideDir] * 0.2f;
    fixedShifts[crossDir] = geometry::Vec2f(crossShift, crossShift);

    if (!detail::placeMenuFit(
            menuRect, fixedShifts[sideDir], areaRect, anchorRect, sideDir)) {
        if (!detail::placeMenuFit(
                menuRect, fixedShifts[crossDir], areaRect, anchorRect, crossDir)) {
            // doesn't fit at aligned locations, fallback
            menuRect.setPosition(anchorRect.pMax());
            menuRect.setWidth((std::min)(menuRect.width(), areaRect.width()));
            menuRect.setHeight((std::min)(menuRect.height(), areaRect.height()));
            geometry::Vec2f newPos = menuRect.position();
            if (menuRect.pMax().x() > areaRect.pMax().x()) {
                newPos[0] = areaRect.pMax().x() - menuRect.width();
            }
            else if (menuRect.pMin().x() < areaRect.pMin().x()) {
                newPos[0] = areaRect.pMin().x();
            }
            if (menuRect.pMax().y() > areaRect.pMax().y()) {
                newPos[1] = areaRect.pMax().y() - menuRect.width();
            }
            else if (menuRect.pMin().y() < areaRect.pMin().y()) {
                newPos[1] = areaRect.pMin().y();
            }
            menuRect.setPosition(newPos);
        }
    }
    return menuRect.position();

    //geometry::Vec2f pos = opener->mapTo(area, opener->size());
    //return pos;
}

void Menu::notifyChanged(bool geometryChanged) {
    if (geometryChanged) {
        requestGeometryUpdate();
    }
    changed().emit();
}

void Menu::removeItem(Widget* widget) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        const MenuItem& item = *it;
        if (item.widget() == widget) {
            preItemRemoved_(item);
            items_.erase(it);
            break;
        }
    }
}

void Menu::closeLayer() {
    if (popupLayer_) {
        popupLayer_->destroy();
    }
}

//bool Menu::close_(bool cascade) {
//    closeSubMenu();
//    bool wasVisible = isVisible();
//    if (isPopupEnabled_) {
//        if (popinParent_) {
//            reparent(popinParent_);
//        }
//        popupLayer_ = nullptr;
//        isOpenAsPopup_ = false;
//    }
//    else if (cascade) {
//        return false;
//    }
//
//    hide();
//    if (wasVisible) {
//        closed().emit(cascade);
//        return true;
//    }
//
//    return false;
//}
//
//void Menu::onActionDeath_(Object* action) {
//    items_.removeIf([=](const MenuItem& item) { return item.action() == action; });
//}

//void Menu::onButtonClicked_(MenuButton* button) {
//    if (button->openedPopupMenu()) {
//        // Close other popups
//        for (auto& item : items_) {
//            MenuButton* menuButton = item.menuButton();
//            if (menuButton == button) {
//                continue;
//            }
//            // XXX move in MenuButton directly ?
//            menuButton->closeMenu();
//        }
//    }
//    else {
//        close_(true);
//    }
//}
//
//void Menu::onPopupClosed_(bool triggered) {
//    if (triggered) {
//        if (isPopupEnabled_) {
//            close_(true);
//        }
//    }
//}

void Menu::onItemAdded_(const MenuItem& item) {
    MenuButton* button = item.button();
    if (button) {
        button->parentMenu_ = this;
        //button->setDirection(orthoDir(direction()));
        button->setDirection(FlexDirection::Row);
    }
    item.action()->triggered().connect(onItemActionTriggeredSlot_());
}

void Menu::preItemRemoved_(const MenuItem& item) {
    MenuButton* button = item.button();
    if (button) {
        button->closeMenuPopup();
        button->parentMenu_ = nullptr;
    }
    item.action()->triggered().disconnect(onItemActionTriggeredSlot_());
}

void Menu::setupWidthOverrides_() const {
    const core::Array<MenuItem>& items = items_;
    if (isShortcutTrackEnabled_) {
        float maxShortcutWidth = 0.f;
        for (const MenuItem& item : items) {
            MenuButton* button = item.button();
            if (button) {
                maxShortcutWidth =
                    (std::max)(maxShortcutWidth, button->preferredShortcutSize().x());
            }
        }
        for (const MenuItem& item : items) {
            MenuButton* button = item.button();
            if (button) {
                button->setShortcutSizeOverrides(maxShortcutWidth, -1.0f);
            }
        }
    }
}

bool Menu::openAsPopup_(Widget* from) {

    MenuButton* button = dynamic_cast<MenuButton*>(from);
    Menu* parentMenu = button ? button->parentMenu() : nullptr;

    // XXX until window popup are implemented we use an OverlayArea.
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

    if (!parentMenu) {
        // Let's protect everything under the popup.
        PopupLayer* popupLayer =
            area->createOverlayWidget<PopupLayer>(OverlayResizePolicy::Stretch, nullptr);
        popupLayer_ = popupLayer;
        popupLayer->resized().connect(onLayerCatchSlot_());
    }

    geometry::Vec2f pos = computePopupPosition(from, area);
    area->addOverlayWidget(this);
    updateGeometry(pos, preferredSize());

    // Let the button know.
    if (button) {
        button->onMenuPopupOpened_(this);
    }

    return true;
}

bool Menu::close_(Action* triggeredAction) {
    if (isPopupEnabled_) {
        if (!isOpenAsPopup_) {
            return false;
        }
        if (host_) {
            reparent(host_);
        }
        host_ = nullptr;
        isOpenAsPopup_ = false;
        hide();
        popupClosed().emit(triggeredAction);
    }
    else if (isVisible()) {
        hide();
    }
    return true;
}

void Menu::onLayerCatch_() {
    closeSubMenu();
    closeLayer();
}

void Menu::onSelfActionTriggered_(Widget* from) {
    if (isPopupEnabled_) {
        open(from);
    }
    else if (isVisible()) {
        close_(nullptr);
    }
    else {
        open(nullptr);
    }
}

void Menu::onItemActionTriggered_(Widget* from) {
    Menu* newPopup = nullptr;
    for (const MenuItem& item : items_) {
        MenuButton* button = item.button();
        if (item.widget() == from) {
            if (button) {
                newPopup = button->menuPopup();
            }
        }
        else if (button) {
            // close other menus
            button->closeMenuPopup();
        }
    }
    if (newPopup) {
        newPopup->popupClosed().connect(onSubMenuPopupClosedSlot_());
        newPopup->aboutToBeDestroyed().connect(onSubMenuPopupDestroySlot_());
        subMenuPopup_ = newPopup;
        subMenuPopupRect_ = newPopup->mapTo(this, newPopup->rect());
        isDeferringOpen_ = false;
        if (!isOpenAsPopup_) {
            if (!popupLayer_) {
                OverlayArea* area = dynamic_cast<OverlayArea*>(newPopup->parent());
                if (!area) {
                    area = topmostOverlayArea();
                }
                if (area) {
                    // Let's protect everything under the popup except this.
                    PopupLayer* popupLayer = area->createOverlayWidget<PopupLayer>(
                        OverlayResizePolicy::Stretch, this);
                    popupLayer_ = popupLayer;
                    popupLayer->resized().connect(onLayerCatchSlot_());
                    popupLayer->backgroundPressed().connect(onLayerCatchSlot_());
                    // Move it over our layer
                    area->addOverlayWidget(newPopup);
                }
            }
        }
    }
    else if (isOpenAsPopup_) {
        close_(static_cast<Action*>(emitter()));
    }
}

void Menu::onSubMenuPopupClosed_(Action* triggeredAction) {
    if (subMenuPopup_ == emitter()) {
        subMenuPopup_->popupClosed().disconnect(onSubMenuPopupClosedSlot_());
        subMenuPopup_->aboutToBeDestroyed().disconnect(onSubMenuPopupDestroySlot_());
        subMenuPopup_ = nullptr;
        // Cascaded close.
        if (triggeredAction && isOpenAsPopup_) {
            close_(triggeredAction);
        }
    }
}

void Menu::onSubMenuPopupDestroy_() {
    if (subMenuPopup_ == emitter()) {
        subMenuPopup_ = nullptr;
    }
}

// Reimplementation of Widget virtual methods

void Menu::onParentWidgetChanged(Widget* newParent) {
    popupLayer_ = dynamic_cast<PopupLayer*>(newParent);
}

void Menu::onWidgetRemoved(Widget* widget) {
    removeItem(widget);
}

void Menu::preMouseMove(MouseEvent* event) {
    Widget* hcc = hoverChainChild();
    if (hcc && hcc->parent() != this) {
        return;
    }

    geometry::Vec2f newHoverPos = event->position();
    geometry::Vec2f delta = newHoverPos - lastHoverPos_;
    float sqDist = delta.squaredLength();
    const bool moved = sqDist > 15.f; // ~4 pixels
    const bool hasOpenSubmenuPopup = subMenuPopup_;

    // A menu is either docked (menu bar) or popup (dropdown).
    // A drop-down menu always opens sub-menus on hover.
    // A menu bar opens its sub-menus on hover only if one is already open.
    const bool shouldOpenSubmenuOnHover = isOpenAsPopup_ || hasOpenSubmenuPopup;

    MenuButton* button = dynamic_cast<MenuButton*>(hcc);
    const bool isHccMenu = button && button->isMenu();

    //if (subMenuPopup_) {
    //    if (moved) {
    //        geometry::Vec2f origin = lastHoverPos_;
    //        geometry::Vec2f dir = newHoverPos - origin;
    //        dir.normalize();
    //        // Assumes pos is out of the popup rect.
    //        geometry::Vec2f planes = {};
    //        geometry::Vec2f hits = {-1.f, -1.f};
    //        int hitPlaneDim = -1;
    //        float hitDist = 0.f;
    //        for (int i = 0; i < 2; ++i) {
    //            float c = dir[i];
    //            // Check parallelism.
    //            if (c == 0.f) {
    //                continue;
    //            }
    //            float o = origin[i];
    //            // Find candidate plane.
    //            float plane = subMenuPopupRect_.pMin()[i];
    //            if (newHoverPos[i] > plane) {
    //                plane = subMenuPopupRect_.pMax()[i];
    //                if (newHoverPos[i] < plane) {
    //                    continue;
    //                }
    //            }
    //            // Calculate dist.
    //            float d = (plane - o) / c;
    //            if (d > hitDist) {
    //                hitPlaneDim = i;
    //                hitDist = d;
    //            }
    //        }
    //        if (hitPlaneDim >= 0) {
    //            const float margin = 50.f;
    //            int hitCrossDim = hitPlaneDim ? 0 : 1;
    //            float vHit = origin[hitCrossDim] + hitDist * dir[hitCrossDim];
    //            geometry::Vec2f hitBounds = geometry::Vec2f(
    //                subMenuPopupRect_.pMin()[hitCrossDim] - margin,
    //                subMenuPopupRect_.pMax()[hitCrossDim] + margin
    //            );
    //            if (vHit < hitBounds[0] || vHit > hitBounds[1]) {
    //                // not directed to open popup -> trigger current
    //                if (isMenu) {
    //                    button->click();
    //                }
    //                else {
    //                    closeSubMenu();
    //                }
    //            }
    //        } else {
    //            // Probably an error since we are inside the popup rect.
    //        }
    //    }
    //}
    //else if (!isDeferringOpen_) {
    //    if (isMenu) {
    //        button->click();
    //    }
    //}

    if (hcc) {
        if (subMenuPopup_ != hcc) {
            closeSubMenu();
            if (isHccMenu && shouldOpenSubmenuOnHover) {
                button->click();
            }
        }
    }

    if (moved) {
        lastHoverPos_ = newHoverPos;
    }
}

void Menu::preMousePress(MouseEvent* event) {
    // Close everything if we click on an openable menu in a docked menu that
    // is already active.
    if (!isOpenAsPopup_ && subMenuPopup_) {
        Widget* hcc = hoverChainChild();
        if (hcc && hcc->parent() != this) {
            return;
        }
        MenuButton* button = dynamic_cast<MenuButton*>(hcc);
        if (button && button->isMenu()) {
            onLayerCatch_();
            event->stopPropagation();
        }
    }
}

bool Menu::onMouseEnter() {
    return false;
}

bool Menu::onMouseLeave() {
    return false;
}

void Menu::onVisible() {
}

void Menu::onHidden() {
    // Only this makes sense.
    closeSubMenu();
    isDeferringOpen_ = true;
    if (popupLayer_) {
        popupLayer_->destroy();
        popupLayer_ = nullptr;
    }
}

void Menu::onResize() {
    reload_ = true;
}

geometry::Vec2f Menu::computePreferredSize() const {
    setupWidthOverrides_();
    geometry::Vec2f ret = Flex::computePreferredSize();

    const float padL = detail::getLength(this, graphics::strings::padding_left);
    const float padR = detail::getLength(this, graphics::strings::padding_right);
    const float padT = detail::getLength(this, graphics::strings::padding_top);
    const float padB = detail::getLength(this, graphics::strings::padding_bottom);
    padding_ = Margins(padT, padR, padB, padL);

    return ret;
}

void Menu::updateChildrenGeometry() {
    setupWidthOverrides_();
    Flex::updateChildrenGeometry();
}

void Menu::onPaintCreate(graphics::Engine* engine) {
    triangles_ =
        engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void Menu::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

    namespace gs = graphics::strings;

    if (reload_) {
        reload_ = false;
        core::FloatArray a;

        // Draw background
        core::Color backgroundColor = detail::getColor(this, gs::background_color);

        if (backgroundColor.a() > 0) {
            style::BorderRadiuses borderRadiuses = detail::getBorderRadiuses(this);
            detail::insertRect(a, backgroundColor, rect(), borderRadiuses);
        }

        // Load triangles data
        engine->updateVertexBufferData(triangles_, std::move(a));
    }

    // Background if needed
    if (isPopupEnabled_) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(triangles_, -1, 0);
    }

    SuperClass::onPaintDraw(engine, options);
}

void Menu::onPaintDestroy(graphics::Engine*) {
    triangles_.reset();
}

} // namespace vgc::ui
