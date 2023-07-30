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

#include <vgc/app/mainwindow.h>

#include <vgc/app/mainwidget.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/widget.h>

namespace vgc::app {

MainWindow::MainWindow(CreateKey key, std::string_view title)
    : ui::Window(key, MainWidget::create()) {

    setTitle(ui::toQt(title));
    resize(QSize(1100, 800));
    setVisible(true);
}

MainWindowPtr MainWindow::create(std::string_view title) {
    return core::createObject<MainWindow>(title);
}

} // namespace vgc::app
