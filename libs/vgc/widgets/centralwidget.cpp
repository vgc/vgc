// Copyright 2018 The VGC Developers
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

#include <vgc/widgets/centralwidget.h>

#include <vgc/widgets/toggleviewaction.h>

namespace vgc {
namespace widgets {

CentralWidget::CentralWidget(
        QWidget* viewer,
        QWidget* toolbar,
        QWidget* console,
        QWidget* panel,
        QWidget* parent) :
    QWidget(parent),
    viewer_(viewer),
    toolbar_(toolbar),
    console_(console),
    panel_(panel)
{
    viewer_->setParent(this);
    toolbar->setParent(this);
    console->setParent(this);
    panel->setParent(this);

    consoleToggleViewAction_ = new ToggleViewAction(tr("Console"), console_, this);
    connect(consoleToggleViewAction_, SIGNAL(toggled(bool)), this, SLOT(updateGeometries_()));

    panelToggleViewAction_ = new ToggleViewAction(tr("Panel"), panel_, this);
    connect(panelToggleViewAction_, SIGNAL(toggled(bool)), this, SLOT(updateGeometries_()));

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    updateGeometries_();
}

CentralWidget::~CentralWidget()
{

}

QSize CentralWidget::sizeHint() const
{
    return QSize(1920, 1080);
}

void CentralWidget::paintEvent(QPaintEvent*)
{

}

void CentralWidget::resizeEvent(QResizeEvent*)
{
    updateGeometries_();
}

void CentralWidget::updateGeometries_()
{
    int m = 5;   // Half-margin size
    int M = 2*m; // Margin size
    int h = height();
    int w = width();

    int x1 = m;
    int x2 = x1;
    if (toolbar_->isVisibleTo(this)) {
        x2 += M + toolbar_->sizeHint().width();
    }
    int x4 = w - m;
    int x3 = x4;
    if (panel_->isVisibleTo(this)) {
        x3 -= M + panel_->sizeHint().width();
    }
    int y1 = m;
    int y3 = h - m;
    int y2 = y3;
    if (console_->isVisibleTo(this)) {
        y2 -= M + console_->sizeHint().height();
    }

    toolbar_ -> setGeometry(x1+m, y1+m, x2-x1-M, y3-y1-M);
    viewer_  -> setGeometry(x2+m, y1+m, x3-x2-M, y2-y1-M);
    console_ -> setGeometry(x2+m, y2+m, x3-x2-M, y3-y2-M);
    panel_   -> setGeometry(x3+m, y1+m, x4-x3-M, y3-y1-M);
}

} // namespace widgets
} // namespace vgc
