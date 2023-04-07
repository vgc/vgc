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

#include <vgc/dom/node.h>

#include <vgc/core/assert.h>
#include <vgc/core/logging.h>
#include <vgc/core/object.h>
#include <vgc/dom/document.h>
#include <vgc/dom/element.h>
#include <vgc/dom/logcategories.h>
#include <vgc/dom/strings.h>

namespace vgc::dom {

namespace detail {

void destroyNode(Node* node) {
    node->destroyObject_();
}

} // namespace detail

Node::Node(Document* document, NodeType nodeType)
    : Object()
    , document_(document)
    , nodeType_(nodeType) {
}

void Node::onDestroyed() {
    document_ = nullptr;
    SuperClass::onDestroyed();
}

void Node::remove() {
    core::History::do_<RemoveNodeOperation>(document()->history(), this);
}

namespace {

bool checkCanReparent_(
    Node* parent,
    Node* child,
    bool simulate = false,
    bool checkSecondRootElement = true) {

    if (parent->document() != child->document()) {
        if (simulate) {
            return false;
        }
        else {
            throw WrongDocumentError(parent, child);
        }
    }

    if (child->nodeType() == NodeType::Document) {
        if (simulate) {
            return false;
        }
        else {
            throw WrongChildTypeError(parent, child);
        }
    }

    if (checkSecondRootElement && parent->nodeType() == NodeType::Document
        && child->nodeType() == NodeType::Element) {
        Element* rootElement = Document::cast(parent)->rootElement();
        if (rootElement && rootElement != child) {
            if (simulate) {
                return false;
            }
            else {
                throw SecondRootElementError(Document::cast(parent));
            }
        }
    }

    if (parent->isDescendantObject(child)) {
        if (simulate) {
            return false;
        }
        else {
            throw ChildCycleError(parent, child);
        }
    }

    return true;
}

} // namespace

void Node::insertChild(Node* nextSibling, Node* child) {
    core::History::do_<MoveNodeOperation>(
        document()->history(), child, this, nextSibling);
}

bool Node::canReparent(Node* newParent) {
    const bool simulate = true;
    return checkCanReparent_(newParent, this, simulate);
}

void Node::reparent(Node* newParent) {
    checkCanReparent_(newParent, this);
    core::History::do_<MoveNodeOperation>(
        document()->history(), this, newParent, nullptr);
}

namespace {

bool checkCanReplace_(Node* oldNode, Node* newNode, bool simulate = false) {

    // Avoid raising ReplaceDocumentError if oldNode == newNode (= Document node)
    if (oldNode == newNode) {
        return true;
    }

    if (oldNode->nodeType() == NodeType::Document) {
        if (simulate) {
            return false;
        }
        else {
            throw ReplaceDocumentError(Document::cast(oldNode), newNode);
        }
    }
    Node* oldNodeParent = oldNode->parent(); // guaranteed non-null

    // Avoid raising SecondRootElementError if oldNode is the root element
    bool checkSecondRootElement = (oldNode->nodeType() != NodeType::Element);

    // All other checks are the same as for reparent(), so we delate.
    return checkCanReparent_(oldNodeParent, newNode, simulate, checkSecondRootElement);
}

} // namespace

bool Node::canReplace(Node* oldNode) {
    const bool simulate = true;
    return checkCanReplace_(oldNode, this, simulate);
}

void Node::replace(Node* oldNode) {
    // TODO: record atomic operations

    // newChid = this
    // willLoseAChild = ignored = this->parent()
    // oldChild = willBeDestroyed = oldNode
    // willHaveAChildReplaced = oldNode->parent()

    checkCanReplace_(oldNode, this);
    if (this == oldNode) {
        // nothing to do
        return;
    }
    // Note: this Node might be a descendant of oldNode, so we need
    // remove it from parent before destroying the old Node.
    Node* parent = oldNode->parent();
    Node* nextSibling = oldNode->nextSibling();
    core::ObjectPtr self = removeObjectFromParent_();

    // TODO: use remove, because this is currently not undoable
    //parent->removeChildObject_()

    oldNode->destroyObject_();
    //oldNode->removeObjectFromParent_();

    parent->insertChildObject_(nextSibling, this);
}

Element* Node::getElementFromPath(const Path& path, core::StringId tagNameFilter) const {
    Element* element = nullptr;
    bool firstAttributeEncountered = false;
    for (const PathSegment& seg : path.segments()) {
        switch (seg.type()) {
        case PathSegmentType::Root:
            element = document()->rootElement();
            break;
        case PathSegmentType::Id:
            // nullptr if not found, handled out of switch
            element = document()->elementById(seg.name());
            break;
        case PathSegmentType::Element:
            if (!element) {
                element = Element::cast(const_cast<Node*>(this));
            }
            if (element) {
                Element* childElement = element->firstChildElement();
                while (childElement) {
                    if (childElement->name() == seg.name()) {
                        break;
                    }
                    childElement = childElement->nextSiblingElement();
                }
                element = childElement; // nullptr if not found, handled out of switch
            }
            break;
        case PathSegmentType::Attribute:
            if (!element) {
                element = Element::cast(const_cast<Node*>(this));
            }
            firstAttributeEncountered = true;
        }
        // Break out of loop if there is an error or we reached the end of
        // the "element part" of the path.
        if (!element /* <- error */ || firstAttributeEncountered) {
            break;
        }
    }
    if (element && !tagNameFilter.isEmpty() && element->tagName() != tagNameFilter) {
        VGC_WARNING(
            LogVgcDom,
            "Path `{}` resolved to an element `{}` but `{}` was expected.",
            path,
            element->tagName(),
            tagNameFilter);
        return nullptr;
    }

    return element;
}

Value Node::getValueFromPath(const Path& path, core::StringId tagNameFilter) const {
    if (path.isAttributePath()) {
        Element* element = getElementFromPath(path);
        if (element) {
            if (!tagNameFilter.isEmpty() && element->tagName() != tagNameFilter) {
                VGC_WARNING(
                    LogVgcDom,
                    "Path `{}` resolved to an element `{}` but `{}` was expected.",
                    path,
                    element->tagName(),
                    tagNameFilter);
                return Value();
            }
            const PathSegment& seg = path.segments().last();
            Value value = element->getAttribute(seg.name());
            if (value.isValid() && seg.isIndexed()) {
                value = value.getItemWrapped(seg.arrayIndex());
            }
            return value;
        }
    }
    return Value();
}

} // namespace vgc::dom
