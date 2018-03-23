// Copyright 2017 The VGC Developers
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

#ifndef VGC_DOM_DOCUMENT_H
#define VGC_DOM_DOCUMENT_H

#include <vgc/core/object.h>
#include <vgc/dom/api.h>
#include <vgc/dom/node.h>

namespace vgc {
namespace dom {

VGC_CORE_DECLARE_PTRS(Element);

VGC_CORE_DECLARE_PTRS(Document);

/// \class vgc::dom::Document
/// \brief Represents a VGC document.
///
/// VGC documents are written to disk as XML files, and represented in memory
/// similarly to the DOM structures typically used in web browsers.
///
class VGC_DOM_API Document: public Node
{
public:
    VGC_CORE_OBJECT(Document)

    /// Creates a new document with no root element.
    ///
    Document();

    /// Casts the given \p node to a Document. Returns nullptr if node is
    /// nullptr or if node->nodeType() != NodeType::Document.
    ///
    /// This is functionaly equivalent to dynamic_cast<Document*>, while being
    /// as fast as static_cast<Document*>. Therefore, always prefer using this
    /// method over static_cast<Document*> or dynamic_cast<Document*>.
    ///
    static Document* cast(Node* node) {
        return (node && node->nodeType() == NodeType::Document) ?
               static_cast<Document*>(node) :
               nullptr;
    }

    /// Sets \p element as the root element of this Document.
    ///
    /// If \p element is already the root element of this document, nothing is
    /// done. If this document already a root element, this root element is
    /// first removed from this document. If \p element already has a parent,
    /// \p element is first removed from the children of its parent.
    ///
    /// Returns a pointer to \p element if it has become (or already was) the
    /// root element of this Document. Returns nullptr otherwise.
    ///
    Element* setRootElement(ElementSharedPtr element);

    /// Returns the root element of this Document.
    ///
    Element* rootElement() const;

    /// Returns the XML declaration of this document.
    ///
    /// The default value is:
    ///
    /// \code
    /// <?xml version="1.0" encoding="UTF-8" standalone="no"?>
    /// \endcode
    ///
    /// An XML declaration is optional (see hasXmlDeclaration()).
    ///
    /// If there is an XML declaration, the XML declaration must start at the
    /// first character of the first line of the XML file, and have the three
    /// following attributes:
    /// - version: mandatory, defaults to "1.0"
    /// - encoding: optional, defaults to "UTF-8"
    /// - standalone: optional, defaults to "no"
    ///
    /// If there is no XML declaration, then hasXmlDeclaration() returns false;
    /// xmlDeclaration() returns ""; and xmlVersion(), xmlEncoding(), and
    /// xmlStandalone() return their default value.
    ///
    /// If this Document was opened from an existing XML file that contained an
    /// XML declaration, then this function returns the existing XML
    /// declaration, preserving whitespaces. However, if some of the attributes
    /// of the XML declaration are changed via setXmlVersion(),
    /// setXmlEncoding(), or setXmlStandalone(), then the XML declaration is
    /// re-generated, that is, existing whitespaces will not be preserved.
    ///
    /// \sa hasXmlDeclaration(), setXmlDeclaration(), setNoXmlDeclaration(),
    /// xmlVersion(), xmlEncoding(), and xmlStandalone().
    ///
    std::string xmlDeclaration() const;

    /// Returns whether this document has an XML declaration.
    ///
    /// The default value is true.
    ///
    /// \sa xmlDeclaration(), setXmlDeclaration(), and setNoXmlDeclaration().
    ///
    bool hasXmlDeclaration() const;

    /// Add an XML declaration to this document based on the current values of
    /// xmlVersion(), hasXmlEncoding(), xmlEncoding(), hasXmlStandalone(), and
    /// xmlStandalone().
    ///
    /// After calling this function, hasXmlDeclaration() = true.
    ///
    /// \sa xmlDeclaration(), hasXmlDeclaration(), and setNoXmlDeclaration().
    ///
    void setXmlDeclaration();

    /// Removes the XML declaration of this document. This changes all
    /// attributes of the XML declaration to their default values.
    ///
    /// \sa xmlDeclaration(), hasXmlDeclaration(), and setXmlDeclaration().
    ///
    void setNoXmlDeclaration();

