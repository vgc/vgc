// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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

#include <vector>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <vgc/geometry/camera2d.h>
#include <vgc/geometry/vec2d.h>

namespace vgc {

namespace scene { class Scene; }

namespace widgets {

class OpenGLViewer : public QOpenGLWidget
{
private:
    Q_OBJECT
    Q_DISABLE_COPY(OpenGLViewer)

public:
    /// This function must be called before creating the first
    /// OpenGLViewer. Its sets the appropriate Qt OpenGLFormat.
    ///
    static void init();

    OpenGLViewer(scene::Scene* scene, QWidget* parent = nullptr);
    ~OpenGLViewer();

    scene::Scene* scene() const {
        return scene_;
    }

private:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void tabletEvent(QTabletEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

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
    scene::Scene* scene_;

    // Moving camera
    bool isSketching_;
    bool isPanning_;
    bool isRotating_;
    bool isZooming_;
    geometry::Vec2d mousePosAtPress_;
    geometry::Camera2d cameraAtPress_;

    // RAM resources synced with GL resources
    struct GLVertex {
        float x, y;
        GLVertex(float x, float y) : x(x), y(y) {}
    };
    std::vector<GLVertex> glVertices_;
    std::vector<int> glVerticesChunkSizes_;

    // GL resources
    QOpenGLShaderProgram shaderProgram_;
    QOpenGLBuffer vbo_;
    QOpenGLVertexArrayObject vao_;

    // Shader locations
    int vertexLoc_;
    int projMatrixLoc_;
    int viewMatrixLoc_;

    // Handle mouse/tablet events
    double width_() const;
    bool isTabletEvent_;
    double tabletPressure_;

    // Polygon mode. This is selected with the n/t/f keys.
    // XXX This is a temporary quick method to switch between
    // render modes. A more engineered method will come later.
    int polygonMode_; // 0: none; 1: lines; 2: fill (i.e., not exactly like OpenGL)

    // Show control points. This is toggled with the "c" key.
    // XXX This is a temporary quick method to switch between
    // render modes. A more engineered method will come later.
    bool showControlPoints_;
    std::vector<GLVertex> controlPointsGlVertices_;
    QOpenGLBuffer controlPointsVbo_;
    QOpenGLVertexArrayObject controlPointsVao_;
    void computeControlPointsGLVertices_();

    // Tesselation mode. This is selected with the i/u/a keys.
    // XXX This is a temporary quick method to switch between
    // tesselation modes. A more engineered method will come later.
    int tesselationMode_; // 0: none; 1: uniform; 2: adaptive

};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_OPENGLVIEWER_H
