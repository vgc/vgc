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

#include <cerrno>
#include <cstring>
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
    // XXX Wouldn't it be easier to simply return a string? We could in fact
    // just raise a ParserError exception with the detailed reason.
    enum class Error {
        None,
        EofInMarkup,           // Encountered EOF while reading markup
        UnsupportedMarkup,     // For now, we don't support comments, CDATA section,and doctype declarations.
        InvalidTagName,        // Tag name was invalid
        InvalidStartTag,       // Start tag was invalid
        InvalidEndTag,         // End tag was invalid
        SecondRootElement,     // Second root element (see SecondRootElementError)
        WrongChildType,        // Wrong child type (see WrongChildType)
        InvalidAttributeName,  // Invalid attribute name
        InvalidAttributeValue, // Invalid attribute value
        InvalidReferenceName,  // Invalid Reference name
        UnknownEntityReference // Unknown entity reference
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
    std::string attributeName_;
    std::string attributeValue_;
    std::string referenceName_;

    // Main function. Nothing read yet.
    void readAll_()
    {
        char c;
        while (error_ == Error::None && in_.get(c)) {
            if (c == '<') {
                readMarkup_();
            }
            else {
                // For now, we ignore everything that is not markup.
            }
        }
    }

    // Read from '<' (not included) to matching '>' (included)
    void readMarkup_()
    {
        char c;
        if (in_.get(c)) {
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

    // Read from '<?' (not included) to matching '?>' (included). For now, we
    // also use this function to read the XML declaration, even though it is
    // technically not a PI. In the future, we may want to actually read the
    // content of the XML declaration, and check that it is valid and that
    // encoding is UTF-8 (the only encoding supported in VGC files).
    void readProcessingInstruction_()
    {
        // PI       ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
        // PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))

        // For now, for simplicity, we accept PIs even if they don't start with a valid name
        bool isClosed = false;
        char c;
        while (!isClosed && in_.get(c)) {
            if (c == '?') {
                if (!in_.get(c)) {
                    error_ = Error::EofInMarkup;
                    return;
                }
                else if (c == '>') {
                    isClosed = true;
                }
                else {
                    // Keep reading PI
                }
            }
            else {
                // Keep reading PI
            }
        }
        if (!isClosed) {
            error_ = Error::EofInMarkup;
            return;
        }
    }

    // Read from '<c' (not included) to matching '>' or '/>' (included)
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

        // Reading attributes or whitespaces until closed
        while (!isClosed && in_.get(c)) {
            if (c == '>') {
                isClosed = true;
                isEmpty = false;
            }
            else if (c == '/') {
                // '/' must be immediately followed by '>'
                if (!(in_.get(c))) {
                    error_ = Error::EofInMarkup;
                    return;
                }
                else if (c == '>') {
                    isClosed = true;
                    isEmpty = true;
                }
                else {
                    error_ = Error::InvalidStartTag;
                    return;
                }
            }
            else if (isWhitespace_(c)) {
                // Keep reading
            }
            else {
                readAttribute_(c);
                if (error_ != Error::None) {
                    return;
                }
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

    // Read from '</' (not included) to matching '>' (included)
    void readEndTag_()
    {
        char c;
        if (!(in_.get(c))) {
            error_ = Error::EofInMarkup;
            return;
        }

        bool isClosed = readTagName_(c);
        if (error_ != Error::None) {
            return;
        }

        while (!isClosed && in_.get(c)) {
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

    // Action to be performed when a start tag is encountered.
    // The name of the tag is available in tagName_.
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

    // Action to be performed when an end tag (or the closing '/>' of an empty
    // element tag) is encountered. The name of the tag is available in
    // tagName_.
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

    // Read from given first character \p c (not included) to first whitespace
    // character (included), or to '>' or '/>' (included) if it follows
    // immediately the tag name with no whitespaces.
    //
    // You must pass empty = nullptr (the default) when reading the name of an
    // end tag, and you must pass a non-null pointer to a bool when reading a
    // start tag. If non-null, it is used as an output parameter to indicate
    // whether the start tag was in fact an empty element tag (e.g.,
    // <tagname/>).
    //
    // Returns whether the tag was closed. Exhaustive cases below.
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
    // Returned value is undefined on error. Check error_.
    //
    // XXX Wouldn't it be a better design to simply call this readName(c),
    // return the first character after the tag name, and let the caller handle
    // this character?
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
        while (!done && in_.get(c)) {
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
                    tagName_ += c;
                    error_ = Error::InvalidEndTag;
                    return false;
                }
                if (!(in_.get(c))) {
                    error_ = Error::EofInMarkup;
                    return false;
                }
                if (c != '>') {
                    // Character '/' must be immediately followed by '>' in start tag
                    tagName_ += '/';
                    tagName_ += c;
                    error_ = Error::InvalidStartTag;
                    return false;
                }
                // => c = '>'
                done = true;
                isClosed = true;
                *empty = true;
            }
            else {
                tagName_ += c;
                error_ = Error::InvalidTagName;
                return false;
            }
        }
        if (!done) {
            error_ = Error::EofInMarkup;
            return false;
        }

        return isClosed;
    }

    // Read from given first character \p c (not included) to '=' (included)
    void readAttribute_(char c)
    {
        // Attribute ::= Name Eq AttValue
        // Eq        ::= S? '=' S?
        // AttValue  ::= '"' ([^<&"] | Reference)* '"'
        //            |  "'" ([^<&'] | Reference)* "'"

        readAttributeName_(c);
        if (error_ != Error::None) {
            return;
        }

        readAttributeValue_();
        if (error_ != Error::None) {
            return;
        }

        onAttribute_();
    }

    // Read from given first character \p c (not included) to '=' (included)
    void readAttributeName_(char c)
    {
        attributeName_.clear();
        attributeName_ += c;

        if (!isNameStartChar_(c)) {
            error_ = Error::InvalidAttributeName;
            return;
        }

        bool isNameRead = false;
        bool isEqRead = false;
        while (!isNameRead && in_.get(c)) {
            if (isNameChar_(c)) {
                attributeName_ += c;
            }
            else if (c == '=') {
                isNameRead = true;
                isEqRead = true;
            }
            else if (isWhitespace_(c)) {
                isNameRead = true;
            }
            else {
                attributeName_ += c;
                error_ = Error::InvalidAttributeName;
                return;
            }
        }
        if (!isNameRead) {
            error_ = Error::EofInMarkup;
            return;
        }

        while (!isEqRead && in_.get(c)) {
            if (c == '=') {
                isEqRead = true;
            }
            else if (isWhitespace_(c)) {
                // Keep reading
            }
            else {
                // Invalid character encountered between name and '='
                attributeName_ += c;
                error_ = Error::InvalidAttributeName;
                return;
            }
        }
        if (!isEqRead) {
            error_ = Error::EofInMarkup;
            return;
        }
    }

    // Read from '=' (not included) to closing '\'' or '\"' (included)
    void readAttributeValue_()
    {
        attributeValue_.clear();

        char c;
        char quoteSign = 0;
        while (!quoteSign && in_.get(c)) {
            if (c == '\"' || c == '\'') {
                quoteSign = c;
            }
            else if (isWhitespace_(c)) {
                // Keep reading
            }
            else {
                error_ = Error::InvalidAttributeValue;
                return;
            }
        }
        if (!quoteSign) {
            error_ = Error::EofInMarkup;
            return;
        }

        bool isClosed = false;
        while (!isClosed && in_.get(c)) {
            if (c == quoteSign) {
                isClosed = true;
            }
            else if (c == '&') {
                char replacementChar = readReference_();
                if (error_ != Error::None) {
                    return;
                }
                attributeValue_ += replacementChar;
            }
            else if (c == '<') {
                // This is illegal XML, so we reject it. In the future, we may
                // want to accept it with a warning, and auto-convert it to
                // &lt; when saving back the file. It is quite unclear why did the
                // W3C decide that '<' was illegal in attribute values.
                attributeValue_ += c;
                error_ = Error::InvalidAttributeValue;
                return;
            }
            else {
                attributeValue_ += c;
            }
        }
        if (!isClosed) {
            error_ = Error::EofInMarkup;
            return;
        }
    }

    // Action to be performed when an element attribute is encountered. The
    // attribute name and string value are available in attributeName_ and
    // attributeValue_.
    void onAttribute_()
    {
        // XXX TODO
        std::cout << "Found attribute \n  name = " << attributeName_ << "\n  value = " << attributeValue_ << std::endl;
    }

    // Read from '&' (not included) to ';' (included). Returns the character
    // represented by the character entity. Supported entities are:
    //   &amp;   -->  &
    //   &lt;    -->  <
    //   &gt;    -->  >
    //   &apos;  -->  '
    //   &quot;  -->  "
    // XXX We do not yet support character references, that is, unicode
    // codes such as '&#...;'
    // TODO support them
    char readReference_()
    {
        // Reference ::= EntityRef | CharRef
        // EntityRef ::= '&' Name ';'
        // CharRef   ::= '&#' [0-9]+ ';'
        //            |  '&#x' [0-9a-fA-F]+ ';'

        referenceName_.clear();

        char c;
        if (in_.get(c)) {
            referenceName_ += c;
            if (!isNameStartChar_(c)) {
                error_ = Error::InvalidReferenceName;
                return ' ';
            }
        }
        else {
            error_ = Error::EofInMarkup;
            return ' ';
        }

        bool isSemicolonRead = false;
        while (!isSemicolonRead && in_.get(c)) {
            if (isNameChar_(c)) {
                referenceName_ += c;
            }
            else if (c == ';') {
                isSemicolonRead = true;
            }
            else {
                error_ = Error::InvalidReferenceName;
                return ' ';
            }
        }
        if (!isSemicolonRead) {
            error_ = Error::EofInMarkup;
            return ' ';
        }

        const std::vector<std::pair<const char*, char>> table = {
            {"amp", '&'},
            {"lt", '<'},
            {"gt", '>'},
            {"apos", '\''},
            {"quot", '\"'},
        };

        for (const auto& pair: table) {
            if (referenceName_ == pair.first) {
                return pair.second;
            }
        }

        error_ = Error::UnknownEntityReference;
        return ' ';
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
        case Parser::Error::InvalidAttributeName: errorString = "Invalid attribute name"; break;
        case Parser::Error::InvalidAttributeValue: errorString = "Invalid attribute value"; break;
        case Parser::Error::InvalidReferenceName: errorString = "Invalid Reference name"; break;
        case Parser::Error::UnknownEntityReference: errorString = "Unknown entity reference"; break;
        }
        vgc::core::warning() << "File " << filePath << " is not a well-formed VGC file: " << errorString << std::endl;
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

void Document::save(const std::string& filePath,
                    const XmlFormattingStyle& style) const
{
    checkAlive();

    std::ofstream out(filePath);
    if (!out.is_open()) {
        throw FileError("Cannot save file " + filePath + ": " +  std::strerror(errno));
    }

    out << xmlDeclaration_ << std::endl;    
    writeChildren(out, style, 0, this);
}

} // namespace dom
} // namespace vgc
