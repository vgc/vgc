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

#ifndef VGC_DOM_EXCEPTIONS_H
#define VGC_DOM_EXCEPTIONS_H

#include <vgc/core/exceptions.h>
#include <vgc/dom/api.h>

namespace vgc::dom {

class Node;
class Document;

namespace detail {

VGC_DOM_API std::string notAliveMsg(const Node* node);
VGC_DOM_API std::string wrongDocumentMsg(const Node* n1, const Node* n2);
VGC_DOM_API std::string wrongChildTypeMsg(const Node* parent, const Node* child);
VGC_DOM_API std::string secondRootElementMsg(const Document* document);
VGC_DOM_API std::string childCycleMsg(const Node* parent, const Node* child);
VGC_DOM_API std::string replaceDocumentMsg(const Document* oldNode, const Node* newNode);

} // namespace detail

/// \class vgc::dom::LogicError
/// \brief Raised when there is a logic error detected in vgc::dom.
///
/// This exception is raised whenever there is a logic error detected in
/// vgc::dom. This is the base class for all logic error exception classes in
/// vgc::dom.
///
/// The class hierarchy for vgc::dom::LogicError exceptions is:
///
/// \verbatim
/// LogicError
///  +-- NotAliveError
///  +-- WrongDocumentError
///  +-- HierarchyRequestError
///       +-- WrongChildTypeError
///       +-- SecondRootElementError
///       +-- ChildCycleError
///       +-- ReplaceDocumentError
/// \endverbatim
///
class VGC_DOM_API_EXCEPTION LogicError : public core::LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a LogicError with the given \p reason.
    ///
    explicit LogicError(const std::string& reason)
        : core::LogicError(reason) {
    }
};

/// \class vgc::dom::WrongDocumentError
/// \brief Raised when two nodes do not belong to the same document
///        but are supposed to.
///
/// This exception is raised in Node::reparent() and Node::replace() if
/// the node that you are trying to reparent or replace belongs to a
/// different Document.
///
/// \sa Node::reparent() and Node::replace().
///
class VGC_DOM_API_EXCEPTION WrongDocumentError : public LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a WrongDocumentError informing that the Node \p n1 and the
    /// Node \p n2 do not belong to the same Document.
    ///
    WrongDocumentError(const Node* n1, const Node* n2)
        : LogicError(detail::wrongDocumentMsg(n1, n2)) {
    }
};

/// \class vgc::dom::HierarchyRequestError
/// \brief Raised when attempting to insert a Node somewhere it doesn't belong.
///
/// This exception is raised whenever a client requests to insert a Node at a
/// position where it cannot be inserted without breaking one of these two
/// invariants:
///
/// 1. A Node only has children of these allowed types:
///    - Document: allowed children are Element (at most one)
///    - Element: allowed children are Element
///
/// 2. A Node is never a child of itself or of any of its descendants (in other
///    words, the document has no cycle).
///
/// In the first case, the exception WrongChildTypeError or
/// SecondRootElementError is raised, while in the second case the exception
/// ChildCycleError is raised, all of which derive from HierarchyRequestError.
///
/// Also, this exception is raised when trying to replace the Document node,
/// see ReplaceDocumentError.
///
/// \sa Node::reparent() and Node::replace().
///
class VGC_DOM_API_EXCEPTION HierarchyRequestError : public LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a HierarchyRequestError with the given \p reason.
    ///
    HierarchyRequestError(const std::string& reason)
        : LogicError(reason) {
    }
};

/// \class vgc::dom::WrongChildTypeError
/// \brief Raised when requested to insert a child Node with incompatible
///        NodeType.
///
/// This exception is raised whenever a client requests to insert a child Node
/// whose type is not one of the allowed type. Here is the list of allowed type
/// according to the type of the parent Node:
/// - Document: allowed children are Element (at most one)
/// - Element: allowed children are Element
///
/// \sa Node::reparent() and Node::replace().
///
class VGC_DOM_API_EXCEPTION WrongChildTypeError : public HierarchyRequestError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a WrongChildTypeError informing that \p parent cannot
    /// have \p child as its child due to incompatible node types.
    ///
    WrongChildTypeError(const Node* parent, const Node* child)
        : HierarchyRequestError(detail::wrongChildTypeMsg(parent, child)) {
    }
};

/// \class vgc::dom::SecondRootElementError
/// \brief Raised when requested to insert a second child Element to a Document.
///
/// Document nodes are only allowed to have at most one Element child node,
/// called its root element (see rootElement()). This exception is raised
/// whenever a requested operation would result in a second root element be
/// inserted as a child of the Document node.
///
class VGC_DOM_API_EXCEPTION SecondRootElementError : public HierarchyRequestError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a SecondRootElementError informing that the Document \p node
    /// cannot have a second root element.
    ///
    SecondRootElementError(const Document* document)
        : HierarchyRequestError(detail::secondRootElementMsg(document)) {
    }
};

