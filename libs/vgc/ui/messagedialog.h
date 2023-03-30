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
    MessageDialog();

public:
    /// Creates a `MessageDialog`.
    ///
    static MessageDialogPtr create();

    /// Adds a paragraph of text to the body of this dialog.
    ///
    void addText(std::string_view text);

private:
    FlexPtr content_;
    FlexPtr body_;

    /*
    LabelPtr title_;
    FlexPtr body_;
    FlexPtr stretch_;
    FlexPtr buttons_;
    core::Array<ActionPtr> actions_;
*/
    void updateSize_();
    void createBodyIfNotCreated_();

    /*
    void createButtonsIfNotCreated_() {
        if (!stretch_) {
            stretch_ = createChild<Flex>(FlexDirection::Column);
            stretch_->addStyleClass(core::StringId("stretch"));
        }
        if (!buttons_) {
            buttons_ = createChild<Flex>(FlexDirection::Row);
            buttons_->addStyleClass(core::StringId("buttons"));
        }
    }

    void clampSizeToMinMax_(geometry::Vec2f& size) {
        using detail::getLengthOrPercentageInPx;
        geometry::Vec2f parentSize = parent()->size();
        float minW = getLengthOrPercentageInPx(this, strings::min_width, parentSize[0]);
        float minH = getLengthOrPercentageInPx(this, strings::min_height, parentSize[0]);
        float maxW = getLengthOrPercentageInPx(this, strings::max_width, parentSize[0]);
        float maxH = getLengthOrPercentageInPx(this, strings::max_height, parentSize[0]);
        maxW = std::abs(maxW);
        maxH = std::abs(maxH);
        minW = core::clamp(minW, 0, maxW);
        minH = core::clamp(minH, 0, maxH);
        size[0] = core::clamp(size[0], minW, maxW);
        size[1] = core::clamp(size[1], minH, maxH);
    }
    */
};

} // namespace vgc::ui

#endif // VGC_UI_MESSAGEDIALOG_H
