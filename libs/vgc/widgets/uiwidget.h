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

#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLWidget>

#include <vgc/ui/widget.h>
#include <vgc/widgets/api.h>

namespace vgc {
namespace widgets {

VGC_CORE_DECLARE_PTRS(UiWidget);
VGC_CORE_DECLARE_PTRS(UiWidgetEngine);

/// \class vgc::widget::UiWidget
/// \brief A QWidget based on a vgc::ui::widget.
///
/// This class is temporary glue code between QtWidgets and vgc::ui,
/// which we will use while we haven't yet completely removed the dependency
/// to QtWidgets.
///
class VGC_WIDGETS_API UiWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    using OpenGLFunctions = QOpenGLFunctions_3_2_Core;

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
    OpenGLFunctions* openGLFunctions() const;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void cleanupGL();

    ui::WidgetSharedPtr widget_;
    widgets::UiWidgetEngineSharedPtr engine_;
};

/// \class vgc::widget::UiWidgetEngine
/// \brief The graphics::Engine used by UiWidget.
///
/// This class is an implementation of graphics::Engine using QPainter and
/// OpenGL calls, with the assumption to be used in a QOpenGLWidget.
///
class VGC_WIDGETS_API UiWidgetEngine: public graphics::Engine
{
    VGC_CORE_OBJECT(UiWidgetEngine)

public:
    /// Creates a new UiWidgetEngine. This constructor is an implementation
    /// detail. In order to create a UiWidgetEngine, please use the following:
    ///
    /// \code
    /// UiWidgetEngineSharedPtr engine = UiWidgetEngine::create();
    /// \endcode
    ///
    UiWidgetEngine(const ConstructorKey&);

    /// Creates a new document with no root element.
    ///
    static UiWidgetEngineSharedPtr create();

    /// Sets the OpenGLFunctions to be used by this UiWidgetEngine. This must
    /// be set before any of abstract API from graphics::Engine are called.
    ///
    void setOpenGLFunctions(UiWidget::OpenGLFunctions* functions) {
        openGLFunctions_ = functions;
    }

    /// Implements graphics::Engine::clear().
    ///
    void clear(const core::Color& color) override;

private:
    UiWidget::OpenGLFunctions* openGLFunctions_;
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_UIWIDGET_H
