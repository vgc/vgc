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

#include <vgc/widgets/panel.h>

#include <QEvent>
#include <QVBoxLayout>

namespace vgc::widgets {

Panel::Panel(const QString& title, QWidget* widget, QWidget* parent)
    : QFrame(parent)
    , widget_(widget) {

    titleBar_ = new PanelTitleBar(title);
    toggleViewAction_ = new ToggleViewAction(title, this, this);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(titleBar_);
    layout->addWidget(widget_);
    setLayout(layout);

    setWindowTitle(title);
}

Panel::~Panel() {
}

bool Panel::event(QEvent* event) {
    QEvent::Type type = event->type();
    if (type == QEvent::ShowToParent || type == QEvent::HideToParent) {
        Q_EMIT visibleToParentChanged();
    }
    return QFrame::event(event);
}

} // namespace vgc::widgets
