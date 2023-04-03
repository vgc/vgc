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

#ifndef VGC_DOM_DOCUMENT_H
#define VGC_DOM_DOCUMENT_H

#include <list>
#include <optional>
#include <unordered_map>

#include <vgc/core/id.h>
#include <vgc/core/object.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/node.h>
#include <vgc/dom/operation.h>
#include <vgc/dom/xmlformattingstyle.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Node);
VGC_DECLARE_OBJECT(Document);
VGC_DECLARE_OBJECT(Element);

/// \class vgc::dom::Document
/// \brief Represents a VGC document.
///
/// VGC documents are written to disk as XML files, and represented in memory
/// as instances of vgc::dom::Document, closely matching the XML structure.
///
/// Here is an example VGC document:
///
/// \code
/// <vgc>
///   <!-- A nice red path shaped like the letter L -->
///   <path
///     positions = "[(0, 0), (200, 0), (200, 100)]"
///     width = "5"
///     color = "rgb(255, 0, 0)" />
///
///   <!-- The same path but green and translated by (300, 0) -->
///   <path
///     positions = "[(300, 0), (500, 0), (500, 100)]"
///     width = "5"
///     color = "rgb(0, 255, 0)" />
/// </vgc>
/// \endcode
///
/// Main Concepts
/// -------------
///
/// A Document is a tree of nodes, where each Node can be either the Document
/// node, an Element node, a Comment node, or other types of nodes (see
/// NodeType for details).
///
/// In the example provided, the node structure is the
/// following:
///
/// \verbatim
/// Document
///  |
///  * - Element "vgc"
///       |
///       * - Comment
///       |
///       * - Element "path"
///       |
///       * - Comment
///       |
///       * - Element "path"
/// \endverbatim
///
/// The root of the Node tree is the Document itself. The Document may have
/// several children nodes (including comments), but at most one of these
/// children may be of NodeType::Element. This Element is called the "root
/// element" of the Document (accessible via Document::rootElement()). Despite
/// the terminology, keep in mind that the root element is NOT the root of the
/// Node tree, since the root element has a parent Node: the Document itself.
///
/// However, the root element is the root of the subtree of the Node tree
/// consisting only of Element nodes. We call this subtree the "Element tree":
///
/// \verbatim
/// Element "vgc"
///  |
///  * - Element "path"
///  |
///  * - Element "path"
/// \endverbatim
///
/// XXX Implement the Element tree embedded in the Node tree. It's probably
/// best to not have dedicated data member such as nextElementSibling_, etc.,
/// but simply to implement nextElementSibling() in terms of
/// nextNodeSilbling().
///
/// XXX It may make sense to either rename them nextNode/nextElement, or maybe
/// to only have "nextSibling" but with filters: nextSibling(filter). The same
/// way, we may either have dedicated iterator class
/// ElementSiblingsIterator/NodeSiblingsIterator, or just a single
/// SiblingsIterator with a filter data member. These are not exclusive, we
/// could have both: childNodes/childElements/children(filter). The advantage
/// of separate functions and iterator classes is that the Element version can
/// automatically cast to Element which make it more convenient to use.
///
/// XML Declaration
/// ---------------
///
/// By "XML declaration", we mean the following line that may
/// appear at the beginning of XML files:
///
///     <?xml version="1.0" encoding="UTF-8" standalone="no"?>
///
/// The XML declaration is optional, and is not considered a
/// Node of the tree.
///
/// Comparison with W3C XML DOM
/// ---------------------------
///
/// The classes in the vgc::dom library follow the same spirit as the W3C XML
/// DOM specifications, but there are a few differences.
///
/// For example, in vgc::dom, element attributes are not considered to be
/// nodes of the tree. This allows to keep a clear separation between
/// the node hierarchy and its data, and we believe results in a cleaner
/// API. For example, in the W3C XML DOM, the API for Node contains
/// functions which make no sense when the Node is in fact an attribute,
/// such as 'childNodes' (an attribute has no child nodes). Also, our
/// nodes are all separately dynamically allocated C++ objects (this allows
/// observers to track Node changes), and we wouldn't want to dynamically
/// allocate all attributes, especially default attributes.
///
class VGC_DOM_API Document : public Node {
private:
    VGC_OBJECT(Document, Node)

protected:
    /// Creates a new document with no root element. This constructor is an
    /// implementation detail only available to derived classes. In order to
    /// create a Document, please use the following:
    ///
    /// ```cpp
    /// DocumentPtr document = Document::create();
    /// ```
    ///
    Document();

public:
    /// Creates a new document with no root element.
    ///
    static DocumentPtr create();

