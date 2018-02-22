// Copyright 2017 The VGC Developers
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

#include <vgc/widgets/font.h>

#include <QFontDatabase>
#include <QWidget>
#include <vgc/core/resources.h>
#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {

void setDefaultFont(QWidget* widget)
{
    // NOT_THREAD_SAFE
    static bool firstCall = true;
    if (firstCall) {
        std::string fontPath = core::resourcePath("widgets/fonts/SourceSansPro-Regular.ttf");
        QFontDatabase::addApplicationFont(toQt(fontPath));
    }

    QFont font = widget->font();
    font.setFamily("Source Sans Pro");
    font.setPointSize(12);
    widget->setFont(font);
}

} // namespace widgets
} // namespace vgc
