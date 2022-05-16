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

#include <vgc/dom/element.h>

#include <vgc/core/logging.h>
#include <vgc/dom/document.h>

namespace vgc {
namespace dom {

Element::Element(Document* document, core::StringId name) :
    Node(document, NodeType::Element),
    name_(name)
{

}

/* static */
Element* Element::create_(Node* parent, core::StringId name)
{
    Element* res = new Element(parent->document(), name);
    res->appendObjectToParent_(parent);
    return res;
}

/* static */
Element* Element::create(Document* parent, core::StringId name)
{
    if (parent->rootElement()) {
        throw SecondRootElementError(parent);
    }
    return create_(parent, name);
}

/* static */
Element* Element::create(Element* parent, core::StringId name)
{
    return create_(parent, name);
}

const Value& Element::getAttribute(core::StringId name) const
{
    if (const AuthoredAttribute* authored = findAuthoredAttribute_(name)) {
        return authored->value();
    }
    /*
     * TODO: find default value from schema.
    else if (const AttributeSpec* builtIn = findBuiltInAttribute_(name)) {
        return builtIn->defaultValue();
    }
    */
    else {
        core::warning() << "Attribute is neither authored nor have a default value.";
        return Value::invalid();
    }
}

void Element::setAttribute(core::StringId name, const Value& value)
{
    // If already authored, update the authored value
    if (AuthoredAttribute* authored = findAuthoredAttribute_(name)) {
        Value oldValue = authored->value();
        authored->setValue(value);
        this->document()->onChangedAuthoredAttribute_(this, name, oldValue, authored->value());
    }
    else {
        // Otherwise, allocate a new AuthoredAttribute
        authoredAttributes_.emplaceLast(name, value);
        this->document()->onCreatedAuthoredAttribute_(this, name, value);
    }
}

void Element::setAttribute(core::StringId name, Value&& value)
{
    // If already authored, update the authored value
    if (AuthoredAttribute* authored = findAuthoredAttribute_(name)) {
        Value oldValue = authored->value();
        authored->setValue(std::move(value));
        this->document()->onChangedAuthoredAttribute_(this, name, oldValue, authored->value());
    }
    else {
        // Otherwise, allocate a new AuthoredAttribute
        authoredAttributes_.emplaceLast(name, std::move(value));
        this->document()->onCreatedAuthoredAttribute_(this, name, authoredAttributes_.last().value());
    }
}

void Element::clearAttribute(core::StringId name)
{
    if (AuthoredAttribute* authored = findAuthoredAttribute_(name)) {
        Value oldValue = authored->value();
        authoredAttributes_.removeAt(std::distance(&authoredAttributes_[0], authored));
        this->document()->onRemovedAuthoredAttribute_(this, name, oldValue);
    }
}

AuthoredAttribute* Element::findAuthoredAttribute_(core::StringId name)
{
    return authoredAttributes_.search(
        [name](const AuthoredAttribute& attr) {
            return attr.name() == name;
        });
}

const AuthoredAttribute* Element::findAuthoredAttribute_(core::StringId name) const
{
    return const_cast<Element*>(this)->findAuthoredAttribute_(name);
}

} // namespace dom
} // namespace vgc
