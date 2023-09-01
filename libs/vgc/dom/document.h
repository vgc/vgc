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
#include <vgc/dom/path.h>
#include <vgc/dom/value.h>
#include <vgc/dom/xmlformattingstyle.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Node);
VGC_DECLARE_OBJECT(Document);
VGC_DECLARE_OBJECT(Element);

class VGC_DOM_API Diff {
public:
    const core::Array<Node*>& createdNodes() const {
        return createdNodes_;
    }

    const core::Array<Node*>& removedNodes() const {
        return removedNodes_;
    }

    const std::set<Node*>& reparentedNodes() const {
        return reparentedNodes_;
    }

    const std::set<Node*>& childrenReorderedNodes() const {
        return childrenReorderedNodes_;
    }

    const std::unordered_map<Element*, core::Array<core::StringId>>&
    modifiedElements() const {
        return modifiedElements_;
    }

    void insertModifiedElementAttributeRecord(Element* element, core::StringId attrName) {
        core::Array<core::StringId>& attrNames = modifiedElements_[element];
        if (!attrNames.contains(attrName)) {
            attrNames.append(attrName);
        }
    }

    void removeModifiedElementAttributeRecords(Element* element) {
        modifiedElements_.erase(element);
    }

    bool isUndoOrRedo() const {
        return isUndoOrRedo_;
    }

private:
    friend Document;

    // TODO: Use NodePtr to prevent dangling pointers when a reader
    //       edits the DOM while processing the lists.
    core::Array<Node*> createdNodes_;
    core::Array<Node*> removedNodes_;
    std::set<Node*> reparentedNodes_;
    std::set<Node*> childrenReorderedNodes_;

    std::unordered_map<Element*, core::Array<core::StringId>> modifiedElements_;

    bool isUndoOrRedo_ = false;

    Diff() = default;

    // it does not change isUndoOrRedo_ value.
    void reset() {
        createdNodes_.clear();
        removedNodes_.clear();
        reparentedNodes_.clear();
        childrenReorderedNodes_.clear();
        modifiedElements_.clear();
    }

    bool isEmpty() const {
        return createdNodes_.empty()              //
               && removedNodes_.empty()           //
               && reparentedNodes_.empty()        //
               && childrenReorderedNodes_.empty() //
               && modifiedElements_.empty();
    }
};

/// \class vgc::dom::Document
/// \brief Represents a VGC document.
///
/// VGC documents are written to disk as XML files, and represented in memory
/// as instances of vgc::dom::Document, closely matching the XML structure.
///
/// Here is an example VGC document:
///
/// ```xml
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
/// ```
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
/// ```
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
/// ```
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
/// ```
/// Element "vgc"
///  |
///  * - Element "path"
///  |
///  * - Element "path"
/// ```
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
    Document(CreateKey);

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
    static DocumentPtr open(std::string_view filePath);

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
    void setXmlVersion(std::string_view version);

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
    void setXmlEncoding(std::string_view encoding);

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

    /// Copies the given `nodes` from this document in the form
    /// of a new document with similar hierarchy.
    /// The resulting document can be used as argument to paste().
    ///
    /// Dependencies are not auto-included.
    ///
    /// Document is not complete and writeable.
    ///
    static DocumentPtr copy(core::ConstSpan<Node*> nodes);

    /// Pastes the node tree of the given `document` in this
    /// document under the given `parent`.
    ///
    /// Returns a list with the top-level nodes (i.e., not including their
    /// children) that have been pasted.
    ///
    static core::Array<dom::Node*> paste(DocumentPtr document, Node* parent);

    /// Returns the `Element` in this document, if any, that has
    /// the given element `id`.
    ///
    Element* elementFromId(core::StringId id) const;

    /// Returns the `Element` in this document, if any, that has
    /// the given internal `id`.
    ///
    Element* elementFromInternalId(core::Id id) const;

    /// Returns the `Element` in this document, if any, that the given `path`
    /// resolves to in the context of the given `workingNode`.
    ///
    static Element* elementFromPath(
        const Path& path,
        const Node* workingNode,
        core::StringId tagNameFilter = {});

    /// Returns the `Value` in this document, if any, that the given `path`
    /// resolves to in the context of the given `workingNode`.
    ///
    static Value valueFromPath(
        const Path& path,
        const Node* workingNode,
        core::StringId tagNameFilter = {});

    /// Enables undo/redo history for this document.
    ///
    /// \sa `disableHistory()`, `history()`, `versionID()`.
    ///
    core::History* enableHistory(core::StringId entrypointName);

    /// Disables undo/redo history for this document.
    ///
    /// \sa `enableHistory()`, `history()`, `versionID()`.
    ///
    void disableHistory();

    /// Returns the undo/redo history of this document.
    ///
    /// \sa `enableHistory()`, `disableHistory()`, `versionID()`.
    ///
    core::History* history() const {
        return history_.get();
    }

    /// Returns the current ID of the current undo/redo version.
    ///
    /// This ID changes each time an operation is performed on the
    /// document.
    ///
    /// \sa `history(), `enableHistory()`, `disableHistory()`.
    ///
    core::Id versionId() const {
        return versionId_;
    }

    /// Returns whether some changes of the DOM have not yet been notified to
    /// listeners of the `changed()` signal.
    ///
    /// \sa `changed()`, `emitPendingDiff()`.
    ///
    bool hasPendingDiff();

    /// Notifies listeners of all the changes that happened since the last call
    /// of `emitPendingDiff()`.
    ///
    /// If `hasPendingDiff()` is false, then this function does nothing and
    /// returns false.
    ///
    /// If `hasPendingDiff()` is true, then the `changed()` signal is emitted
    /// (even if the `Diff` is empty after compression), and this function
    /// returns true.
    ///
    /// \sa `changed()`, `hasPendingDiff()`.
    ///
    bool emitPendingDiff();

    /// This signals is emitted to notify of changes in the DOM.
    ///
    /// Note that this signal is not emitted instantly when changes happen.
    /// Instead, the DOM keeps track of all pending changes in a `Diff`, and
    /// notify listeners to all these changes at once when `emitPendingDiff()`
    /// is called.
    ///
    /// \sa `hasPendingDiff()`, `emitPendingDiff()`.
    ///
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

    void onElementIdChanged_(Element* element, core::StringId oldId);
    void onElementNameChanged_(Element* element);
    void onElementAboutToBeDestroyed_(Element* element);

    void numberNodesDepthFirstRec_(Node* node, Int& i);

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

    void compressPendingDiff_();

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

    static Element* resolveElementPartOfPath_(
        const Path& path,
        Path::ConstSegmentIterator& segIt,
        Path::ConstSegmentIterator segEnd,
        const Node* workingNode,
        core::StringId tagNameFilter = {});

    static Value resolveAttributePartOfPath_(
        const Path& path,
        Path::ConstSegmentIterator& segIt,
        Path::ConstSegmentIterator segEnd,
        const Element* element);

    void preparePathsUpdateRec_(const Node* node);
    void updatePathsRec_(const Node* node, const PathUpdateData& pud);
};

} // namespace vgc::dom

#endif // VGC_DOM_DOCUMENT_H
