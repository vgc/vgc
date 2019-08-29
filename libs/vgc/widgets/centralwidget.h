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

#ifndef VGC_WIDGETS_CENTRALWIDGET_H
#define VGC_WIDGETS_CENTRALWIDGET_H

#include <vector>

#include <QWidget>

#include <vgc/widgets/api.h>

namespace vgc {
namespace widgets {

/// \class vgc::widgets::CentralWidget
/// \brief The central widget of the MainWindow, providing toolbars and docks.
///
class VGC_WIDGETS_API CentralWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs a CentralWidget.
    ///
    explicit CentralWidget(
        QWidget* viewer,
        QWidget* toolbar,
        QWidget* console,
        QWidget* panel,
        QWidget* parent = nullptr);

    /// Destructs the CentralWidget.
    ///
    ~CentralWidget() override;

    /// Reimplements QWidget::sizeHint().
    ///
    QSize sizeHint() const override;

    /// Returns the toggle view action for the console.
    ///
    QAction* consoleToggleViewAction() const {
        return consoleToggleViewAction_;
    }

    /// Returns the toggle view action for the panel.
    ///
    QAction* panelToggleViewAction() const {
        return panelToggleViewAction_;
    }

protected:
    /// Reimplements QWidget::paintEvent().
    ///
    void paintEvent(QPaintEvent*) override;

    /// Reimplements QWidget::resizeEvent().
    ///
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void updateGeometries_();

private:
    // Ad-hoc widgets and sizes for now. Will change to more generic
    // splitting mechanism later.
    QWidget* viewer_;
    QWidget* toolbar_;
    QWidget* console_;
    QWidget* panel_;
    QAction* consoleToggleViewAction_;
    QAction* panelToggleViewAction_;
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_MENUBAR_H
