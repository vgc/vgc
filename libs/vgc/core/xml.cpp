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

#include <vgc/core/xml.h>

#include <vgc/core/assert.h>
#include <vgc/core/format.h>
#include <vgc/core/io.h>

namespace vgc::core {

VGC_DEFINE_ENUM(
    XmlTokenType,
    (None, "None"),
    (Invalid, "Invalid"),
    (StartDocument, "StartDocument"),
    (EndDocument, "EndDocument"),
    (StartElement, "StartElement"),
    (EndElement, "EndElement"),
    (CharacterData, "CharacterData"),
    (Comment, "Comment"),
    (ProcessingInstruction, "ProcessingInstruction"))

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
    return isNameStartChar_(c) || c == '-' || c == '.' || ('0' <= c && c <= '9');
}

} // namespace

namespace detail {

class XmlStreamReaderImpl {
public:
    std::string ownedData; // Empty if constructed via fromView()
    std::string_view data; // Points either to ownedData or external data
    const char* end = {};  // End of the data

    XmlTokenType tokenType = {};
    const char* tokenStart = {}; // Start of the last read token
    const char* tokenEnd = {};   // End of the last read token

    const char* cursor = {}; // Current cursor position while parsing

    // Name of a StartElement or EndElement
    const char* nameStart = {};
    const char* nameEnd = {};
    std::string_view name() const {
        return std::string_view(nameStart, nameEnd - nameStart);
    }

    bool isSelfClosing = {}; // Whether the current start element is self-closing

    // Content of CharacterData
    std::string characterData;

    // Store attribute data.
    //
    // Note: we use arrays of size `reservedAttributes_`, for two reasons:
    //
    // 1. Avoid destructing `XmlStreamAttributeData` objects, so that
    //    we can reuse the capacity of data.value strings.
    //
    // 2. Control exactly when are the array resized, so that we can update the
    //    `XmlStreamAttributeData` pointers in `XmlStreamAttributeView` objects.
    //
    Int numAttributes_ = 0;      // semantic size of the arrays
    Int reservedAttributes_ = 0; // actual size of the arrays
    core::Array<detail::XmlStreamAttributeData> attributesData_;
    core::Array<XmlStreamAttributeView> attributes_;
    ConstSpan<XmlStreamAttributeView> attributes() const {
        return ConstSpan<XmlStreamAttributeView>(attributes_.data(), numAttributes_);
    }
    void clearAttributes() {
        numAttributes_ = 0;
    }
    detail::XmlStreamAttributeData& appendAttribute() {
        Int indexOfAppendedAttribute = numAttributes_;
        ++numAttributes_;
        if (reservedAttributes_ < numAttributes_) {
            if (reservedAttributes_ < 8) {
                reservedAttributes_ = 8; // [1]
            }
            else {
                reservedAttributes_ *= 2; // [2]
            }
            // We now have numAttributes <= reservedAttributes_.
            //
            // Proof:
            // - Entering the function: numAttributes_ <=  reservedAttributes_ (invariant)
            // - Executing first line:  numAttributes_ <=  reservedAttributes_ + 1
            // - Executing inner if: reservedAttributes_ (= k) increases by at least one
            //   [1] k in [0..7]:  k' = 8     (> k)
            //   [2] k in [8...]:  k' = 2 * k (> k)
            //
            attributesData_.resize(reservedAttributes_, {});
            attributes_.resize(reservedAttributes_, {});
            for (Int i = 0; i < reservedAttributes_; ++i) {
                attributes_.getUnchecked(i).d_ = &attributesData_.getUnchecked(i);
            }
        }
        return attributesData_.getUnchecked(indexOfAppendedAttribute);
    }

    void init() {
        const char* start = data.data();
        end = start + data.size();
        tokenType = XmlTokenType::StartDocument;
        tokenStart = start;
        tokenEnd = start;
        cursor = start;
    }

