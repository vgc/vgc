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

VGC_CORE_DECLARE_PTRS(Document);
VGC_CORE_DECLARE_PTRS(Element);

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

    /// Creates a new document with a `vgc` root element. This is equivalent
    /// to Document("vgc"). Note that it is not possible to create a Document
    /// with no root element, since a valid Document always has a root element.
    ///
    Document();

    /// Sets the root element of this Document.
    /// This takes ownership of the element.
    /// XXX what if existing?
    ///
    void setRootElement(ElementSharedPtr element);

    /// Returns the root element of this Document.
    ///
    Element* rootElement() const {
        return rootElement_;
    }

private:
    Element* rootElement_;

};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_DOCUMENT_H
