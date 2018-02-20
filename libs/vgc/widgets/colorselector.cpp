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

namespace vgc {
namespace widgets {

ColorSelector::ColorSelector(const core::Color& initialColor, QWidget* parent) :
    QToolButton(parent),
    color_(initialColor)
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

void ColorSelector::onClicked_()
{
    QColor initialColor = toQt(color_);
    QColor c = QColorDialog::getColor(
                initialColor,
                0,
                tr("select the color"),
                QColorDialog::ShowAlphaChannel);

    setColor(fromQt(c));
}

void ColorSelector::updateIcon()
{
    // Create icon
    QSize pixmapSize = iconSize();
    QPixmap pixmap(pixmapSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(QBrush(toQt(color())));
    QRect rect(QPoint(0, 0), pixmapSize);
    painter.drawEllipse(rect);

    // Set pixmap as tool icon
    setIcon(pixmap);
}

} // namespace widgets
} // namespace vgc
