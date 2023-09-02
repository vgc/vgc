// Copyright 2023 The VGC Developers
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

#ifndef VGC_UI_TOOLTIP_H
#define VGC_UI_TOOLTIP_H

#include <vgc/ui/dialog.h>
#include <vgc/ui/flex.h>
#include <vgc/ui/label.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Tooltip);

/// \class vgc::ui::Tooltip
/// \brief A dialog to show temporary information on Mouse Hover.
///
class VGC_UI_API Tooltip : public Dialog {
private:
    VGC_OBJECT(Tooltip, Dialog)

protected:
    Tooltip(CreateKey, std::string_view text);

public:
    /// Creates an empty `Tooltip`.
    ///
    static TooltipPtr create();

    /// Creates a `Tooltip` with the given `text`.
    ///
    static TooltipPtr create(std::string_view text);

    /// Returns the text shown in this `Tooltip`.
    ///
    std::string_view text() const;

    /// Sets the text to show in this `Tooltip`.
    ///
    void setText(std::string_view text);

    /// Returns the shortcut shown in this `Tooltip`.
    ///
    Shortcut shortcut() const {
        return shortcut_;
    }

    /// Sets the text to show in this `Tooltip`.
    ///
    void setShortcut(const Shortcut& shortcut);

    /// Returns whether the text is visible.
    ///
    bool isTextVisible() const;

    /// Sets whether the shortcut is visible. By default, it is visible.
    ///
    void setTextVisible(bool visible);

    /// Returns whether the shortcut is visible.
    ///
    bool isShortcutVisible() const;

    /// Sets whether the shortcut is visible. By default, it is hidden.
    ///
    void setShortcutVisible(bool visible);

protected:
    // Reimplementations of Widget virtual methods
    bool computeIsHovered(MouseHoverEvent* event) const override;

private:
    FlexPtr content_;
    LabelPtr textLabel_;
    LabelPtr shortcutLabel_;
    Shortcut shortcut_;
};

} // namespace vgc::ui

#endif // VGC_UI_TOOLTIP_H
