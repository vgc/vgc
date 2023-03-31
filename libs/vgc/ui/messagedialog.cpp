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

#include <vgc/ui/messagedialog.h>

#include <vgc/ui/button.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h> // getLengthOrPercentageInPx

namespace vgc::ui {

MessageDialog::MessageDialog()
    : Dialog() {

    addStyleClass(strings::MessageDialog);
    content_ = createContent<Flex>(FlexDirection::Column);
}

MessageDialogPtr MessageDialog::create() {
    return MessageDialogPtr(new MessageDialog());
}

void MessageDialog::clear() {
    clearTitle();
    clearBody();
    clearButtons();
}

void MessageDialog::clearTitle() {
    if (title_) {
        title_->destroy();
        title_ = LabelPtr();
        updateSize_();
    }
}

void MessageDialog::clearBody() {
    if (body_) {
        body_->destroy();
        body_ = FlexPtr();
        updateSize_();
    }
}

void MessageDialog::clearButtons() {
    bool shouldUpdateSize = false;
    if (buttons_) {
        buttons_->destroy();
        buttons_ = FlexPtr();
        shouldUpdateSize = true;
    }
    actions_.clear();
    if (shouldUpdateSize) {
        updateSize_();
    }
}

void MessageDialog::setTitle(std::string_view text) {
    if (content_) {
        if (!title_) {
            title_ = content_->createChild<Label>();
            title_->addStyleClass(strings::title);
            // Move as first child
            content_->insertChild(content_->firstChild(), title_.get());
        }
        title_->setText(text);
        updateSize_();
    }
}

void MessageDialog::addText(std::string_view text) {
    if (content_) {
        createBodyIfNotCreated_();
        if (body_) {
            body_->createChild<Label>(text);
            updateSize_();
        }
    }
}

void MessageDialog::addCenteredText(std::string_view text) {
    if (content_) {
        createBodyIfNotCreated_();
        Label* label = body_->createChild<Label>(text);
        label->addStyleClass(strings::centered);
        updateSize_();
    }
}

namespace {

geometry::Vec2f clampSizeToMinMax(const Widget* widget, const geometry::Vec2f& size) {
    using detail::getLengthOrPercentageInPx;
    geometry::Vec2f refSize;
    if (widget->parent()) {
        refSize = widget->parent()->size();
    }
    // TODO: add minSize()/maxSize() to Widget
    float minW = getLengthOrPercentageInPx(widget, strings::min_width, refSize[0]);
    float minH = getLengthOrPercentageInPx(widget, strings::min_height, refSize[1]);
    float maxW = getLengthOrPercentageInPx(widget, strings::max_width, refSize[0]);
    float maxH = getLengthOrPercentageInPx(widget, strings::max_height, refSize[1]);
    maxW = std::abs(maxW);
    maxH = std::abs(maxH);
    minW = core::clamp(minW, 0, maxW);
    minH = core::clamp(minH, 0, maxH);
    return geometry::Vec2f(
        core::clamp(size[0], minW, maxW), //
        core::clamp(size[1], minH, maxH));
}

} // namespace

void MessageDialog::updateSize_() {
    geometry::Vec2f oldSize = size();
    geometry::Vec2f newSize = preferredSize();
    newSize = clampSizeToMinMax(this, newSize);
    geometry::Vec2f oldPos = position();
    geometry::Vec2f newPos = oldPos + 0.5f * (oldSize - newSize);
    updateGeometry(newPos, newSize);
}

void MessageDialog::createBodyIfNotCreated_() {
    if (content_ && !body_) {
        body_ = content_->createChild<Flex>(FlexDirection::Column);
        body_->addStyleClass(strings::body);
        if (title_) {
            // move just after title
            content_->insertChild(title_->nextSibling(), title_.get());
        }
        else {
            // Move as first child
            content_->insertChild(content_->firstChild(), title_.get());
        }
    }
}

void MessageDialog::createButtonsIfNotCreated_() {
    if (content_ && !buttons_) {
        buttons_ = content_->createChild<Flex>(FlexDirection::Row);
        buttons_->addStyleClass(strings::buttons);
    }
}

Action* MessageDialog::addButton_(std::string_view text) {
    if (content_) {

        // Currently, we can't remove existing actions from widgets, so we don't
        // create the action as child of this dialog, but instead as root object.
        //
        // TODO: better system to create/destroy actions.
        // How to assign shortcuts? (e.g., Enter key for OK, etc.).
        //
        createButtonsIfNotCreated_();
        ActionPtr action = Action::create(text);
        actions_.append(action);
        buttons_->createChild<Button>(action.get());
        updateSize_();
        return action.get();
    }
    else {
        return nullptr;
    }
}

} // namespace vgc::ui
