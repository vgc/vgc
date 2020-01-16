// Copyright 2020 The VGC Developers
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

#include <vgc/widgets/uiwidget.h>

#include <QMouseEvent>

namespace vgc {
namespace widgets {

UiWidget::UiWidget(ui::WidgetSharedPtr widget, QWidget* parent) :
    QOpenGLWidget(parent),
    widget_(widget),
    engine_(UiWidgetEngine::create())
{

}

UiWidget::~UiWidget()
{
    makeCurrent();
    cleanupGL();
    doneCurrent();
}

namespace {

ui::MouseEventSharedPtr convertEvent(QMouseEvent* /*event*/)
{
    return ui::MouseEvent::create();
}

} // namespace

void UiWidget::mouseMoveEvent(QMouseEvent *event)
{
    ui::MouseEventSharedPtr e = convertEvent(event);
    event->setAccepted(widget_->mouseMoveEvent(e.get()));
}

void UiWidget::mousePressEvent(QMouseEvent *event)
{
    ui::MouseEventSharedPtr e = convertEvent(event);
    event->setAccepted(widget_->mousePressEvent(e.get()));
}

void UiWidget::mouseReleaseEvent(QMouseEvent *event)
{
    ui::MouseEventSharedPtr e = convertEvent(event);
    event->setAccepted(widget_->mouseReleaseEvent(e.get()));
}

UiWidget::OpenGLFunctions* UiWidget::openGLFunctions() const
{
    return context()->versionFunctions<OpenGLFunctions>();
}

void UiWidget::initializeGL()
{
    engine_->setOpenGLFunctions(openGLFunctions());
    widget_->initialize(engine_.get());
}

void UiWidget::resizeGL(int w, int h)
{
    widget_->resize(engine_.get(), int_cast<Int>(w), int_cast<Int>(h));
}

void UiWidget::paintGL()
{
    widget_->paint(engine_.get());
}

void UiWidget::cleanupGL()
{
    widget_->cleanup(engine_.get());
}

UiWidgetEngine::UiWidgetEngine(const ConstructorKey&) :
    graphics::Engine()
{

}

/* static */
UiWidgetEngineSharedPtr UiWidgetEngine::create()
{
    return std::make_shared<UiWidgetEngine>(ConstructorKey());
}

void UiWidgetEngine::clear(const core::Color& color)
{
    openGLFunctions_->glClearColor(
        static_cast<float>(color.r()),
        static_cast<float>(color.g()),
        static_cast<float>(color.b()),
        static_cast<float>(color.a()));
    openGLFunctions_->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace widgets
} // namespace vgc
