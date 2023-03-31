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

float Dialog::preferredWidthForHeight(float height) const {
    PreferredWidthForHeightCalculator calc(this, height);
    if (content()) {
        float contentTargetHeight = calc.getChildrenTargetHeight();
        calc.addWidth(content()->preferredWidthForHeight(contentTargetHeight));
    }
    calc.addPaddingAndBorder();
    return calc.compute();
}

float Dialog::preferredHeightForWidth(float width) const {
    PreferredHeightForWidthCalculator calc(this, width);
    if (content()) {
        float contentTargetWidth = calc.getChildrenTargetWidth();
        calc.addHeight(content()->preferredHeightForWidth(contentTargetWidth));
    }
    calc.addPaddingAndBorder();
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