    bool readNext() {
        tokenStart = tokenEnd; // == cursor
        bool res = readNext_();
        tokenEnd = cursor;
        return res;
    }

private:
    // Same API as std::ifstream, to make it easier to support streams later.
    bool get(char& c) {
        if (cursor == end) {
            return false;
        }
        else {
            c = *cursor;
            ++cursor;
            return true;
        }
    }

    void unget() {
        --cursor;
    }

    bool readNext_() {
        if (cursor == end) {
            tokenType = XmlTokenType::EndDocument;
            return false;
        }
        else if (tokenType == XmlTokenType::StartElement && isSelfClosing) {
            tokenType = XmlTokenType::EndElement;
            onEndTag_();
            return true;
        }
        else {
            char c;
            get(c);
            if (c == '<') {
                readMarkup_();
            }
            else {
                unget();
                readCharacterData_();
            }
            return true;
        }
    }

    // https://www.w3.org/TR/REC-xml/#syntax
    //
    // CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*)
    //
    // => Everything that does not include `<`, `&`, or `]]>`.
    //
    // TODO: any white space that is at the top level of the document entity (that is, outside the document element and not inside any other markup)
    // should normally not be considered "character data". However, we do want to report it somehow.
    //
    // Maybe as part of the "DocumentStart", and "DocumentEnd" tokens?
    //
    void readCharacterData_() {
        tokenType = XmlTokenType::CharacterData;
        characterData.clear();
        char c;
        while (get(c)) {
            if (c == '&') {
                // character reference or entity reference.
                c = readReference_();
            }
            else if (c == '<') {
                unget();
                break;
            }
            else if (c == ']') {
                if (get(c) && c == ']') {
                    if (get(c) && c == '>') {
                        throw XmlSyntaxError("Unexpected `]]>` outside CDATA section.");
                    }
                }
            }
            characterData += c;
        };
    }

