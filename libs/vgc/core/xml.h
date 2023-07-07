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

#ifndef VGC_CORE_XML_H
#define VGC_CORE_XML_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/core/span.h>

// Note: there are three general approaches for XML parsers:
// 1. Generating a DOM tree
// 2. Callback-based (SAX)
// 3. Stream-based (StAX)
//
// Both SAX and StAX are lightweight/fast, but SAX has the advantage to allow
// client code to more easily stop processing or skip whole elements if not
// relevant for them.
//
// Also, it is easy to implement DOM and SAX based on StAX.
//
// For these reasons, we made the choice to use StAX, with a design similar to
// Qt for easier client code compatibility between the two.

namespace vgc::core {

namespace detail {

class XmlStreamReaderImpl;

struct VGC_CORE_API XmlStreamAttributeData {

    //  <edge   positions = "[(12, 42), (20, 30)]" />
    //          \_______/    ^
    //            name    rawValueIndex
    //       \___________________________________/
    //                     rawText
    //
    std::string_view rawText;
    std::string_view name;    // subset of rawText
    size_t rawValueIndex = 0; // index into rawText

    // Resolved value
    std::string value;
};

} // namespace detail

/// \enum vgc::core::XmlEventType
/// \brief The type of an XML token.
///
enum class XmlEventType : Int8 {
    NoEvent,       // nothing has been read yet
    StartDocument, // can be an xml-declaration or a missing xml-declaration
    EndDocument,
    StartElement, // can be a start tag (<b>) or an empty element tag (<img/>)
    EndElement,   // can be an end tag (</b>) or just after an empty element tag
    Characters,   // can be character data, CDATA section, resolved entities,
    Comment,
    ProcessingInstruction,

    // TODO:
    // Space,         // non-content whitespace (e.g., outside of the document element)
    // DoctypeDeclaration,

    // Currently, entity references are automatically resolved and merged with
    // adjacent character data. When implemented, CDATA sections and character
    // references will also at first be automatically merged with adjacent
    // character data.
    //
    // In the future, we may may to report them as separate events so that it
    // is possible to rewrite the file exactly as it was initially.
    //
    // CharacterReference, // Only reported if not automatically resolved
    // EntityReference,    // Only reported if not automatically resolved
    // CDataSection,       // Only if not automatically merged as Characters
    //
    // Also see: javax.xml.stream.isCoalescing
};

// Terminology note:
// - StartElement means either start not

VGC_CORE_API
VGC_DECLARE_ENUM(XmlEventType)

/// \class vgc::core::XmlStreamAttributeView
/// \brief A non-owning view of an attribute read by `XmlStreamReader`
///
class VGC_CORE_API XmlStreamAttributeView {
public:
    /// Returns the name of this attribute.
    ///
    std::string_view name() const {
        return d_->name;
    }

    /// Returns the value of this attribute.
    ///
    /// The returned value have all character references and entity references
    /// resolved (e.g., `&lt;` is replaced by `<`), and does not include the
    /// quotation marks surrounding the value.
    ///
    /// \sa `rawValue()`, `quotationMark()`.
    ///
    std::string_view value() const {
        return d_->value;
    }

    /// Returns the raw text of this attribute, that is, all characters
    /// including leading whitespace, whitespace between name and value, equal
    /// sign and quotation marks, as well as original character references and
    /// entity references (e.g., `&lt;` is not replaced by `<`).
    ///
    std::string_view rawText() const {
        return d_->rawText;
    }

    /// Returns the leading whitespace of this attribute, that is, all the
    /// whitespace characters before the name of the attribute, and after the
    /// end of the last attribute (or the end of the element name if this is
    /// the first attribute).
    ///
    /// This is useful for applications that need to later rewrite the parsed
    /// XML document while preserving the original formatting.
    ///
    std::string_view leadingWhitespace() const {
        size_t n = d_->name.data() - d_->rawText.data();
        return d_->rawText.substr(0, n);
    }

    /// Returns the raw text separating the name of the attribute and the value
    /// of the attribute.
    ///
    /// This consists of zero or more whitespace characters, followed by the
    /// character `=`, followed by zero or more whitespace characters.
    ///
    std::string_view separator() const {
        Int separatorIndex = d_->name.data() - d_->rawText.data() + d_->name.size();
        Int n = d_->rawValueIndex - 1 - separatorIndex;
        return d_->rawText.substr(separatorIndex, n);
    }

    /// Returns the raw text of the value of this attribute, including original
    /// character references and entity references (e.g., `&lt;` is not
    /// replaced to `<`), but excluding quotation signs.
    ///
    std::string_view rawValue() const {
        Int n = d_->rawText.size() - 1 - d_->rawValueIndex;
        return d_->rawText.substr(d_->rawValueIndex, n);
    }

