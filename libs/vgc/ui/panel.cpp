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

#include <vgc/ui/panel.h>

#include <vgc/ui/preferredsizecalculator.h>
#include <vgc/ui/strings.h>

namespace vgc::ui {

Panel::Panel(CreateKey key, std::string_view title)
    : Widget(key)
    , title_(title) {

    addStyleClass(strings::Panel);
}

PanelPtr Panel::create(std::string_view title) {
    return core::createObject<Panel>(title);
}

void Panel::setTitle(std::string_view title) {
    if (title_ == title) {
        return;
    }
    title_ = title;
    titleChanged().emit();
}

Widget* Panel::body() const {
    return firstChild();
}

void Panel::setBody(Widget* newBody) {
    Widget* oldBody = body();
    if (oldBody == newBody) {
        return;
    }
    if (oldBody) {
        if (newBody) {
            newBody->replace(oldBody);
        }
        else {
            oldBody->destroy();
        }
    }
    else {
        insertChild(firstChild(), newBody);
    }
}

float Panel::preferredWidthForHeight(float height) const {
    // TODO: padding / border
    Widget* body_ = body();
    if (body()) {
        return body_->preferredWidthForHeight(height);
    }
    else {
        return 0.0f;
    }
}

float Panel::preferredHeightForWidth(float width) const {
    // TODO: padding / border
    Widget* body_ = body();
    if (body_) {
        return body_->preferredHeightForWidth(width);
    }
    else {
        return 0.0f;
    }
}

void Panel::onWidgetAdded(Widget* child, bool) {
    // A panel can only have one child, so we remove all others
    while (firstChild() && firstChild() != child) {
        firstChild()->destroy();
    }
    while (lastChild() && lastChild() != child) {
        lastChild()->destroy();
    }
    requestGeometryUpdate();
}

void Panel::onWidgetRemoved(Widget*) {
    requestGeometryUpdate();
}

geometry::Vec2f Panel::computePreferredSize() const {
    PreferredSizeCalculator calc(this);
    Widget* body = this->body();
    if (body) {
        calc.add(body->preferredSize());
    }
    calc.addPaddingAndBorder();
    return calc.compute();
}

void Panel::updateChildrenGeometry() {
    Widget* body_ = body();
    if (body_) {
        body_->updateGeometry(contentRect());
    }
}

} // namespace vgc::ui
