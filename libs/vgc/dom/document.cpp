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

bool isNameStartChar_(char c)
{
    // Reference: https://www.w3.org/TR/xml/#NT-NameStartChar
    //
    //   NameStartChar ::= ":" | [A-Z] | "_" | [a-z] |
    //                     [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] | [#x370-#x37D] | [#x37F-#x1FFF] |
    //                     [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] | [#x3001-#xD7FF] |
    //                     [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
    //
    // XML files are allowed to have quite fancy characters in names.
    // However, we disallow those in VGC files.
    //
    return ('a' <= c && c <= 'z' ) || ('A' <= c && c <= 'Z' ) || c == ':'  || c == '_';
}

bool isNameChar_(char c)
{
    // Reference: https://www.w3.org/TR/xml/#NT-NameChar
    //
    //   NameChar ::= NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
    //
    // Note: #xB7 is the middle-dot. It's allow in XML but we don't.
    //
    // XML files are allowed to have quite fancy characters in names.
    // However, we disallow those in VGC files.
    //
    return isNameStartChar_(c) || c == '-' || c == '.' || ('a' <= c && c <= 'z' );
}

class Parser {
public:
    enum class Error {
        None,
        EofInMarkup,       // Encountered EOF while reading markup
        UnsupportedMarkup, // For now, we don't support comments, CDATA section,and doctype declarations.
        InvalidTagName,    // Tag name was invalid
        InvalidStartTag,   // Start tag was invalid
        InvalidEndTag,     // End tag was invalid
        SecondRootElement, // Second root element (see SecondRootElementError)
        WrongChildType     // Wrong child type (see WrongChildType)
    };

    Parser(std::ifstream& in, Document* document) :
        in_(in),
        error_(Error::None),
        currentNode_(document)
    {
        readAll_();
    }

    Error error() const {
        return error_;
    }

    // Returns a relevant tag name if and error occured
    const std::string& tagName() const {
        return tagName_;
    }

private:
    std::ifstream& in_;
    Error error_;
    Node* currentNode_;
    std::string tagName_;

    void readAll_()
    {
        char c;
        while (error_ == Error::None && in_ >> c) {
            if (c == '<') {
                readMarkup_();
            }
            else {
                // For now, we ignore everything that is not markup.
            }
        }
    }

    void readMarkup_()
    {
        char c;
        if (in_ >> c) {
            if (c == '?') {
                readProcessingInstruction_();
            }
            else if (c == '/') {
                readEndTag_();
            }
            else if (c == '!') {
                error_ = Error::UnsupportedMarkup;
                return;
            }
            else {
                readStartTag_(c);
            }
        }
        else {
            error_ = Error::EofInMarkup;
            return;
        }
    }

    void readProcessingInstruction_()
    {
        // Note: '<?' already read.
        // For now, we just ignore the whole PI
        bool isClosed = false;
        char c;
        while (!isClosed && in_ >> c) {
            if (c == '>') { // TODO: what if it is within quotes?
                isClosed = true;
            }
            else {
                // Keep reading
            }
        }
        if (!isClosed) {
            error_ = Error::EofInMarkup;
            return;
        }
    }

    void readStartTag_(char c)
    {
        bool isEmpty = false;
        bool isClosed = readTagName_(c, &isEmpty);
        if (error_ != Error::None) {
            return;
        }

        onStartTag_();
        if (error_ != Error::None) {
            return;
        }

        while (!isClosed && in_ >> c) {
            // Reading attributes or whitespaces until closed
            // For now, ignore attributes, just read and ignore everything.
            if (c == '>') { // TODO: what if it is within quotes?
                isClosed = true;
            }
            else {
                // Keep reading
            }
        }
        if (!isClosed) {
            error_ = Error::EofInMarkup;
            return;
        }

        if (isEmpty) {
            onEndTag_();
        }
    }

    void readEndTag_()
    {
        char c;
        if (!(in_ >> c)) {
            error_ = Error::EofInMarkup;
            return;
        }

        bool isClosed = readTagName_(c);
        if (error_ != Error::None) {
            return;
        }

        while (!isClosed && in_ >> c) {
            if (isWhitespace_(c)) {
                // Keep reading whitespaces
            }
            else if (c == '>') {
                isClosed = true;
            }
            else {
                error_ = Error::InvalidEndTag;
                return;
            }
        }

        if (!isClosed) {
            error_ = Error::EofInMarkup;
            return;
        }

        onEndTag_();
        if (error_ != Error::None) {
            return;
        }
    }

