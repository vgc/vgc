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

#include <vgc/dom/exceptions.h>

#include <vgc/core/stringutil.h>
#include <vgc/dom/document.h>
#include <vgc/dom/node.h>

namespace vgc {
namespace dom {

LogicError::~LogicError()
{

}

// Note: in all the res.reserve() calls below, we assume that:
// - Addresses of nodes take at most 14 chars (AMD64 architecture is defined to
//   have only 48bit of virtual address space, thus 12 hex chars + leading "0x")
// - node types have at most 8 chars (currently, the longest is "Document")

namespace {
std::string notAliveReason_(const Node* node)
{
    std::string res;
    res.reserve(32);
    res.append("Node ");
    res.append(core::toString(node));
    res.append(" is not alive");
    return res;
}
} // namespace

NotAliveError::NotAliveError(const Node* node) :
    LogicError(notAliveReason_(node))
{

}

NotAliveError::~NotAliveError()
{

}

namespace {
std::string wrongDocumentReason_(const Node* n1, const Node* n2)
{
    std::string res;
    res.reserve(133);
    res.append("Node ");
    res.append(core::toString(n1));
    res.append(" and Node ");
    res.append(core::toString(n2));
    res.append("belong to different documents (resp. Document ");
    res.append(core::toString(n1->document()));
    res.append(" and Document ");
    res.append(core::toString(n2->document()));
    res.append(")");
    return res;
}
} // namespace

WrongDocumentError::WrongDocumentError(const Node* n1, const Node* n2) :
    LogicError(wrongDocumentReason_(n1, n2))
{

}

WrongDocumentError::~WrongDocumentError()
{

}

HierarchyRequestError::~HierarchyRequestError()
{

}

namespace {
std::string wrongChildTypeReason_(const Node* parent, const Node* child)
{
    std::string res;
    res.reserve(96);
    res.append("Node ");
    res.append(core::toString(child));
    res.append(" (type = ");
    res.append(toString(child->nodeType()));
    res.append(") cannot be a child of Node ");
    res.append(core::toString(parent));
    res.append(" (type = ");
    res.append(toString(parent->nodeType()));
    res.append(")");
    return res;
}
} // namespace

WrongChildTypeError::WrongChildTypeError(const Node* parent, const Node* child) :
    HierarchyRequestError(wrongChildTypeReason_(parent, child))
{

}

WrongChildTypeError::~WrongChildTypeError()
{

}

namespace {
std::string secondRootElementReason_(const Document* document)
{
    std::string res;
    res.reserve(94);
    res.append("Document ");
    res.append(core::toString(document));
    res.append(" cannot have a second root element (existing Element is ");
    res.append(core::toString(document->rootElement()));
    res.append(")");
    return res;
}
} // namespace

SecondRootElementError::SecondRootElementError(const Document* document) :
    HierarchyRequestError(secondRootElementReason_(document))
{

}

SecondRootElementError::~SecondRootElementError()
{

}

} // namespace dom
} // namespace vgc
