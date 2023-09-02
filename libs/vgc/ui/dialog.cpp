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

#include <vgc/geometry/range1f.h>
#include <vgc/style/strings.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/preferredsizecalculator.h>
#include <vgc/ui/strings.h>
#include <vgc/ui/window.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

VGC_DEFINE_ENUM(
    DialogLocationType,
    (Cursor, "Cursor"),
    (Widget, "Widget"),
    (Window, "Window"))

Dialog::Dialog(CreateKey key)
    : Widget(key) {

    addStyleClass(strings::Dialog);
}

DialogPtr Dialog::create() {
    return core::createObject<Dialog>();
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

namespace {

template<Int dimension>
geometry::Range1f mapTo1f(Widget* from, Widget* to, const geometry::Rect2f& rect) {
    geometry::Rect2f r = from->mapTo(to, rect);
    return geometry::Range1f(r.pMin()[dimension], r.pMax()[dimension]);
}

template<Int dimension>
void setPosition(
    geometry::Rect2f& dialogRect,
    Widget* overlayArea,
    DialogLocation location) {

    // Compute the anchor range, in overlayArea coordinates, with which the
    // dialog should be aligned.
    //
    geometry::Range1f anchor;
    if (location.type() == DialogLocationType::Cursor) {
        Widget* root = overlayArea->root();
        ui::Window* window = root->window();
        geometry::Vec2f windowPos(window->mapFromGlobal(globalCursorPosition()));
        geometry::Vec2f localPos(root->mapTo(overlayArea, windowPos));
        anchor.setPosition(localPos[dimension]);
        anchor.setSize(0);
    }
    else if (location.type() == DialogLocationType::Widget) {
        Widget* widget = location.widget();
        if (widget) {
            anchor = mapTo1f<dimension>(widget, overlayArea, widget->rect());
        }
    }
    else if (location.type() == DialogLocationType::Window) {
        Widget* root = overlayArea->root();
        anchor = mapTo1f<dimension>(root, overlayArea, root->rect());
    }

    // Compute dialog position based on dialog size and anchor range.
    //
    float anchorPos = anchor.position();
    float anchorSize = anchor.size();
    float dialogSize = dialogRect.size()[dimension];
    float dialogPos = 0;
    switch (location.align()) {
    case geometry::RangeAlign::Center:
        dialogPos = anchorPos + 0.5f * (anchorSize - dialogSize);
        break;
    case geometry::RangeAlign::Min:
        dialogPos = anchorPos;
        break;
    case geometry::RangeAlign::Max:
        dialogPos = anchorPos + anchorSize - dialogSize;
        break;
    case geometry::RangeAlign::OutMin:
        dialogPos = anchorPos - dialogSize;
        break;
    case geometry::RangeAlign::OutMax:
        dialogPos = anchorPos + anchorSize;
        break;
    }

    // Set dialog position.
    //
    if constexpr (dimension == 0) {
        dialogRect.setX(dialogPos);
    }
    else {
        dialogRect.setY(dialogPos);
    }
}

} // namespace

void Dialog::showAt(DialogLocation horizontalLocation, DialogLocation verticalLocation) {

    // Determine in which overlay area should the dialog be added
    OverlayArea* overlayArea = nullptr;
    if (horizontalLocation.widget()) {
        overlayArea = horizontalLocation.widget()->topmostOverlayArea();
    }
    if (!overlayArea && verticalLocation.widget()) {
        overlayArea = verticalLocation.widget()->topmostOverlayArea();
    }
    if (!overlayArea) {
        VGC_WARNING(LogVgcUi, "Could not find an overlay area where to show the dialog");
        return;
    }

    // Add dialog to overlay area
    overlayArea->addOverlayWidget(this);

    // Compute dialog geometry
    geometry::Rect2f dialogRect({}, preferredSize());
    setPosition<0>(dialogRect, overlayArea, horizontalLocation);
    setPosition<1>(dialogRect, overlayArea, verticalLocation);
    updateGeometry(dialogRect);
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
