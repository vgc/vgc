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

#include <sstream>

#include <QFontDatabase>

#include <vgc/core/paths.h>
#include <vgc/ui/qtutil.h>
#include <vgc/widgets/logcategories.h>

namespace vgc::widgets {

namespace {

std::string fontFamilyInfo(const std::string& family) {
    std::stringstream ss;
    QFontDatabase fd;
    ss << "Font Family: " << family << "\n";
    ss << "  Styles:\n";
    QString f = ui::toQt(family);
    QStringList styles = fd.styles(f);
    Q_FOREACH (const QString& s, styles) {
        // clang-format off
        ss << "    " << ui::fromQt(s) << ":\n";
        ss << "        weight:             " << fd.weight(f, s) << "\n";
        ss << "        bold:               " << (fd.bold(f, s)               ? "true" : "false") << "\n";
        ss << "        italic:             " << (fd.italic(f, s)             ? "true" : "false") << "\n";
        ss << "        isBitmapScalable:   " << (fd.isBitmapScalable(f, s)   ? "true" : "false") << "\n";
        ss << "        isFixedPitch:       " << (fd.isFixedPitch(f, s)       ? "true" : "false") << "\n";
        ss << "        isScalable:         " << (fd.isScalable(f, s)         ? "true" : "false") << "\n";
        ss << "        isSmoothlyScalable: " << (fd.isSmoothlyScalable(f, s) ? "true" : "false") << "\n";
        // clang-format on
        QList<int> pointSizes = fd.pointSizes(f, s);
        ss << "        pointSizes:         [";
        std::string delimiter = "";
        for (int ps : pointSizes) {
            ss << delimiter << ps;
            delimiter = ", ";
        }
        ss << "]\n";
        QList<int> smoothSizes = fd.smoothSizes(f, s);
        ss << "        smoothSizes:        [";
        delimiter = "";
        for (int size : smoothSizes) {
            ss << delimiter << size;
            delimiter = ", ";
        }
        ss << "]\n";
    }
    return ss.str();
}

} // namespace

void addDefaultApplicationFonts() {
    // clang-format off
    std::vector<std::pair<std::string, std::string>> fontNames{
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
    // clang-format on

    for (const auto& name : fontNames) {
        std::string fontExtension = ".ttf";
        std::string fontSubFolder = "/TTF/";
        std::string filename = name.first + "-" + name.second + fontExtension;
        std::string filepath = "widgets/fonts/" + name.first + fontSubFolder + filename;
        int id = widgets::addApplicationFont(filepath);
        if (id == -1) {
            VGC_WARNING(LogVgcWidgetsFonts, "Failed to add font \"{}\".", filepath);
        }
        else {
            VGC_DEBUG(LogVgcWidgetsFonts, "Added font file \"{}\".", filepath);
        }
    }

    VGC_DEBUG(LogVgcWidgetsFonts, fontFamilyInfo("Source Sans Pro"));
    VGC_DEBUG(LogVgcWidgetsFonts, fontFamilyInfo("Source Code Pro"));
}

int addApplicationFont(const std::string& name) {
    std::string fontPath = core::resourcePath(name);
    int id = QFontDatabase::addApplicationFont(ui::toQt(fontPath));
    return id;
}

void printFontFamilyInfo(const std::string& family) {
    VGC_INFO(LogVgcWidgetsFonts, fontFamilyInfo(family));
}

} // namespace vgc::widgets
