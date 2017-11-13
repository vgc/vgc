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

#include <vgc/widgets/widget.h>

#include <QPainter>
#include <QSplitter>
#include <QHBoxLayout>
#include <vgc/scene/scene.h>
#include <vgc/widgets/console.h>
#include <vgc/widgets/viewer.h>

namespace vgc {
namespace widgets {

Widget::Widget(
    scene::Scene* scene,
    core::PythonInterpreter* interpreter,
    QWidget* parent) :

    QWidget(parent),
    scene_(scene)
{
    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(new Viewer(scene));
    splitter->addWidget(new Console(interpreter));

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(splitter);

    setLayout(layout);
    setMinimumSize(800, 600);

    connect(scene, &scene::Scene::changed,
            this, (void (Widget::*)()) &Widget::update);
}

Widget::~Widget()
{

}

} // namespace widgets
} // namespace vgc
