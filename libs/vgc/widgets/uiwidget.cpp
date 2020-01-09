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

namespace vgc {
namespace widgets {

UiWidget::UiWidget(ui::WidgetSharedPtr widget, QWidget* parent) :
    QOpenGLWidget(parent),
    widget_(widget)
{

}

UiWidget::~UiWidget()
{
    makeCurrent();
    cleanupGL();
    doneCurrent();
}

void UiWidget::initializeGL()
{
    widget_->initialize();
}

void UiWidget::resizeGL(int w, int h)
{
    widget_->resize(w, h);
}

void UiWidget::paintGL()
{
    widget_->paint();
}

void UiWidget::cleanupGL()
{
    widget_->cleanup();
}

} // namespace widgets
} // namespace vgc
