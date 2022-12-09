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

#include <vgc/app/mainwidget.h>

#include <vgc/core/compiler.h>
#include <vgc/core/io.h>
#include <vgc/core/paths.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/widget.h>

namespace vgc::app {

MainWidget::MainWidget()
    : ui::OverlayArea()
    , overlay_(this) {

    // Note: in the future, we may want to make overlay_ a child of MainWidget
    // instead, which is why we keep it as a data member.

    mainLayout_ = overlay_->createChild<ui::Column>();
    overlay_->setAreaWidget(mainLayout_);
    mainLayout_->addStyleClass(core::StringId("main-layout"));
    std::string path = core::resourcePath("ui/stylesheets/default.vgcss");
    std::string styleSheet = core::readFile(path);
    styleSheet += ".main-layout { "
                  "    row-gap: 0dp; "
                  "    padding-top: 0dp; "
                  "    padding-right: 0dp; "
                  "    padding-bottom: 0dp; "
                  "    padding-left: 0dp; }";
#ifdef VGC_CORE_OS_MACOS
    styleSheet += ".root { font-size: 13dp; }";
#endif
    overlay_->setStyleSheet(styleSheet);
    overlay_->addStyleClass(ui::strings::root);
}

MainWidgetPtr MainWidget::create() {
    return MainWidgetPtr(new MainWidget());
}

} // namespace vgc::app
