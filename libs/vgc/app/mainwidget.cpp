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

#include <vgc/core/io.h>
#include <vgc/core/os.h>
#include <vgc/core/paths.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/widget.h>

namespace vgc::app {

MainWidget::MainWidget(CreateKey key)
    : ui::OverlayArea(key)
    , overlayArea_(this) {

    // Note: in the future, we may want to make overlay_ a child of MainWidget
    // instead, which is why we keep it as a data member.

    // Setup overlay area
    std::string path = core::resourcePath("ui/stylesheets/default.vgcss");
    std::string styleSheet = core::readFile(path);
    styleSheet += ".main-layout { "
                  "    row-gap: 0dp; "
                  "    padding-top: 0dp; "
                  "    padding-right: 0dp; "
                  "    padding-bottom: 0dp; "
                  "    padding-left: 0dp; }";
    overlayArea_->setStyleSheet(styleSheet);
    overlayArea_->addStyleClass(ui::strings::root);
#ifdef VGC_CORE_OS_MACOS
    overlayArea_->addStyleClass(ui::strings::macos);
#endif

    // Create main layout
    ui::Column* mainLayout = overlayArea_->createChild<ui::Column>();
    mainLayout->addStyleClass(core::StringId("main-layout"));
    overlayArea_->setAreaWidget(mainLayout);

    // Create menu bar
    menuBar_ = mainLayout->createChild<ui::Menu>("Menu");
    menuBar_->setDirection(ui::FlexDirection::Row);
    menuBar_->addStyleClass(core::StringId("horizontal")); // TODO: move to Flex or Menu.
    menuBar_->addStyleClass(core::StringId("main-menu-bar"));
    menuBar_->setShortcutTrackEnabled(false);
    nativeMenuBar_ = NativeMenuBar::create(menuBar_);

    // Create panel area
    panelArea_ = ui::PanelArea::createTabs(mainLayout);
}

MainWidgetPtr MainWidget::create() {
    return core::createObject<MainWidget>();
}

} // namespace vgc::app
