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

#include <vgc/ui/iconwidget.h>

#include <vgc/ui/preferredsizecalculator.h>

namespace vgc::ui {

IconWidget::IconWidget(CreateKey key, std::string_view filePath)
    : Widget(key) {

    addStyleClass(strings::IconWidget);
    setFilePath(filePath);
}

void IconWidget::setFilePath(std::string_view filePath) {
    if (icon_.get()) {
        removeChildStylableObject(icon_.get());
    }
    icon_ = nullptr;
    if (!filePath.empty()) {
        icon_ = graphics::Icon::create(filePath);
        appendChildStylableObject(icon_.get());
    }
    isIconGeometryDirty_ = true;
    requestGeometryUpdate();
    requestRepaint();
}

IconWidgetPtr IconWidget::create(std::string_view filePath) {
    return core::createObject<IconWidget>(filePath);
}

void IconWidget::onResize() {
    SuperClass::onResize();
    isIconGeometryDirty_ = true;
}

void IconWidget::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    if (!icon_) {
        return;
    }

    if (isIconGeometryDirty_) {
        isIconGeometryDirty_ = false;

        // Get icon Size
        geometry::Vec2f iconSize = icon_->size();
        if (iconSize[0] == 0 || iconSize[1] == 0) {
            return;
        }

        // Get content rect and size
        geometry::Rect2f contentRect = this->contentRect();
        geometry::Vec2f contentSize = contentRect.size();

        // Determine scale to apply to the icon so that it fits in the content rect
        // while preserving the aspect ratio of the icon.
        iconScale_ = contentSize[0] / iconSize[0];
        if (iconSize[1] * iconScale_ >= contentSize[1]) {
            iconScale_ = contentSize[1] / iconSize[1];
        }

        // Determine position of the icon so that it is centered
        geometry::Vec2f scaledIconSize = iconScale_ * iconSize;
        iconPosition_ = contentRect.position() + 0.5f * (contentSize - scaledIconSize);
    }

    // Draw scaled icon
    engine->pushViewMatrix();
    geometry::Mat4f m = engine->viewMatrix();
    m.translate(iconPosition_);
    m.scale(iconScale_);
    engine->setViewMatrix(m);
    icon_->draw(engine);
    engine->popViewMatrix();
}

geometry::Vec2f IconWidget::computePreferredSize() const {

    using namespace style::literals;
    const style::Length preferredContentSizeIfAuto_ = 100.0_dp;

    PreferredSizeCalculator calc(this);
    calc.add(preferredContentSizeIfAuto_, preferredContentSizeIfAuto_);
    calc.addPaddingAndBorder();
    return calc.compute();
}

} // namespace vgc::ui
