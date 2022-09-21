// Copyright 2021 The VGC Developers
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

// Implementation Notes
// --------------------
//
// This is basically like a complex QSplitter allowing you to split and resize
// in both directions. See the following for inspiration on how to implement
// missing features:
//
// https://github.com/qt/qtbase/blob/5.12/src/widgets/widgets/qsplitter.cpp
//

#include <vgc/widgets/toolbar.h>

namespace {

int iconWidth = 64;

} // namespace

namespace vgc::widgets {

Toolbar::Toolbar(QWidget* parent)
    : QToolBar(parent) {

    QSize iconSize(iconWidth, iconWidth);

    setOrientation(Qt::Vertical);
    setMovable(false);
    setIconSize(iconSize);
    setFocusPolicy(Qt::ClickFocus);

    auto colorPalettePtr = ui::ColorPalette::create();
    colorPalette_ = colorPalettePtr.get();
    colorPaletteq_ = new UiWidget(colorPalettePtr, this);
    // Note: it would be nice to std::move() the shared ptr instead of the
    // above copy. This requires implementing UiWidget(WidgetPtr&&),
    // but probably not worth it as this is temporary code anyway.
    addWidget(colorPaletteq_);

    colorPalette_->colorSelected().connect(
        [this]() { this->onColorPaletteColorSelected_(); });
}

Toolbar::~Toolbar() {
}

core::Color Toolbar::color() const {
    return colorPalette_->selectedColor();
}

void Toolbar::resizeEvent(QResizeEvent* event) {

    // Manually update height of the color palette, because QToolbar doesn't
    // automatically update the height of its children, even if they called
    // updateGeometry() and their sizeHint() or heightForWidth() changed.
    //
    float colorPaletteHeight_ = colorPaletteq_->heightForWidth(width());
    int colorPaletteHeight = static_cast<int>(colorPaletteHeight_);
    colorPaletteq_->setMinimumHeight(colorPaletteHeight);
    colorPaletteq_->setMaximumHeight(colorPaletteHeight);
    colorPaletteq_->show();
    QToolBar::resizeEvent(event);
}

void Toolbar::onColorPaletteColorSelected_() {
    Q_EMIT colorChanged(color());
}

} // namespace vgc::widgets
