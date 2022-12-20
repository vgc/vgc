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

#include <vgc/dom/document.h>

#include <cerrno>
#include <cstring>
#include <fstream>

#include <vgc/core/logging.h>
#include <vgc/dom/element.h>
#include <vgc/dom/io.h>
#include <vgc/dom/operation.h>
#include <vgc/dom/schema.h>
#include <vgc/dom/strings.h>

namespace vgc::dom {

Document::Document()
    : Node(this, NodeType::Document)
    , hasXmlDeclaration_(true)
    , hasXmlEncoding_(true)
    , hasXmlStandalone_(true)
    , xmlVersion_("1.0")
    , xmlEncoding_("UTF-8")
    , xmlStandalone_(false)
    , versionId_(core::genId()) {

    generateXmlDeclaration_();
}

/* static */
DocumentPtr Document::create() {
    return DocumentPtr(new Document());
}

namespace {

bool isWhitespace_(char c) {
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

bool isNameStartChar_(char c) {
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
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == ':' || c == '_';
}

bool isNameChar_(char c) {
    // Reference: https://www.w3.org/TR/xml/#NT-NameChar
    //
    //   NameChar ::= NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
    //
    // Note: #xB7 is the middle-dot. It's allow in XML but we don't.
    //
    // XML files are allowed to have quite fancy characters in names.
    // However, we disallow those in VGC files.
    //
    return isNameStartChar_(c) || c == '-' || c == '.' || ('a' <= c && c <= 'z');
}

class Parser {
public:
    static DocumentPtr parse(std::ifstream& in) {
        DocumentPtr res = Document::create();
        Parser parser(in, res.get());
        parser.readAll_();
        return res;
    }

private:
    std::ifstream& in_;
    Node* currentNode_;
    std::string tagName_;
    const ElementSpec* elementSpec_;
    std::string attributeName_;
    std::string attributeValue_;
    std::string referenceName_;

    // Create the parser object
    Parser(std::ifstream& in, Document* document)
        : in_(in)
        , currentNode_(document)
        , elementSpec_(nullptr) {

        readAll_();
    }

    // Main function. Nothing read yet.
    void readAll_() {
        char c;
        while (in_.get(c)) {
            if (c == '<') {
                readMarkup_();
            }
            else {
                // For now, we ignore everything that is not markup.
            }
        }
    }

    // Read from '<' (not included) to matching '>' (included)
    void readMarkup_() {
        char c;
        if (in_.get(c)) {
            if (c == '?') {
                readProcessingInstruction_();
            }
            else if (c == '/') {
                readEndTag_();
            }
            else if (c == '!') {
                throw XmlSyntaxError("Unexpected '<!': Comments, CDATA sections, and "
                                     "DOCTYPE declaration are not yet supported.");
            }
            else {
                readStartTag_(c);
            }
        }
        else {
            throw XmlSyntaxError("Unexpected end-of-file after reading '<' in markup. "
                                 "Expected '?', '/', '!', or tag name.");
        }
    }

    // Read from '<?' (not included) to matching '?>' (included). For now, we
    // also use this function to read the XML declaration, even though it is
    // technically not a PI. In the future, we may want to actually read the
    // content of the XML declaration, and check that it is valid and that
    // encoding is UTF-8 (the only encoding supported in VGC files).
    void readProcessingInstruction_() {
        // PI       ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
        // PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))

        // For now, for simplicity, we accept PIs even if they don't start with
        // a valid name
        bool isClosed = false;
        char c;
        while (!isClosed && in_.get(c)) {
            if (c == '?') {
                if (!in_.get(c)) {
                    throw XmlSyntaxError(
                        "Unexpected end-of-file after reading '?' in processing "
                        "instruction. Expected '>' or further instructions.");
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
            throw XmlSyntaxError("Unexpected end-of-file while reading processing "
                                 "instruction. Expected '?>' or further instructions.");
        }
    }

    // Read from '<c' (not included) to matching '>' or '/>' (included)
    void readStartTag_(char c) {
        bool isEmpty = false;
        bool isClosed = readTagName_(c, &isEmpty);

        onStartTag_();

        // Reading attributes or whitespaces until closed
        while (!isClosed && in_.get(c)) {
            if (c == '>') {
                isClosed = true;
                isEmpty = false;
            }
            else if (c == '/') {
                // '/' must be immediately followed by '>'
                if (!(in_.get(c))) {
                    throw XmlSyntaxError(
                        "Unexpected end-of-file after reading '/' in start tag '"
                        + tagName_ + "'. Expected '>'.");
                }
                else if (c == '>') {
                    isClosed = true;
                    isEmpty = true;
                }
                else {
                    throw XmlSyntaxError(
                        std::string("Unexpected '") + c
                        + "' after reading '/' in start tag '" + tagName_
                        + "'. Expected '>'.");
                }
            }
            else if (isWhitespace_(c)) {
                // Keep reading
            }
            else {
                readAttribute_(c);
            }
        }
        if (!isClosed) {
            throw XmlSyntaxError(
                "Unexpected end-of-file while reading start tag '" + tagName_
                + "'. Expected whitespaces, attribute name, '>', or '/>'");
        }

        if (isEmpty) {
            onEndTag_();
        }
    }

    // Read from '</' (not included) to matching '>' (included)
    void readEndTag_() {
        char c;
        if (!(in_.get(c))) {
            throw XmlSyntaxError("Unexpected end-of-file after reading '</' in end tag. "
                                 "Expected tag name.");
        }

        bool isClosed = readTagName_(c);

        while (!isClosed && in_.get(c)) {
            if (isWhitespace_(c)) {
                // Keep reading whitespaces
            }
            else if (c == '>') {
                isClosed = true;
            }
            else {
                throw XmlSyntaxError(
                    std::string("Unexpected '") + c + "' while reading end tag '"
                    + tagName_ + "'. Expected whitespaces or '>'.");
            }
        }
        if (!isClosed) {
            throw XmlSyntaxError(
                "Unexpected end-of-file while reading end tag '" + tagName_
                + "'. Expected whitespaces or '>'.");
        }

        onEndTag_();
    }

    // Action to be performed when a start tag is encountered.
    // The name of the tag is available in tagName_.
    void onStartTag_() {
        elementSpec_ = schema().findElementSpec(tagName_);
        if (!elementSpec_) {
            throw VgcSyntaxError(
                "Unknown element name '" + tagName_
                + "'. Excepted an element name defined in the VGC schema.");
        }

        if (currentNode_->nodeType() == NodeType::Document) {
            Document* doc = Document::cast(currentNode_);
            if (doc->rootElement()) {
                throw XmlSyntaxError(
                    "Unexpected second root element '" + tagName_ + "'. A root element '"
                    + doc->rootElement()->tagName().string()
                    + "' has already been defined, and there cannot be more than one.");
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
            // Note: this cannot happen yet, but we keep it as safeguard for
            // the future when adding more node types, or when restricting
            // in the schema which elements are allowed to be children of other
            // elements.
            throw XmlSyntaxError(
                "Unexpected element '" + tagName_
                + "'. Elements of this type are not allowed as children of the current "
                  "node type '"
                + core::toString(currentNode_->nodeType()) + "'.");
        }
    }

    // Action to be performed when an end tag (or the closing '/>' of an empty
    // element tag) is encountered. The name of the tag is available in
    // tagName_.
    void onEndTag_() {
        if (!currentNode_ || currentNode_->nodeType() != NodeType::Element) {
            throw XmlSyntaxError(
                "Unexpected end tag '" + tagName_
                + "'. It does not have a matching start tag.");
        }
        else if (tagName_ != Element::cast(currentNode_)->tagName()) {
            throw XmlSyntaxError(
                "Unexpected end tag '" + tagName_ + "'. Its matching start '"
                + Element::cast(currentNode_)->tagName().string()
                + "' has a different name.");
        }
        else {
            currentNode_ = currentNode_->parent();
            if (currentNode_->nodeType() == NodeType::Element) {
                tagName_ = Element::cast(currentNode_)->tagName();
                elementSpec_ = schema().findElementSpec(tagName_);
            }
            else {
                tagName_.clear();
                elementSpec_ = nullptr;
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
    bool readTagName_(char c, bool* empty = nullptr) {
        bool isClosed = false;

        tagName_.clear();
        tagName_ += c;

        if (!isNameStartChar_(c)) {
            throw XmlSyntaxError(
                std::string("Unexpected '") + c
                + "' while reading start character of tag name. Expected valid name "
                  "start character.");
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
                    throw XmlSyntaxError(
                        "Unexpected '/' while reading end tag name '" + tagName_
                        + "'. Expected valid name characters, whitespaces, or '>'.");
                }
                if (in_.get(c)) {
                    if (c == '>') {
                        done = true;
                        isClosed = true;
                        *empty = true;
                    }
                    else {
                        throw XmlSyntaxError(
                            "Unexpected end-of-file after reading '/' after reading "
                            "start tag name '"
                            + tagName_ + "'. Expected '>'.");
                    }
                }
                else {
                    throw XmlSyntaxError(
                        "Unexpected end-of-file after reading '/' after reading start "
                        "tag name '"
                        + tagName_ + "'. Expected '>'.");
                }
            }
            else {
                throw XmlSyntaxError(
                    std::string("Unexpected '") + c + "' while reading "
                    + (empty ? "start" : "end") + " tag name '" + tagName_
                    + "'. Expected valid name characters, whitespaces, "
                    + (empty ? "'>', or '/>" : "or '>'") + ".");
            }
        }
        if (!done) {
            throw XmlSyntaxError(
                std::string("Unexpected end-of-file while reading ")
                + (empty ? "start" : "end") + " tag name '" + tagName_
                + "'. Expected valid name characters, whitespaces, "
                + (empty ? "'>', or '/>" : "or '>'") + ".");
        }

        return isClosed;
    }

    // Read from given first character \p c (not included) to '=' (included)
    void readAttribute_(char c) {
        // Attribute ::= Name Eq AttValue
        // Eq        ::= S? '=' S?
        // AttValue  ::= '"' ([^<&"] | Reference)* '"'
        //            |  "'" ([^<&'] | Reference)* "'"

        readAttributeName_(c);
        readAttributeValue_();
        onAttribute_();
    }

    // Read from given first character \p c (not included) to '=' (included)
    void readAttributeName_(char c) {
        attributeName_.clear();
        attributeName_ += c;

        if (!isNameStartChar_(c)) {
            throw XmlSyntaxError(
                std::string("Unexpected '") + c
                + "' while reading start character of attribute name in start tag "
                + tagName_ + ". Expected valid name start character.");
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
                throw XmlSyntaxError(
                    std::string("Unexpected '") + c + "' while reading attribute name '"
                    + attributeName_ + "' in start tag '" + tagName_
                    + "'. Expected valid name characters, whitespaces, or '='.");
            }
        }
        if (!isNameRead) {
            throw XmlSyntaxError(
                "Unexpected end-of-file while reading attribute name '" + attributeName_
                + "' in start tag '" + tagName_
                + "'. Expected valid name characters, whitespaces, or '='.");
        }

        while (!isEqRead && in_.get(c)) {
            if (c == '=') {
                isEqRead = true;
            }
            else if (isWhitespace_(c)) {
                // Keep reading
            }
            else {
                throw XmlSyntaxError(
                    std::string("Unexpected '") + c + "' after reading attribute name '"
                    + attributeName_ + "' in start tag '" + tagName_
                    + "'. Expected whitespaces or '='.");
            }
        }
        if (!isEqRead) {
            throw XmlSyntaxError(
                "Unexpected end-of-file after reading attribute name '" + attributeName_
                + "' in start tag '" + tagName_ + "'. Expected whitespaces or '='.");
        }
    }

    // Read from '=' (not included) to closing '\'' or '\"' (included)
    void readAttributeValue_() {
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
                throw XmlSyntaxError(
                    std::string("Unexpected '") + c
                    + "' after reading '=' after reading attribute name '"
                    + attributeName_ + "' in start tag '" + tagName_
                    + "'. Expected '\"' (double quote), or '\'' (single quote), or "
                      "whitespaces.");
            }
        }
        if (!quoteSign) {
            throw XmlSyntaxError(
                "Unexpected end-of-file after reading '=' "
                "after reading attribute name '"
                + attributeName_ + "' in start tag '" + tagName_
                + "'. Expected '\"' (double quote), or '\'' (single quote), or "
                  "whitespaces.");
        }

        bool isClosed = false;
        while (!isClosed && in_.get(c)) {
            if (c == quoteSign) {
                isClosed = true;
            }
            else if (c == '&') {
                char replacementChar = readReference_();
                attributeValue_ += replacementChar;
            }
            else if (c == '<') {
                // This is illegal XML, so we reject it. In the future, we may
                // want to accept it with a warning, and auto-convert it to
                // &lt; when saving back the file. It is quite unclear why did the
                // W3C decide that '<' was illegal in attribute values.
                throw XmlSyntaxError(
                    "Unexpected '<' while reading value of attribute '" + attributeName_
                    + "' in start tag '" + tagName_
                    + "'. This character is now allowed in attribute values, please "
                      "replace it with '&lt;'.");
            }
            else {
                attributeValue_ += c;
            }
        }
        if (!isClosed) {
            throw XmlSyntaxError(
                "Unexpected end-of-file while reading value of attribute '"
                + attributeName_ + "' in start tag '" + tagName_
                + "'. Expected more characters or the closing quote '" + quoteSign
                + "'.");
        }
    }

    // Action to be performed when an element attribute is encountered. The
    // attribute name and string value are available in attributeName_ and
    // attributeValue_.
    void onAttribute_() {
        core::StringId name(attributeName_);

        ValueType valueType = {};
        if (name == strings::name) {
            valueType = ValueType::StringId;
        }
        else if (name == strings::id) {
            valueType = ValueType::StringId;
        }
        else {
            const AttributeSpec* spec = elementSpec_->findAttributeSpec(name);
            if (!spec) {
                throw VgcSyntaxError(
                    "Unknown attribute '" + attributeName_ + "' for element '" + tagName_
                    + "'. Excepted an attribute name defined in the VGC schema.");
            }
            valueType = spec->valueType();
        }

        Value value = parseValue(attributeValue_, valueType);
        Element::cast(currentNode_)->setAttribute(name, value);
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
    char readReference_() {
        // Reference ::= EntityRef | CharRef
        // EntityRef ::= '&' Name ';'
        // CharRef   ::= '&#' [0-9]+ ';'
        //            |  '&#x' [0-9a-fA-F]+ ';'

        referenceName_.clear();

        char c;
        if (in_.get(c)) {
            referenceName_ += c;
            if (!isNameStartChar_(c)) {
                throw XmlSyntaxError(
                    std::string("Unexpected '") + c
                    + "' while reading start character of entity reference name. "
                      "Expected valid name start character.");
            }
        }
        else {
            throw XmlSyntaxError(
                "Unexpected end-of-file while reading start character of entity "
                "reference name. Expected valid name start character.");
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
                throw XmlSyntaxError(
                    std::string("Unexpected '") + c
                    + "' while reading entity reference name '" + referenceName_
                    + "'. Expected valid name characters or ';'.");
            }
        }
        if (!isSemicolonRead) {
            throw XmlSyntaxError(
                "Unexpected end-of-file while reading entity reference name '"
                + referenceName_ + "'. Expected valid name characters or ';'.");
        }

        const std::vector<std::pair<const char*, char>> table = {
            {"amp", '&'},
            {"lt", '<'},
            {"gt", '>'},
            {"apos", '\''},
            {"quot", '\"'},
        };

        for (const auto& pair : table) {
            if (referenceName_ == pair.first) {
                return pair.second;
            }
        }

        throw XmlSyntaxError("Unknown entity reference '&" + referenceName_ + ";'.");
    }
};

} // namespace

/* static */
DocumentPtr Document::open(const std::string& filePath) {
    // Note: in the future, we want to be able to detect formatting style of
    // input XML files, and preserve this style, as well as existing
    // non-significant whitespaces, etc. This is why we write our own parser,
    // since XML parsers typically discard all non-significant data. For
    // example, <foo name="hello"> and <foo    name="hello"> would typically
    // generate the same XmlStream events and we wouldn't be able to preserve
    // the formatting when saving back.

    std::ifstream in(filePath);
    if (!in.is_open()) {
        throw FileError("Cannot open file " + filePath + ": " + std::strerror(errno));
    }

    return Parser::parse(in);
}

Element* Document::rootElement() const {
    for (Node* node : children()) {
        if (node->nodeType() == NodeType::Element) {
            return Element::cast(node);
        }
    }
    return nullptr;
}

std::string Document::xmlDeclaration() const {
    return xmlDeclaration_;
}

bool Document::hasXmlDeclaration() const {
    return hasXmlDeclaration_;
}

void Document::setXmlDeclaration() {
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlDeclaration() {
    hasXmlDeclaration_ = false;
    generateXmlDeclaration_();
}

std::string Document::xmlVersion() const {
    return xmlVersion_;
}

void Document::setXmlVersion(const std::string& version) {
    xmlVersion_ = version;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

std::string Document::xmlEncoding() const {
    return xmlEncoding_;
}

bool Document::hasXmlEncoding() const {
    return hasXmlEncoding_;
}

void Document::setXmlEncoding(const std::string& encoding) {
    xmlEncoding_ = encoding;
    hasXmlEncoding_ = true;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlEncoding() {
    xmlEncoding_ = "UTF-8";
    hasXmlEncoding_ = false;
    generateXmlDeclaration_();
}

bool Document::xmlStandalone() const {
    return xmlStandalone_;
}

bool Document::hasXmlStandalone() const {
    return hasXmlStandalone_;
}

void Document::setXmlStandalone(bool standalone) {
    xmlStandalone_ = standalone;
    hasXmlStandalone_ = true;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlStandalone() {
    xmlStandalone_ = false;
    hasXmlStandalone_ = false;
    generateXmlDeclaration_();
}

void Document::generateXmlDeclaration_() {
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

void Document::save(const std::string& filePath, const XmlFormattingStyle& style) const {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        throw FileError("Cannot save file " + filePath + ": " + std::strerror(errno));
    }

    out << xmlDeclaration_ << std::endl;
    writeChildren(out, style, 0, this);
}

Element* Document::elementById(core::StringId id) const {
    auto it = elementByIdMap_.find(id);
    if (it != elementByIdMap_.end()) {
        return it->second;
    }
    return nullptr;
}

Element* Document::elementByInternalId(core::Id id) const {
    auto it = elementByInternalIdMap_.find(id);
    if (it != elementByInternalIdMap_.end()) {
        return it->second;
    }
    return nullptr;
}

core::History* Document::enableHistory(core::StringId entrypointName) {
    if (!history_) {
        history_ = core::History::create(entrypointName);
        history_->headChanged().connect(onHistoryHeadChanged());
    }
    return history_.get();
}

bool Document::emitPendingDiff() {
    for (const auto& [node, oldRelatives] : previousRelativesMap_) {
        if (pendingDiff_.createdNodes_.contains(node)) {
            continue;
        }
        if (pendingDiff_.removedNodes_.contains(node)) {
            continue;
        }

        NodeRelatives newRelatives(node);
        if (oldRelatives.parent_ != newRelatives.parent_) {
            pendingDiff_.reparentedNodes_.insert(node);
        }
        else if (
            oldRelatives.nextSibling_ != newRelatives.nextSibling_
            || oldRelatives.previousSibling_ != newRelatives.previousSibling_) {
            // this introduces false positives if old siblings were removed or
            // new siblings were added.
            pendingDiff_.childrenReorderedNodes_.insert(newRelatives.parent_);
        }
    }
    previousRelativesMap_.clear();

    // remove created and removed nodes from modified elements
    auto& modifiedElements = pendingDiff_.modifiedElements_;
    for (auto it = modifiedElements.begin(), last = modifiedElements.end(); it != last;) {
        Node* node = it->first;
        if (pendingDiff_.createdNodes_.contains(node)) {
            it = modifiedElements.erase(it);
            continue;
        }
        if (pendingDiff_.removedNodes_.contains(node)) {
            it = modifiedElements.erase(it);
            continue;
        }
        ++it;
    }

    if (!pendingDiff_.isEmpty()) {

        // XXX todo: emit node signals in here ?

        changed().emit(pendingDiff_);
        pendingDiff_.reset();
        pendingDiffKeepAllocPointers_.clear();
        return true;
    }

    return false;
}

void Document::onHistoryHeadChanged_() {
    emitPendingDiff();
}

void Document::onCreateNode_(Node* node) {
    pendingDiff_.createdNodes_.emplaceLast(node);
}

void Document::onRemoveNode_(Node* node) {
    pendingDiff_.removedNodes_.emplaceLast(node);
    pendingDiffKeepAllocPointers_.emplaceLast(node);
}

void Document::onMoveNode_(Node* node, const NodeRelatives& savedRelatives) {
    previousRelativesMap_.try_emplace(node, savedRelatives);
}

void Document::onChangeAttribute_(Element* element, core::StringId name) {
    pendingDiff_.modifiedElements_[element].insert(name);
}

} // namespace vgc::dom
