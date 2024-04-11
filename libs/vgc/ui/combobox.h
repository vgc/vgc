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
#include <vgc/ui/menubutton.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ComboBox);
VGC_DECLARE_OBJECT(Menu);

namespace detail {

struct ComboBoxItem {
    std::string text;
};

} // namespace detail

/// \class vgc::ui::ComboBox
/// \brief A widget to select one among multiple options.
///
class VGC_UI_API ComboBox : public MenuButton {
private:
    VGC_OBJECT(ComboBox, Flex)

protected:
    ComboBox(CreateKey);

public:
    /// Creates a `ComboBox`.
    ///
    static ComboBoxPtr create();

    /// Returns the number of items in this combo box.
    ///
    Int numItems() const {
        return items_.length();
    }

    /// Returns the index of the current item, or -1 if there is no current item.
    ///
    Int currentIndex() const {
        return currentIndex_;
    }

    /// Sets the current item to the one at index `index`.
    ///
    /// If `index` is not in [0, `numItems() - 1`], then this is equivalent to `unset()`.
    ///
    void setCurrentIndex(Int index);
    VGC_SLOT(setCurrentIndex)

    /// This signal is emitted whenever the `currentIndex()` of this combo box
    /// changes.
    ///
    VGC_SIGNAL(currentIndexChanged, (Int, index))

    /// Adds a text item to this combo box.
    ///
    void addItem(std::string_view text);

private:
    // State
    core::Array<detail::ComboBoxItem> items_;
    Int currentIndex_ = -1;

    // Owned widget
    MenuSharedPtr menu_;
};

} // namespace vgc::ui

#endif // VGC_UI_COMBOBOX_H
