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

#include <vgc/widgets/toggleviewaction.h>

namespace vgc::widgets {

ToggleViewAction::ToggleViewAction(const QString& text, QWidget* widget, QObject* parent)
    : QAction(text, parent)
    , widget_(widget) {

    setCheckable(true);
    setChecked(widget->isVisibleTo(widget->parentWidget()));
    connect(this, &QAction::toggled, this, &ToggleViewAction::onToggled_);

    // TODO we should add an eventFilter that listens to the ShowToParent and
    // HideToParent events of widget, so that we can react accordingly when the
    // value of `widget->isVisibleTo(widget->parentWidget())` changes due to
    // some code calling `widget->show()` or `widget->hide()` directly, without
    // using this ToggleViewAction.

    // TODO We should also probably listen to ParentChange and/or
    // ParentAboutToChange. See comment in the implementation of
    // PanelArea::addPanel().
}

void ToggleViewAction::onToggled_(bool checked) {
    widget_->setVisible(checked);
}

} // namespace vgc::widgets
