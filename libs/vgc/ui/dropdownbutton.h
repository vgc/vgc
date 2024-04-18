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

#ifndef VGC_UI_DROPDOWNBUTTON_H
#define VGC_UI_DROPDOWNBUTTON_H

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
VGC_DECLARE_OBJECT(DropdownButton);

/// \enum vgc::ui::DropDirection
/// \brief The direction in which a dropdown overlay should appear.
///
enum class DropDirection {
    Horizontal,
    Vertical,
};

/// \class vgc::ui::DropdownButton
/// \brief A button with the ability to open a dropdown overlay.
///
class VGC_UI_API DropdownButton : public Button {
private:
    VGC_OBJECT(DropdownButton, Button)
    friend Menu;

protected:
    DropdownButton(CreateKey, Action* action, FlexDirection layoutDirection);

public:
    /// Creates an DropdownButton with the given `action`.
    ///
    static DropdownButtonPtr
    create(Action* action, FlexDirection layoutDirection = FlexDirection::Column);

    void setDropDirection(DropDirection direction) {
        dropDirection_ = direction;
    }

    DropDirection dropDirection() const {
        return dropDirection_;
    }

    Menu* popupMenu() const {
        return popupMenu_;
    };

    void closePopupMenu();

    VGC_SIGNAL(menuPopupOpened);
    VGC_SIGNAL(menuPopupClosed, (bool, recursive));

private:
    DropDirection dropDirection_ = DropDirection::Horizontal;
    Menu* popupMenu_ = nullptr;

    // The menu calls this when it opens as a popup.
    void onMenuPopupOpened_(Menu* menu);

    void onMenuPopupClosed_(bool recursive);
    VGC_SLOT(onMenuPopupClosedSlot_, onMenuPopupClosed_);
};

} // namespace vgc::ui

#endif // VGC_UI_DROPDOWNBUTTON_H
