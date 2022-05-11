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

#include <vgc/ui/internal/qopenglengine.h>

#include <vgc/core/exceptions.h>
#include <vgc/core/paths.h>

namespace vgc::ui::internal {

/*
DESIGN PSEUDOCODE:
Context {
int id;
}
Engine {
Context* ctx;
}

Foo = Engine or Context

Widget {
Foo ref;
rid resourceId;

paintDrawInternal(Engine* drawingEngine) {
if (drawingEngine.ref != ref) {
onPaintDestroy(ref);
ctx = drawingEngine->ctx;
onPaintCreate(drawingEngine);
ctx->onDestroy().connect(this->onCtxDestroyed());
}

paintDraw(drawingEngine);
}

void onCtxDestroyed_(Context*) {
onPaintDestroy(ctx);
}
VGC_SLOT(onCtxDestroyed, onCtxDestroyed_);

virtual onPaintCreate(Engine*);
virtual onPaintDraw(Engine*);
virtual onPaintDestroy(Foo*);
}

*/

namespace {

// Returns the file path of a shader file as a QString
QString shaderPath_(const std::string& name) {
    std::string path = core::resourcePath("graphics/opengl/" + name);
    return toQt(path);
}

struct XYRGBVertex {
    float x, y, r, g, b;
};

} // namespace

class QOpenglTrianglesBuffer;
using QOpenglTrianglesBufferPtr = std::shared_ptr<QOpenglTrianglesBuffer>;

class QOpenglTrianglesBuffer : public graphics::TrianglesBuffer {
private:
    explicit QOpenglTrianglesBuffer(QOpenglEngine* engine);

public:
    ~QOpenglTrianglesBuffer() {
        release_();
    }

    [[nodiscard]] static QOpenglTrianglesBufferPtr create(QOpenglEngine* engine) {
        return std::shared_ptr<QOpenglTrianglesBuffer>(new QOpenglTrianglesBuffer(engine));
    }

    void load(const float* data, Int length) override;
    void draw() override;

    void bind() {
        if (vao_) {
            vao_->bind();
            vbo_.bind();
        }
    }

    void unbind() {
        if (vao_) {
            vbo_.release();
            vao_->release();
        }
    }

    QOpenglEngine* engine() const {
        return static_cast<QOpenglEngine*>(TrianglesBuffer::engine());
    }

private:
    QOpenGLBuffer vbo_;
    QOpenGLVertexArrayObject* vao_ = nullptr;
    Int numVertices_ = 0;
    Int allocSize_ = 0;

