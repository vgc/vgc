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

#ifndef VGC_DOM_EXCEPTIONS_H
#define VGC_DOM_EXCEPTIONS_H

#include <vgc/core/exceptions.h>
#include <vgc/dom/api.h>

namespace vgc {
namespace dom {

class Node;
class Document;

/// \class vgc::dom::LogicError
/// \brief Raised when there is a logic error detected in vgc::dom.
///
/// This exception is raised whenever there is a logic error detected in
/// vgc::dom. This is the base class for all logic error exception classes in
/// vgc::dom.
///
/// The class hierarchy for vgc::dom exceptions is:
///
/// \verbatim
/// LogicError
///  +-- NotAliveError
///  +-- WrongDocumentError
///  +-- HierarchyRequestError
///       +-- WrongChildTypeError
///       +-- SecondRootElementError
/// \endverbatim
///
class VGC_DOM_API LogicError : public core::LogicError {
public:
    /// Constructs a LogicError with the given \p reason.
    ///
    explicit LogicError(const std::string& reason) : core::LogicError(reason) {}

    /// Destructs the LogicError.
    ///
    ~LogicError();
};

/// \class vgc::dom::NotAliveError
/// \brief Raised when attempting to use a Node which is not alive.
///
/// This exception is raised whenever trying to perform an operation
/// involving a Node that has already been destroyed.
///
/// \sa Node::isAlive() and Node::destroy().
///
class VGC_DOM_API NotAliveError : public LogicError {
public:
    /// Constructs a NotAliveError informing that the Node \p node is not alive.
    ///
    NotAliveError(const Node* node);

    /// Destructs the NotAliveError.
    ///
    ~NotAliveError();
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
class VGC_DOM_API WrongDocumentError : public LogicError {
public:
    /// Constructs a WrongDocumentError informing that the Node \p n1 and the
    /// Node \p n2 do not belong to the same Document.
    ///
    WrongDocumentError(const Node* n1, const Node* n2);

    /// Destructs the WrongDocumentError.
    ///
    ~WrongDocumentError();
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
class VGC_DOM_API HierarchyRequestError : public LogicError {
public:
    /// Constructs a HierarchyRequestError with the given \p reason.
    ///
    HierarchyRequestError(const std::string& reason) : LogicError(reason) {}

    /// Destructs the HierarchyRequestError.
    ///
    ~HierarchyRequestError();
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
class VGC_DOM_API WrongChildTypeError : public HierarchyRequestError {
public:
    /// Constructs a WrongChildTypeError informing that \p parent cannot
    /// have \p child as its child due to incompatible node types.
    ///
    WrongChildTypeError(const Node* parent, const Node* child);

    /// Destructs the WrongChildTypeError.
    ///
    ~WrongChildTypeError();
};

/// \class vgc::dom::SecondRootElementError
/// \brief Raised when requested to insert a second child Element to a Document.
///
/// Document nodes are only allowed to have at most one Element child node,
/// called its root element (see rootElement()). This exception is raised
/// whenever a requested operation would result in a second root element be
/// inserted as a child of the Document node.
///
class VGC_DOM_API SecondRootElementError : public HierarchyRequestError {
public:
    /// Constructs a SecondRootElementError informing that the Document \p node
    /// cannot have a second root element.
    ///
    SecondRootElementError(const Document* document);

    /// Destructs the SecondRootElementError.
    ///
    ~SecondRootElementError();
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
class VGC_DOM_API ChildCycleError : public HierarchyRequestError {
public:
    /// Constructs a ChildCycleError informing that \p parent cannot have \p
    /// child as its child because \p parent is a descendant of \p child.
    ///
    ChildCycleError(const Node* parent, const Node* child);

    /// Destructs the ChildCycleError.
    ///
    ~ChildCycleError();
};

/// \class vgc::dom::ReplaceDocumentError
/// \brief Raised when requested to replace the Document node.
///
/// The Document node can never be replaced by another node, and this exception
/// is raised whenever newNode->replace(oldNode) is called and oldNode is the
/// Document node (unless newNode is also the Document node, in which case
/// replace() does nothing).
///
class VGC_DOM_API ReplaceDocumentError : public HierarchyRequestError {
public:
    /// Constructs a ReplaceDocumentError informing that \p newNode cannot
    /// replace \p oldNode because \p oldNode is the Document node.
    ///
    ReplaceDocumentError(const Document* oldNode, const Node* newNode);

    /// Destructs the ReplaceDocumentError.
    ///
    ~ReplaceDocumentError();
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_EXCEPTIONS_H
