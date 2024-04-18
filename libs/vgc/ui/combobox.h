// Copyright 2024 The VGC Developers
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

#ifndef VGC_UI_COMBOBOX_H
#define VGC_UI_COMBOBOX_H

#include <string_view>

#include <vgc/core/array.h>
#include <vgc/core/stopwatch.h>
#include <vgc/ui/dropdownbutton.h>
#include <vgc/ui/menu.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ComboBox);

/// \class vgc::ui::ComboBox
/// \brief A widget to select one among multiple options.
///
class VGC_UI_API ComboBox : public DropdownButton {
private:
    VGC_OBJECT(ComboBox, DropdownButton)

protected:
    ComboBox(CreateKey, std::string_view title);

public:
    /// Creates a `ComboBox`.
    ///
    static ComboBoxPtr create(std::string_view title = "");

    /// Returns the number of items in this combo box.
    ///
    Int numItems() const;

    /// Returns the index of the current item, or -1 if there is no current item.
    ///
    Int index() const {
        return index_;
    }

    /// Unsets the current item, that is, re-initialize the combo box to have
    /// no current item. After calling this function, `index()` becomes -1.
    ///
    void unset() {
        setIndex(-1);
    }

    /// Sets the current item to be the one at the given `index`.
    ///
    /// If `index` is not in [0, `numItems() - 1`], then this is equivalent to calling
    /// `unset()`.
    ///
    void setIndex(Int index);
    VGC_SLOT(setIndex)

    /// This signal is emitted whenever the `index()` of this combo box
    /// changes.
    ///
    VGC_SIGNAL(indexChanged, (Int, index))

    /// Adds a text item to this combo box.
    ///
    void addItem(std::string_view text);

private:
    std::string title_;  // text when no current item is set
    Int index_ = -1;     // index of current item
    MenuSharedPtr menu_; // drop-down menu, storing all the item data

    void setText_(std::string_view text);

    void setItem_(Widget* item, Int index);

    void onSelectItem_(Widget* from);
    VGC_SLOT(onSelectItem_)

    // Ad-hoc detection of double-click to give focus

    bool wasItemSelectedSinceMenuOpened_ = false;
    core::Stopwatch menuOpenedWatch_;

    void onMenuOpened_();
    VGC_SLOT(onMenuOpened_)

    void onMenuClosed_();
    VGC_SLOT(onMenuClosed_)

    // Possible actions when the combo box has focus

    void onSelectPreviousItem_();
    VGC_SLOT(onSelectPreviousItem_)

    void onSelectNextItem_();
    VGC_SLOT(onSelectNextItem_)
};

VGC_DECLARE_OBJECT(ComboBoxMenu);

/// \class vgc::ui::ComboBoxMenu
/// \brief A menu typically used for combo boxes.
///
class VGC_UI_API ComboBoxMenu : public Menu {
private:
    VGC_OBJECT(ComboBoxMenu, Menu)

protected:
    ComboBoxMenu(CreateKey, std::string_view title, Widget* comboBox);

public:
    /// Creates a `ComboBoxMenu` associated with the given `comboBox`.
    ///
    static ComboBoxMenuPtr create(std::string_view title, Widget* comboBox);

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    WidgetWeakPtr comboBox_;
    // Note: using a Widget rather than a ComboBox makes the class more re-usable
};

} // namespace vgc::ui

#endif // VGC_UI_COMBOBOX_H
