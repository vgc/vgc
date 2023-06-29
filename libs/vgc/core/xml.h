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
#include <string>
#include <string_view>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>

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

} // namespace detail

/// \enum vgc::core::XmlTokenType
/// \brief The type of an XML token.
///
enum class XmlTokenType : Int8 {
    None,
    Invalid,
    StartDocument,
    EndDocument,
    StartTag,
    EndTag,
    CharacterData,
    Comment,

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

    /// If the previously read token was `CharacterData`, returns the
    /// content of the character data.
    ///
    std::string_view characterData() const;

private:
    std::unique_ptr<detail::XmlStreamReaderImpl> impl_;
};

} // namespace vgc::core

#endif // VGC_CORE_XML_H
