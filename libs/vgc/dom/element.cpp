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

#include <vgc/core/logging.h>

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

const Value& Element::getAttribute(core::StringId name) const
{
    if (AuthoredAttribute_* authored = findAuthoredAttribute_(name)) {
        return authored->value;
    }
    else if (BuiltInAttribute* builtIn = findBuiltInAttribute_(name)) {
        return builtIn->defaultValue();
    }
    else {
        core::warning() << "Attribute is neither authored not built-in.";
        return Value::invalid();
    }
}

void Element::setAttribute(core::StringId name, const Value& value)
{
    // If already authored, update the authored value
    if (AuthoredAttribute_* authored = findAuthoredAttribute_(name)) {
        authored->value = value;
    }

    // Otherwise, allocate a new AuthoredAttribute
    authoredAttributes_.emplace_back(new AuthoredAttribute_ {name, value});

}

Element::AuthoredAttribute_* Element::findAuthoredAttribute_(core::StringId name) const
{
    for (const auto& a : authoredAttributes_) {
        if (a->name == name) {
            return a.get();
        }
    }
    return nullptr;
}

BuiltInAttribute* Element::findBuiltInAttribute_(core::StringId /*name*/) const
{
    // XXX TODO
    return nullptr;
}

} // namespace dom
} // namespace vgc