    /// Opens the file given by its \p filePath.
    ///
    /// Exceptions:
    /// - Raises FileError if the document cannot be opened due to system errors.
    /// - Raises ParseError if the document cannot be opened due to syntax errors.
    ///
    static DocumentPtr open(const std::string& filePath);

    /// Casts the given \p node to a Document. Returns nullptr if node is
    /// nullptr or if node->nodeType() != NodeType::Document.
    ///
    /// This is functionaly equivalent to dynamic_cast<Document*>, while being
    /// as fast as static_cast<Document*>. Therefore, always prefer using this
    /// method over static_cast<Document*> or dynamic_cast<Document*>.
    ///
    static Document* cast(Node* node) {
        return (node && node->nodeType() == NodeType::Document)
                   ? static_cast<Document*>(node)
                   : nullptr;
    }

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
    /// \sa xmlDeclaration(), hasXmlStandalone(), setXmlStandalone(), and
    /// setNoXmlStandalone().
    ///
    bool xmlStandalone() const;

    /// Returns whether the 'standalone' attribute of the XML declaration is
    /// specified.
    ///
    /// The default value is true.
    ///
    /// \sa xmlDeclaration(), xmlStandalone(), setXmlStandalone(), and
    /// setNoXmlStandalone().
    ///
    bool hasXmlStandalone() const;

    /// Sets the 'standalone' value of the XML declaration. This automatically
    /// changes hasXmlDeclaration() and hasXmlStandalone() to true.
    ///
    /// \sa xmlDeclaration(), xmlStandalone(), hasXmlStandalone(), and
    /// setNoXmlStandalone().
    ///
    void setXmlStandalone(bool standalone);

    /// Removes the 'standalone' attribute of the XML declaration. After calling
    /// this function, hasXmlStandalone() = false and xmlStandalone() = false.
    ///
    /// \sa xmlDeclaration(), xmlStandalone(), hasXmlStandalone(), and
    /// setXmlStandalone().
    ///
    void setNoXmlStandalone();

    /// Saves the document to the file given by its \p filePath.
    ///
    /// Raises a FileError exception if the document cannot be saved.
    ///
    void save(
        const std::string& filePath,
        const XmlFormattingStyle& style = XmlFormattingStyle()) const;

    Element* elementById(core::StringId id) const;

    Element* elementByInternalId(core::Id id) const;

    core::History* enableHistory(core::StringId entrypointName);
    void disableHistory();

    core::History* history() const {
        return history_.get();
    }

    core::Id versionId() const {
        return versionId_;
    }

    bool emitPendingDiff();

    VGC_SIGNAL(changed, (const Diff&, diff))

protected:
    VGC_SLOT(onHistoryHeadChanged, onHistoryHeadChanged_)
    VGC_SLOT(onHistoryAboutToUndo, onHistoryAboutToUndo_)
    VGC_SLOT(onHistoryUndone, onHistoryUndone_)
    VGC_SLOT(onHistoryAboutToRedo, onHistoryAboutToRedo_)
    VGC_SLOT(onHistoryRedone, onHistoryRedone_)

private:
    // XML declaration
    bool hasXmlDeclaration_;
    bool hasXmlEncoding_;
    bool hasXmlStandalone_;
    std::string xmlVersion_;
    std::string xmlEncoding_;
    bool xmlStandalone_;
    void generateXmlDeclaration_();
    std::string xmlDeclaration_;

    // Utilities
    friend Element; // XXX <- create accessors ?
    std::unordered_map<core::StringId, Element*> elementByIdMap_;
    std::unordered_map<core::Id, Element*> elementByInternalIdMap_;

    // Operations
    friend class CreateElementOperation;
    friend class RemoveNodeOperation;
    friend class MoveNodeOperation;
    friend class SetAttributeOperation;
    friend class RemoveAuthoredAttributeOperation;

    //friend Node;
    //friend class Element;

    core::HistoryPtr history_;
    core::Id versionId_;
    Diff pendingDiff_;
    core::Array<NodePtr> pendingDiffKeepAllocPointers_;
    std::unordered_map<Node*, NodeRelatives> previousRelativesMap_;

    void onHistoryHeadChanged_();
    void onHistoryAboutToUndo_();
    void onHistoryUndone_();
    void onHistoryAboutToRedo_();
    void onHistoryRedone_();

    // TODO: differentiate history head changes from user changes
    // For instance, we don't want a plugin to perform an automatic
    // DOM modification in response to DOM changes caused by an undo.
    void onCreateNode_(Node* node);
    void onRemoveNode_(Node* node);
    void onMoveNode_(Node* node, const NodeRelatives& savedRelatives);
    void onChangeAttribute_(Element* element, core::StringId name);
};

} // namespace vgc::dom

#endif // VGC_DOM_DOCUMENT_H
