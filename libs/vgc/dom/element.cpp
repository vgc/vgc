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
#include <vgc/dom/logcategories.h>
#include <vgc/dom/operation.h>
#include <vgc/dom/schema.h>
#include <vgc/dom/strings.h>

namespace vgc::dom {

Element::Element(Document* document, core::StringId tagName)
    : Node(document, NodeType::Element)
    , tagName_(tagName) {
}

void Element::onDestroyed() {
    if (uniqueId_ != core::StringId()) {
        document()->elementByIdMap_.erase(uniqueId_);
        uniqueId_ = core::StringId();
    }
    SuperClass::onDestroyed();
}

/* static */
Element* Element::create_(Node* parent, core::StringId tagName) {
    Document* document = parent->document();
    Element* e = new Element(document, tagName);
    core::History::do_<CreateElementOperation>(
        parent->document()->history(), e, parent, nullptr);
    return e;
}

/* static */
Element* Element::create(Document* parent, core::StringId tagName) {
    if (parent->rootElement()) {
        throw SecondRootElementError(parent);
    }

    return create_(parent, tagName);
}

/* static */
Element* Element::create(Element* parent, core::StringId tagName) {
    return create_(parent, tagName);
}

core::StringId Element::getOrCreateId() const {
    if (uniqueId_ == core::StringId()) {
        // create a new id !
        const ElementSpec* es = schema().findElementSpec(tagName_);
        core::StringId prefix = tagName_;
        if (es && es->defaultIdPrefix() != core::StringId()) {
            prefix = es->defaultIdPrefix();
        }
        auto& elementByIdMap = document()->elementByIdMap_;
        for (Int i = 0; i < core::IntMax; ++i) {
            core::StringId id = core::StringId(core::format("{}{}", prefix, i));
            if (elementByIdMap.find(id) == elementByIdMap.end()) {
                Element* ncThis = const_cast<Element*>(this);
                ncThis->setAttribute(strings::id, id);
                break;
            }
        }
    }
    return uniqueId_;
}

const Value& Element::getAuthoredAttribute(core::StringId name) const {
    if (const AuthoredAttribute* authored = findAuthoredAttribute_(name)) {
        return authored->value();
    }
    return Value::invalid();
}

const Value& Element::getAttribute(core::StringId name) const {
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
        VGC_WARNING(LogVgcDom, "Attribute is neither authored nor have a default value.");
        return Value::invalid();
    }
}

void Element::setAttribute(core::StringId name, const Value& value) {
    core::History::do_<SetAttributeOperation>(document()->history(), this, name, value);
}

void Element::clearAttribute(core::StringId name) {
    if (AuthoredAttribute* authored = findAuthoredAttribute_(name)) {
        Int index = std::distance(&authoredAttributes_[0], authored);
        core::History::do_<RemoveAuthoredAttributeOperation>(
            document()->history(), this, name, index);
    }
}

AuthoredAttribute* Element::findAuthoredAttribute_(core::StringId name) {
    return authoredAttributes_.search(
        [name](const AuthoredAttribute& attr) { return attr.name() == name; });
}

const AuthoredAttribute* Element::findAuthoredAttribute_(core::StringId name) const {
    return const_cast<Element*>(this)->findAuthoredAttribute_(name);
}

void Element::onAttributeChanged_(
    core::StringId name,
    const Value& oldValue,
    const Value& newValue) {
    if (name == strings::name) {
        name_ = newValue.hasValue() ? newValue.getStringId() : core::StringId();
    }
    else if (name == strings::id) {
        auto& elementByIdMap = document()->elementByIdMap_;
        if (uniqueId_ != core::StringId()) {
            elementByIdMap.erase(uniqueId_);
            uniqueId_ = core::StringId();
        }
        if (newValue.hasValue()) {
            uniqueId_ = newValue.getStringId();
            elementByIdMap[uniqueId_] = this;
        }
    }
    attributeChanged().emit(name, oldValue, newValue);
}

} // namespace vgc::dom