    // Read from '<' (not included) to matching '>' (included)
    void readMarkup_() {
        char c;
        if (get(c)) {
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
    // encoding is UTF-8 (the only encoding that we support).
    //
    void readProcessingInstruction_() {
        // PI       ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
        // PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))

        tokenType = XmlTokenType::ProcessingInstruction;

        // For now, for simplicity, we accept PIs even if they don't start with
        // a valid name
        bool isClosed = false;
        char c;
        while (!isClosed && get(c)) {
            if (c == '?') {
                if (!get(c)) {
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
    //
    void readStartTag_(char c) {

        tokenType = XmlTokenType::StartElement;

        bool isAngleBracketClosed = readTagName_(c, &isSelfClosing);
        onStartTag_();

        // Initialize attributes.
        // Note: we set attributeStart now so that it includes leading whitespace.
        clearAttributes();
        const char* attributeStart = cursor;

        // Reading attributes or whitespaces until closed
        while (!isAngleBracketClosed && get(c)) {
            if (c == '>') {
                isAngleBracketClosed = true;
                isSelfClosing = false;
            }
            else if (c == '/') {
                // '/' must be immediately followed by '>'
                if (!(get(c))) {
                    throw XmlSyntaxError(format(
                        "Unexpected end-of-file after reading '/' in start tag '{}'. "
                        "Expected '>'.",
                        name()));
                }
                else if (c == '>') {
                    isAngleBracketClosed = true;
                    isSelfClosing = true;
                }
                else {
                    throw XmlSyntaxError(format(
                        "Unexpected '{}' after reading '/' in start tag '{}'. "
                        "Expected '>'.",
                        c,
                        name()));
                }
            }
            else if (isWhitespace_(c)) {
                // Keep reading
            }
            else {
                // Append a new attribute and initialize its rawText so that
                // attributeStart is available in readAttribute_(c)
                XmlStreamAttributeData& attribute = appendAttribute();
                attribute.rawText = std::string_view(attributeStart, 1);

                // Read the attribute
                readAttribute_(attribute, c);

                // Set the value of rawText_ and attributeStart of next attribute
                Int n = cursor - attributeStart;
                attribute.rawText = std::string_view(attributeStart, n);
                attributeStart = cursor;
            }
        }
        if (!isAngleBracketClosed) {
            throw XmlSyntaxError(format(
                "Unexpected end-of-file while reading start tag '{}'. "
                "Expected whitespaces, attribute name, '>', or '/>'",
                name()));
        }
    }

    // Read from '</' (not included) to matching '>' (included)
    void readEndTag_() {
        char c;
        if (!(get(c))) {
            throw XmlSyntaxError("Unexpected end-of-file after reading '</' in end tag. "
                                 "Expected tag name.");
        }

        tokenType = XmlTokenType::EndElement;
        bool isAngleBracketClosed = readTagName_(c);

        while (!isAngleBracketClosed && get(c)) {
            if (isWhitespace_(c)) {
                // Keep reading whitespaces
            }
            else if (c == '>') {
                isAngleBracketClosed = true;
            }
            else {
                throw XmlSyntaxError(format(
                    "Unexpected '{}' while reading end tag '{}'. "
                    "Expected whitespaces or '>'.",
                    c,
                    name()));
            }
        }
        if (!isAngleBracketClosed) {
            throw XmlSyntaxError(format(
                "Unexpected end-of-file while reading end tag '{}'. "
                "Expected whitespaces or '>'.",
                name()));
        }

        onEndTag_();
    }

    // Action to be performed when a start tag is encountered.
    // The name of the tag is available in tagName_.
    void onStartTag_() {
        /*
        elementSpec_ = schema().findElementSpec(tagName_);
        if (!elementSpec_) {
            throw VgcSyntaxError(
                "Unknown element name '" + tagName_
                + "'. Expected an element name defined in the VGC schema.");
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
        */
    }

    // Action to be performed when an end tag (or the closing '/>' of an empty
    // element tag) is encountered. The name of the tag is available in
    // tagName_.
    void onEndTag_() {
        /*
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
        */
    }

    // Read from given first character \p c (not included) to first whitespace
    // character (included), or to '>' or '/>' (included) if it follows
    // immediately the tag name with no whitespaces.
    //
    // You must pass empty = nullptr (the default) when reading the name of an
    // end tag, and you must pass a non-null pointer to a bool when reading a
    // start tag. If non-null, it is used as an output parameter to indicate
    // whether the start tag was in fact a self-closing element tag (e.g.,
    // <tagname/>).
    //
    // Returns whether the angle bracket of tag was closed. Exhaustive cases below.
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
    // XXX Wouldn't it be a better design to simply call this readName(c),
    // return the first character after the tag name, and let the caller handle
    // this character?
    //
    bool readTagName_(char c, bool* isSelfClosing = nullptr) {

        if (!isNameStartChar_(c)) {
            throw XmlSyntaxError(format(
                "Unexpected '{}' while reading start character of tag name. "
                "Expected valid name start character.",
                c));
        }

        bool isAngleBracketClosed = false;
        nameStart = cursor - 1;
        nameEnd = cursor;

        bool done = false;
        while (!done && get(c)) {
            if (isNameChar_(c)) {
                nameEnd = cursor;
            }
            else if (isWhitespace_(c)) {
                done = true;
                isAngleBracketClosed = false;
            }
            else if (c == '>') {
                done = true;
                isAngleBracketClosed = true;
                if (isSelfClosing) {
                    *isSelfClosing = false;
                }
            }
            else if (c == '/') {
                if (!isSelfClosing) {
                    throw XmlSyntaxError(format(
                        "Unexpected '/' while reading end tag name '{}'. "
                        "Expected valid name characters, whitespaces, or '>'.",
                        name()));
                }
                if (get(c)) {
                    if (c == '>') {
                        done = true;
                        isAngleBracketClosed = true;
                        *isSelfClosing = true;
                    }
                    else {
                        throw XmlSyntaxError(format(
                            "Unexpected '{}' after reading '/' after reading start "
                            "tag name '{}'. Expected '>'.",
                            c,
                            name()));
                    }
                }
                else {
                    throw XmlSyntaxError(format(
                        "Unexpected end-of-file after reading '/' after reading start "
                        "tag name '{}'. Expected '>'.",
                        name()));
                }
            }
            else {
                throw XmlSyntaxError(format(
                    "Unexpected '{}' while reading {} tag name '{}'. Expected valid name "
                    "",
                    c,
                    isSelfClosing ? "start" : "end",
                    name(),
                    (isSelfClosing ? "'>', or '/>" : "or '>'")));
            }
        }
        if (!done) {
            throw XmlSyntaxError(format(
                "Unexpected end-of-file while reading {} tag name '{}'. Expected valid "
                "name characters, whitespaces, {}.",
                (isSelfClosing ? "start" : "end"),
                name(),
                (isSelfClosing ? "'>', or '/>" : "or '>'")));
        }

        return isAngleBracketClosed;
    }

    // Read from given first character \p c (not included) to the quotation mark
    // of the attribute value (included)
    void readAttribute_(XmlStreamAttributeData& attribute, char c) {
        // Attribute ::= Name Eq AttValue
        // Eq        ::= S? '=' S?
        // AttValue  ::= '"' ([^<&"] | Reference)* '"'
        //            |  "'" ([^<&'] | Reference)* "'"

        readAttributeName_(attribute, c);
        readAttributeValue_(attribute);
        onAttribute_();
    }

    // Read from given first character \p c (not included) to '=' (included)
    void readAttributeName_(XmlStreamAttributeData& attribute, char c) {

        if (!isNameStartChar_(c)) {
            throw XmlSyntaxError(format(
                "Unexpected '{}' while reading start character of attribute name in "
                "start tag '{}'. Expected valid name start character.",
                c,
                name()));
        }

        const char* attributeNameStart = cursor - 1;
        const char* attributeNameEnd = cursor;
        auto attributeName = [&]() {
            return std::string_view(
                attributeNameStart, attributeNameEnd - attributeNameStart);
        };

        bool isNameRead = false;
        bool isEqRead = false;
        while (!isNameRead && get(c)) {
            if (isNameChar_(c)) {
                attributeNameEnd = cursor;
            }
            else if (c == '=') {
                isNameRead = true;
                isEqRead = true;
            }
            else if (isWhitespace_(c)) {
                isNameRead = true;
            }
            else {
                throw XmlSyntaxError(format(
                    "Unexpected '{}' while reading attribute name '{}' in start tag "
                    "'{}'. Expected valid name characters, whitespaces, or '='.",
                    c,
                    attributeName(),
                    name()));
            }
        }
        if (!isNameRead) {
            throw XmlSyntaxError(format(
                "Unexpected end-of-file while reading attribute name '{}' in start tag "
                "'{}'. Expected valid name characters, whitespaces, or '='.",
                c,
                attributeName(),
                name()));
        }

        while (!isEqRead && get(c)) {
            if (c == '=') {
                isEqRead = true;
            }
            else if (isWhitespace_(c)) {
                // Keep reading
            }
            else {
                throw XmlSyntaxError(format(
                    "Unexpected '{}' after reading attribute name '{}' in start tag "
                    "'{}'. Expected whitespaces or '='.",
                    c,
                    attributeName(),
                    name()));
            }
        }
        if (!isEqRead) {
            throw XmlSyntaxError(format(
                "Unexpected end-of-file after reading attribute name '{}' in start tag "
                "'{}'. Expected whitespaces or '='.",
                c,
                attributeName(),
                name()));
        }

        attribute.name = attributeName();
    }

    // Read from '=' (not included) to closing '\'' or '\"' (included)
    void readAttributeValue_(XmlStreamAttributeData& attribute) {

        attribute.value.clear();
        attribute.rawValueIndex = {};

        char c;
        char quotationMark = 0;
        while (!quotationMark && get(c)) {
            if (c == '\"' || c == '\'') {
                quotationMark = c;
                attribute.rawValueIndex = cursor - attribute.rawText.data();
            }
            else if (isWhitespace_(c)) {
                // Keep reading
            }
            else {
                throw XmlSyntaxError(format(
                    "Unexpected '{}' after reading '=' after reading attribute name '{}' "
                    "in start tag '{}'. Expected '\"' (double quote), or '\'' (single "
                    "quote), or whitespaces.",
                    c,
                    attribute.name,
                    name()));
            }
        }
        if (!quotationMark) {
            throw XmlSyntaxError(format(
                "Unexpected end-of-file after reading '=' after reading attribute name "
                "'{}' in start tag '{}'. Expected '\"' (double quote), or '\'' (single "
                "quote), or whitespaces.",
                c,
                attribute.name,
                name()));
        }

        bool isClosed = false;
        while (!isClosed && get(c)) {
            if (c == quotationMark) {
                isClosed = true;
            }
            else if (c == '&') {
                char replacementChar = readReference_();
                attribute.value += replacementChar;
            }
            else if (c == '<') {
                // This is illegal XML, so we reject it to be compliant. In the
                // future, we may want to have a "lax" mode where we accept it
                // with a warning. It is unclear why the W3C decided that '<'
                // was illegal in attribute values, since there is no
                // ambiguity: no markup is allowed in attribute value (perhaps
                // to make it easier to find all markup with regular
                // expressions?).
                throw XmlSyntaxError(format(
                    "Unexpected '<' while reading value of attribute '{}' in start tag "
                    "'{}'. This character is not allowed in attribute values, please "
                    "replace it with '&lt;'.",
                    attribute.name,
                    name()));
            }
            else {
                attribute.value += c;
            }
        }
        if (!isClosed) {
            throw XmlSyntaxError(format(
                "Unexpected end-of-file while reading value of attribute '{}' in start "
                "tag "
                "'{}'. Expected more characters or the closing quotation mark '{}'.",
                attribute.name,
                name(),
                quotationMark));
        }
    }

    // Action to be performed when an element attribute is encountered. The
    // attribute name and string value are available in attributeName_ and
    // attributeValue_.
    void onAttribute_() {
        /*
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
                    + "'. Expected an attribute name defined in the VGC schema.");
            }
            valueType = spec->valueType();
        }

        Value value = parseValue(attributeValue_, valueType);
        Element::cast(currentNode_)->setAttribute(name, value);
        */
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

        char c;
        if (get(c)) {
            if (!isNameStartChar_(c)) {
                throw XmlSyntaxError(format(
                    "Unexpected '{}' while reading start character of entity reference "
                    "name. Expected valid name start character.",
                    c));
            }
        }
        else {
            throw XmlSyntaxError(
                "Unexpected end-of-file while reading start character of entity "
                "reference name. Expected valid name start character.");
        }

        const char* referenceNameStart = cursor - 1;
        const char* referenceNameEnd = cursor;
        auto referenceName = [&]() {
            return std::string_view(
                referenceNameStart, referenceNameEnd - referenceNameStart);
        };

        bool isSemicolonRead = false;
        while (!isSemicolonRead && get(c)) {
            if (isNameChar_(c)) {
                referenceNameEnd = cursor;
            }
            else if (c == ';') {
                isSemicolonRead = true;
            }
            else {
                throw XmlSyntaxError(format(
                    "Unexpected '{}' while reading entity reference name '{}'. "
                    "Expected valid name characters or ';'.",
                    c,
                    referenceName()));
            }
        }
        if (!isSemicolonRead) {
            throw XmlSyntaxError(format(
                "Unexpected end-of-file while reading entity reference name '{}'. "
                "Expected valid name characters or ';'.",
                referenceName()));
        }

        const std::vector<std::pair<const char*, char>> table = {
            {"amp", '&'},
            {"lt", '<'},
            {"gt", '>'},
            {"apos", '\''},
            {"quot", '\"'},
        };

        for (const auto& pair : table) {
            if (referenceName() == pair.first) {
                return pair.second;
            }
        }

        throw XmlSyntaxError(format("Unknown entity reference '&{};'.", referenceName()));
    }
};

} // namespace detail

XmlStreamReader::XmlStreamReader(ConstructorKey)
    : impl_(std::make_unique<detail::XmlStreamReaderImpl>()) {
}

XmlStreamReader::XmlStreamReader(std::string_view data)
    : XmlStreamReader(ConstructorKey{}) {

    impl_->ownedData = data;        // Copy the data
    impl_->data = impl_->ownedData; // Reference the copy
    impl_->init();
}

XmlStreamReader::XmlStreamReader(std::string&& data)
    : XmlStreamReader(ConstructorKey{}) {

    impl_->ownedData = std::move(data); // Move the data
    impl_->data = impl_->ownedData;     // Reference the moved-to data holder
    impl_->init();
}

XmlStreamReader XmlStreamReader::fromView(std::string_view data) {
    XmlStreamReader res(ConstructorKey{});
    res.impl_->data = data;
    res.impl_->init();
    return res;
}

XmlStreamReader XmlStreamReader::fromFile(std::string_view filePath) {
    return XmlStreamReader(readFile(filePath));
}

XmlStreamReader::XmlStreamReader(const XmlStreamReader& other)
    : XmlStreamReader(ConstructorKey{}) {

    *impl_ = *other.impl_;
}

XmlStreamReader::XmlStreamReader(XmlStreamReader&& other)
    : impl_(std::move(other.impl_)) {
}

XmlStreamReader& XmlStreamReader::operator=(const XmlStreamReader& other) {
    if (this != &other) {
        *impl_ = *other.impl_;
    }
    return *this;
}

XmlStreamReader& XmlStreamReader::operator=(XmlStreamReader&& other) {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

XmlStreamReader::~XmlStreamReader() {
    // Must be in the .cpp otherwise unique_ptr cannot call the
    // destructor of XmlStreamReaderImpl (incomplete type).
}

XmlTokenType XmlStreamReader::tokenType() const {
    return impl_->tokenType;
}

bool XmlStreamReader::readNext() const {
    return impl_->readNext();
}

std::string_view XmlStreamReader::rawText() const {
    return std::string_view(impl_->tokenStart, impl_->tokenEnd - impl_->tokenStart);
}

std::string_view XmlStreamReader::name() const {
    XmlTokenType type = tokenType();
    if (type != XmlTokenType::StartElement && type != XmlTokenType::EndElement) {
        throw LogicError(format(
            "Cannot call XmlStreamReader::name() "
            "if tokenType() (= {}) is not {} or {}.",
            type,
            XmlTokenType::StartElement,
            XmlTokenType::EndElement));
    }
    return impl_->name();
}

std::string_view XmlStreamReader::characterData() const {
    XmlTokenType type = tokenType();
    if (type != XmlTokenType::CharacterData) {
        throw LogicError(format(
            "Cannot call XmlStreamReader::characterData() "
            "if tokenType() (= {}) is not {}.",
            type,
            XmlTokenType::CharacterData));
    }
    return impl_->characterData;
}

ConstSpan<XmlStreamAttributeView> XmlStreamReader::attributes() const {
    XmlTokenType type = tokenType();
    if (type != XmlTokenType::StartElement) {
        throw LogicError(format(
            "Cannot call XmlStreamReader::attributes() "
            "if tokenType() (= {}) is not {}.",
            type,
            XmlTokenType::CharacterData));
    }
    return impl_->attributes();
}

std::optional<XmlStreamAttributeView>
XmlStreamReader::attribute(std::string_view name) const {
    for (XmlStreamAttributeView attr : attributes()) {
        if (attr.name() == name) {
            return attr;
        }
    }
    return std::nullopt;
}

} // namespace vgc::core
