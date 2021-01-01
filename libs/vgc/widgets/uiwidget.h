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
#include <vgc/ui/widget.h>
#include <vgc/widgets/api.h>

namespace vgc {
namespace widgets {

VGC_DECLARE_OBJECT(UiWidget);
VGC_DECLARE_OBJECT(UiWidgetEngine);

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

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    OpenGLFunctions* openGLFunctions() const;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void cleanupGL();

    void onRepaintRequested();

    ui::WidgetPtr widget_;
    widgets::UiWidgetEnginePtr engine_;

    // Projection and view matrices
    core::Mat4f proj_;

    // Shader program
    QOpenGLShaderProgram shaderProgram_;
    int posLoc_;
    int colLoc_;
    int projLoc_;
    int viewLoc_;

    // Ensure that we don't call onPaintDestroy() if onPaintCreate()
    // has not been called
    bool isInitialized_;
};

/// \class vgc::widget::UiWidgetEngine
/// \brief The graphics::Engine used by UiWidget.
///
/// This class is an implementation of graphics::Engine using QPainter and
/// OpenGL calls, with the assumption to be used in a QOpenGLWidget.
///
class VGC_WIDGETS_API UiWidgetEngine : public graphics::Engine {
private:
    VGC_OBJECT(UiWidgetEngine)

protected:
    /// Creates a new UiWidgetEngine. This constructor is an implementation
    /// detail. In order to create a UiWidgetEngine, please use the following:
    ///
    /// \code
    /// UiWidgetEnginePtr engine = UiWidgetEngine::create();
    /// \endcode
    ///
    UiWidgetEngine();

public:
    /// Creates a new UiWidgetEngine.
    ///
    static UiWidgetEnginePtr create();

    /// Initializes the engine with the given OpenGL parameters required to
    /// perform its OpenGL calls.
    ///
    void initialize(UiWidget::OpenGLFunctions* functions,
                    QOpenGLShaderProgram* shaderProgram,
                    int posLoc, int colLoc, int projLoc, int viewLoc);

    // Implementation of graphics::Engine API
    void clear(const core::Color& color) override;
    core::Mat4f projectionMatrix() override;
    void setProjectionMatrix(const core::Mat4f& m) override;
    void pushProjectionMatrix() override;
    void popProjectionMatrix() override;
    core::Mat4f viewMatrix() override;
    void setViewMatrix(const core::Mat4f& m) override;
    void pushViewMatrix() override;
    void popViewMatrix() override;
    Int createTriangles() override;
    void loadTriangles(Int id, const float* data, Int length) override;
    void drawTriangles(Int id) override;
    void destroyTriangles(Int id) override;

private:
    UiWidget::OpenGLFunctions* openGLFunctions_;
    QOpenGLShaderProgram* shaderProgram_;
    int posLoc_;
    int colLoc_;
    int projLoc_;
    int viewLoc_;
    core::Array<core::Mat4f> projectionMatrices_;
    core::Array<core::Mat4f> viewMatrices_;
    graphics::IdGenerator trianglesIdGenerator_;
    struct TrianglesBuffer {
        // Note: we use a pointer for the VAO because copying
        // QOpenGLVertexArrayObject is disabled
        QOpenGLBuffer vboTriangles;
        QOpenGLVertexArrayObject* vaoTriangles;
        int numVertices;
    };
    core::Array<TrianglesBuffer> trianglesBuffers_;
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_UIWIDGET_H
