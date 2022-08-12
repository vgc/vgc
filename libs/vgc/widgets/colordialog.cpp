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

#include <vgc/widgets/colordialog.h>
#include <vgc/widgets/font.h>

namespace {

void setWindowFlag_(QWidget* w, Qt::WindowType flag, bool on) {
    // Note: Since Qt 5.9, there is a method QWidget::setWindowFlag(), but at
    // the time of this writing we target Qt 5.6, reason why we use this.
    //
    // Note 2: Qt 5.7 also introduces QFlags::setFlag(), which would make
    // the code below easier as well.

    Qt::WindowFlags flags = w->windowFlags();
    if (on)
        flags |= flag;
    else
        flags &= ~flag;
    w->setWindowFlags(flags);
}

} // namespace

namespace vgc::widgets {

ColorDialog::ColorDialog(QWidget* parent)
    : QColorDialog(parent)
    , isGeometrySaved_(false) {

    // On KDE, The ColorDialog has a minimize button that I'd wish to see gone.
    // The line below was an attempt to remove it, but in fact, the
    // Qt::WindowMinimizeButtonHint was already set to false. Therefore, I
    // believe that the line below is useless, but I leave it here for
    // documentation and for robustness in case the behaviour is
    // platform-dependent. My current position is to give up and leave the
    // minimize button since it is a very minor issue. I've already spent 1h
    // trying to fix this, which is the maximum time I am willing to allocate
    // on this issue for now.
    //
    setWindowFlag_(this, Qt::WindowMinimizeButtonHint, false);

    connect(this, &ColorDialog::finished, this, &ColorDialog::onFinished_);

    // Remove the border color of the luminance picker. We'd prefer to do this
    // in qss, but it does not seem possible. By default, it is a "sunken"
    // frame explicitly drawn using qDrawShadePanel(). See:
    // qtbase/src/widgets/dialogs/qcolordialog.cpp/QColorLuminancePicker::paintEvent
    //
    Q_FOREACH (QObject* obj, children()) {
        QWidget* w = qobject_cast<QWidget*>(obj);
        const QMetaObject* mobj = obj->metaObject();
        if (w && mobj && mobj->className() == QString("QColorLuminancePicker")) {
            QPalette p = palette();
            p.setColor(QPalette::Dark, QColor(Qt::transparent));
            p.setColor(QPalette::Light, QColor(Qt::transparent));
            w->setPalette(p);
        }
    }
}

void ColorDialog::closeEvent(QCloseEvent* event) {
    saveGeometry_();
    QDialog::closeEvent(event);
}

void ColorDialog::hideEvent(QHideEvent* event) {
    saveGeometry_();
    QDialog::hideEvent(event);
}

void ColorDialog::showEvent(QShowEvent* event) {
    restoreGeometry_();
    QDialog::showEvent(event);
}

void ColorDialog::onFinished_(int) {
    saveGeometry_();
}

void ColorDialog::saveGeometry_() {
    isGeometrySaved_ = true;
    savedGeometry_ = geometry();
}

void ColorDialog::restoreGeometry_() {
    if (isGeometrySaved_)
        setGeometry(savedGeometry_);
}

} // namespace vgc::widgets
