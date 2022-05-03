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

#ifndef VGC_UI_WINDOW_H
#define VGC_UI_WINDOW_H

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QWindow>

#include <vgc/core/array.h>
#include <vgc/graphics/idgenerator.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Window);

/// \class vgc::ui::Window
/// \brief A window able to contain a vgc::ui::widget.
///
class VGC_UI_API Window : public core::Object, public QWindow
{
    // optional
    //Q_OBJECT

public:
    /// Constructs a UiWidget wrapping the given vgc::ui::Widget.
    ///
    Window(ui::WidgetPtr widget);

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

    QOpenGLFunctions_3_2_Core* oglf_;

    virtual void paint();
    virtual void cleanup();

    // Projection and view matrices
    core::Mat4f proj_;

    // Shader program
    QOpenGLShaderProgram shaderProgram_;
    QOpenGLContext* m_context = nullptr;

    int posLoc_;
    int colLoc_;
    int projLoc_;
    int viewLoc_;
};

} // namespace vgc::ui

#endif // VGC_UI_WINDOW_H