    void onStartTag_()
    {
        if (currentNode_->nodeType() == NodeType::Document) {
            Document* doc = Document::cast(currentNode_);
            if (doc->rootElement()) {
                error_ = Error::SecondRootElement;
                return;
            }
            else {
                currentNode_ = Element::create(doc, tagName_);
            }
        }
        else if (currentNode_->nodeType() == NodeType::Element) {
            Element* element = Element::cast(currentNode_);
            currentNode_ = Element::create(element, tagName_);
        }
        else {
            error_ = Error::WrongChildType;
            return;
        }
    }

    void onEndTag_()
    {
        if (!currentNode_ || currentNode_->nodeType() != NodeType::Element) {
            // End tag with no corresponding start tag!
            error_ = Error::InvalidEndTag;
        }
        else if (tagName_ != Element::cast(currentNode_)->name().string()) {
            // End tagName doesn't match corresponding start tagName!
            error_ = Error::InvalidEndTag;
        }
        else {
            currentNode_ = currentNode_->parent();
            if (currentNode_->nodeType() == NodeType::Element) {
                tagName_ = Element::cast(currentNode_)->name().string();
            }
            else {
                tagName_.clear();
            }
        }
    }

    // Use empty = nullptr when reading the name of an end tag, and a non-null
    // pointer to a bool when reading a start tag. If non-null, it is used as
    // an output param to indicate whether the start tag was in was an empty
    // element tag (e.g., <tagname/>).
    //
    // Returned whether the tag was closed. Exhaustive cases below.
    //
    // If empty == nullptr:
    //     "</tagname " => returns false
    //     "</tagname>" => returns true
    //
    // If empty != nullptr:
    // 1. "<tagname " => returns false, don't set *empty
    // 1. "<tagname>" => returns true, set *empty = false
    // 1. "<tagname/>" => returns true, set *empty = true
    //
    // Return value is undefined on error. Check error_.
    //
    bool readTagName_(char c, bool* empty = nullptr)
    {
        bool isClosed = false;

        tagName_.clear();
        tagName_ += c;

        if (!isNameStartChar_(c)) {
            error_ = Error::InvalidTagName;
            return false;
        }

        bool done = false;
        while (!done && in_ >> c) {
            if (isNameChar_(c)) {
                tagName_ += c;
            }
            else if (isWhitespace_(c)) {
                done = true;
                isClosed = false;
            }
            else if (c == '>') {
                done = true;
                isClosed = true;
                if (empty) {
                    *empty = false;
                }
            }
            else if (c == '/') {
                if (!empty) {
                    // The following is not a valid end tag: </tagname/>
                    error_ = Error::InvalidEndTag;
                    return false;
                }
                if (!(in_ >> c)) {
                    error_ = Error::EofInMarkup;
                    return false;
                }
                if (c != '>') {
                    // Character '/' must be immediately followed by '>' in start tag
                    error_ = Error::InvalidStartTag;
                    return false;
                }
                // => c = '>'
                done = true;
                isClosed = true;
                *empty = true;
            }
            else {
                error_ = Error::InvalidTagName;
                return false;
            }
        }

        return isClosed;
    }
};

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

    // Parse file and populate document
    Parser parser(in, res.get());
    if (parser.error() != Parser::Error::None) {
        std::string errorString;
        switch (parser.error()) {
        case Parser::Error::None: errorString = "Unknown error"; break;
        case Parser::Error::EofInMarkup: errorString = "End-of-file encountered while reading markup"; break;
        case Parser::Error::UnsupportedMarkup: errorString = "Unsupported markup encountered (example: comment, CDATA, DOCTYPE)"; break;
        case Parser::Error::InvalidTagName: errorString = "Tag name " + parser.tagName() + " not valid"; break;
        case Parser::Error::InvalidStartTag: errorString = "Start tag " + parser.tagName() + " not valid"; break;
        case Parser::Error::InvalidEndTag: errorString = "End tag " + parser.tagName() + " not valid"; break;
        case Parser::Error::SecondRootElement: errorString = "Second root element"; break;
        case Parser::Error::WrongChildType: errorString = "Wrong child type"; break;
        }
        vgc::core::warning() << "File " << filePath << "is not a well-formed VGC file: " << errorString << std::endl;
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
