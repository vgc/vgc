// Copyright 2022 The VGC Developers
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

#include <vgc/dom/operation.h>

#include <vgc/dom/document.h>
#include <vgc/dom/element.h>
#include <vgc/dom/node.h>

namespace vgc::dom {

// XXX todos:
// - check create/remove conflicts in diff
//     -> if remove is followed by create then remove the elem from diff
//     -> if there is a remove or create, remove the attribute changes

CreateElementOperation::~CreateElementOperation() {
    if (keepAlive_) {
        detail::destroyNode(element_.get());
    }
}

void CreateElementOperation::do_() {
    redo_();
}

void CreateElementOperation::undo_() {
    Document* document = element_->document();
    element_->removeObjectFromParent_();
    document->onRemoveNode_(element_.get());
    keepAlive_ = true;
}

void CreateElementOperation::redo_() {
    Document* document = element_->document();
    element_->insertObjectToParent_(parent_, nextSibling_);
    document->onCreateNode_(element_.get());
    keepAlive_ = false;
}

Document* CreateElementOperation::document() const {
    return element_->document();
}

void RemoveNodeOperation::do_() {
    savedRelatives_ = NodeRelatives(node_.get());
    redo_();
}

void RemoveNodeOperation::undo_() {
    Document* document = node_->document();
    node_->insertObjectToParent_(savedRelatives_.parent(), savedRelatives_.nextSibling());
    document->onCreateNode_(node_.get());
    keepAlive_ = false;
}

void RemoveNodeOperation::redo_() {
    Document* document = node_->document();
    node_->removeObjectFromParent_();
    document->onRemoveNode_(node_.get());
    keepAlive_ = true;
}

void MoveNodeOperation::do_() {
    oldRelatives_ = NodeRelatives(node_);
    redo_();
    newRelatives_ = NodeRelatives(node_);
}

void MoveNodeOperation::undo_() {
    Document* document = node_->document();
    node_->insertObjectToParent_(oldRelatives_.parent(), oldRelatives_.nextSibling());
    document->onMoveNode_(node_, newRelatives_);
}

void MoveNodeOperation::redo_() {
    Document* document = node_->document();
    node_->insertObjectToParent_(newRelatives_.parent(), newRelatives_.nextSibling());
    document->onMoveNode_(node_, oldRelatives_);
}

void SetAttributeOperation::do_() {
    Document* document = element_->document();
    // If already authored, update the authored value
    if (AuthoredAttribute* authored = element_->findAuthoredAttribute_(name_)) {
        isNew_ = false;
        oldValue_ = authored->value();
        index_ = std::distance(&element_->authoredAttributes_[0], authored);
        authored->setValue(newValue_);
    }
    else { // Otherwise, allocate a new AuthoredAttribute
        isNew_ = true;
        oldValue_ = Value::none();
        index_ = element_->authoredAttributes_.size();
        element_->authoredAttributes_.emplaceLast(name_, newValue_);
    }
    document->onChangeAttribute_(element_, name_);
    element_->onAttributeChanged_(name_, oldValue_, newValue_);
}

void SetAttributeOperation::undo_() {
    Document* document = element_->document();
    if (isNew_) {
        element_->authoredAttributes_.removeLast();
    }
    else {
        element_->authoredAttributes_[index_].setValue(oldValue_);
    }
    document->onChangeAttribute_(element_, name_);
    element_->onAttributeChanged_(name_, newValue_, oldValue_);
}

void SetAttributeOperation::redo_() {
    Document* document = element_->document();
    if (isNew_) {
        element_->authoredAttributes_.emplaceLast(name_, newValue_);
    }
    else {
        element_->authoredAttributes_[index_].setValue(newValue_);
    }
    document->onChangeAttribute_(element_, name_);
    element_->onAttributeChanged_(name_, oldValue_, newValue_);
}

void RemoveAuthoredAttributeOperation::do_() {
    oldValue_ = element_->authoredAttributes_[index_].value();
    redo_();
    element_->onAttributeChanged_(name_, oldValue_, Value());
}

void RemoveAuthoredAttributeOperation::undo_() {
    Document* document = element_->document();
    element_->authoredAttributes_.emplace(index_, name_, oldValue_);
    document->onChangeAttribute_(element_, name_);
    element_->onAttributeChanged_(name_, Value(), oldValue_);
}

void RemoveAuthoredAttributeOperation::redo_() {
    Document* document = element_->document();
    element_->authoredAttributes_.removeAt(index_);
    document->onChangeAttribute_(element_, name_);
    element_->onAttributeChanged_(name_, oldValue_, Value());
}

namespace {

OperationIndex lastId = 0;

} // namespace

OperationIndex genOperationIndex() {
    // XXX make this thread-safe ?
    return ++lastId;
}

} // namespace vgc::dom