    /// Returns the XML version of this document.
    ///
    /// The default value is "1.0".
    ///
    /// \sa xmlDeclaration() and setXmlVersion().
    ///
    std::string xmlVersion() const;

    /// Sets the 'version' value of the XML declaration. This automatically
    /// changes hasXmlDeclaration() to true.
    ///
    /// \sa xmlDeclaration() and xmlVersion().
    ///
    void setXmlVersion(const std::string& version);

    /// Returns the value of the 'encoding' attribute of the XML declaration.
    ///
    /// The default is "UTF-8".
    ///
    /// If hasXmlEncoding() is false, then this returns "UTF-8".
    ///
    /// \sa xmlDeclaration(), hasXmlEncoding(), setXmlEncoding(), and
    /// setNoXmlEncoding().
    ///
    std::string xmlEncoding() const;

    /// Returns whether the 'encoding' attribute of the XML declaration is
    /// specified.
    ///
    /// The default value is true.
    ///
    /// \sa xmlDeclaration(), xmlEncoding(), setXmlEncoding(), and
    /// setNoXmlEncoding().
    ///
    bool hasXmlEncoding() const;

    /// Sets the 'encoding' value of the XML declaration. This automatically
    /// changes hasXmlDeclaration() and hasXmlEncoding() to true.
    ///
    /// \sa xmlDeclaration(), xmlEncoding(), hasXmlEncoding(), and
    /// setNoXmlEncoding().
    ///
    void setXmlEncoding(const std::string& encoding);

    /// Removes the 'encoding' attribute of the XML declaration. After calling
    /// this function, hasXmlEncoding() = false and xmlEncoding() = "UTF-8".
    ///
    /// \sa xmlDeclaration(), xmlEncoding(), hasXmlEncoding(), and
    /// setXmlEncoding().
    ///
    void setNoXmlEncoding();

    /// Returns the value of the 'standalone' attribute of the XML declaration
    /// as a boolean.
    ///
    /// The default is false.
    ///
    /// If hasXmlStandalone() is false, then this returns false.
    ///
    /// \sa xmlDeclaration(), xmlStandaloneString(), hasXmlStandalone(),
    /// setXmlStandalone(), and setNoXmlStandalone().
    ///
    bool xmlStandalone() const;

    /// Returns "yes" if xmlStandalone() is true; otherwise returns "no".
    ///
    /// \sa xmlDeclaration(), xmlStandalone(), hasXmlStandalone(),
    /// setXmlStandalone(), and setNoXmlStandalone().
    ///
    std::string xmlStandaloneString() const;

    /// Returns whether the 'standalone' attribute of the XML declaration is
    /// specified.
    ///
    /// The default value is true.
    ///
    /// \sa xmlDeclaration(), xmlStandalone(), xmlStandaloneString(),
    /// setXmlStandalone(), and setNoXmlStandalone().
    ///
    bool hasXmlStandalone() const;

    /// Sets the 'standalone' value of the XML declaration. This automatically
    /// changes hasXmlDeclaration() and hasXmlStandalone() to true.
    ///
    /// \sa xmlDeclaration(), xmlStandalone(), xmlStandaloneString(),
    /// hasXmlStandalone(), and setNoXmlStandalone().
    ///
    void setXmlStandalone(bool standalone);

    /// Removes the 'standalone' attribute of the XML declaration. After calling
    /// this function, hasXmlStandalone() = false and xmlStandalone() = false.
    ///
    /// \sa xmlDeclaration(), xmlStandalone(), xmlStandaloneString(),
    /// hasXmlStandalone(), and setXmlStandalone().
    ///
    void setNoXmlStandalone();

    /// Saves the document to the file given by its \p filePath.
    ///
    void save(const std::string& filePath);

private:
    // XML declaration
    bool hasXmlDeclaration_;
    bool hasXmlEncoding_;
    bool hasXmlStandalone_;
    std::string xmlVersion_;
    std::string xmlEncoding_;
    bool xmlStandalone_;
    std::string xmlDeclaration_;
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_DOCUMENT_H
