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

#include <vgc/dom/element.h>

namespace vgc {
namespace dom {

Element::Element(core::StringId name) :
    Node(NodeType::Element),
    name_(name)
{

}

const std::vector<BuiltInAttribute>& Element::builtInAttributes() const
{
    // XXX TODO
}

Attribute Element::attr(core::StringId name)
{
    // XXX For now, let's assume we only author attributes that are not built-in.

    // Return attribute that is already authored
    for (const detail::AuthoredAttributeSharedPtr& a : authoredAttributes_) {
        if (a->name() == name) {
            return Attribute(this, name, nullptr, a.get());
        }
    }

    // Return attribute that is not yet authored
    return Attribute(this, name, nullptr, nullptr);
}

} // namespace dom
} // namespace vgc
