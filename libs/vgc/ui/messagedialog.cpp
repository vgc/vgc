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

#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

MessageDialog::MessageDialog()
    : Dialog() {

    addStyleClass(strings::MessageDialog);
    content_ = createContent<Flex>(FlexDirection::Column);
}

MessageDialogPtr MessageDialog::create() {
    return MessageDialogPtr(new MessageDialog());
}

/*
static MessageDialogPtr createFrom(Widget* initiator) {
    ui::OverlayArea* overlayArea = initiator->topmostOverlayArea();
    return overlayArea->createOverlayWidget<Popup>(ui::OverlayResizePolicy::None);
}

void clear() {
    clearTitle();
    clearBody();
    clearButtons();
}

void clearTitle() {
    if (title_) {
        title_->destroy();
        title_ = LabelPtr();
        updateSize();
    }
}

void clearBody() {
    if (body_) {
        body_->destroy();
        body_ = FlexPtr();
        updateSize();
    }
}

void clearButtons() {
    bool shouldUpdateSize = false;
    if (buttons_) {
        buttons_->destroy();
        buttons_ = FlexPtr();
        shouldUpdateSize = true;
    }
    if (stretch_) {
        stretch_->destroy();
        stretch_ = FlexPtr();
        shouldUpdateSize = true;
    }
    actions_.clear();
    if (shouldUpdateSize) {
        updateSize();
    }
}

void setTitle(std::string_view text) {
    if (!title_) {
        title_ = createChild<Label>();
        title_->addStyleClass(core::StringId("title"));
        // TODO: move before all other siblings
    }
    title_->setText(text);
    updateSize();
}
*/

void MessageDialog::addText(std::string_view text) {
    if (content_) {
        createBodyIfNotCreated_();
        body_->createChild<Label>(text);
        updateSize_();
    }
}

/*
void addCenteredText(std::string_view text) {
    createBodyIfNotCreated_();
    Label* label = body_->createChild<Label>(text);
    label->addStyleClass(core::StringId("centered"));
    updateSize();
}

template<typename Function>
void addButton(std::string_view text, Function onClick) {
    createButtonsIfNotCreated_();

    // Currently, we can't remove existing actions from a widget, so we
    // don't create the action as a children of this Popup, but instead
    // create it as root object.
    //
    // TODO: better system to create/destroy actions.
    // How to assign shortcuts? (e.g., Enter key for OK, etc.).
    //
    ActionPtr action = Action::create(text);
    actions_.append(action);
    buttons_->createChild<Button>(action.get());
    action->triggered().connect(onClick);
    updateSize();
}

void initPositionAndSize(geometry::Vec2f mousePosition, Widget* relativeTo) {
    // TODO: better positionning (e.g., see Menu).
    // For now we open the popup to the right of the mouse position.
    using namespace style::literals;
    style::Length popupMargin = 10_dp;
    float popupMarginInPx = popupMargin.toPx(styleMetrics());
    geometry::Vec2f popupPosition = mousePosition;
    popupPosition += geometry::Vec2f(popupMarginInPx, 0);
    popupPosition = relativeTo->mapTo(parent(), popupPosition);
    popupPosition[0] = std::round(popupPosition[0]);
    popupPosition[1] = std::round(popupPosition[1]);
    geometry::Vec2f popupSize = preferredSize();
    clampSizeToMinMax_(popupSize);
    updateGeometry(popupPosition, popupSize);
}
*/

namespace {

geometry::Vec2f clampSizeToMinMax(const Widget* widget, const geometry::Vec2f& size) {
    using detail::getLengthOrPercentageInPx;
    geometry::Vec2f refSize;
    if (widget->parent()) {
        refSize = widget->parent()->size();
    }
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
    clampSizeToMinMax(this, newSize);
    geometry::Vec2f oldPos = position();
    geometry::Vec2f newPos = oldPos + 0.5f * (oldSize - newSize);
    updateGeometry(newPos, newSize);
}

void MessageDialog::createBodyIfNotCreated_() {
    if (!body_) {
        body_ = content_->createChild<Flex>(FlexDirection::Column);
        body_->addStyleClass(core::StringId("body"));
        // TODO: move just after title
    }
}

/*
void MessageDialog::createButtonsIfNotCreated_() {
    if (!stretch_) {
        stretch_ = createChild<Flex>(FlexDirection::Column);
        stretch_->addStyleClass(core::StringId("stretch"));
    }
    if (!buttons_) {
        buttons_ = createChild<Flex>(FlexDirection::Row);
        buttons_->addStyleClass(core::StringId("buttons"));
    }
}

*/

} // namespace vgc::ui
