// Copyright 2020 The VGC Developers
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

#ifndef VGC_WIDGETS_UIWIDGET_H
#define VGC_WIDGETS_UIWIDGET_H

#include <QOpenGLWidget>

#include <vgc/ui/widget.h>
#include <vgc/widgets/api.h>

namespace vgc {
namespace widgets {

/// \class vgc::widget::UiWidget
/// \brief A QWidget based on a vgc::ui::widget
///
/// This class is temporary glue code between QtWidgets and vgc::ui,
/// which we will use while we haven't yet completely removed the dependency
/// to QtWidgets.
///
class VGC_WIDGETS_API UiWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    /// Constructs a UiWidget wrapping the given vgc::ui::Widget.
    ///
    UiWidget(ui::WidgetSharedPtr widget, QWidget* parent = nullptr);

    /// Destructs the UiWidget.
    ///
    ~UiWidget() override;

    /// Returns the underlying vgc::ui::Widget
    ///
    ui::Widget* widget() { return widget_.get(); }

private:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void cleanupGL();

    ui::WidgetSharedPtr widget_;
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_UIWIDGET_H
