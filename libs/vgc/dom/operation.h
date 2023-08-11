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

#ifndef VGC_DOM_OPERATION_H
#define VGC_DOM_OPERATION_H

#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/history.h>
#include <vgc/core/object.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/document.h>
#include <vgc/dom/element.h>
#include <vgc/dom/node.h>
#include <vgc/dom/value.h>

namespace vgc::dom {

class VGC_DOM_API Operation : public core::Operation {
protected:
    Operation()
        : doneDocumentVersionId_(core::genId()) {
    }

    core::Id undoneDocumentVersionId() const {
        return undoneDocumentVersionId_;
    }

    void setUndoneDocumentVersionId(core::Id versionId) {
        undoneDocumentVersionId_ = versionId;
    }

    core::Id doneDocumentVersionId() const {
        return doneDocumentVersionId_;
    }

private:
    core::Id undoneDocumentVersionId_ = {};
    core::Id doneDocumentVersionId_;
};

class VGC_DOM_API CreateElementOperation : public Operation {
protected:
    CreateElementOperation(Element* element, Node* parent, Node* nextSibling)
        : element_(element)
        , parent_(parent)
        , nextSibling_(nextSibling) {
    }

    ~CreateElementOperation();

public:
    Element* element() const {
        return element_.get();
    }

    Document* document() const;

    Node* parent() const {
        return parent_;
    }

    Node* nextSibling() const {
        return nextSibling_;
    }

protected:
    void do_() override;
    void undo_() override;
    void redo_() override;

private:
    ElementPtr element_;
    Node* parent_ = nullptr;
    Node* nextSibling_ = nullptr;
    bool keepAlive_ = false;
};

class VGC_DOM_API RemoveNodeOperation : public Operation {
protected:
    RemoveNodeOperation(Node* node)
        : node_(node) {
    }

    ~RemoveNodeOperation() {
        if (keepAlive_) {
            detail::destroyNode(node_.get());
        }
    }

public:
    Node* node() const {
        return node_.get();
    }

    const NodeRelatives& savedRelatives() const {
        return savedRelatives_;
    }

protected:
    void do_() override;
    void undo_() override;
    void redo_() override;

private:
    NodePtr node_;
    bool keepAlive_ = true;
    NodeRelatives savedRelatives_;
};

class VGC_DOM_API MoveNodeOperation : public Operation {
protected:
    MoveNodeOperation(Node* node, Node* newParent, Node* newNextSibling)
        : node_(node)
        , newRelatives_(newParent, nullptr, newNextSibling) {
    }

public:
    Node* node() const {
        return node_;
    }

    const NodeRelatives& oldRelatives() const {
        return oldRelatives_;
    }

    const NodeRelatives& newRelatives() const {
        return newRelatives_;
    }

protected:
    void do_() override;
    void undo_() override;
    void redo_() override;

private:
    Node* node_;
    NodeRelatives oldRelatives_;
    NodeRelatives newRelatives_;
};

class VGC_DOM_API SetAttributeOperation : public Operation {
protected:
    friend Operation;

    SetAttributeOperation(Element* element, core::StringId name, Value value)
        : element_(element)
        , name_(name)
        , newValue_(value) {
    }

public:
    Element* element() const {
        return element_;
    }

    core::StringId name() const {
        return name_;
    }

    const Value& oldValue() const {
        return oldValue_;
    }

    const Value& newValue() const {
        return newValue_;
    }

protected:
    void do_() override;
    void undo_() override;
    void redo_() override;

private:
    Element* element_;
    core::StringId name_;
    Int index_ = -1; // XXX would be difficult to keep this
                     // optimization when coalescing in finalize()..
    bool isNew_ = false;

    // TODO: oldValue_ could be shared with previous setAttribute newValue_
    //       of the same attribute.
    //       Or should we store values as string here ?
    Value oldValue_;
    Value newValue_;
};

class VGC_DOM_API RemoveAuthoredAttributeOperation : public Operation {
protected:
    RemoveAuthoredAttributeOperation(Element* element, core::StringId name, Int index)
        : element_(element)
        , name_(name)
        , index_(index) {
    }

public:
    Element* element() const {
        return element_;
    }

    core::StringId name() const {
        return name_;
    }

    const Value& oldValue() const {
        return oldValue_;
    }

protected:
    void do_() override;
    void undo_() override;
    void redo_() override;

private:
    Element* element_;
    core::StringId name_;
    Int index_ = -1;
    Value oldValue_;
};

using OperationIndex = UInt32;
OperationIndex genOperationIndex();

} // namespace vgc::dom

#endif // VGC_DOM_OPERATION_H
