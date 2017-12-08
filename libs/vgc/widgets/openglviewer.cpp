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

#include <vgc/widgets/openglviewer.h>

#include <cmath>
#include <QMouseEvent>
#include <vgc/core/resources.h>

namespace vgc {
namespace widgets {

namespace {

// Returns the file path of a shader file as a QString
QString shaderPath_(const std::string& name) {
    std::string path = core::resourcePath("shaders/" + name);
    return QString::fromStdString(path);
}

} // end namespace

void OpenGLViewer::init()
{
    // Set OpenGL 3.2, core profile
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(16);
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
}

OpenGLViewer::OpenGLViewer(QWidget *parent) :
    QOpenGLWidget(parent)
{
    // Set view matrix
    viewMatrix_.setToIdentity();
}

OpenGLViewer::~OpenGLViewer()
{
    // Make OpenGL context current and call cleanupGL()
    makeCurrent();
    cleanupGL();
    doneCurrent();
}

void OpenGLViewer::mousePressEvent(QMouseEvent* /*event*/)
{
    // Create new curve
    curves_.push_back(Curve());
}

void OpenGLViewer::mouseMoveEvent(QMouseEvent* event)
{
    // Append new point to last curve
    if (curves_.size() > 0) {
        curves_.back().push_back(event->pos());
        repaint();
    }
}

void OpenGLViewer::mouseReleaseEvent(QMouseEvent* /*event*/)
{
    // Nothing to do
}

OpenGLViewer::OpenGLFunctions* OpenGLViewer::openGLFunctions() const
{
    return context()->versionFunctions<OpenGLFunctions>();
}

void OpenGLViewer::initializeGL()
{
    OpenGLFunctions* f = openGLFunctions();

    // Initialize shader program
    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Vertex, shaderPath_("shader.v.glsl"));
    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Fragment, shaderPath_("shader.f.glsl"));
    shaderProgram_.link();

    // Get shader locations
    shaderProgram_.bind();
    vertexLoc_     = shaderProgram_.attributeLocation("vertex");
    projMatrixLoc_ = shaderProgram_.uniformLocation("projMatrix");
    viewMatrixLoc_ = shaderProgram_.uniformLocation("viewMatrix");
    shaderProgram_.release();

    // Create VBO
    vbo_.create();

    // Create VAO
    vao_.create();
    GLsizei  stride  = sizeof(GLVertex);
    GLvoid* pointer = reinterpret_cast<void*>(offsetof(GLVertex, x));
    vao_.bind();
    vbo_.bind();
    f->glEnableVertexAttribArray(vertexLoc_);
    f->glVertexAttribPointer(
                vertexLoc_, // index of the generic vertex attribute
                2,          // number of components   (x and y components)
                GL_FLOAT,   // type of each component
                GL_FALSE,   // should it be normalized
                stride,     // byte offset between consecutive vertex attributes
                pointer);   // byte offset between the first attribute and the pointer given to allocate()
    vbo_.release();
    vao_.release();

    // Set clear color
    f->glClearColor(1, 1, 1, 1);
}

#undef near
#undef far

void OpenGLViewer::resizeGL(int w, int h)
{
    // Set projection matrix
    float left   = 0.0f;
    float right  = w;
    float bottom = h;
    float top    = 0.0f;
    float near   = -1.0f;
    float far    = 1.0f;
    projMatrix_.setToIdentity();
    projMatrix_.ortho(left, right, bottom, top, near, far);
}

void OpenGLViewer::paintGL()
{
    OpenGLFunctions* f = openGLFunctions();

    // Update VBO
    // Note: for simplicity, we perform this at each paintGL. In an actual app,
    // it would be much better to do this only once after each mouse event.
    computeGLVertices_();
    vbo_.bind();
    vbo_.allocate(glVertices_.data(), glVertices_.size() * sizeof(GLVertex));
    vbo_.release();

    // Clear color and depth buffer
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind shader program
    shaderProgram_.bind();

    // Set uniform values
    shaderProgram_.setUniformValue(projMatrixLoc_, projMatrix_);
    shaderProgram_.setUniformValue(viewMatrixLoc_, viewMatrix_);

    // Draw triangles
    vao_.bind();
    int firstIndex = 0;
    for (const Curve& curve: curves_) {
        size_t n = curve.size();
        if (n >= 2) {
            int nIndices = 2 * n;
            f->glDrawArrays(GL_TRIANGLE_STRIP, firstIndex, nIndices);
            firstIndex += nIndices;
        }
    }
    vao_.release();

    // Release shader program
    shaderProgram_.release();
}

void OpenGLViewer::cleanupGL()
{
    // Destroy VAO
    vao_.destroy();

    // Destroy VBO
    vbo_.destroy();
}

QPointF OpenGLViewer::computeNormal_(const QPoint& p, const QPoint& q)
{
    // Get difference
    QPointF d = q-p;

    // Normalize difference to get tangent
    const double length = std::sqrt(d.x()*d.x() + d.y() * d.y());
    if (length > 1e-6)
        d /= length;
    else
        d = QPointF(1.0, 0.0);

    // Return vector orthogonal to tangent
    return QPointF(-d.y(), d.x());
}

void OpenGLViewer::computeGLVertices_()
{
    glVertices_.clear();
    float halfwidth = 6.0;
    for (const Curve& curve: curves_)
    {
        size_t n = curve.size();
        if (n >= 2)
        {
            for (unsigned int i=0; i<n; ++i)
            {
                // Compute normal for sample
                QPointF normal;
                if (i==0)
                    normal = computeNormal_(curve[0], curve[1]);
                else if (i == n-1)
                    normal = computeNormal_(curve[n-2], curve[n-1]);
                else
                    normal = computeNormal_(curve[i-1], curve[i+1]);

                // Compute left and right GL vertices from centerline + normal + width
                QPointF leftPos  = curve[i] + halfwidth * normal;
                QPointF rightPos = curve[i] - halfwidth * normal;
                GLVertex leftVertex(leftPos.x(), leftPos.y());
                GLVertex rightVertex(rightPos.x(), rightPos.y());

                // Add vertices to list of vertices
                glVertices_.push_back(leftVertex);
                glVertices_.push_back(rightVertex);
            }
        }
    }
}

} // namespace widgets
} // namespace vgc
