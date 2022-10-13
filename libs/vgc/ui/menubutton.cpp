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

#include <vgc/ui/menubutton.h>

#include <vgc/graphics/strings.h>
#include <vgc/ui/detail/paintutil.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/preferredsizecalculator.h>
#include <vgc/ui/strings.h>

namespace vgc::ui {

namespace {

using Length = double;

float hintSpacing(float spacing) {
    if (spacing <= 0) {
        return 0;
    }
    return (std::max)(1.f, (std::round)(spacing));
}

float getSpacing(const Widget* w, core::StringId id, bool hint) {
    style::StyleValue spacing = w->style(id);
    if (spacing.has<Length>()) {
        float value = static_cast<float>(spacing.to<Length>());
        return hint ? hintSpacing(value) : value;
    }
    return 0;
}

} // namespace

namespace {

void applySizeOverrides(geometry::Vec2f& size, const geometry::Vec2f& overrides) {
    if (overrides.x() >= 0) {
        size.setX(overrides.x());
    }
    if (overrides.y() >= 0) {
        size.setY(overrides.y());
    }
}

float allocSize(float requested, float& remaining) {
    float ret = std::min(requested, remaining);
    remaining -= ret;
    return ret;
}

} // namespace

MenuButton::MenuButton(Action* action, FlexDirection layoutDirection)
    : Button(action, layoutDirection) {

    addStyleClass(strings::MenuButton);
    setShortcutVisible(true);
}

MenuButtonPtr MenuButton::create(Action* action, FlexDirection layoutDirection) {
    return MenuButtonPtr(new MenuButton(action, layoutDirection));
}

bool MenuButton::closePopupMenu() {
    Menu* menu = popupMenu();
    return menu && menu->close();
}

void MenuButton::onMenuPopupOpened_(Menu* menu) {
    if (popupMenu_ == menu) {
        return;
    }
    if (popupMenu_) {
        closePopupMenu();
    }
    popupMenu_ = menu;
    popupMenu_->popupClosed().connect(onMenuPopupClosedSlot_());
    setActive(true);
    menuPopupOpened().emit();
}

void MenuButton::onMenuPopupClosed_(Action* triggeredAction) {
    setActive(false);
    popupMenu_->popupClosed().disconnect(onMenuPopupClosedSlot_());
    popupMenu_ = nullptr;
    menuPopupClosed().emit(triggeredAction);
}

// Reimplementation of Widget virtual methods

void MenuButton::onParentWidgetChanged(Widget* newParent) {
    SuperClass::onParentWidgetChanged(newParent);
    parentMenu_ = dynamic_cast<Menu*>(newParent);
}

namespace {

class Calculator {
public:
    Calculator(const MenuButton* button)
        : calc_(button)
        , mainDir_(button->mainDirectionIndex())
        , crossDir_(1 - mainDir_) {
    }

    void addTrackIfVisible(Widget* item, const geometry::Vec2f& overrides) {
        if (item->visibility() != Visibility::Invisible) {
            geometry::Vec2f size = item->preferredSize();
            applySizeOverrides(size, overrides);
            addTrack(size);
        }
    }

    void addTrackIfPositive(float value) {
        if (value > 0.f) {
            addTrack({value, value});
        }
    }

    void addTrack(const geometry::Vec2f& size) {
        calc_.addTo(mainDir_, size[mainDir_]);
        maxCrossDir_ = std::max(maxCrossDir_, size[crossDir_]);
        ++numTracks_;
    }