    /// Returns the quotation mark used to surround the attribute value.
    ///
    /// This is equal to either `'` or `"`.
    ///
    char quotationMark() const {
        return d_->rawText[d_->rawValueIndex - 1];
    }

private:
    detail::XmlStreamAttributeData* d_ = nullptr;

    friend detail::XmlStreamReaderImpl;
    XmlStreamAttributeView() {
    }
};

/// \class vgc::core::XmlStreamReader
/// \brief Reads an XML document using a stream-based API
///
/// `XmlStreamReader` provides a stream-based API (also known as
/// ["StAX"](https://en.wikipedia.org/wiki/StAX)) to read an XML document.
///
/// The basic usage consists in creating an instance of `XmlStreamReader`, then
/// calling its `readNext()` method until it returns `false`, indicating that
/// the end of the document was reached:
///
/// ```cpp
/// using vgc::core::XmlStreamReader;
/// using vgc::core::XmlEventType;
/// using vgc::core::print;
///
/// XmlStreamReader xml = "<hello><world/></hello>";
/// std::string indent = "";
/// while (xml.readNext()) {
///     switch (xml.eventType()) {
///     case XmlEventType::StartElement:
///         print("{}{}\n", indent, xml.name());
///         indent.push_back(' ');
///         break;
///     case XmlEventType::EndElement:
///         indent.pop_back();
///         break;
///     default:
///         break;
///     }
/// }
/// ```
///
/// Output:
///
/// ```
/// hello
///  world
/// ```
///
/// If the input document is not a well-formed XML document, then `XmlParseError`
/// is raised by `readNext()` when encountering the ill-formed syntax.
///
/// If you call a method which is not compatible with the current `eventType()`
/// (for example, calling `comment()` for an event that is not `Comment`), then
/// `LogicError` is raised.
///
/// # Initial State
///
/// When creating an `XmlStreamReader`, its `eventType()` is initially equal to
/// `NoEvent`, and most methods would raise `LogicError` if called.
///
/// # Prolog
///
/// The prolog of an XML document is defined as everything that appears before
/// the root element.
///
/// It may contain:
/// - an XML declaration (always first if present),
/// - a document type definition (DTD),
/// - comments, processing instructions, or whitespaces between the
///   XML declaration or the DTD, or before the root element.
///
/// # XML Declaration
///
/// The XML Declaration is an optional header for XML files that looks like this:
///
/// ```
/// <?xml version="1.0" encoding="UTF-8" standalone="no"?>
/// ```
///
/// Where `version` is mandatory, but `encoding` and `standalone` are optional.
///
/// After calling `readNext()` for the first time, the `eventType()` will
/// become equal to `StartDocument` (unless `XmlParseError` is raised), and
/// from this moment on it is possible to query the content of the XML
/// declaration with `hasXmlDeclaration()`, `xmlDeclaration()`, `version()`,
/// `encoding()`, `isEncodingSet()`, `standalone()`, and `isStandaloneSet()`.
///
/// # Document Type Declaration
///
/// For now, document type declarations are not supported by `XmlStreamReader`.
///
/// # Start/End Elements
///
/// XML elements are reported as `StartElement` and `EndElement` events. When
/// `eventType()` is equal to `StartElement` and `EndElement`, then `name()`
/// provides the name of the element.
///
/// In case of "empty-element tags" (also called "self-closing", e.g., `<img
/// />`), then there are still both a `StartElement` event and `EndElement`
/// event reported. The `rawText()` of the `StartElement` event will be equal
/// to the whole empty-element tag, while the `rawText()` of the `EndElement`
/// event will be empty.
///
/// If the event is `StartElement`, then it is possible to query its attributes
/// with `attributes()`, `numAttributes()`, `attribute(index)`,
/// `attribute(name)`, attributeName(index)`, `attributeValue(index)`, and
/// `attributeValue(name)`.
///
/// # Characters
///
/// Content between the elements, comments, or processing instructions is referred to
/// as "characters". It consists either of raw characters, or entity references that
/// have been resolved to their corresponding characters, or CDATA sections.
///
/// Note that for now, CDATA sections are not supported.
///
/// # Comments
///
/// Comments are reported as `Comment` events, whose content is then available
/// via `comment()`.
///
/// # Processing Instructions
///
/// Processing instructions refer to application-specific data
/// that looks like:
///
/// ```
/// <?php echo "Hello World!";?>
/// ```
///
/// These are reported as `ProcessingInstruction` events, and its "target"
/// (e.g., `php`) and data (e.g., ` echo "Hello World!";`) can be queried via
/// `processingInstructionTarget()` and `processingInstructionData()`.
///
class VGC_CORE_API XmlStreamReader {
private:
    // No-init private constructor
    struct ConstructorKey {};
    XmlStreamReader(ConstructorKey);

public:
    /// Creates an `XmlStreamReader` reading the given `data`.
    ///
    /// A copy of the data is kept in this `XmlStreamReader`. If you wish to
    /// avoid the copy, you can use one of the following options, but care must
    /// be taken to avoid undefined behavior:
    ///
    /// ```cpp
    /// std::string data;
    /// XmlStreamReader reader(std::move(data));
    /// // Warning: unfefined behavior if `data` is used after this point!
    /// ```
    ///
    /// Or:
    ///
    /// ```cpp
    /// std::string data;
    /// XmlStreamReader reader = XmlStreamReader::fromView(data);
    /// // Warning: unfefined behavior if `reader` is used after `data` is destructed!
    /// ```
    ///
    XmlStreamReader(std::string_view data);

