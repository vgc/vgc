// Copyright 2021 The VGC Developers
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

#include <vgc/widgets/stylesheets.h>

#include <QApplication>
#include <vgc/core/algorithm.h>
#include <vgc/core/io.h>
#include <vgc/core/os.h>
#include <vgc/core/paths.h>
#include <vgc/ui/qtutil.h>

namespace vgc::widgets {

void setApplicationStyleSheet(const std::string& name) {
    if (qApp) {
        std::string path = core::resourcePath(name);
        std::string s = core::readFile(path);

        // Convert resource paths to absolute paths
        s = core::replace(s, "vgc:/", core::resourcesPath() + "/");

// Set platform dependent font size
#if defined(VGC_CORE_OS_WINDOWS)
        std::string fontSize = "10.5pt";
#elif defined(VGC_CORE_OS_MACOS)
        std::string fontSize = "13pt";
#else
        std::string fontSize = "11pt";
#endif
        s = core::replace(s, "@font-size", fontSize);

        // Set stylesheet
        qApp->setStyleSheet(ui::toQt(s));
    }
}

} // namespace vgc::widgets
