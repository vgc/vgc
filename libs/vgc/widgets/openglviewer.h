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

#ifndef VGC_WIDGETS_OPENGLVIEWER_H
#define VGC_WIDGETS_OPENGLVIEWER_H

#include <list>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/performancelog.h>
#include <vgc/dom/document.h>
#include <vgc/dom/element.h>
#include <vgc/geometry/camera2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/widgets/api.h>
#include <vgc/widgets/pointingdeviceevent.h>

namespace vgc::dom {
class Element;
}

namespace vgc::widgets {

class VGC_WIDGETS_API OpenGLViewer : public QOpenGLWidget {
private:
    Q_OBJECT
    Q_DISABLE_COPY(OpenGLViewer)

public:
    /// This function must be called before creating the first
    /// OpenGLViewer. It sets the appropriate Qt OpenGLFormat.
    ///
    static void init();

    OpenGLViewer(dom::Document* document, QWidget* parent = nullptr);
    ~OpenGLViewer();

    /// Reimplements QWidget::minimumSizeHint().
    ///
    QSize minimumSizeHint() const override;

    dom::Document* document() const {
        return document_;
    }

    // Changes the observed document
    void setDocument(dom::Document* document);

    // XXX temporary. WIll be deferred to separate class.
    void setCurrentColor(const core::Color& color) {
        currentColor_ = color;
    }

    /// Creates and manages new performance logs as children of the given \p
    /// parent.
    ///
    void startLoggingUnder(core::PerformanceLog* parent);

    /// Destroys the currently managed logs whose parent is the given \p
    /// parent, if any.
    ///
    void stopLoggingUnder(core::PerformanceLog* parent);

Q_SIGNALS:
    /// This signal is emitted when a render is completed.
    ///
    void renderCompleted();

private:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void tabletEvent(QTabletEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void pointingDevicePress(const PointingDeviceEvent& event);
    void pointingDeviceMove(const PointingDeviceEvent& event);
    void pointingDeviceRelease(const PointingDeviceEvent& event);

private:
    using OpenGLFunctions = QOpenGLFunctions_3_2_Core;
    OpenGLFunctions* openGLFunctions() const;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void cleanupGL();

private:
    void computeGLVertices_();

private:
    // Camera (provides view matrix + projection matrix)
    geometry::Camera2d camera_;

    // Scene
    dom::Document* document_;
    core::UndoGroup* drawCurveUndoGroup_ = nullptr;
    core::ConnectionHandle documentChangedConnectionHandle_;

    void onDocumentChanged_(const dom::Diff& diff);

    // Moving camera
    bool isSketching_ = false;
    bool isPanning_ = false;
    bool isRotating_ = false;
    bool isZooming_ = false;
    geometry::Vec2d pointingDevicePosAtPress_ = {};
    geometry::Camera2d cameraAtPress_;

    // Shader program
    QOpenGLShaderProgram shaderProgram_;
    int vertexLoc_ = -1;
    int projMatrixLoc_ = -1;
    int viewMatrixLoc_ = -1;
    int colorLoc_ = -1;

    // OpenGL resources
    struct CurveGLResources {
        explicit CurveGLResources(dom::Element* element)
            : element(element) {
        }

        // Drawing triangles
        QOpenGLBuffer vboTriangles;
        QOpenGLVertexArrayObject*
            vaoTriangles; // Pointer because copy of QOpenGLVertexArrayObject is disabled
        GLsizei numVerticesTriangles;
        core::Color trianglesColor;

        // Drawing control points
        QOpenGLBuffer vboControlPoints;
        QOpenGLVertexArrayObject*
            vaoControlPoints; // Pointer because copy of QOpenGLVertexArrayObject is disabled
        GLsizei numVerticesControlPoints;

        bool inited_ = false;
        dom::Element* element;
    };

    using CurveGLResourcesIterator = std::list<CurveGLResources>::iterator;

    std::list<CurveGLResources> curveGLResources_; // in draw order
    std::list<CurveGLResources> removedGLResources_;
    std::map<dom::Element*, CurveGLResourcesIterator> curveGLResourcesMap_;

    struct unwrapped_less {
        template<typename It>
        bool operator()(const It& lh, const It& rh) const {
            return &*lh < &*rh;
        }
    };

    std::set<CurveGLResourcesIterator, unwrapped_less> toUpdate_;
    bool needsSort_ = false;

    void updateGLResources_();
    CurveGLResourcesIterator appendCurveGLResources_(dom::Element* element);
    void updateCurveGLResources_(CurveGLResources& r);
    static void destroyCurveGLResources_(CurveGLResources& r);

    // Make sure to disallow concurrent usage of the mouse and the tablet to
    // avoid conflicts. This also acts as a work around the following Qt bugs:
    // 1. At least in Linus/X11, mouse events are generated even when tablet
    //    events are accepted.
    // 2. At least in Linux/X11, a TabletRelease is sometimes followed by both a
    //    MouseMove and a MouseRelease, see https://github.com/vgc/vgc/issues/9.
    //
    // We also disallow concurrent usage of different mouse buttons, in
    // particular:
    // 1. We ignore mousePressEvent() if there has already been a
    //    mousePressEvent() with another event->button() and no matching
    //    mouseReleaseEvent().
    // 2. We ignore mouseReleaseEvent() if the value of event->button() is
    //    different from its value in mousePressEvent().
    //
    bool mousePressed_;  // whether there's been a mouse press event
                         // with no matching mouse release event.
    bool tabletPressed_; // whether there's been a tablet press event
                         // with no matching tablet release event.
    Qt::MouseButton pointingDeviceButtonAtPress_; // value of event->button at press

    // Polygon mode. This is selected with the n/t/f keys.
    // XXX This is a temporary quick method to switch between
    // render modes. A more engineered method will come later.
    int polygonMode_; // 0: fill; 1: lines (i.e., not exactly like OpenGL)

    // Show control points. This is toggled with the "p" key.
    // XXX This is a temporary quick method to switch between
    // render modes. A more engineered method will come later.
    bool showControlPoints_;

    // Tesselation mode. This is selected with the i/u/a keys.
    // XXX This is a temporary quick method to switch between
    // tesselation modes. A more engineered method will come later.
    int requestedTesselationMode_; // 0: none; 1: uniform; 2: adaptive
    int currentTesselationMode_;

    // XXX This is a temporary test, will be deferred to separate classes. Here
    // is an example of how responsibilities could be separated:
    //
    // Widget: Creates an OpenGL context, receive graphical user input.
    // Renderer: Renders the document to the given OpenGL context.
    // Controller: Modifies the document based on user input (could be in the
    // form of "Action" instances).
    //
    void startCurve_(const geometry::Vec2d& p, double width = 1.0);
    void continueCurve_(const geometry::Vec2d& p, double width = 1.0);
    core::Color currentColor_;

    // Performance logging
    core::PerformanceLogTask renderTask_;
    core::PerformanceLogTask updateTask_;
    core::PerformanceLogTask drawTask_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_OPENGLVIEWER_H
