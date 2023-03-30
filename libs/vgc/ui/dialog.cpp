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

#include <vgc/ui/dialog.h>

#include <vgc/style/strings.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/preferredsizecalculator.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Dialog::Dialog()
    : Widget() {

    addStyleClass(strings::Dialog);
}

DialogPtr Dialog::create() {
    return DialogPtr(new Dialog());
}

void Dialog::setContent(Widget* widget) {
    if (widget != firstChild()) {
        if (firstChild()) {
            if (widget) {
                widget->replace(firstChild());
            }
            else {
                firstChild()->destroy();
            }
        }
        else { // => widget is non null
            addChild(widget);
        }
        requestGeometryUpdate();
    }
}

void Dialog::showAt(Widget* widget) {
    if (widget) {
        OverlayArea* overlayArea = widget->topmostOverlayArea();
        if (overlayArea) {
            overlayArea->addOverlayWidget(this);
        }
    }
}

namespace {

// TODO: publicize this class
class PreferredXForYCalculator {
public:
    PreferredXForYCalculator(const Widget* self, float y)
        : self_(self)
        , y_(y) {

        borderWidth_ = detail::getLengthInPx(self, style::strings::border_width);
        absolute_ += 2 * borderWidth_;
    }

    float getChildY(Widget* content, core::StringId padY1_, core::StringId padY2_) {
        if (!content) {
            return 0;
        }
        float padY1 = detail::getLengthOrPercentageInPx(self_, padY1_, y_);
        float padY2 = detail::getLengthOrPercentageInPx(self_, padY2_, y_);
        float childY = y_ - padY1 - padY2 - 2 * borderWidth_;
        childY = (std::max)(0.0f, childY);
        return childY;
    }

    void addChildPreferredX(float childPreferredX) {
        absolute_ += childPreferredX;
    }

    void addPadX(core::StringId padX) {
        style::LengthOrPercentage lp = detail::getLengthOrPercentage(self_, padX);
        if (lp.isPercentage()) {
            relative_ += lp.value();
        }
        else {
            float dummyRefLength = 1.0f;
            absolute_ += lp.toPx(self_->styleMetrics(), dummyRefLength);
        }
    }

    float compute() {
        constexpr float maxRelative = 0.99f;
        relative_ = core::clamp(relative_, 0.f, maxRelative);
        return absolute_ / (1.0f - relative_);
    }

private:
    float y_;
    const Widget* self_;
    float borderWidth_ = 0;
    float absolute_ = 0;
    float relative_ = 0;
};

} // namespace

float Dialog::preferredWidthForHeight(float height) const {
    namespace ss = style::strings;
    PreferredXForYCalculator calc(this, height);
    if (content()) {
        float childY = calc.getChildY(content(), ss::padding_top, ss::padding_bottom);
        calc.addChildPreferredX(content()->preferredWidthForHeight(childY));
    }
    calc.addPadX(ss::padding_left);
    calc.addPadX(ss::padding_right);
    return calc.compute();
}

float Dialog::preferredHeightForWidth(float width) const {
    namespace ss = style::strings;
    PreferredXForYCalculator calc(this, width);
    if (content()) {
        float childY = calc.getChildY(content(), ss::padding_left, ss::padding_right);
        calc.addChildPreferredX(content()->preferredHeightForWidth(childY));
    }
    calc.addPadX(ss::padding_top);
    calc.addPadX(ss::padding_bottom);
    return calc.compute();
}

void Dialog::onWidgetAdded(Widget* child, bool) {
    // A dialog can only have one child, so we remove all others
    while (firstChild() && firstChild() != child) {
        firstChild()->destroy();
    }
    while (lastChild() && lastChild() != child) {
        lastChild()->destroy();
    }
    child->addStyleClass(strings::content);
    requestGeometryUpdate();
}

void Dialog::onWidgetRemoved(Widget*) {
    requestGeometryUpdate();
}

geometry::Vec2f Dialog::computePreferredSize() const {
    PreferredSizeCalculator calc(this);
    if (content()) {
        calc.add(content()->preferredSize());
    }
    calc.addPaddingAndBorder();
    return calc.compute();
}

void Dialog::updateChildrenGeometry() {
    if (content()) {
        content()->updateGeometry(contentRect());
    }
}

} // namespace vgc::ui