    void release_();
    void release() override;
};

QOpenglTrianglesBuffer::QOpenglTrianglesBuffer(QOpenglEngine* engine) :
    graphics::TrianglesBuffer(engine)
{
    // Create VBO/VAO for rendering triangles
    vbo_.create();
    vao_ = new QOpenGLVertexArrayObject();
    vao_->create();
}

void QOpenglTrianglesBuffer::load(const float* data, Int length)
{
    if (!vao_) return;
    if (length < 0) {
        throw core::NegativeIntegerError(core::format(
            "Negative length ({}) provided to loadTriangles()", length));
    }
    numVertices_ = length / 5;
    Int dataSize = numVertices_ * sizeof(XYRGBVertex);
    static_assert(sizeof(XYRGBVertex) == 5 * sizeof(float));

    vbo_.bind();
    if (dataSize > allocSize_) {
        vbo_.allocate(data, dataSize);
        allocSize_ = dataSize;
    }
    else if (dataSize * 2 < allocSize_) {
        vbo_.allocate(data, dataSize);
        allocSize_ = dataSize;
    }
    else {
        vbo_.write(0, data, dataSize);
    }
    vbo_.release();
}

void QOpenglTrianglesBuffer::draw()
{
    if (!vao_) return;
    vao_->bind();
    auto api = engine()->api();
    api->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    api->glDrawArrays(GL_TRIANGLES, 0, numVertices_);
    vao_->release();
}

void QOpenglTrianglesBuffer::release_()
{
    if (!vao_) return;
    vao_->destroy();
    delete vao_;
    vao_ = nullptr;
    vbo_.destroy();
}

void QOpenglTrianglesBuffer::release()
{
    release_();
}

QOpenglEngine::QOpenglEngine() :
    QOpenglEngine(new QOpenGLContext(), false)
{
}

QOpenglEngine::QOpenglEngine(QOpenGLContext* ctx, bool isExternalCtx) :
    graphics::Engine(),
    ctx_(ctx),
    isExternalCtx(isExternalCtx),
    projectionMatrices_({core::Mat4f::identity}),
    viewMatrices_({core::Mat4f::identity})
{
}

void QOpenglEngine::onDestroyed()
{
    if (!isExternalCtx) {
        delete ctx_;
    }
}

/* static */
QOpenglEnginePtr QOpenglEngine::create()
{
    return QOpenglEnginePtr(new QOpenglEngine());
}

/* static */
QOpenglEnginePtr QOpenglEngine::create(QOpenGLContext* ctx)
{
    return QOpenglEnginePtr(new QOpenglEngine(ctx));
}

void QOpenglEngine::clear(const core::Color& color)
{
    api_->glClearColor(
        static_cast<float>(color.r()),
        static_cast<float>(color.g()),
        static_cast<float>(color.b()),
        static_cast<float>(color.a()));
    api_->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

core::Mat4f QOpenglEngine::projectionMatrix()
{
    return projectionMatrices_.last();
}

void QOpenglEngine::setProjectionMatrix(const core::Mat4f& m)
{
    projectionMatrices_.last() = m;
    shaderProgram_.setUniformValue(projLoc_, toQtMatrix(m));
}

void QOpenglEngine::pushProjectionMatrix()
{
    // Note: we need a temporary copy by value, since the address
    // of the current projection matrix might change during "append" in
    // case of memory re-allocation.
    core::Mat4f m = projectionMatrix();
    projectionMatrices_.append(m);
}

void QOpenglEngine::popProjectionMatrix()
{
    projectionMatrices_.removeLast();
    shaderProgram_.setUniformValue(projLoc_, toQtMatrix(projectionMatrices_.last()));
}

core::Mat4f QOpenglEngine::viewMatrix()
{
    return viewMatrices_.last();
}

void QOpenglEngine::setViewMatrix(const core::Mat4f& m)
{
    viewMatrices_.last() = m;
    shaderProgram_.setUniformValue(viewLoc_, toQtMatrix(m));
}

void QOpenglEngine::pushViewMatrix()
{
    // Note: we need a temporary copy by value, since the address
    // of the current view matrix might change during "append" in
    // case of memory re-allocation.
    core::Mat4f m = viewMatrix();
    viewMatrices_.append(m);
}

void QOpenglEngine::popViewMatrix()
{
    viewMatrices_.removeLast();
    shaderProgram_.setUniformValue(viewLoc_, toQtMatrix(viewMatrices_.last()));
}

graphics::TrianglesBufferPtr QOpenglEngine::createTriangles()
{
    auto r = QOpenglTrianglesBuffer::create(this);
    GLsizei stride  = sizeof(XYRGBVertex);
    GLvoid* posPointer = reinterpret_cast<void*>(offsetof(XYRGBVertex, x));
    GLvoid* colPointer = reinterpret_cast<void*>(offsetof(XYRGBVertex, r));
    GLboolean normalized = GL_FALSE;
    r->bind();
    api_->glEnableVertexAttribArray(posLoc_);
    api_->glEnableVertexAttribArray(colLoc_);
    api_->glVertexAttribPointer(posLoc_, 2, GL_FLOAT, normalized, stride, posPointer);
    api_->glVertexAttribPointer(colLoc_, 3, GL_FLOAT, normalized, stride, colPointer);
    r->unbind();
    return r;
}

void QOpenglEngine::setTarget(QSurface* qw)
{
    if (!api_) {
        initContext(qw);
    }
    current_ = qw;
    ctx_->makeCurrent(qw);

#ifdef VGC_QOPENGL_EXPERIMENT
    auto fmt = ctx_->format();
    OutputDebugString(core::format("Ctx swap behavior: {}\n", (int)fmt.swapBehavior()).c_str());
    OutputDebugString(core::format("Ctx swap interval: {}\n", fmt.swapInterval()).c_str());
#endif
}

void QOpenglEngine::setViewport(Int x, Int y, Int width, Int height)
{
    api_->glViewport(x, y, width, height);
}

void QOpenglEngine::bindPaintShader()
{
    shaderProgram_.bind();
}

void QOpenglEngine::releasePaintShader()
{
    shaderProgram_.release();
}

void QOpenglEngine::present()
{
    ctx_->swapBuffers(current_);
}

void QOpenglEngine::initContext(QSurface* qw)
{
    if (ctx_ == nullptr) {
        throw core::LogicError("ctx_ is null.");
    }
    if (api_ != nullptr) {
        throw core::LogicError("already initialized.");
    }

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(8);
    format.setSwapInterval(0);
    ctx_->setFormat(format);
    ctx_->create();

    ctx_->makeCurrent(qw);

    setupContext();
}

void QOpenglEngine::setupContext() {
    // Initialize shader program
    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Vertex, shaderPath_("iv4pos_iv4col_um4proj_um4view_ov4fcol.v.glsl"));
    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Fragment, shaderPath_("iv4fcol.f.glsl"));
    shaderProgram_.link();

    // Get shader locations
    shaderProgram_.bind();
    posLoc_  = shaderProgram_.attributeLocation("pos");
    colLoc_  = shaderProgram_.attributeLocation("col");
    projLoc_ = shaderProgram_.uniformLocation("proj");
    viewLoc_ = shaderProgram_.uniformLocation("view");
    shaderProgram_.release();

    // Initialize engine
    api_ = ctx_->versionFunctions<QOpenGLFunctions_3_2_Core>();
    api_->initializeOpenGLFunctions();

    // Initialize widget for painting.
    // Note that initializedGL() is never called if the widget is never visible.
    // Therefore it's important to keep track whether it has been called, so that
    // we don't call onPaintDestroy() without first calling onPaintCreate()

    /*ctx_->makeCurrent(this);

    oglf_->glViewport(0, 0, width(), height());

    oglf_->glClearColor(1.f, 0, 0, 1.f);
    oglf_->glClear(GL_COLOR_BUFFER_BIT);

    m_context->swapBuffers(this);

    widget_->onPaintCreate(engine_.get());*/
}

} // namespace vgc::ui::internal

