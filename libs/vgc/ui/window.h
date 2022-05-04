// Copyright 2022 The VGC Developers
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

#ifndef VGC_UI_WINDOW_H
#define VGC_UI_WINDOW_H

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QWindow>

#include <vgc/core/array.h>
#include <vgc/graphics/idgenerator.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Window);
VGC_DECLARE_OBJECT(QWindowEngine);


/// \class vgc::widget::QWindowEngine
/// \brief The graphics::Engine for windows and widgets.
///
/// This class is an implementation of graphics::Engine using QOpenGLContext and
/// OpenGL calls.
///
class VGC_UI_API QWindowEngine : public graphics::Engine {
private:
    VGC_OBJECT(QWindowEngine, graphics::Engine)

protected:
    QWindowEngine();

public:
    using OpenGLFunctions = QOpenGLFunctions_3_2_Core;

    /// Creates a new OpenglEngine.
    ///
    static QWindowEnginePtr create();

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

    bool setViewport(Int x, Int y, Int width, Int height);

protected:
    bool initContext();

private:
    QOpenGLContext* ctx_ = nullptr;
    QOpenGLFunctions_3_2_Core* api_ = nullptr;

    // Shader
    QOpenGLShaderProgram shaderProgram_;
    int posLoc_;
    int colLoc_;
    int projLoc_;
    int viewLoc_;

    // Matrices
    core::Mat4f proj_;
    core::Array<core::Mat4f> projectionMatrices_;
    core::Array<core::Mat4f> viewMatrices_;

    // Triangles
    struct TrianglesBuffer {
        // Note: we use a pointer for the VAO because copying
        // QOpenGLVertexArrayObject is disabled
        QOpenGLBuffer vboTriangles;
        QOpenGLVertexArrayObject* vaoTriangles;
        int numVertices;
    };
    core::Array<TrianglesBuffer> trianglesBuffers_;
    graphics::IdGenerator trianglesIdGenerator_;
};


/// \class vgc::ui::Window
/// \brief A window able to contain a vgc::ui::widget.
///
class VGC_UI_API Window : public core::Object, public QWindow {
    // optional
    //Q_OBJECT

protected:
    /// Constructs a UiWidget wrapping the given vgc::ui::Widget.
    ///
    Window(ui::WidgetPtr widget);

public:
    /// Destructs the UiWidget.
    ///
    ~Window() noexcept override;

    static WindowPtr create(ui::WidgetPtr widget);

    /// Returns the contained vgc::ui::Widget
    ///
    ui::Widget* widget() { return widget_.get(); }

public:
    // overrides
    //QSize sizeHint() const override;
    //QVariant inputMethodQuery(Qt::InputMethodQuery querty) const override;

protected:
    void setupDefaultEngine();

    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    // try filtering event()..
    //void enterEvent(QEvent* event) override;
    //void leaveEvent(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void resizeEvent(QResizeEvent *ev) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    //void inputMethodEvent(QInputMethodEvent* event) override;
    bool event(QEvent* e) override;

private:
    void onRepaintRequested();

    ui::WidgetPtr widget_;
    graphics::EnginePtr engine_;

    // Ensure that we don't call onPaintDestroy() if onPaintCreate()
    // has not been called
    bool isInitialized_;

    /////////////////////////////
    // TO FACTOR OUT
    ////////////////////////////

    virtual void paint();
    virtual void cleanup();
};

} // namespace vgc::ui

#endif // VGC_UI_WINDOW_H