    /// Creates an `XmlStreamReader` reading the given `data`.
    ///
    XmlStreamReader(std::string&& data);

    /// Creates an `XmlStreamReader` reading the given `data`.
    ///
    /// It is the responsibility of the caller to keep `data` alive as long as
    /// the `XmlStreamReader` is used.
    ///
    static XmlStreamReader fromView(std::string_view data);

    /// Creates an `XmlStreamReader` reading the content of the file given by its
    /// `filePath`.
    ///
    /// Exceptions:
    /// - Raises `FileError` if the file cannot be read for any reason.
    ///
    static XmlStreamReader fromFile(std::string_view filePath);

    /// Copy-constructs an `XmlStreamReader`.
    ///
    XmlStreamReader(const XmlStreamReader& other);

    /// Move-constructs an `XmlStreamReader`.
    ///
    XmlStreamReader(XmlStreamReader&& other) noexcept;

    /// Copy-assigns an `XmlStreamReader`.
    ///
    XmlStreamReader& operator=(const XmlStreamReader& other);

    /// Move-assigns an `XmlStreamReader`.
    ///
    XmlStreamReader& operator=(XmlStreamReader&& other) noexcept;

    /// Destructs this `XmlStreamReader`.
    ///
    ~XmlStreamReader();

    /// Reads until the next event.
    ///
    /// Returns false if the event is `EndDocument`, otherwise return true.
    ///
    /// Exceptions:
    /// - `XmlSyntaxError` if encountered not well-formed XML data.
    ///
    bool readNext();

    /// Reads the next `StartElement` or `EndDocument` event.
    ///
    /// Returns false if the event is `EndDocument` , otherwise return true.
    ///
    /// Exceptions:
    /// - `XmlSyntaxError` if encountered not well-formed XML data.
    ///
    bool readNextStartElement();

    /// Skips the current element, that is, keeps reading until the
    /// `EndElement` event that closes the element is reached.
    ///
    /// You would typically call this function on response to a `StartElement`
    /// event whose `name` is an unsupported element, in order to skip it as
    /// well as all its children.
    ///
    /// After calling this function, `eventType()` is equal to `EndElement`,
    /// and the new current element becomes the parent of the previous current
    /// element. This means that if you call `skipElement()` successively, you
    /// go up the stack of unclosed `StartElement`.
    ///
    /// Exceptions:
    /// - `XmlSyntaxError` if encountered not well-formed XML data.
    /// - `LogicError` if no `StartElement` event has been reported yet
    /// - `LogicError` if all reported `StartElement` have already had their
    ///   corresponding `EndElement` reported.
    ///
    void skipElement();

    /// Returns the type of the token that has just been read.
    ///
    XmlEventType eventType() const;

    /// Returns the raw text of the token, that is, all characters including
    /// any markup and whitespaces.
    ///
    /// Calling this function for all tokens allows you to exactly reconstruct
    /// the input data.
    ///
    std::string_view rawText() const;

    /// Returns whether there is an XML declaration in the document.
    ///
    /// This can only be called if `eventType()` is not `NoEvent`, that is, if
    /// the `StartDocument` event was reached.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is `NoEvent`.
    ///
    /// \sa `xmlDeclaration()`.
    ///
    bool hasXmlDeclaration() const;

    /// Returns the XML declaration of the document.
    ///
    /// Returns an empty string if `hasXmlDeclaration()` if false.
    ///
    /// This can only be called if `eventType()` is not `NoEvent`, that is, if
    /// the `StartDocument` event was reached.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is `NoEvent`.
    ///
    /// \sa `hasXmlDeclaration()`, `version()`, `encoding()`, `isStandalone()`,
    ///     `isEncodingSet()`, `isStandaloneSet()`.
    ///
    std::string_view xmlDeclaration() const;

