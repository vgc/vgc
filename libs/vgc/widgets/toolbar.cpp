// Copyright 2020 The VGC Developers
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

#include <vgc/ui/colorpalette.h>

namespace vgc {
namespace widgets {

Toolbar::Toolbar(QWidget* parent) :
    QToolBar(parent)
{     
     int iconWidth = 48;
     QSize iconSize(iconWidth,iconWidth);

     setOrientation(Qt::Vertical);
     setMovable(false);
     setIconSize(iconSize);

     colorToolButton_ = new ColorToolButton();
     colorToolButton_->setToolTip(tr("Current color (C)"));
     colorToolButton_->setStatusTip(tr("Click to open the color selector"));
     colorToolButton_->setIconSize(iconSize);
     colorToolButton_->updateIcon();

     colorToolButtonAction_ = addWidget(colorToolButton_);
     colorToolButtonAction_->setText(tr("Color"));
     colorToolButtonAction_->setToolTip(tr("Color (C)"));
     colorToolButtonAction_->setStatusTip(tr("Click to open the color selector"));
     colorToolButtonAction_->setShortcut(QKeySequence(Qt::Key_C));
     colorToolButtonAction_->setShortcutContext(Qt::ApplicationShortcut);

     // XXX This is temporary code to test the unfinished color palette.
     // Eventually, we'll move the whole Toolbar class to vgc::ui.
     // For now, just set showColorPalette to true for testing.
     bool showColorPalette = false;
     if (showColorPalette) {
         colorPalette_ = new UiWidget(ui::ColorPalette::create(), this);
         colorPalette_->setMinimumSize(iconWidth, 300);
         addWidget(colorPalette_);
     }

     connect(colorToolButtonAction_, SIGNAL(triggered()), colorToolButton_, SLOT(click()));
     connect(colorToolButton_, &ColorToolButton::colorChanged, this, &Toolbar::colorChanged);
}

Toolbar::~Toolbar()
{

}

core::Color Toolbar::color() const
{
    return colorToolButton_->color();
}

} // namespace widgets
} // namespace vgc
