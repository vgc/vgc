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
/// Each Node is solely responsible for the lifetime of its childrenNodes(), and
/// stores shared pointers to its children. Observers should not normally hold
/// shared pointers to nodes (this would prevent their destruction despite
/// no meaningful existence possible), but instead hold either weak pointers
/// or raw pointers (when the lifetime of the object is guaranteed via other
/// means).
///
/// Typically, a Document is loaded from an XML file, and Nodes remember the
/// location in the file where the data was initially loaded. However, to
/// preserve memory, the corresponding string is typically not hold by Nodes,
/// unless otherwise stated (for example, when users directly edit the string
/// in an XML node editor).
///
class VGC_DOM_API Node: public core::Object<Node>
{
public:
    /// Creates a Node.
    ///
    Node();
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_NODE_H
