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

#ifndef VGC_APP_MAINWIDGET_H
#define VGC_APP_MAINWIDGET_H

#include <vgc/app/api.h>
#include <vgc/ui/column.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/overlayarea.h>

namespace vgc::app {

VGC_DECLARE_OBJECT(MainWidget);

/// \class vgc::app::MainWidget
/// \brief Provides a menu bar, a panel area, and other common widgets.
///
/// This widget provides common widgets organized in a familiar layout
/// that many applications need, for example:
///
/// - An `OverlayArea` at the top-level to be able to show popups
/// - A menu bar at the top, where you can add your own menus
/// - A `PanelArea`, on which you can set a `Panel`, or further
///   subdivide into sub-PanelArea
///
/// Note that we also provides the convenient class `MainWindow`, which
/// automatically creates a `MainWidget` and displays it inside a `Window`.
///
class VGC_APP_API MainWidget : public ui::OverlayArea {
    VGC_OBJECT(MainWidget, ui::OverlayArea)

protected:
    MainWidget();

public:
    /// Creates a `MainWidget`.
    ///
    static MainWidgetPtr create();

private:
    ui::OverlayArea* overlay_ = nullptr;
    ui::Column* mainLayout_ = nullptr;
    ui::Menu* menuBar_ = nullptr;
};

} // namespace vgc::app

#endif // VGC_APP_MAINWIDGET_H