    geometry::Vec2f compute() {
        calc_.addTo(crossDir_, maxCrossDir_);
        if (numTracks_ > 0) {
            core::StringId gap = (mainDir_ == 0) ? strings::column_gap : strings::row_gap;
            calc_.addTo(mainDir_, gap, numTracks_ - 1);
        }
        return calc_.compute();
    }

private:
    PreferredSizeCalculator calc_;
    Int mainDir_;
    Int crossDir_;
    Int numTracks_ = 0;
    float maxCrossDir_ = 0.f;
};

} // namespace

geometry::Vec2f MenuButton::computePreferredSize() const {
    // we must return preferred size without the layouting overrides
    Calculator calc(this);
    calc.addTrackIfVisible(iconWidget(), iconSizeOverrides_);
    calc.addTrackIfVisible(textLabel(), textSizeOverrides_);
    calc.addTrackIfVisible(shortcutLabel(), shortcutSizeOverrides_);
    calc.addTrackIfPositive(arrowSizeOverride_);
    return calc.compute();
}

// XXX use flex gap
void MenuButton::updateChildrenGeometry() {
    namespace gs = graphics::strings;
    namespace ss = style::strings;

    using namespace strings;

    const geometry::Vec2f size = this->size();

    const bool hint = (style(gs::pixel_hinting) == gs::normal);
    const Margins padding(
        getSpacing(this, ss::padding_top, hint),
        getSpacing(this, ss::padding_right, hint),
        getSpacing(this, ss::padding_bottom, hint),
        getSpacing(this, ss::padding_left, hint));

    const geometry::Rect2f contentBox = geometry::Rect2f({}, size) - padding;
    if (contentBox.width() <= 0 || contentBox.height() <= 0) {
        iconWidget()->updateGeometry(0, 0, 0, 0);
        textLabel()->updateGeometry(0, 0, 0, 0);
        shortcutLabel()->updateGeometry(0, 0, 0, 0);
        return;
    }

    geometry::Vec2f iconSize = {};
    geometry::Vec2f shortcutSize = {};

    if (iconWidget()->visibility() != Visibility::Invisible) {
        iconSize = iconWidget()->preferredSize();
        applySizeOverrides(iconSize, iconSizeOverrides_);
    }
    if (shortcutLabel()->visibility() != Visibility::Invisible) {
        shortcutSize = shortcutLabel()->preferredSize();
        applySizeOverrides(shortcutSize, shortcutSizeOverrides_);
    }
    float arrowSize = arrowSizeOverride_ >= 0 ? arrowSizeOverride_ : 0.f;

    bool reverse = false;
    switch (direction()) {
    case FlexDirection::ColumnReverse:
        reverse = true;
        [[fallthrough]];
    case FlexDirection::Column: {
        // vertical layout
        const float gapH = getSpacing(this, column_gap, hint);
        float wc = contentBox.width();
        float hc = contentBox.height();
        float xc = padding.left();
        float y0 = padding.top();
        float y4 = y0 + hc;
        float hr = hc;
        float hArrow = allocSize(arrowSize, hr);
        float hGap3 = (hArrow > 0.f) ? allocSize(gapH, hr) : 0.f;
        float hIcon = allocSize(iconSize.y(), hr);
        float hGap1 = (hIcon > 0.f) ? allocSize(gapH, hr) : 0.f;
        float hShortcut = allocSize(shortcutSize.y(), hr);
        float hGap2 = (hShortcut > 0.f) ? allocSize(gapH, hr) : 0.f;
        float hText = hr;
        float y = y0;
        if (!reverse) {
            iconWidget()->updateGeometry(xc, y, wc, hIcon);
            y += hIcon + hGap1;
            textLabel()->updateGeometry(xc, y, wc, hText);
            y += hText + hGap2;
            shortcutLabel()->updateGeometry(xc, y, wc, hShortcut);
            // y += hShortcut + hGap3;
            // XXX arrow, todo
        }
        else {
            // XXX arrow, todo
            y += hArrow + hGap3;
            shortcutLabel()->updateGeometry(xc, y, wc, hShortcut);
            y += hShortcut + hGap2;
            textLabel()->updateGeometry(xc, y, wc, hText);
            y += hText + hGap1;
            iconWidget()->updateGeometry(xc, y, wc, y4 - y);
        }
        break;
    }
    case FlexDirection::RowReverse:
        reverse = true;
        [[fallthrough]];
    case FlexDirection::Row:
    default: {
        // horizontal layout
        const float gapH = getSpacing(this, column_gap, hint);
        float wc = contentBox.width();
        float hc = contentBox.height();
        float yc = padding.top();
        float x0 = padding.left();
        float x4 = x0 + wc;
        float wr = wc;
        float wArrow = allocSize(arrowSize, wr);
        float wGap3 = (wArrow > 0.f) ? allocSize(gapH, wr) : 0.f;
        float wIcon = allocSize(iconSize.x(), wr);
        float wGap1 = (wIcon > 0.f) ? allocSize(gapH, wr) : 0.f;
        float wShortcut = allocSize(shortcutSize.x(), wr);
        float wGap2 = (wShortcut > 0.f) ? allocSize(gapH, wr) : 0.f;
        float wText = wr;
        float x = x0;
        if (!reverse) {
            iconWidget()->updateGeometry(x, yc, wIcon, hc);
            x += wIcon + wGap1;
            textLabel()->updateGeometry(x, yc, wText, hc);
            x += wText + wGap2;
            shortcutLabel()->updateGeometry(x, yc, wShortcut, hc);
            // x += wShortcut + wGap3;
            // XXX arrow, todo
        }
        else {
            // XXX arrow, todo
            x += wArrow + wGap3;
            shortcutLabel()->updateGeometry(x, yc, wShortcut, hc);
            x += wShortcut + wGap2;
            textLabel()->updateGeometry(x, yc, wText, hc);
            x += wText + wGap1;
            iconWidget()->updateGeometry(x, yc, x4 - x, hc);
        }
        // XXX todo arrow (widget or manual)
        break;
    }
    }
}

} // namespace vgc::ui
