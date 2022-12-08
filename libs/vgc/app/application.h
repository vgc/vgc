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

#ifndef VGC_APP_APPLICATION_H
#define VGC_APP_APPLICATION_H

#include <QApplication>

#include <vgc/app/api.h>
#include <vgc/core/object.h>

namespace vgc::app {

VGC_DECLARE_OBJECT(Application);

/// \class vgc::app::Application
/// \brief Represents an instance of a VGC application
///
class Application : public vgc::core::Object {
    VGC_OBJECT(Application, vgc::core::Object)

public:
    static ApplicationPtr create(int argc, char* argv[]);

protected:
    Application(int argc, char* argv[]);

private:
    // Note: we use QApplication (from Qt Widgets) rather than QGuiApplication
    // (from Qt Gui) since for now, we use QFileDialog and QMessageBox, which
    // are QWidgets and require an instance of QApplication.
    //
    QApplication application_;
};

} // namespace vgc::app

#endif // VGC_APP_APPLICATION_H
