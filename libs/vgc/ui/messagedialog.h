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

#ifndef VGC_UI_MESSAGEDIALOG_H
#define VGC_UI_MESSAGEDIALOG_H

#include <vgc/ui/dialog.h>
#include <vgc/ui/flex.h>
#include <vgc/ui/label.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(MessageDialog);

/// \class vgc::ui::MessageDialog
/// \brief A dialog to show simple messages and/or questions to users.
///
class VGC_UI_API MessageDialog : public Dialog {
private:
    VGC_OBJECT(MessageDialog, Dialog)

protected:
    /// This is an implementation details. Please use
    /// MessageDialog::create() instead.
    ///
    MessageDialog(CreateKey);

public:
    /// Creates a `MessageDialog`.
    ///
    static MessageDialogPtr create();

    /// Removes all elements in this dialog, making it empty (no title, no body,
    /// no buttons).
    ///
    void clear();

    /// Removes the title of this dialog.
    ///
    void clearTitle();

    /// Removes all elements in the body of this dialog.
    ///
    void clearBody();

    /// Removes all buttons of this dialog.
    ///
    void clearButtons();

    /// Sets the title of this dialog.
    ///
    void setTitle(std::string_view text);

    /// Adds a paragraph of text to the body of this dialog.
    ///
    void addText(std::string_view text);

    /// Adds a centered paragraph of text to the body of this dialog.
    ///
    void addCenteredText(std::string_view text);

    /// Adds a button to this dialog, calling the given function on click.
    ///
    template<typename Function>
    void addButton(std::string_view text, Function onClick) {
        Action* action = addButton_(text);
        if (action) {
            action->triggered().connect(onClick);
        }
    }

private:
    FlexPtr content_;
    LabelPtr title_;
    FlexPtr body_;
    FlexPtr buttons_;
    core::Array<ActionPtr> actions_;

    void updateSize_();
    void createBodyIfNotCreated_();
    void createButtonsIfNotCreated_();
    Action* addButton_(std::string_view text);
};

} // namespace vgc::ui

#endif // VGC_UI_MESSAGEDIALOG_H