    /// Returns the XML version of the document, as declared in the XML
    /// declaration, or "1.0" if there is no XML declaration.
    ///
    /// This can only be called if `eventType()` is not `NoEvent`, that is, if
    /// the `StartDocument` event was reached.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is `NoEvent`.
    ///
    /// \sa `xmlDeclaration()`.
    ///
    std::string_view version() const;

    /// Returns the character encoding of the document, as declared in the XML
    /// declaration, or "UTF-8" if there is no XML declaration, or if the
    /// encoding was omitted in the XML declaration.
    ///
    /// This can only be called if `eventType()` is not `NoEvent`, that is, if
    /// the `StartDocument` event was reached.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is `NoEvent`.
    ///
    /// \sa `xmlDeclaration()`, `isEncodingSet()`.
    ///
    std::string_view encoding() const;

    /// Returns whether there is an XML declaration with an explicit encoding
    /// specified.
    ///
    /// This can only be called if `eventType()` is not `NoEvent`, that is, if
    /// the `StartDocument` event was reached.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is `NoEvent`.
    ///
    /// \sa `xmlDeclaration()`, `encoding()`.
    ///
    bool isEncodingSet() const;

    /// Returns whether the document is "standalone", as declared in the XML
    /// declaration, or `false` if there is no XML declaration, or if the
    /// standalone attribute was omitted in the XML declaration.
    ///
    /// This can only be called if `eventType()` is not `NoEvent`, that is, if
    /// the `StartDocument` event was reached.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is `NoEvent`.
    ///
    /// \sa `xmlDeclaration()`, `isStandaloneSet()`.
    ///
    bool isStandalone() const;

    /// Returns whether there is an XML declaration with an explicit standalone
    /// attribute specified.
    ///
    /// This can only be called if `eventType()` is not `NoEvent`, that is, if
    /// the `StartDocument` event was reached.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is `NoEvent`.
    ///
    /// \sa `xmlDeclaration()`, `isStandalone()`.
    ///
    bool isStandaloneSet() const;

    /// Returns the name of the current `StartElement` or `EndElement`.
    ///
    /// Exceptions:    ///
    ///- `LogicError` is raised if `eventType()` is not `StartElement` or
    ///  `EndElement`.
    ///
    std::string_view name() const;

    /// Returns the content of `Characters`
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `Characters`.
    ///
    std::string_view characters() const;

    /// Returns all the attributes of the current `StartElement` token.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `StartElement`.
    ///
    ConstSpan<XmlStreamAttributeView> attributes() const;

    /// Returns the number of attributes of a StartElement.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `StartElement`.
    ///
    Int numAttributes() const {
        return attributes().length();
    }

    /// Returns the attribute of the current `StartElement` token
    /// given by its attribute `index`.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `StartElement`.
    /// - `IndexError` is raised if `index` is not in `[0, numAttributes() - 1]`.
    ///
    XmlStreamAttributeView attribute(Int index) const {
        return attributes()[index];
    }

    /// Returns the attribute of the current `StartElement` token given by its
    /// `name`. Returns `std::nullopt` if there is no attribute with the given
    /// name.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `StartElement`.
    ///
    std::optional<XmlStreamAttributeView> attribute(std::string_view name) const;

    /// Returns the name of the attribute at the given `index`.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `StartElement`.
    /// - `IndexError` is raised if `index` is not in `[0, numAttributes() - 1]`.
    ///
    std::string_view attributeName(Int index) const {
        return attribute(index).name();
    }

    /// Returns the value of the attribute at the given `index`.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `StartElement`.
    /// - `IndexError` is raised if `index` is not in `[0, numAttributes() - 1]`.
    ///
    std::string_view attributeValue(Int index) const {
        return attribute(index).value();
    }

    /// Returns the value of the attribute given by its `name`. Returns
    /// `std::nullopt` if there is no attribute with the given name.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `StartElement`.
    ///
    std::optional<std::string_view> attributeValue(std::string_view name) const {
        if (std::optional<XmlStreamAttributeView> attr = attribute(name)) {
            return attr->value();
        }
        else {
            return std::nullopt;
        }
    }

    /// Returns the content of the comment.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `Comment`.
    ///
    std::string_view comment() const;

    /// Returns the target of the processing instruction.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `ProcessingInstruction`.
    ///
    /// \sa `processingInstructionData()`.
    ///
    std::string_view processingInstructionTarget() const;

    /// Returns the data of the processing instruction.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `eventType()` is not `ProcessingInstruction`.
    ///
    /// \sa `processingInstructionTarget()`.
    ///
    std::string_view processingInstructionData() const;

private:
    std::unique_ptr<detail::XmlStreamReaderImpl> impl_;

    const detail::XmlStreamReaderImpl* impl() const {
        return impl_.get();
    }

    detail::XmlStreamReaderImpl* impl() {
        return impl_.get();
    }
};

} // namespace vgc::core

#endif // VGC_CORE_XML_H
