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

#include <vgc/widgets/colortoolbutton.h>

#include <QColorDialog>
#include <QPainter>

#include <vgc/ui/qtutil.h>

namespace vgc::widgets {

ColorToolButton::ColorToolButton(
    const core::Color& initialColor,
    QWidget* parent,
    ColorDialog* colorDialog)

    : QToolButton(parent)
    , color_(initialColor)
    , colorDialog_(colorDialog) {

    connect(this, SIGNAL(clicked()), this, SLOT(onClicked_()));
    updateIcon();
    setFocusPolicy(Qt::NoFocus);
}

core::Color ColorToolButton::color() const {
    return color_;
}

void ColorToolButton::setColor(const core::Color& color) {
    if (color_ != color) {
        color_ = color;
        updateIcon();
        Q_EMIT colorChanged(color_);
    }
}

void ColorToolButton::updateIcon() {
    // Icon size
    QSize pixmapSize = iconSize();

    // Disk size
    int margin = 1;
    QPoint diskTopLeft(margin, margin);
    QSize diskSize = pixmapSize - QSize(2 * margin, 2 * margin);

    // Draw disk in QPixmap
    QPixmap pixmap(pixmapSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(QBrush(ui::toQt(color())));
    painter.drawEllipse(QRect(diskTopLeft, diskSize));

    // Set pixmap as tool icon
    setIcon(pixmap);
}

ColorDialog* ColorToolButton::colorDialog() {
    if (colorDialog_ == nullptr) {
        colorDialog_ = new ColorDialog(this);
        connect(
            colorDialog_, &ColorDialog::destroyed, this, &ColorToolButton::onClicked_);
        connect(
            colorDialog_,
            &ColorDialog::currentColorChanged,
            this,
            &ColorToolButton::onColorDialogCurrentColorChanged_);
        connect(
            colorDialog_,
            &ColorDialog::finished,
            this,
            &ColorToolButton::onColorDialogFinished_);
    }

    return colorDialog_;
}

void ColorToolButton::onClicked_() {
    previousColor_ = color_;
    colorDialog()->setCurrentColor(ui::toQt(color_));
    colorDialog()->show();
    colorDialog()->raise();
    colorDialog()->activateWindow();

    // At least on KDE, we also need this. Indeed, users have the option to
    // "minimize" the dialog, which causes it to disapear with no trace on the
    // taskbar. Without the code below, clicking on the color tool button again
    // would not make it reappear. The only thing that would make it reappear
    // is to minimize the whole app, and deminimizing it. Ideally, I would like
    // to make the dialog non-minimizable, but it doesn't seem possible on all
    // platforms, see also the comment in the constructor of ColorDialog.
    //
    if (colorDialog()->isMinimized()) {
        colorDialog()->showNormal();
    }
}

void ColorToolButton::onColorDialogDestroyed_() {
    colorDialog_ = nullptr;
}

void ColorToolButton::onColorDialogCurrentColorChanged_(const QColor& color) {
    setColor(ui::fromQt(color));
}

void ColorToolButton::onColorDialogFinished_(int result) {
    if (result == QDialog::Rejected) {
        setColor(previousColor_);
    }
}

} // namespace vgc::widgets
