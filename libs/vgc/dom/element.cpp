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
    , tagName_(tagName)
    , internalId_(core::genId()) {

    document->elementByInternalIdMap_[internalId_] = this;
}

void Element::onDestroyed() {
    Document* doc = document();
    if (id_ != core::StringId()) {
        doc->elementByIdMap_.erase(id_);
        id_ = core::StringId();
    }
    doc->elementByInternalIdMap_.erase(internalId_);
    SuperClass::onDestroyed();
}

/* static */
Element* Element::create_(Node* parent, core::StringId tagName) {
    Document* doc = parent->document();
    Element* e = new Element(doc, tagName);
    core::History::do_<CreateElementOperation>(doc->history(), e, parent, nullptr);
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
    if (id_ == core::StringId()) {
        // create a new id !
        const ElementSpec* elementSpec = schema().findElementSpec(tagName_);
        core::StringId prefix = tagName_;
        if (elementSpec && elementSpec->defaultIdPrefix() != core::StringId()) {
            prefix = elementSpec->defaultIdPrefix();
        }
        auto& elementByIdMap = document()->elementByIdMap_;
        for (Int i = 0; i < core::IntMax; ++i) {
            core::StringId id = core::StringId(core::format("{}{}", prefix, i));
            if (elementByIdMap.find(id) == elementByIdMap.end()) {
                Element* ncThis = const_cast<Element*>(this);
                // This also registers id to the elementByIdMap.
                ncThis->setAttribute(strings::id, id);
                break;
            }
        }
    }
    return id_;
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
    else {
        const ElementSpec* elementSpec = schema().findElementSpec(tagName_);
        const AttributeSpec* attributeSpec = elementSpec->findAttributeSpec(name);

        if (attributeSpec) {
            return attributeSpec->defaultValue();
        }
        else {
            VGC_WARNING(
                LogVgcDom, "Attribute is neither authored nor have a default value.");
            return Value::invalid();
        }
    }
}

std::optional<Element*> Element::getElementFromPathAttribute(
    core::StringId name,
    core::StringId tagNameFilter) const {

    const dom::Value& value = getAttribute(name);
    const dom::Path* pathPtr = nullptr;

    if (value.type() == dom::ValueType::NoneOrPath) {
        const dom::NoneOr<dom::Path>& noneOrPath = value.getNoneOrPath();
        if (!noneOrPath.has_value()) {
            return std::nullopt;
        }
        pathPtr = &noneOrPath.value();
    }
    else {
        // cast as path and throws if it is not one
        pathPtr = &value.getPath();
    }

    // resolve path (relative to this element if the path is relative
    dom::Element* element = elementFromPath(*pathPtr);

    if (!element) {
        VGC_WARNING(
            LogVgcDom,
            "Path in attribute `{}` of element `{}` could not be resolved ({}).",
            name,
            tagName(),
            *pathPtr);
        return nullptr;
    }

    if (!tagNameFilter.isEmpty() && element->tagName() != tagNameFilter) {
        VGC_WARNING(
            LogVgcDom,
            "Path in attribute `{}` of element `{}` resolved to an element `{}` but `{}` "
            "was expected.",
            name,
            tagName(),
            element->tagName(),
            tagNameFilter);
        return nullptr;
    }

    return element;
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
        if (id_ != core::StringId()) {
            elementByIdMap.erase(id_);
            id_ = core::StringId();
        }
        if (newValue.hasValue()) {
            id_ = newValue.getStringId();
            elementByIdMap[id_] = this;
        }
    }
    attributeChanged().emit(name, oldValue, newValue);
}

} // namespace vgc::dom
