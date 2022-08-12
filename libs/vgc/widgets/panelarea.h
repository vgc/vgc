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

#ifndef VGC_WIDGETS_PANELAREA_H
#define VGC_WIDGETS_PANELAREA_H

#include <vector>

#include <QFrame>
#include <QLayout>

#include <vgc/widgets/api.h>
#include <vgc/widgets/panel.h>

namespace vgc::widgets {

class Panel;

/// \class vgc::widgets::PanelArea
/// \brief An area where Panel widgets can be added or removed.
///
// Note: QFrame defines the enum value QFrame::Panel. Beware of name conflicts!
//
class VGC_WIDGETS_API PanelArea : public QFrame {
private:
    Q_OBJECT

public:
    /// Constructs a PanelArea.
    ///
    explicit PanelArea(QWidget* parent = nullptr);

    /// Destructs the PanelArea.
    ///
    ~PanelArea() override;

    /// Adds a Panel to this PanelArea, wrapping the given widget.
    ///
    widgets::Panel* addPanel(const QString& title, QWidget* widget);

    /// Returns the panel wrapping the given widget, or nullptr if not found.
    ///
    widgets::Panel* panel(QWidget* widget);

Q_SIGNALS:
    /// This signal is emitted whenever this PanelArea is shown or hidden
    /// relative to its parent.
    ///
    /// \sa isVisibleTo(), QEvent::ShowToParent, QEvent::HideToParent.
    ///
    void visibleToParentChanged();

protected:
    /// Reimplements QFrame::event().
    ///
    virtual bool event(QEvent* event) override;

private Q_SLOTS:
    void onPanelVisibleToParentChanged_();

private:
    QLayout* layout_;
    std::vector<widgets::Panel*> panels_;
    void updateVisibility_();
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_PANELAREA_H
