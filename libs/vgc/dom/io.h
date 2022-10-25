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

#ifndef VGC_DOM_IO_H
#define VGC_DOM_IO_H

#include <string>
#include <vgc/dom/api.h>
#include <vgc/dom/element.h>
#include <vgc/dom/node.h>
#include <vgc/dom/xmlformattingstyle.h>

namespace vgc::dom {

/// Writes spaces and/or tabs to the given output stream \p out in order to
/// correctly indent start/end XML tags, based on the given XML formatting \p
/// style and \p indentLevel.
///
template<typename OutputStream>
void writeIndent(OutputStream& out, const XmlFormattingStyle& style, int indentLevel) {
    char c = (style.indentStyle == XmlIndentStyle::Spaces) ? ' ' : '\t';
    out << std::string(indentLevel * style.indentSize, c);
}

/// Writes spaces and/or tabs to the given output stream \p out in order to
/// correctly indent XML attributes, based on the given XML formatting \p style
/// and \p indentLevel.
///
template<typename OutputStream>
void writeAttributeIndent(
    OutputStream& out,
    const XmlFormattingStyle& style,
    int indentLevel) {

    char c = (style.indentStyle == XmlIndentStyle::Spaces) ? ' ' : '\t';
    out << std::string(indentLevel * style.indentSize + style.attributeIndentSize, c);
}

/// Writes all the children of the given Node \p node to the given output
/// stream \p out, respecting the given XML formatting \p style and current \p
/// indentLevel.
///
template<typename OutputStream>
void writeChildren(
    OutputStream& out,
    const XmlFormattingStyle& style,
    int indentLevel,
    const Node* node) {

    for (Node* child : node->children()) {
        if (Element* element = Element::cast(child)) {
            writeIndent(out, style, indentLevel);
            out << '<' << element->tagName();
            for (const AuthoredAttribute& a : element->authoredAttributes()) {
                out << '\n';
                writeAttributeIndent(out, style, indentLevel);
                out << a.name() << "=\"" << core::toString(a.value()) << "\"";
            }
            out << ">\n";
            writeChildren(out, style, indentLevel + 1, child);
            writeIndent(out, style, indentLevel);
            out << "</" << element->tagName() << ">\n";
        }
    }
}

} // namespace vgc::dom

#endif // VGC_DOM_IO_H
