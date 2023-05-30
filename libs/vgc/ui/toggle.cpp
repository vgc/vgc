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

#include <vgc/ui/toggle.h>

#include <vgc/core/array.h>
#include <vgc/style/strings.h>
#include <vgc/style/types.h>
#include <vgc/ui/preferredsizecalculator.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

namespace {

namespace strings_ {

core::StringId back("back");
core::StringId front("front");

} // namespace strings_

} // namespace

Toggle::Toggle()
    : Widget() {

    back_ = createChild<Widget>();
    front_ = createChild<Widget>();

    addStyleClass(strings::Toggle);
    back_->addStyleClass(strings_::back);
    front_->addStyleClass(strings_::front);
    updateStyleClasses_();
}

TogglePtr Toggle::create() {
    return TogglePtr(new Toggle());
}

void Toggle::setState(bool state) {
    if (state_ != state) {
        state_ = state;
        updateStyleClasses_();
        requestGeometryUpdate();
        toggled().emit(state);
    }
}

bool Toggle::toggle() {
    bool isTogglable = true;
    if (isTogglable) {
        setState(!state());
        return true;
    }
    else {
        return false;
    }
}

bool Toggle::click(const geometry::Vec2f& pos) {
    bool isTogglable = true;
    if (isTogglable) {
        toggle();
        clicked().emit(this, pos);
        return true;
    }
    else {
        return false;
    }
}

bool Toggle::onMouseMove(MouseMoveEvent* event) {
    if (isPressed_) {
        if (rect().contains(event->position())) {
            addStyleClass(strings::pressed);
        }
        else {
            removeStyleClass(strings::pressed);
        }
        return true;
    }
    else {
        return false;
    }
}

bool Toggle::onMousePress(MousePressEvent* event) {
    if (event->button() == MouseButton::Left) {
        pressed().emit(this, event->position());
        addStyleClass(strings::pressed);
        isPressed_ = true;
        return true;
    }
    else {
        return false;
    }
}

bool Toggle::onMouseRelease(MouseReleaseEvent* event) {
    if (isPressed_ && event->button() == MouseButton::Left) {
        released().emit(this, event->position());
        if (rect().contains(event->position())) {
            click(event->position());
        }
        removeStyleClass(strings::pressed);
        isPressed_ = false;
        return true;
    }
    else {
        return false;
    }
}

void Toggle::onMouseEnter() {
    addStyleClass(strings::hovered);
}

void Toggle::onMouseLeave() {
    removeStyleClass(strings::hovered);
}

geometry::Vec2f Toggle::computePreferredSize() const {
    PreferredSizeCalculator calc(this);
    calc.add(back_->preferredSize());
    calc.addMargin(back_.get());
    calc.addPaddingAndBorder();
    return calc.compute();
}

void Toggle::updateChildrenGeometry() {

    using detail::getLengthOrPercentageInPx;
    namespace ss = style::strings;

    geometry::Rect2f rect = contentRect();
    geometry::Vec2f frontSize = front_->preferredSize();
    geometry::Vec2f frontPosition;

    if (state() == on) {
        // Place front element to the right of the toggle
        frontPosition[0] =
            rect.xMax()
            - getLengthOrPercentageInPx(front_.get(), ss::margin_left, frontSize[0])
            - frontSize[0];
    }
    else { // state() == off
        // Place front element to the left of the toggle
        frontPosition[0] =
            rect.xMin()
            + getLengthOrPercentageInPx(front_.get(), ss::margin_right, frontSize[0]);
    }
    frontPosition[1] = rect.yMin() + 0.5f * (rect.height() - frontSize[1]);

    back_->updateGeometry(rect.position(), rect.size());
    front_->updateGeometry(frontPosition, frontSize);
}

void Toggle::updateStyleClasses_() {
    if (state() == on) {
        replaceStyleClass(strings::off, strings::on);
    }
    else {
        replaceStyleClass(strings::on, strings::off);
    }
}

} // namespace vgc::ui
