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

/// \enum vgc::core::XmlTokenType
/// \brief The type of an XML token.
///
enum class XmlTokenType : Int8 {
    None,
    Invalid,
    StartDocument,
    EndDocument,
    StartElement,
    EndElement,
    CharacterData,
    Comment,
    ProcessingInstruction,

    // TODO:
    // DoctypeDeclaration,

    // Currently, character reference and entity references are automatically
    // resolved and merge with adjacent character data. Also, CData sections
    // are automatically merged with adjacent character data.
    //
    // In the future, we may may to report them as separate tokens so that it
    // is possible to rewrite the file exactly as it was initially.
    //
    // CharacterReference, // Only reported if not automatically resolved
    // EntityReference,    // Only reported if not automatically resolved
    // CDataSection,       // Only if not automatically merged as CharacterData
};

VGC_CORE_API
VGC_DECLARE_ENUM(XmlTokenType)

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
/// \brief Reads an XML file using a stream-based API
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
    XmlStreamReader(XmlStreamReader&& other);

    /// Copy-assigns an `XmlStreamReader`.
    ///
    XmlStreamReader& operator=(const XmlStreamReader& other);

    /// Move-assigns an `XmlStreamReader`.
    ///
    XmlStreamReader& operator=(XmlStreamReader&& other);

    /// Destructs this `XmlStreamReader`.
    ///
    ~XmlStreamReader();

    /// Returns the type of the token that has just been read.
    ///
    XmlTokenType tokenType() const;

    /// Reads the next token.
    ///
    /// Returns false if the token is `EndDocument` or `Invalid`, otherwise
    /// return true.
    ///
    bool readNext() const;

    /// Returns the raw text of the token, that is, all characters including
    /// any markup and whitespaces.
    ///
    /// Calling this function for all tokens allows you to exactly reconstruct
    /// the input data.
    ///
    std::string_view rawText() const;

    /// Returns the name of the current `StartElement` or `EndElement`.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `tokenType()` is not `StartElement` or `EndElement`.
    ///
    std::string_view name() const;

    /// Returns the content of the current character data.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `tokenType()` is not `CharacterData`.
    ///
    std::string_view characterData() const;

    /// Returns all the attributes of the current `StartElement` token.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `tokenType()` is not `StartElement`.
    ///
    ConstSpan<XmlStreamAttributeView> attributes() const;

    /// Returns the number of attributes of a StartElement.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `tokenType()` is not `StartElement`.
    ///
    Int numAttributes() const {
        return attributes().length();
    }

    /// Returns the attribute of the current `StartElement` token
    /// given by its attribute `index`.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `tokenType()` is not `StartElement`.
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
    /// - `LogicError` is raised if `tokenType()` is not `StartElement`.
    ///
    std::optional<XmlStreamAttributeView> attribute(std::string_view name) const;

    /// Returns the name of the attribute at the given `index`.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `tokenType()` is not `StartElement`.
    /// - `IndexError` is raised if `index` is not in `[0, numAttributes() - 1]`.
    ///
    std::string_view attributeName(Int index) const {
        return attribute(index).name();
    }

    /// Returns the value of the attribute at the given `index`.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `tokenType()` is not `StartElement`.
    /// - `IndexError` is raised if `index` is not in `[0, numAttributes() - 1]`.
    ///
    std::string_view attributeValue(Int index) const {
        return attribute(index).value();
    }

    /// Returns the value of the attribute given by its `name`. Returns
    /// `std::nullopt` if there is no attribute with the given name.
    ///
    /// Exceptions:
    /// - `LogicError` is raised if `tokenType()` is not `StartElement`.
    ///
    std::optional<std::string_view> attributeValue(std::string_view name) const {
        if (std::optional<XmlStreamAttributeView> attr = attribute(name)) {
            return attr->value();
        }
        else {
            return std::nullopt;
        }
    }

private:
    std::unique_ptr<detail::XmlStreamReaderImpl> impl_;
};

} // namespace vgc::core

#endif // VGC_CORE_XML_H
