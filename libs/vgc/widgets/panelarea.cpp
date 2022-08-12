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

#include <vgc/widgets/panelarea.h>

#include <QEvent>
#include <QVBoxLayout>

namespace vgc::widgets {

PanelArea::PanelArea(QWidget* parent)
    : QFrame(parent) {

    layout_ = new QVBoxLayout();
    layout_->setAlignment(Qt::AlignTop);
    setLayout(layout_);
    updateVisibility_();
}

PanelArea::~PanelArea() {
}

widgets::Panel* PanelArea::addPanel(const QString& title, QWidget* widget) {
    // Create new panel.
    // Note: we need to set `this` as parent in the constructor (rather than
    // relying on layout->addWidget()), otherwise its toggleViewAction() won't
    // be initialized to the correct check-state. See comment in the
    // implementation of ToggleViewAction::ToggleViewAction().
    widgets::Panel* panel = new widgets::Panel(title, widget, this);
    panels_.push_back(panel);

    // Listen to panel's visibility change
    connect(
        panel,
        &widgets::Panel::visibleToParentChanged,
        this,
        &PanelArea::onPanelVisibleToParentChanged_);

    // Add to layout and return
    layout_->addWidget(panel);
    updateVisibility_();
    return panel;
}

widgets::Panel* PanelArea::panel(QWidget* widget) {
    if (widget) {
        for (widgets::Panel* panel : panels_) {
            if (panel->widget() == widget) {
                return panel;
            }
        }
    }
    return nullptr;
}

bool PanelArea::event(QEvent* event) {
    QEvent::Type type = event->type();
    if (type == QEvent::ShowToParent || type == QEvent::HideToParent) {
        Q_EMIT visibleToParentChanged();
    }
    return QFrame::event(event);
}

void PanelArea::onPanelVisibleToParentChanged_() {
    updateVisibility_();
}

void PanelArea::updateVisibility_() {
    // Check whether any of the panels is visible
    bool hasVisibleChildren = false;
    for (widgets::Panel* panel : panels_) {
        if (panel->isVisibleTo(this)) {
            hasVisibleChildren = true;
        }
    }

    // Show this PanelArea if at least one panel is visible, and
    // hide this PanelArea if no more panels are visible.
    if (isVisibleTo(parentWidget()) != hasVisibleChildren) {
        setVisible(hasVisibleChildren);
    }
}

} // namespace vgc::widgets
