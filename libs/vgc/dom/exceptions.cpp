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

#include <vgc/dom/exceptions.h>

#include <vgc/core/format.h>
#include <vgc/dom/document.h>
#include <vgc/dom/node.h>

namespace vgc::dom {

namespace detail {

std::string wrongDocumentMsg(const Node* n1, const Node* n2) {
    return core::format(
        "Node {} and Node {} belong to different documents"
        " (resp. Document {} and Document {})",
        core::toAddressString(n1),
        core::toAddressString(n1),
        core::toAddressString(n1->document()),
        core::toAddressString(n2->document()));
}

std::string wrongChildTypeMsg(const Node* parent, const Node* child) {
    return core::format(
        "Node {} (type = {}) cannot be a child of Node {} (type = {})",
        core::toAddressString(child),
        core::toString(child->nodeType()),
        core::toAddressString(parent),
        core::toString(parent->nodeType()));
}

std::string secondRootElementMsg(const Document* document) {
    return core::format(
        "Document {} cannot have a second root element (existing Element is {})",
        core::toAddressString(document),
        core::toAddressString(document->rootElement()));
}

std::string childCycleMsg(const Node* parent, const Node* child) {
    return core::format(
        "Node {} cannot be a child of Node {}"
        " because the latter is a descendant of the former",
        core::toAddressString(child),
        core::toAddressString(parent));
}

std::string replaceDocumentMsg(const Document* oldNode, const Node* newNode) {
    return core::format(
        "Node {} cannot replace Document node {}",
        core::toAddressString(newNode),
        core::toAddressString(oldNode));
}

} // namespace detail

VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(LogicError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(WrongDocumentError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(HierarchyRequestError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(WrongChildTypeError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(SecondRootElementError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(ChildCycleError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(ReplaceDocumentError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(RuntimeError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(ParseError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(XmlSyntaxError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(VgcSyntaxError)
VGC_CORE_EXCEPTIONS_DEFINE_ANCHOR(FileError)

} // namespace vgc::dom
