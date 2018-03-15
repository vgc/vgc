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

#include <vgc/dom/document.h>
#include <vgc/dom/element.h>

namespace vgc {
namespace dom {

Document::Document() :
    Node()
{

}

DocumentSharedPtr Document::create()
{
    return std::shared_ptr<Document>(new Document());

    // Note: I can't use make_shared<> because constructor is protected. I may
    // want to define a private template method "template <Args...>
    // MyObject::make_shared_(...)" via a macro VGC_CORE_OBJECT(Document) to
    // make it more convenient for all objects to define their create() factory
    // functions.
}

void Document::setRootElement(ElementSharedPtr element)
{
    Node::removeAllChildren_();
    Node::addChild_(element);
}

} // namespace dom
} // namespace vgc
