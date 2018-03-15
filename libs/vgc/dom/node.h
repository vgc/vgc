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

#ifndef VGC_DOM_NODE_H
#define VGC_DOM_NODE_H

#include <vector>
#include <vgc/core/object.h>
#include <vgc/dom/api.h>

namespace vgc {
namespace dom {

VGC_CORE_DECLARE_PTRS(Node);

/// \class vgc::dom::Node
/// \brief Represents a node of the DOM. All dom classes derive from Node.
///
/// This is the base class for all classes in the DOM. For example, the
/// Document is a node, each Element is a Node, and even each Attribute is a
/// Node.
///
/// Each Node has children() nodes and is responsible for the lifetime of
/// these children.
///
/// Typically, a Document is loaded from an XML file, and Nodes remember the
/// location in the file where the data was initially loaded. However, to
/// preserve memory, the corresponding string is typically not hold by Nodes,
/// unless otherwise stated (for example, when users directly edit the string
/// in an XML node editor).
///
class VGC_DOM_API Node: public core::Object
{
public:
    /// Returns the parent of this Node.
    ///
    /// \sa children() and isRootNode().
    ///
    Node* parent() const {
        return parent_;
    }

    /// Returns whether this node is a root node, that is, whether it has no
    /// parent. For example, a dom::Document is always a root node. However,
    /// note that a dom::Element is never a root node, even the rootElement.
    /// Indeed, the parent of the rootElement is the Document it belongs to.
    ///
    /// \sa parent().
    ///
    bool isRootNode() const {
        return !parent();
    }

    /// Returns the children of this Node.
    ///
    /// \sa parent().
    ///
    const std::vector<NodeSharedPtr>& children() const {
        return children_;
    }

protected:
    /// Constructs a root Node, that is, a Node with no parent.
    ///
    /// \sa isRootNode().
    ///
    Node();

private:
    Node* parent_;
    std::vector<NodeSharedPtr> children_;
    void addChild_(NodeSharedPtr node);
    void removeAllChildren_();

    // Allow each type of node to set children/parent, since they have
    // different constraints that Node does not know about (for example, an
    // Attribute cannot have a child Element). However, note that subclasses of
    // these are not befriended, that is, a PathElement cannot directly
    // manipulate its children_.
    friend class Attribute;
    friend class Document;
    friend class Element;
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_NODE_H
