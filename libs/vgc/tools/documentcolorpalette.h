// Copyright 2023 The VGC Developers
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

#ifndef VGC_TOOLS_DOCUMENTCOLORPALETTE_H
#define VGC_TOOLS_DOCUMENTCOLORPALETTE_H

#include <vgc/core/color.h>
#include <vgc/core/colors.h>
#include <vgc/tools/api.h>
#include <vgc/ui/module.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Document);

} // namespace vgc::dom

namespace vgc::tools {

VGC_DECLARE_OBJECT(DocumentColorPalette);

/// \class vgc::tools::DocumentColorPaletteSaver
/// \brief Saves the color palette of a document.
///
/// This class is a temporary workaround before we implement a better system
/// for managing document color palettes. Assume that it will be deleted in the
/// near future.
///
class VGC_TOOLS_API DocumentColorPaletteSaver {
private:
    DocumentColorPaletteSaver(const core::Array<core::Color>& colors, dom::Document* doc);

public:
    ~DocumentColorPaletteSaver();
    DocumentColorPaletteSaver(const DocumentColorPaletteSaver&) = delete;
    DocumentColorPaletteSaver& operator=(const DocumentColorPaletteSaver&) = delete;

private:
    friend DocumentColorPalette;

    bool isUndoOpened_ = false;
    dom::Document* doc_ = nullptr;
};

/// \class vgc::tools::DocumentColorPalette
/// \brief A module to access the color palette of the active document.
///
/// This class is a temporary workaround before we implement a better system
/// for managing document color palettes. Assume that it will be deleted in the
/// near future.
///
class VGC_TOOLS_API DocumentColorPalette : public ui::Module {
private:
    VGC_OBJECT(DocumentColorPalette, ui::Module)

protected:
    DocumentColorPalette(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `DocumentColorPalette` module.
    ///
    static DocumentColorPalettePtr create(const ui::ModuleContext& context);

    /// Returns the document that this document color palette is operating on.
    ///
    dom::Document* document() const;

    /// Sets the document that this document color palette is operating on.
    ///
    void setDocument(dom::Document* document);

    /// Returns the colors of the document's color palette.
    ///
    const core::Array<core::Color>& colors() const {
        return colors_;
    }

    /// Sets the colors of the document's color palette.
    ///
    void setColors(const core::Array<core::Color>& colors);
    VGC_SLOT(setColors)

    /// This signal is emitted whenever the colors of the document's color
    /// palette changed.
    ///
    VGC_SIGNAL(colorsChanged, (const core::Array<core::Color>&, colors))

    /// Saves the current document color palette via the temporary
    /// `DocumentColorPaletteSaver` RAII class.
    ///
    /// ```
    /// {
    ///     auto saver = documentColorPalette->saver();
    ///     document->save();
    /// }
    /// ```
    ///
    DocumentColorPaletteSaver saver() {
        return DocumentColorPaletteSaver(colors(), document());
        // Note: the above uses C++17 guaranteed copy-elision since
        // DocumentColorPaletteSaver has no copy constructor
    }

private:
    dom::DocumentPtr document_;
    core::Array<core::Color> colors_;

    void onDocumentChanged_();
};

} // namespace vgc::tools

#endif // VGC_TOOLS_DOCUMENTCOLORPALETTE_H