/// \class vgc::dom::ChildCycleError
/// \brief Raised when requested to make a Node a child of itself or of one of
///        its descendants
///
/// The DOM tree is not allowed to have cycles. Therefore, this exception is
/// raised whenever a requested operation would result in a cycle, that is,
/// when attempting to insert a Node as a child of itself or of one of its
/// descendants.
///
class VGC_DOM_API_EXCEPTION ChildCycleError : public HierarchyRequestError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a ChildCycleError informing that \p parent cannot have \p
    /// child as its child because \p parent is a descendant of \p child.
    ///
    ChildCycleError(const Node* parent, const Node* child)
        : HierarchyRequestError(detail::childCycleMsg(parent, child)) {
    }
};

/// \class vgc::dom::ReplaceDocumentError
/// \brief Raised when requested to replace the Document node.
///
/// The Document node can never be replaced by another node, and this exception
/// is raised whenever newNode->replace(oldNode) is called and oldNode is the
/// Document node (unless newNode is also the Document node, in which case
/// replace() does nothing).
///
class VGC_DOM_API_EXCEPTION ReplaceDocumentError : public HierarchyRequestError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a ReplaceDocumentError informing that \p newNode cannot
    /// replace \p oldNode because \p oldNode is the Document node.
    ///
    ReplaceDocumentError(const Document* oldNode, const Node* newNode)
        : HierarchyRequestError(detail::replaceDocumentMsg(oldNode, newNode)) {
    }
};

/// \class vgc::dom::RuntimeError
/// \brief Raised when there is a runtime error detected in vgc::dom.
///
/// This exception is raised whenever there is a runtime error detected in
/// vgc::dom. This is the base class for all runtime error exception classes in
/// vgc::dom.
///
/// The class hierarchy for vgc::dom::RuntimeError exceptions is:
///
/// \verbatim
/// RuntimeError
///  +-- ParseError
///       +-- XmlSyntaxError
///       +-- VgcSyntaxError
///  +-- FileError
/// \endverbatim
///
class VGC_DOM_API_EXCEPTION RuntimeError : public core::RuntimeError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a LogicError with the given \p reason.
    ///
    explicit RuntimeError(const std::string& reason)
        : core::RuntimeError(reason) {
    }
};

/// \class vgc::dom::ParseError
/// \brief Raised when parsing an input file or string failed.
///
/// This exception is raised by Document::open() if the input file is not a
/// well-formed VGC Document, either due to an XML syntax error, or a VGC
/// syntax error (An XML attribute does not have the expected VGC syntax for
/// attributes)
///
/// \sa XmlSyntaxError, VgcSyntaxError
///
class VGC_DOM_API_EXCEPTION ParseError : public RuntimeError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a ParseError with the given \p reason.
    ///
    ParseError(const std::string& reason)
        : RuntimeError(reason) {
    }
};

/// \class vgc::dom::XmlSyntaxError
/// \brief Raised when an input file or string is not a valid XML document.
///
/// This exception is raised when an input file or string is not a valid XML
/// document. For example, <path></vertex> is not a valid XML fragment because
/// the end tag does not match the start tag.
///
class VGC_DOM_API_EXCEPTION XmlSyntaxError : public ParseError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a XmlSyntaxError with the given \p reason.
    ///
    XmlSyntaxError(const std::string& reason)
        : ParseError(reason) {
    }
};

/// \class vgc::dom::VgcSyntaxError
/// \brief Raised when an input file or string is not a valid VGC document.
///
/// This exception is raised when an input file or string is a valid XML
/// document, but not a valid VGC document. For example, <path positions=""> is
/// a valid XML start tag, butit is not a valid VGC start tag:, because
/// positions is an attribute of ValueType::Vec2dArray and "" is not a valid
/// Vec2dArray. A correct start tag would be for example <path positions="[]">.
///
class VGC_DOM_API_EXCEPTION VgcSyntaxError : public ParseError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a VgcSyntaxError with the given \p reason.
    ///
    VgcSyntaxError(const std::string& reason)
        : ParseError(reason) {
    }
};

/// \class vgc::dom::FileError
/// \brief Raised when failed to open a file or save to a file.
///
/// This exception is raised by Document::open() if the input file cannot be
/// opened (for example, due to file permissions, or because the file does not
/// exist), and raised by Document::save() if the file cannot be written to
/// (most likely due to file permissions).
///
class VGC_DOM_API_EXCEPTION FileError : public RuntimeError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a FileError with the given \p reason.
    ///
    FileError(const std::string& reason)
        : RuntimeError(reason) {
    }
};

} // namespace vgc::dom

#endif // VGC_DOM_EXCEPTIONS_H
