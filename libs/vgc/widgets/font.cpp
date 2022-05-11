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

#include <vgc/widgets/font.h>

#include <iostream>
#include <QFontDatabase>
#include <vgc/core/logging.h>
#include <vgc/core/paths.h>
#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {

void addDefaultApplicationFonts()
{
    const bool printFontInfo = false;
    std::vector<std::pair<std::string, std::string>> fontNames {
        {"SourceCodePro", "Black"},
        {"SourceCodePro", "BlackIt"},
        {"SourceCodePro", "Bold"},
        {"SourceCodePro", "BoldIt"},
        {"SourceCodePro", "ExtraLight"},
        {"SourceCodePro", "ExtraLightIt"},
        {"SourceCodePro", "It"},
        {"SourceCodePro", "Light"},
        {"SourceCodePro", "LightIt"},
        {"SourceCodePro", "Medium"},
        {"SourceCodePro", "MediumIt"},
        {"SourceCodePro", "Regular"},
        {"SourceCodePro", "Semibold"},
        {"SourceCodePro", "SemiboldIt"},
        {"SourceSansPro", "Black"},
        {"SourceSansPro", "BlackIt"},
        {"SourceSansPro", "Bold"},
        {"SourceSansPro", "BoldIt"},
        {"SourceSansPro", "ExtraLight"},
        {"SourceSansPro", "ExtraLightIt"},
        {"SourceSansPro", "It"},
        {"SourceSansPro", "Light"},
        {"SourceSansPro", "LightIt"},
        {"SourceSansPro", "Regular"},
        {"SourceSansPro", "Semibold"},
        {"SourceSansPro", "SemiboldIt"},
    };

    for (const auto& name : fontNames) {
        std::string fontExtension = ".ttf";
        std::string fontSubFolder = "/TTF/";
        std::string filename = name.first + "-" + name.second + fontExtension;
        std::string filepath = "widgets/fonts/" + name.first + fontSubFolder + filename;
        int id = widgets::addApplicationFont(filepath);
        if (id == -1) {
            core::warning() << "Failed to add font \"" + filepath + "\".\n";
        }
        else {
            if (printFontInfo) {
                std::cout << "Added font file: " + filepath + "\n";
            }
        }
    }

    if (printFontInfo) {
        widgets::printFontFamilyInfo("Source Sans Pro");
        widgets::printFontFamilyInfo("Source Code Pro");
    }
}

int addApplicationFont(const std::string& name)
{
    std::string fontPath = core::resourcePath(name);
    int id = QFontDatabase::addApplicationFont(toQt(fontPath));
    return id;
}

void printFontFamilyInfo(const std::string& family)
{
    QFontDatabase fd;
    std::cout << "Font Family: " << family << "\n";
    std::cout << "  Styles:\n";
    QString f = toQt(family);
    QStringList styles = fd.styles(f);
    Q_FOREACH (const QString& s, styles) {
        std::cout << "    " << fromQt(s) << ":\n";
        std::cout << "        weight:             " << fd.weight(f, s) << "\n";
        std::cout << "        bold:               " << (fd.bold(f, s)               ? "true" : "false") << "\n";
        std::cout << "        italic:             " << (fd.italic(f, s)             ? "true" : "false") << "\n";
        std::cout << "        isBitmapScalable:   " << (fd.isBitmapScalable(f, s)   ? "true" : "false") << "\n";
        std::cout << "        isFixedPitch:       " << (fd.isFixedPitch(f, s)       ? "true" : "false") << "\n";
        std::cout << "        isScalable:         " << (fd.isScalable(f, s)         ? "true" : "false") << "\n";
        std::cout << "        isSmoothlyScalable: " << (fd.isSmoothlyScalable(f, s) ? "true" : "false") << "\n";
        QList<int> pointSizes = fd.pointSizes(f, s);
        std::cout << "        pointSizes:         [";
        std::string delimiter = "";
        for (int ps : pointSizes) {
            std::cout << delimiter << ps;
            delimiter = ", ";
        }
        std::cout << "]\n";
        QList<int> smoothSizes = fd.smoothSizes(f, s);
        std::cout << "        smoothSizes:        [";
        delimiter = "";
        for (int ss : smoothSizes) {
            std::cout << delimiter << ss;
            delimiter = ", ";
        }
        std::cout << "]\n";
    }
}

} // namespace widgets
} // namespace vgc
