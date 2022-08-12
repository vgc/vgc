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

#ifndef VGC_WIDGETS_PANEL_H
#define VGC_WIDGETS_PANEL_H

#include <QFrame>

#include <vgc/widgets/api.h>
#include <vgc/widgets/paneltitlebar.h>
#include <vgc/widgets/toggleviewaction.h>

namespace vgc::widgets {

/// \class vgc::widgets::Panel
/// \brief A widget that can be added to a PanelArea, typically on the side
///        of the CentralWidget.
///
/// This class is similar in spirit to QDockWidget, but reimplemented to fit
/// the overall interface design of VGC. Panels are typically created by calling
/// CentralWidget::addPanel().
///
// Note: QFrame defines the enum value QFrame::Panel. Beware of name conflicts!
//
class VGC_WIDGETS_API Panel : public QFrame {
private:
    Q_OBJECT

public:
    /// Constructs a Panel wrapping the given widget.
    ///
    /// The window title of the Panel is set to title. This title will appear
    /// in the title bar above the wrapped widget, and in any menu where the
    /// toggleViewAction() is inserted.
    ///
    explicit Panel(const QString& title, QWidget* widget, QWidget* parent = nullptr);

    /// Destructs the Panel.
    ///
    ~Panel() override;

    /// Returns the widget wrapped by this Panel.
    ///
    QWidget* widget() const {
        return widget_;
    }

    /// Returns a checkable action that can be used to show or hide this panel.
    ///
    /// The action's text is set to the title given in the constructor of this
    /// Panel.
    ///
    ToggleViewAction* toggleViewAction() const {
        return toggleViewAction_;
    }

Q_SIGNALS:
    /// This signal is emitted whenever this Panel is shown or hidden relative
    /// to its parent.
    ///
    /// \sa isVisibleTo(), QEvent::ShowToParent, QEvent::HideToParent.
    ///
    void visibleToParentChanged();

protected:
    /// Reimplements QFrame::event().
    ///
    virtual bool event(QEvent* event) override;

private:
    PanelTitleBar* titleBar_;
    QWidget* widget_;
    ToggleViewAction* toggleViewAction_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_PANEL_H
