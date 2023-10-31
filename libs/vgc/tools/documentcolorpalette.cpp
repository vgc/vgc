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

#include <vgc/tools/documentcolorpalette.h>

#include <vgc/dom/document.h>
#include <vgc/dom/element.h>

namespace vgc::tools {

namespace {

core::StringId s_color("color");
core::StringId s_colorpalette("colorpalette");
core::StringId s_colorpaletteitem("colorpaletteitem");
core::StringId s_user("user");
core::StringId s_Add_to_Palette("Add to Palette");

core::Array<core::Color> getColorPalette_(dom::Document* doc) {

    // Get colors
    core::Array<core::Color> colors;
    dom::Element* root = doc->rootElement();
    for (dom::Element* user : root->childElements(s_user)) {
        for (dom::Element* colorpalette : user->childElements(s_colorpalette)) {
            for (dom::Element* item : colorpalette->childElements(s_colorpaletteitem)) {
                core::Color color = item->getAttribute(s_color).getColor();
                colors.append(color);
            }
        }
    }

    // Delete <user> element.
    //
    // This is temporary: a better system would be it in the document. See also
    // comments on DocumentColorPaletteSaver.
    //
    dom::Element* user = root->firstChildElement(s_user);
    while (user) {
        dom::Element* nextUser = user->nextSiblingElement(s_user);
        user->remove();
        user = nextUser;
    }

    return colors;
}

} // namespace

DocumentColorPaletteSaver::DocumentColorPaletteSaver(
    const core::Array<core::Color>& colors,
    dom::Document* doc)

    : doc_(doc) {

    if (!doc) {
        return;
    }

    // The current implementation adds the colors to the DOM now, save, then
    // abort the "add color" operation so that it doesn't appear as an undo.
    //
    // Ideally, we should instead add the color to the DOM directly when the
    // user clicks the "add to palette" button (so it would be an undoable
    // action), and the color list view should listen to DOM changes to update
    // the color list. This way, even plugins could populate the color palette
    // by modifying the DOM.
    //
    core::History* history = doc->history();
    if (history) {
        history->createUndoGroup(s_Add_to_Palette);
        isUndoOpened_ = true;
    }

    // TODO: reuse existing colorpalette element instead of creating new one.
    dom::Element* root = doc->rootElement();
    dom::Element* user = dom::Element::create(root, s_user);
    dom::Element* colorpalette = dom::Element::create(user, s_colorpalette);
    for (const core::Color& color : colors) {
        dom::Element* item = dom::Element::create(colorpalette, s_colorpaletteitem);
        item->setAttribute(s_color, color);
    }
}

DocumentColorPaletteSaver::~DocumentColorPaletteSaver() {
    if (isUndoOpened_) {
        core::History* history = doc_->history();
        if (history) {
            history->abort();
        }
    }
}

DocumentColorPalette::DocumentColorPalette(
    CreateKey key,
    const ui::ModuleContext& context)

    : Module(key, context) {
}

DocumentColorPalettePtr DocumentColorPalette::create(const ui::ModuleContext& context) {
    return core::createObject<DocumentColorPalette>(context);
}

void DocumentColorPalette::setDocument(dom::Document* document) {
    if (document == this->document()) {
        return;
    }
    document_ = document;
    onDocumentChanged_();
}

void DocumentColorPalette::setColors(const core::Array<core::Color>& colors) {
    if (colors_ == colors) {
        return;
    }
    colors_ = colors;
    core::Array<core::Color> copy = colors;
    colorsChanged().emit(copy);
}

void DocumentColorPalette::onDocumentChanged_() {
    dom::Document* document = this->document();
    core::Array<core::Color> colors;
    if (document) {
        colors = getColorPalette_(document);
    }
    setColors(colors);
}

} // namespace vgc::tools
