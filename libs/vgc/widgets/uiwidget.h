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

#ifndef VGC_WIDGETS_UIWIDGET_H
#define VGC_WIDGETS_UIWIDGET_H

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include <vgc/core/array.h>
#include <vgc/graphics/idgenerator.h>
#include <vgc/ui/internal/qopenglengine.h>
#include <vgc/ui/widget.h>
#include <vgc/ui/window.h>
#include <vgc/widgets/api.h>

namespace vgc {
namespace widgets {

VGC_DECLARE_OBJECT(UiWidget);

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
    UiWidget(ui::WidgetPtr widget, QWidget* parent = nullptr);

    /// Destructs the UiWidget.
    ///
    ~UiWidget() override;

    /// Returns the underlying vgc::ui::Widget
    ///
    ui::Widget* widget() { return widget_.get(); }

    // overrides
    QSize sizeHint() const override;
    QVariant inputMethodQuery(Qt::InputMethodQuery querty) const override;

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent* event) override;
#else
    void enterEvent(QEnterEvent* event) override;
#endif
    void leaveEvent(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    bool event(QEvent* e) override;

private:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void cleanupGL();

    void onRepaintRequested();
    void onFocusRequested();

    ui::WidgetPtr widget_;
    ui::internal::QglEnginePtr engine_;

    // Projection and view matrices
    geometry::Mat4f proj_;

    // Ensure that we don't call onPaintDestroy() if onPaintCreate()
    // has not been called
    bool isInitialized_;
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_UIWIDGET_H
