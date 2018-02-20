// Copyright 2017 The VGC Developers
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

#include <vector>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include <vgc/core/color.h>
#include <vgc/geometry/camera2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/scene/scene.h>

namespace vgc {
namespace widgets {

class OpenGLViewer : public QOpenGLWidget
{
private:
    Q_OBJECT
    Q_DISABLE_COPY(OpenGLViewer)

public:
    /// This function must be called before creating the first
    /// OpenGLViewer. It sets the appropriate Qt OpenGLFormat.
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

    // Shader program
    QOpenGLShaderProgram shaderProgram_;
    int vertexLoc_;
    int projMatrixLoc_;
    int viewMatrixLoc_;
    int colorLoc_;

    // OpenGL resources
    struct CurveGLResources {
        // Drawing triangles
        QOpenGLBuffer vboTriangles;
        QOpenGLVertexArrayObject* vaoTriangles; // Pointer because copy of QOpenGLVertexArrayObject is disabled
        int numVerticesTriangles;
        core::Color trianglesColor;

        // Drawing control points
        QOpenGLBuffer vboControlPoints;
        QOpenGLVertexArrayObject* vaoControlPoints; // Pointer because copy of QOpenGLVertexArrayObject is disabled
        int numVerticesControlPoints;
    };
    std::vector<CurveGLResources> curveGLResources_;
    void updateGLResources_();
    void createCurveGLResources_(int i);
    void updateCurveGLResources_(int i);
    void destroyCurveGLResources_(int i);

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

    // Tesselation mode. This is selected with the i/u/a keys.
    // XXX This is a temporary quick method to switch between
    // tesselation modes. A more engineered method will come later.
    int requestedTesselationMode_; // 0: none; 1: uniform; 2: adaptive
    int currentTesselationMode_;
};

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_OPENGLVIEWER_H
