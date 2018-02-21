// Copyright 2017 The VGC Developers
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

#include <vgc/widgets/colorselector.h>

#include <QColorDialog>
#include <QPainter>
#include <vgc/widgets/qtutil.h>

// Note for later, on how to preserve dialog position: From Qt doc on QDialog:
//
// If you invoke the show() function after hiding a dialog, the dialog will be
// displayed in its original position. This is because the window manager
// decides the position for windows that have not been explicitly placed by the
// programmer. To preserve the position of a dialog that has been moved by the
// user, save its position in your closeEvent() handler and then move the
// dialog to that position, before showing it again.

namespace vgc {
namespace widgets {

ColorSelector::ColorSelector(
        const core::Color& initialColor,
        QWidget* parent,
        ColorDialog* colorDialog) :

    QToolButton(parent),
    color_(initialColor),
    colorDialog_(colorDialog)
{
    connect(this, SIGNAL(clicked()), this, SLOT(onClicked_()));
    updateIcon();
    setFocusPolicy(Qt::NoFocus);
}

core::Color ColorSelector::color() const
{
    return color_;
}

void ColorSelector::setColor(const core::Color& color)
{
    color_ = color;
    updateIcon();
    Q_EMIT colorChanged(color_);
}

void ColorSelector::updateIcon()
{
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
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(QBrush(toQt(color())));
    painter.drawEllipse(QRect(diskTopLeft, diskSize));

    // Set pixmap as tool icon
    setIcon(pixmap);
}

ColorDialog* ColorSelector::colorDialog()
{
    if (colorDialog_ == nullptr) {
        colorDialog_ = new ColorDialog(this);
        connect(colorDialog_, &ColorDialog::destroyed,
                this, &ColorSelector::onClicked_);
        connect(colorDialog_, &ColorDialog::currentColorChanged,
                this, &ColorSelector::onColorDialogCurrentColorChanged_);
        connect(colorDialog_, &ColorDialog::finished,
                this, &ColorSelector::onColorDialogFinished_);
    }

    return colorDialog_;
}

void ColorSelector::onClicked_()
{
    previousColor_ = color_;
    colorDialog()->setCurrentColor(toQt(color_));
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

void ColorSelector::onColorDialogDestroyed_()
{
    colorDialog_ = nullptr;
}

void ColorSelector::onColorDialogCurrentColorChanged_(const QColor& color)
{
    setColor(fromQt(color));
}

void ColorSelector::onColorDialogFinished_(int result)
{
    if (result == QDialog::Rejected) {
        setColor(previousColor_);
    }
}

} // namespace widgets
} // namespace vgc
