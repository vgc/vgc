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

#ifndef VGC_DOM_ELEMENT_H
#define VGC_DOM_ELEMENT_H

#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/node.h>

namespace vgc {
namespace dom {

VGC_CORE_DECLARE_PTRS(Element);

/// \class vgc::dom::Element
/// \brief Represents an element of the DOM.
///
class VGC_DOM_API Element: public Node
{
public:
    VGC_CORE_OBJECT(Element)

    /// Creates a new Element with the given \p name.
    ///
    Element(core::StringId name);

    /// Returns the name of the element. This is equivalent to tagName()
    /// in the W3C DOM Specification.
    ///
    core::StringId name() const {
        return name_;
    }

private:
    core::StringId name_;
};

/// Defines the name of an element, retrievable via the
/// VGC_DOM_ELEMENT_GET_NAME(key) macro. This must only be used in .cpp files
/// where subclasses of Element are defined. Never use this in header files.
/// Also, the corresponding VGC_DOM_ELEMENT_GET_NAME(key) can only be used in
/// the same .cpp file where the name has been defined.
///
/// Example:
///
/// \code
/// VGC_DOM_ELEMENT_DEFINE_NAME(foo, "foo")
///
/// Foo::Foo() : Element(VGC_DOM_ELEMENT_GET_NAME(foo)) { }
/// \endcode
///
#define VGC_DOM_ELEMENT_DEFINE_NAME(key, name)                   \
    static vgc::core::StringId VGC_DOM_ELEMENT_NAME_##key##_() { \
        static vgc::core::StringId s(name);                      \
        return s;                                                \
    }

/// Retrieves the element name defined via
/// VGC_DOM_ELEMENT_DEFINE_NAME(key, name)
///
#define VGC_DOM_ELEMENT_GET_NAME(key) VGC_DOM_ELEMENT_NAME_##key##_()

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_ELEMENT_H
