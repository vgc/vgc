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

#ifndef VGC_APP_MAINWINDOW_H
#define VGC_APP_MAINWINDOW_H

#include <QApplication>

#include <vgc/app/api.h>
#include <vgc/app/mainwidget.h>
#include <vgc/ui/window.h>

namespace vgc::app {

VGC_DECLARE_OBJECT(MainWindow);

/// \class vgc::app::MainWindow
/// \brief A window with built-in common widgets such as a menu bar and panel area.
///
/// The class `MainWindow` is a subclass of `Window` that contains
/// a `MainWidget`.
///
class VGC_APP_API MainWindow : public ui::Window {
    VGC_OBJECT(MainWindow, ui::Window)

protected:
    MainWindow(std::string_view title);

public:
    /// Creates a `MainWindow`.
    ///
    static MainWindowPtr create(std::string_view title);

    /// Returns the `MainWidget` owned by this `MainWindow`.
    ///
    MainWidget* mainWidget() {
        return static_cast<MainWidget*>(widget());
    }
};

} // namespace vgc::app

#endif // VGC_APP_MAINWINDOW_H
