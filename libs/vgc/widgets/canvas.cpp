//// Copyright 2022 The VGC Developers
//// See the COPYRIGHT file at the top-level directory of this distribution
//// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
////
//// Licensed under the Apache License, Version 2.0 (the "License");
//// you may not use this file except in compliance with the License.
//// You may obtain a copy of the License at
////
////     http://www.apache.org/licenses/LICENSE-2.0
////
//// Unless required by applicable law or agreed to in writing, software
//// distributed under the License is distributed on an "AS IS" BASIS,
//// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//// See the License for the specific language governing permissions and
//// limitations under the License.
//
//#include <vgc/widgets/canvas.h>
//
//#include <cassert>
//#include <cmath>
//
//#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
//#include <QOpenGLVersionFunctionsFactory>
//#endif
//#include <QBitmap>
//#include <QMouseEvent>
//#include <QPainter>
//
//#include <vgc/core/assert.h>
//#include <vgc/core/os.h>
//#include <vgc/core/paths.h>
//#include <vgc/core/stopwatch.h>
//#include <vgc/geometry/vec2f.h>
//#include <vgc/geometry/curve.h>
//#include <vgc/widgets/qtutil.h>
//
//namespace vgc::widgets {
//
//namespace {
//
//// Returns the file path of a shader file as a QString
//QString shaderPath_(const std::string& name) {
//    std::string path = core::resourcePath("graphics/opengl/" + name);
//    return toQt(path);
//}
//
//QMatrix4x4 toQtMatrix(const geometry::Mat4d& m) {
//    return QMatrix4x4(
//        static_cast<float>(m(0,0)), static_cast<float>(m(0,1)), static_cast<float>(m(0,2)), static_cast<float>(m(0,3)),
//        static_cast<float>(m(1,0)), static_cast<float>(m(1,1)), static_cast<float>(m(1,2)), static_cast<float>(m(1,3)),
//        static_cast<float>(m(2,0)), static_cast<float>(m(2,1)), static_cast<float>(m(2,2)), static_cast<float>(m(2,3)),
//        static_cast<float>(m(3,0)), static_cast<float>(m(3,1)), static_cast<float>(m(3,2)), static_cast<float>(m(3,3)));
//}
//
//double width_(const PointingDeviceEvent& event) {
//    const double defaultWidth = 6.0;
//    return event.hasPressure() ?
//        2 * event.pressure() * defaultWidth :
//        defaultWidth;
//}
//
//core::StringId PATH("path");
//core::StringId POSITIONS("positions");
//core::StringId WIDTHS("widths");
//core::StringId COLOR("color");
//
//void drawCrossCursor(QPainter& painter) {
//    painter.setPen(QPen(Qt::color1, 1.0));
//    painter.drawLine(16, 0, 16, 10);
//    painter.drawLine(16, 22, 16, 32);
//    painter.drawLine(0, 16, 10, 16);
//    painter.drawLine(22, 16, 32, 16);
//    painter.drawPoint(16, 16);
//}
//
//QCursor crossCursor() {
//    // Draw bitmap
//    QBitmap bitmap(32, 32);
//    QPainter bitmapPainter(&bitmap);
//    bitmapPainter.fillRect(0, 0, 32, 32, QBrush(Qt::color0));
//    drawCrossCursor(bitmapPainter);
//
//    // Draw mask
//    QBitmap mask(32, 32);
//    QPainter maskPainter(&mask);
//    maskPainter.fillRect(0, 0, 32, 32, QBrush(Qt::color0));
//#ifndef VGC_CORE_OS_WINDOWS
//    // Make the cursor color XOR'd on Windows, black on other platforms. Ideally,
//    // we'd prefer XOR'd on all platforms, but it's only supported on Windows.
//    // See Qt doc for QCursor(const QBitmap &bitmap, const QBitmap &mask).
//    drawCrossCursor(maskPainter);
//#endif
//
//    // Create and return cursor
//    return QCursor(bitmap, mask);
//}
//
//} // namespace
//
//Canvas::Canvas(dom::Document* document)
//    : document_(document),
//    polygonMode_(2),
//    showControlPoints_(false),
//    requestedTesselationMode_(2),
//    currentTesselationMode_(2),
//    renderTask_("Render"),
//    updateTask_("Update"),
//    drawTask_("Draw")
//{
//    // Set ClickFocus policy to be able to accept keyboard events (default
//    // policy is NoFocus).
//    //setFocusPolicy(Qt::ClickFocus);
//
//    // Set cursor. For now we assume that we are in a drawing tool, and
//    // therefore use a cross cursor. In the future, each tool should specify
//    // which cursor should be drawn in the viewer.
//    //setCursor(crossCursor());
//
//    //setAttribute(Qt::WA_AlwaysStackOnTop);
//    //setAttribute(Qt::WA_PaintOnScreen);
//
//    //connect(this, &Canvas::aboutToCompose, this, &Canvas::aboutToCompose_);
//    //connect(this, &Canvas::frameSwapped, this, &Canvas::frameSwapped_);
//
//    documentChangedConnectionHandle_ = document_->changed().connect(
//        [this](const dom::Diff& diff) {
//            this->onDocumentChanged_(diff);
//        });
//}
//
//void Canvas::setDocument(dom::Document* document)
//{
//    cleanupGraphics();
//
//    if (documentChangedConnectionHandle_) {
//        document_->disconnect(documentChangedConnectionHandle_);
//    }
//
//    document_ = document;
//    documentChangedConnectionHandle_ = document_->changed().connect(
//        [this](const dom::Diff& diff) {
//            this->onDocumentChanged_(diff);
//        });
//
//    repaint(); // XXX rename requestRepaint
//}
//
//void Canvas::onDestroyed()
//{
//    Widget::onDestroyed();
//    // Make OpenGL context current and call cleanupGL()
//    //cleanupGraphics();
//}
//
////QSize Canvas::minimumSizeHint() const
////{
////    return QSize(160, 120);
////}
//
//void Canvas::startLoggingUnder(core::PerformanceLog* parent)
//{
//    core::PerformanceLog* renderLog = renderTask_.startLoggingUnder(parent);
//    updateTask_.startLoggingUnder(renderLog);
//    drawTask_.startLoggingUnder(renderLog);
//}
//
//void Canvas::stopLoggingUnder(core::PerformanceLog* parent)
//{
//    core::PerformanceLogPtr renderLog = renderTask_.stopLoggingUnder(parent);
//    updateTask_.stopLoggingUnder(renderLog.get());
//    drawTask_.stopLoggingUnder(renderLog.get());
//}
//
////void Canvas::aboutToCompose_()
////{
////    std::cout << "aboutToCompose  " << sw_.elapsed() * 1000.0 << std::endl;
////}
////
////void Canvas::frameSwapped_()
////{
////    std::cout << "frameSwapped    " << sw_.elapsed() * 1000.0 << std::endl;
////}
//
//bool Canvas::onMousePress(ui::MouseEvent* event)
//{
//    if (mousePressed_ || tabletPressed_) {
//        return;
//    }
//    //core::Stopwatch sw {};
//    mousePressed_ = true;
//    pointingDeviceButtonAtPress_ = event->button();
//    pointingDevicePress(PointingDeviceEvent(event));
//    //std::cout << "mousePressEvent:" << sw.elapsed() << std::endl;
//}
//
//bool Canvas::onMouseMove(ui::MouseEvent* event)
//{
//    if (!mousePressed_) {
//        return;
//    }
//    //core::Stopwatch sw {};
//    pointingDeviceMove(PointingDeviceEvent(event));
//    //std::cout << "mouseMoveEvent:" << sw.elapsed() << std::endl;
//}
//
//bool Canvas::onMouseRelease(ui::MouseEvent* event)
//{
//    if (!mousePressed_ || pointingDeviceButtonAtPress_ != event->button()) {
//        return;
//    }
//    pointingDeviceRelease(PointingDeviceEvent(event));
//    mousePressed_ = false;
//}
//
//void Canvas::tabletEvent(QTabletEvent* event)
//{
//    switch(event->type()) {
//    case QEvent::TabletPress:
//        if (mousePressed_ || tabletPressed_) {
//            return;
//        }
//        tabletPressed_ = true;
//        pointingDeviceButtonAtPress_ = event->button();
//        pointingDevicePress(PointingDeviceEvent(event));
//        break;
//
//    case QEvent::TabletMove:
//        if (!tabletPressed_) {
//            return;
//        }
//        pointingDeviceMove(PointingDeviceEvent(event));
//        break;
//
//    case QEvent::TabletRelease:
//        if (!tabletPressed_ || pointingDeviceButtonAtPress_ != event->button()) {
//            return;
//        }
//        pointingDeviceRelease(PointingDeviceEvent(event));
//        tabletPressed_ = false;
//        break;
//
//    default:
//        // nothing
//        break;
//    }
//}
//
//void Canvas::pointingDevicePress(const PointingDeviceEvent& event)
//{
//    if (isSketching_ || isPanning_ || isRotating_ || isZooming_) {
//        return;
//    }
//
//    if (event.modifiers() == Qt::NoModifier &&
//        event.button() == Qt::LeftButton)
//    {
//        isSketching_ = true;
//        // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
//        // and should be cached), but let's keep it like this for now for testing.
//        geometry::Vec2d viewCoords = event.pos();
//        geometry::Vec2d worldCoords = camera_.viewMatrix().inverted() * viewCoords;
//        startCurve_(worldCoords, width_(event));
//    }
//    else if (event.modifiers() == Qt::AltModifier &&
//        event.button() == Qt::LeftButton)
//    {
//        isRotating_ = true;
//        pointingDevicePosAtPress_ = event.pos();
//        cameraAtPress_ = camera_;
//    }
//    else if (event.modifiers() == Qt::AltModifier &&
//        event.button() == Qt::MiddleButton)
//    {
//        isPanning_ = true;
//        pointingDevicePosAtPress_ = event.pos();
//        cameraAtPress_ = camera_;
//    }
//    else if (event.modifiers() == Qt::AltModifier &&
//        event.button() == Qt::RightButton)
//    {
//        isZooming_ = true;
//        pointingDevicePosAtPress_ = event.pos();
//        cameraAtPress_ = camera_;
//    }
//}
//
//void Canvas::pointingDeviceMove(const PointingDeviceEvent& event)
//{
//    // Note: event.button() is always NoButton for move events. This is why
//    // we use the variable isPanning_ and isSketching_ to remember the current
//    // mouse action. In the future, we'll abstract this mechanism in a separate
//    // class.
//
//    if (isSketching_) {
//        // XXX This is very inefficient (shouldn't use generic 4x4 matrix inversion,
//        // and should be cached), but let's keep it like this for now for testing.
//        geometry::Vec2d viewCoords = event.pos();
//        geometry::Vec2d worldCoords = camera_.viewMatrix().inverted() * viewCoords;
//        continueCurve_(worldCoords, width_(event));
//    }
//    else if (isPanning_) {
//        geometry::Vec2d mousePos = event.pos();
//        geometry::Vec2d delta = pointingDevicePosAtPress_ - mousePos;
//        camera_.setCenter(cameraAtPress_.center() + delta);
//        update();
//    }
//    else if (isRotating_) {
//        // Set new camera rotation
//        // XXX rotateViewSensitivity should be a user preference
//        //     (the signs in front of dx and dy too)
//        const double rotateViewSensitivity = 0.01;
//        geometry::Vec2d mousePos = event.pos();
//        geometry::Vec2d deltaPos = pointingDevicePosAtPress_ - mousePos;
//        double deltaRotation = rotateViewSensitivity * (deltaPos.x() - deltaPos.y());
//        camera_.setRotation(cameraAtPress_.rotation() + deltaRotation);
//
//        // Set new camera center so that rotation center = mouse pos at press
//        geometry::Vec2d pivotViewCoords = pointingDevicePosAtPress_;
//        geometry::Vec2d pivotWorldCoords = cameraAtPress_.viewMatrix().inverted() * pivotViewCoords;
//        geometry::Vec2d pivotViewCoordsNow = camera_.viewMatrix() * pivotWorldCoords;
//        camera_.setCenter(camera_.center() - pivotViewCoords + pivotViewCoordsNow);
//
//        update();
//    }
//    else if (isZooming_) {
//        // Set new camera zoom
//        // XXX zoomViewSensitivity should be a user preference
//        //     (the signs in front of dx and dy too)
//        const double zoomViewSensitivity = 0.005;
//        geometry::Vec2d mousePos = event.pos();
//        geometry::Vec2d deltaPos = pointingDevicePosAtPress_ - mousePos;
//        const double s = std::exp(zoomViewSensitivity * (deltaPos.y() - deltaPos.x()));
//        camera_.setZoom(cameraAtPress_.zoom() * s);
//
//        // Set new camera center so that zoom center = mouse pos at press
//        geometry::Vec2d pivotViewCoords = pointingDevicePosAtPress_;
//        geometry::Vec2d pivotWorldCoords = cameraAtPress_.viewMatrix().inverted() * pivotViewCoords;
//        geometry::Vec2d pivotViewCoordsNow = camera_.viewMatrix() * pivotWorldCoords;
//        camera_.setCenter(camera_.center() - pivotViewCoords + pivotViewCoordsNow);
//
//        update();
//    }
//}
//
//void Canvas::pointingDeviceRelease(const PointingDeviceEvent&)
//{
//    isSketching_ = false;
//    isRotating_ = false;
//    isPanning_ = false;
//    isZooming_ = false;
//
//    if (drawCurveUndoGroup_) {
//        drawCurveUndoGroup_->close();
//        drawCurveUndoGroup_ = nullptr;
//    }
//}
//
//void Canvas::keyPressEvent(QKeyEvent* event)
//{
//    switch (event->key()) {
//    case Qt::Key_N:
//        polygonMode_ = 0;
//        update();
//        break;
//    case Qt::Key_T:
//        polygonMode_ = 1;
//        update();
//        break;
//    case Qt::Key_F:
//        polygonMode_ = 2;
//        update();
//        break;
//    case Qt::Key_I:
//        requestedTesselationMode_ = 0;
//        update();
//        break;
//    case Qt::Key_U:
//        requestedTesselationMode_ = 1;
//        update();
//        break;
//    case Qt::Key_A:
//        requestedTesselationMode_ = 2;
//        update();
//        break;
//    case Qt::Key_P:
//        showControlPoints_ = !showControlPoints_;
//        update();
//        break;
//    default:
//        break;
//    }
//
//    // Don't factor out "update()" here, to avoid unnecessary redraws for keys
//    // not handled here, including modifiers.
//}
//
//void Canvas::initializeGraphics()
//{
//    OpenGLFunctions* f = openGLFunctions();
//
//    // Initialize shader program
//    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Vertex, shaderPath_("shader.v.glsl"));
//    shaderProgram_.addShaderFromSourceFile(QOpenGLShader::Fragment, shaderPath_("shader.f.glsl"));
//    shaderProgram_.link();
//
//    // Get shader locations
//    shaderProgram_.bind();
//    vertexLoc_     = shaderProgram_.attributeLocation("vertex");
//    projMatrixLoc_ = shaderProgram_.uniformLocation("projMatrix");
//    viewMatrixLoc_ = shaderProgram_.uniformLocation("viewMatrix");
//    colorLoc_      = shaderProgram_.uniformLocation("color");
//    shaderProgram_.release();
//
//    // Set clear color
//    f->glClearColor(1, 1, 1, 1);
//}
//
//void Canvas::resizeGraphics(int w, int h)
//{
//    camera_.setViewportSize(w, h);
//}
//
//void Canvas::paintGraphics()
//{
//    //auto ctxFormat = context()->format();
//    //std::cout << "context()->format():" << std::endl;
//    //std::cout << "  swapInterval:" << ctxFormat.swapInterval() << std::endl;
//    //std::cout << "  swapBehavior:" << ctxFormat.swapBehavior() << std::endl;
//
//    if (isSketching_)
//        return;
//
//    // Measure rendering time
//    renderTask_.start();
//
//    OpenGLFunctions* f = openGLFunctions();
//
//    updateGLResources_();
//
//    drawTask_.start();
//
//    // Clear color and depth buffer
//    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    // Bind shader program
//    shaderProgram_.bind();
//
//    // Set uniform values
//    shaderProgram_.setUniformValue(projMatrixLoc_, toQtMatrix(camera_.projectionMatrix()));
//    shaderProgram_.setUniformValue(viewMatrixLoc_, toQtMatrix(camera_.viewMatrix()));
//
//    // Draw triangles
//    if (polygonMode_ > 0) {
//        f->glPolygonMode(GL_FRONT_AND_BACK, (polygonMode_ == 1) ? GL_LINE : GL_FILL);
//        for (CurveGLResources& r : curveGLResources_) {
//            shaderProgram_.setUniformValue(
//                colorLoc_,
//                static_cast<float>(r.trianglesColor.r()),
//                static_cast<float>(r.trianglesColor.g()),
//                static_cast<float>(r.trianglesColor.b()),
//                static_cast<float>(r.trianglesColor.a()));
//            r.vaoTriangles->bind();
//            f->glDrawArrays(GL_TRIANGLE_STRIP, 0, r.numVerticesTriangles);
//            r.vaoTriangles->release();
//        }
//    }
//
//    // Draw control points
//    if (showControlPoints_) {
//        shaderProgram_.setUniformValue(colorLoc_, 1.0f, 0.0f, 0.0f, 1.0f);
//        f->glPointSize(10.0);
//        for (CurveGLResources& r : curveGLResources_) {
//            r.vaoControlPoints->bind();
//            f->glDrawArrays(GL_POINTS, 0, r.numVerticesControlPoints);
//            r.vaoControlPoints->release();
//        }
//    }
//
//    // Release shader program
//    shaderProgram_.release();
//    drawTask_.stop();
//
//    // Complete measure of rendering time
//    renderTask_.stop();
//
//    // Inform that the render is completed
//
//    std::cout << "renderCompleted " << std::endl;
//    sw_.restart();
//    Q_EMIT renderCompleted();
//}
//
//void Canvas::cleanupGL()
//{
//    for (CurveGLResources& r : curveGLResources_) {
//        destroyCurveGLResources_(r);
//    }
//    curveGLResources_.clear();
//    curveGLResourcesMap_.clear();
//}
//
//void Canvas::onDocumentChanged_(const dom::Diff& diff)
//{
//    for (dom::Node* node : diff.removedNodes()) {
//        dom::Element* e = dom::Element::cast(node);
//        if (!e) {
//            continue;
//        }
//        auto it = curveGLResourcesMap_.find(e);
//        if (it != curveGLResourcesMap_.end()) {
//            removedGLResources_.splice(removedGLResources_.begin(), curveGLResources_, it->second);
//            curveGLResourcesMap_.erase(it);
//        }
//    }
//
//    bool needsSort = false;
//
//    dom::Element* root = document_->rootElement();
//    for (dom::Node* node : diff.reparentedNodes()) {
//        dom::Element* e = dom::Element::cast(node);
//        if (!e) {
//            continue;
//        }
//        if (e->parent() == root) {
//            needsSort = true;
//            auto it = appendCurveGLResources_(e);
//            toUpdate_.insert(it);
//        }
//        else {
//            auto it = curveGLResourcesMap_.find(e);
//            if (it != curveGLResourcesMap_.end()) {
//                removedGLResources_.splice(removedGLResources_.begin(), curveGLResources_, it->second);
//                curveGLResourcesMap_.erase(it);
//            }
//        }
//    }
//
//    for (dom::Node* node : diff.createdNodes()) {
//        dom::Element* e = dom::Element::cast(node);
//        if (!e) {
//            continue;
//        }
//        if (e->parent() == root) {
//            needsSort = true;
//            auto it = appendCurveGLResources_(e);
//            toUpdate_.insert(it);
//        }
//    }
//
//    if (!needsSort) {
//        for (dom::Node* node : diff.childrenReorderedNodes()) {
//            if (node == root) {
//                needsSort = true;
//                break;
//            }
//        }
//    }
//
//    if (needsSort) {
//        auto insert = curveGLResources_.begin();
//        for (dom::Node* node : root->children()) {
//            dom::Element* e = dom::Element::cast(node);
//            if (!e) {
//                continue;
//            }
//            auto it = curveGLResourcesMap_[e]; // works unless there is a bug
//            if (insert == it) {
//                ++insert;
//            }
//            else {
//                curveGLResources_.splice(insert, curveGLResources_, it);
//            }
//        }
//    }
//
//    // XXX it's possible that update is done twice if the element is both modified and reparented..
//
//    const auto& modifiedElements = diff.modifiedElements();
//    for (CurveGLResourcesIterator it = curveGLResources_.begin(); it != curveGLResources_.end(); ++it) {
//        auto it2 = modifiedElements.find(it->element);
//        if (it2 != modifiedElements.end()) {
//            toUpdate_.insert(it);
//        }
//    }
//
//    // ask for redraw
//    update();
//}
//
//void Canvas::updateGLResources_()
//{
//    updateTask_.start();
//
//    for (CurveGLResources& r : removedGLResources_) {
//        destroyCurveGLResources_(r);
//    }
//    removedGLResources_.clear();
//
//    bool tesselationModeChanged = requestedTesselationMode_ != currentTesselationMode_;
//    if (tesselationModeChanged) {
//        currentTesselationMode_ = requestedTesselationMode_;
//        for (CurveGLResources& r : curveGLResources_) {
//            updateCurveGLResources_(r);
//        }
//    }
//    else {
//        for (auto it : toUpdate_) {
//            updateCurveGLResources_(*it);
//        }
//    }
//    toUpdate_.clear();
//
//    updateTask_.stop();
//}
//
//Canvas::CurveGLResourcesIterator Canvas::appendCurveGLResources_(dom::Element* element)
//{
//    auto it = curveGLResources_.emplace(curveGLResources_.end(), CurveGLResources(element));
//    curveGLResourcesMap_.emplace(element, it);
//    return it;
//}
//
//void Canvas::updateCurveGLResources_(CurveGLResources& r)
//{
//    if (!r.inited_) {
//        OpenGLFunctions* f = openGLFunctions();
//        GLuint vertexLoc = static_cast<GLuint>(vertexLoc_);
//
//        // Create VBO/VAO for rendering triangles
//        r.vboTriangles.create();
//        r.vaoTriangles = new QOpenGLVertexArrayObject();
//        r.vaoTriangles->create();
//        GLsizei stride  = sizeof(geometry::Vec2f);
//        GLvoid* pointer = nullptr;
//        r.vaoTriangles->bind();
//        r.vboTriangles.bind();
//        f->glEnableVertexAttribArray(vertexLoc);
//        f->glVertexAttribPointer(
//            vertexLoc, // index of the generic vertex attribute
//            2,         // number of components (x and y components)
//            GL_FLOAT,  // type of each component
//            GL_FALSE,  // should it be normalized
//            stride,    // byte offset between consecutive vertex attributes
//            pointer);  // byte offset between the first attribute and the pointer given to allocate()
//        r.vboTriangles.release();
//        r.vaoTriangles->release();
//
//        // Setup VBO/VAO for rendering control points
//        r.vboControlPoints.create();
//        r.vaoControlPoints = new QOpenGLVertexArrayObject();
//        r.vaoControlPoints->create();
//        r.vaoControlPoints->bind();
//        r.vboControlPoints.bind();
//        f->glEnableVertexAttribArray(vertexLoc);
//        f->glVertexAttribPointer(
//            vertexLoc, // index of the generic vertex attribute
//            2,         // number of components (x and y components)
//            GL_FLOAT,  // type of each component
//            GL_FALSE,  // should it be normalized
//            stride,    // byte offset between consecutive vertex attributes
//            pointer);  // byte offset between the first attribute and the pointer given to allocate()
//        r.vboControlPoints.release();
//        r.vaoControlPoints->release();
//        r.inited_ = true;
//    }
//
//    geometry::Vec2dArray triangulation;
//    geometry::Vec2fArray glVerticesControlPoints;
//
//    dom::Element* path = r.element;
//    geometry::Vec2dArray positions = path->getAttribute(POSITIONS).getVec2dArray();
//    core::DoubleArray widths = path->getAttribute(WIDTHS).getDoubleArray();
//    core::Color color = path->getAttribute(COLOR).getColor();
//
//    if (1) {
//        // Convert the dom::Path to a geometry::Curve
//        // XXX move this logic to dom::Path
//
//        VGC_CORE_ASSERT(positions.size() == widths.size());
//        Int nControlPoints = positions.length();
//        geometry::Curve curve;
//        curve.setColor(color);
//        for (Int j = 0; j < nControlPoints; ++j) {
//            curve.addControlPoint(positions[j], widths[j]);
//        }
//
//        // Triangulate the curve
//        double maxAngle = 0.05;
//        int minQuads = 1;
//        int maxQuads = 64;
//        if (requestedTesselationMode_ == 0) {
//            maxQuads = 1;
//        }
//        else if (requestedTesselationMode_ == 1) {
//            minQuads = 10;
//            maxQuads = 10;
//        }
//        triangulation = curve.triangulate(maxAngle, minQuads, maxQuads);
//
//        const core::DoubleArray& d = curve.positionData();
//        Int ncp = core::int_cast<GLsizei>(d.length() / 2);
//        for (Int j = 0; j < ncp; ++j) {
//            glVerticesControlPoints.append(geometry::Vec2f(static_cast<float>(d[2*j]),
//                static_cast<float>(d[2*j+1])));
//        }
//    }
//    else { // simplest impl for perf comparison
//
//        Int ncp = positions.length();
//
//        using geometry::Vec2d;
//
//        // simple segments !
//        triangulation.resizeNoInit(4 * (positions.length() - 1));
//        Vec2d prevPoint = positions[0];
//        double prevWidth = widths[0];
//        for (Int i = 1; i < positions.length(); ++i) {
//            Vec2d nextPoint = positions[i];
//            double nextWidth = widths[i];
//
//            Vec2d seg = nextPoint - prevPoint;
//            Vec2d delta = seg.orthogonalized().normalized();
//
//            Int j = (i - 1) * 4;
//            triangulation[j + 0] = prevPoint - delta * prevWidth;
//            triangulation[j + 1] = prevPoint + delta * prevWidth;
//            triangulation[j + 2] = nextPoint - delta * nextWidth;
//            triangulation[j + 3] = nextPoint + delta * nextWidth;
//
//            prevPoint = nextPoint;
//            prevWidth = nextWidth;
//        }
//
//        for (Int j = 0; j < ncp; ++j) {
//            const Vec2d& v = positions[j];
//            glVerticesControlPoints.append(
//                geometry::Vec2f(
//                    static_cast<float>(v.x()),
//                    static_cast<float>(v.y())));
//        }
//    }
//
//    // Convert triangles to single-precision and transfer to GPU
//    //
//    // XXX For the doubles to floats, we should either:
//    //     - have a public helper function to do this
//    //     - directly compute the triangulation using floats (although
//    //       using doubles is more precise for intersection tests)
//    //
//    r.numVerticesTriangles = core::int_cast<GLsizei>(triangulation.length());
//    geometry::Vec2fArray glVerticesTriangles;
//    for(const geometry::Vec2d& v: triangulation) {
//        glVerticesTriangles.append(geometry::Vec2f(static_cast<float>(v[0]),
//            static_cast<float>(v[1])));
//    }
//    r.vboTriangles.bind();
//    int n = core::int_cast<int>(r.numVerticesTriangles) * static_cast<int>(sizeof(geometry::Vec2f));
//    r.vboTriangles.allocate(glVerticesTriangles.data(), n);
//    r.vboTriangles.release();
//
//    // Transfer control points vertex data to GPU
//    r.numVerticesControlPoints = glVerticesControlPoints.length();
//    r.vboControlPoints.bind();
//    n = core::int_cast<int>(r.numVerticesControlPoints) * static_cast<int>(sizeof(geometry::Vec2f));
//    r.vboControlPoints.allocate(glVerticesControlPoints.data(), n);
//    r.vboControlPoints.release();
//
//    // Set color
//    r.trianglesColor = color;
//}
//
//void Canvas::destroyCurveGLResources_(CurveGLResources& r)
//{
//    r.vaoTriangles->destroy();
//    delete r.vaoTriangles;
//    r.vboTriangles.destroy();
//
//    r.vaoControlPoints->destroy();
//    delete r.vaoControlPoints;
//    r.vboControlPoints.destroy();
//}
//
//void Canvas::startCurve_(const geometry::Vec2d& p, double width)
//{
//    // XXX CLEAN
//    static core::StringId Draw_Curve("Draw Curve");
//
//    core::Stopwatch sw {};
//
//    //sw.restart();
//    drawCurveUndoGroup_ = document()->history()->createUndoGroup(Draw_Curve);
//    //std::cout << "createUndoGroup:" << sw.elapsed() << std::endl;
//
//    //sw.restart();
//    drawCurveUndoGroup_->undone().connect(
//        [this](core::UndoGroup*, bool /*isAbort*/) {
//            // isAbort should be true since we have no sub-group
//            isSketching_ = false;
//            drawCurveUndoGroup_ = nullptr;
//        });
//    //std::cout << "drawCurveUndoGroup_->undone().connect):" << sw.elapsed() << std::endl;
//
//    //sw.restart();
//    dom::Element* root = document_->rootElement();
//    dom::Element* path = dom::Element::create(root, PATH);
//    //std::cout << "dom::Element::create:" << sw.elapsed() << std::endl;
//
//    //sw.restart();
//    path->setAttribute(POSITIONS, geometry::Vec2dArray());
//    path->setAttribute(WIDTHS, core::DoubleArray());
//    path->setAttribute(COLOR, currentColor_);
//    //std::cout << "path->setAttribute() (x3):" << sw.elapsed() << std::endl;
//
//    //sw.restart();
//    continueCurve_(p, width);
//    //std::cout << "continueCurve_():" << sw.elapsed() << std::endl;
//}
//
//void Canvas::continueCurve_(const geometry::Vec2d& p, double width)
//{
//    // XXX CLEAN
//    core::Stopwatch sw {};
//
//    //sw.restart();
//    dom::Element* root = document_->rootElement();
//    dom::Element* path = root->lastChildElement();
//    //std::cout << "path from document:" << sw.elapsed() << std::endl;
//
//    if (path) {
//        // Should I make this more efficient? If so, we have a few choices:
//        // duplicate the API of arrays within value and provide fine-grain
//        // "changed" signals. And/or allow to pass a lambda that modifies the
//        // underlying value. The dom::Value will call the lambda to mutate the
//        // value, then emit a generic changed signal. I could also let clients
//        // freely mutate the value and trusteing them in sending a changed
//        // signal themselves.
//
//        //sw.restart();
//        geometry::Vec2dArray positions = path->getAttribute(POSITIONS).getVec2dArray();
//        core::DoubleArray widths = path->getAttribute(WIDTHS).getDoubleArray();
//        positions.append(p);
//        widths.append(width);
//        //std::cout << "compute new positions + widths:" << sw.elapsed() << std::endl;
//
//        //sw.restart();
//        path->setAttribute(POSITIONS, std::move(positions));
//        path->setAttribute(WIDTHS, std::move(widths));
//        //std::cout << "path->setAttribute() (x2):" << sw.elapsed() << std::endl;
//
//        //sw.restart();
//        document()->emitPendingDiff();
//        //std::cout << "document()->emitPendingDiff():" << sw.elapsed() << std::endl;
//    }
//}
//
//
//} // namespace vgc::widgets
