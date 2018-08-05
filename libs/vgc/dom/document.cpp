// Copyright 2018 The VGC Developers
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

#include <vgc/dom/document.h>

#include <fstream>
#include <vgc/core/logging.h>
#include <vgc/dom/element.h>
#include <vgc/dom/io.h>

namespace vgc {
namespace dom {

Document::Document() :
    Node(this, NodeType::Document),
    hasXmlDeclaration_(true),
    hasXmlEncoding_(true),
    hasXmlStandalone_(true),
    xmlVersion_("1.0"),
    xmlEncoding_("UTF-8"),
    xmlStandalone_(false)
{
    generateXmlDeclaration_();
}

Document::~Document()
{

}

namespace {

bool isWhitespace_(char c)
{
    // Reference: https://www.w3.org/TR/REC-xml/#NT-S
    //
    //   S ::= (#x20 | #x9 | #xD | #xA)+  [= (' ' | '\n' | '\r' | '\t')+]
    //
    //   Note:
    //
    //   The presence of #xD [= carriage return '\r'] in the above production
    //   is maintained purely for backward compatibility with the First
    //   Edition. As explained in 2.11 End-of-Line Handling, all #xD characters
    //   literally present in an XML document are either removed or replaced by
    //   #xA characters before any other processing is done. The only way to
    //   get a #xD character to match this production is to use a character
    //   reference in an entity value literal.
    //
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';

    // TODO: remove '\r' characters or replace them with '\n' if encountered
}

// Returns null if cannot create (i.e., if illegal XML).
Element* createElement_(Node* parent, const std::string& tagName)
{
    // TODO: check that natagNameme is a valid tag name

    if (parent->nodeType() == NodeType::Document) {
        Document* doc = Document::cast(parent);
        if (doc->rootElement()) {
            // Input XML file has two root elements!
            return nullptr;
        }
        else {
            return Element::create(doc, tagName);
        }
    }
    else if (parent->nodeType() == NodeType::Element) {
        Element* element = Element::cast(parent);
         return Element::create(element, tagName);
    }
    else {
        // Elements can't be children of anything else than the
        // Document node or another Element node.
        return nullptr;
    }
}

} // namespace

/* static */
DocumentSharedPtr Document::open(const std::string& filePath)
{
    // Note: in the future, we want to be able to detect formatting style of
    // input XML files, and preserve this style, as well as existing
    // non-significant whitespaces, etc. This is why we write our own parser,
    // since XML parsers typically discard all non-significant data. For
    // example, <foo name="hello"> and <foo    name="hello"> would typically
    // generate the same XmlStream events and we wouldn't be able to preserve
    // the formatting when saving back.

    DocumentSharedPtr res;

    std::ifstream in(filePath);
    if (!in.is_open()) {
        vgc::core::warning() << "Could not open file " << filePath << std::endl;
        return res;
    }

    // Create new document
    res = create();

    // State
    enum State {
        NO_STATE,
        MARKUP,
        PROCESSING_INSTRUCTION,
        START_TAG,
        END_TAG
    };
    enum SubState {
        NO_SUB_STATE,
        TAG_NAME,
        ATTRIBUTE_NAME,
        ATTRIBUTE_VALUE,
    };
    State state = NO_STATE;
    SubState subState = NO_SUB_STATE;
    bool aborted = false;
    Node* currentNode = res.get();

    // Buffers
    std::string tagName;
    std::string attributeName;
    std::string attributeValue;

    // Read character by character
    char c;
    while (!aborted && in >> c)
    {
        if (subState != NO_SUB_STATE)
        {
            switch (subState) {

            case NO_SUB_STATE:
                break;

            case TAG_NAME:
                if ( isWhitespace_(c) || c == '>') { // TODO handle '/' case too
                    if (state == START_TAG) {
                        currentNode = createElement_(currentNode, tagName);
                        if (!currentNode) {
                            aborted = true;
                            state = NO_STATE;
                            subState = NO_SUB_STATE;
                        }
                    }
                    else { // state == READING_END_TAG
                        if (!currentNode || currentNode->nodeType() != NodeType::Element) {
                            // End tag with no corresponding start tag!
                            aborted = true;
                            state = NO_STATE;
                            subState = NO_SUB_STATE;
                        }
                        else if (tagName != Element::cast(currentNode)->name().string()) {
                            // End tagName doesn't match corresponding start tagName!
                            aborted = true;
                            state = NO_STATE;
                            subState = NO_SUB_STATE;
                        }
                        else {
                            currentNode = currentNode->parent();
                        }
                    }
                    subState = NO_SUB_STATE;

                    // Additional steps if closing the tag now
                    if (c == '>') {
                        state = NO_STATE;
                    }
                }
                else {
                    tagName += c;
                }
                break;

            case ATTRIBUTE_NAME:
                // TODO
                break;

            case ATTRIBUTE_VALUE:
                // TODO
                break;
            }
        }
        else
        {
            switch (state) {

            // For now, we ignore the content of all text nodes.
            //
            case NO_STATE:
                if (c == '<') {
                    state = MARKUP;
                    subState = NO_SUB_STATE;
                }
                break;

            case MARKUP:
                if (c == '?') {
                    state = PROCESSING_INSTRUCTION;
                    subState = NO_SUB_STATE;
                }
                else if (c == '!') {
                    // Note: we don't yet support comments, CDATA section,
                    // and doctype declarations.
                    aborted = true;
                    state = NO_STATE;
                    subState = NO_SUB_STATE;
                }
                else if (c == '/') {
                    // Note: white spaces between '</' and the tag name are
                    // illegal, so we can safely assume that the next character
                    // is the first character of the tag name.
                    state = END_TAG;
                    subState = TAG_NAME;
                    tagName.clear();
                }
                else {
                    // Note: white spaces between '<' and the tag name are
                    // illegal, so we can safely assume that c is the first
                    // character of the tag name.
                    state = START_TAG;
                    subState = TAG_NAME;
                    tagName.clear();
                    tagName += c;
                }
                break;

            // For now, we ignore all processing instructions.
            //
            // Note: '>' is only allowed as the closing character of the
            // processing instruction, therefore the code below is correct for
            // any well-formed XML files. In practice, many parsers do accept
            // '>' within attribute values (even though this is illegal XML)
            // and in the future we may want to do the same (possibly, only in
            // a "non-strict" mode, and emitting a warning, and auto-fixing
            // when saving back, etc.)
            //
            case PROCESSING_INSTRUCTION:
                if (c == '>') {
                    state = NO_STATE;
                    subState = NO_SUB_STATE;
                }
                break;

            case START_TAG:
                // Note: we know that tagName has already been read because
                // subState = NO_SUB_STATE
                //
                // TODO: parse attributes (for now, we ignore them)
                //
                if (c == '>') {
                    state = NO_STATE;
                }
                break;
                // TODO: handle self-closing start tags

            case END_TAG:
                // Note: we know that tagName has already been read because
                // subState = NO_SUB_STATE
                if (c == '>') {
                    state = NO_STATE;
                }
                break;
            }
        }
    }

    if (aborted) {
        res.reset();
    }

    return res;
}

Element* Document::rootElement() const
{
    checkAlive();

    for (Node* node : children()) {
        if (node->nodeType() == NodeType::Element) {
            return Element::cast(node);
        }
    }
    return nullptr;
}

std::string Document::xmlDeclaration() const
{
    checkAlive();

    return xmlDeclaration_;
}

bool Document::hasXmlDeclaration() const
{
    checkAlive();

    return hasXmlDeclaration_;
}

void Document::setXmlDeclaration()
{
    checkAlive();

    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlDeclaration()
{
    checkAlive();

    hasXmlDeclaration_ = false;
    generateXmlDeclaration_();
}

std::string Document::xmlVersion() const
{
    checkAlive();

    return xmlVersion_;
}

void Document::setXmlVersion(const std::string& version)
{
    checkAlive();

    xmlVersion_ = version;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

std::string Document::xmlEncoding() const
{
    checkAlive();

    return xmlEncoding_;
}

bool Document::hasXmlEncoding() const
{
    checkAlive();

    return hasXmlEncoding_;
}

void Document::setXmlEncoding(const std::string& encoding)
{
    checkAlive();

    xmlEncoding_ = encoding;
    hasXmlEncoding_ = true;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlEncoding()
{
    checkAlive();

    xmlEncoding_ = "UTF-8";
    hasXmlEncoding_ = false;
    generateXmlDeclaration_();
}

bool Document::xmlStandalone() const
{
    checkAlive();

    return xmlStandalone_;
}

bool Document::hasXmlStandalone() const
{
    checkAlive();

    return hasXmlStandalone_;
}

void Document::setXmlStandalone(bool standalone)
{
    checkAlive();

    xmlStandalone_ = standalone;
    hasXmlStandalone_ = true;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlStandalone()
{
    checkAlive();

    xmlStandalone_ = false;
    hasXmlStandalone_ = false;
    generateXmlDeclaration_();
}

void Document::generateXmlDeclaration_()
{
    xmlDeclaration_.clear();
    if (hasXmlDeclaration_) {
        xmlDeclaration_.append("<?xml version=\"");
        xmlDeclaration_.append(xmlVersion_);
        xmlDeclaration_.append("\"");
        if (hasXmlEncoding_) {
            xmlDeclaration_.append(" encoding=\"");
            xmlDeclaration_.append(xmlEncoding_);
            xmlDeclaration_.append("\"");
        }
        if (hasXmlStandalone_) {
            xmlDeclaration_.append(" standalone=\"");
            xmlDeclaration_.append(xmlStandalone_ ? "yes" : "no");
            xmlDeclaration_.append("\"");
        }
        xmlDeclaration_.append("?>");
    }
}

bool Document::save(const std::string& filePath,
                    const XmlFormattingStyle& style) const
{
    checkAlive();

    std::ofstream out(filePath);
    if (!out.is_open()) {
        vgc::core::warning() << "Could not write file " << filePath << std::endl;
        return false;
    }

    out << xmlDeclaration_ << std::endl;    
    writeChildren(out, style, 0, this);

    return true;
}

} // namespace dom
} // namespace vgc
