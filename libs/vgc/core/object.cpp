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

#include <vgc/core/format.h>
#include <vgc/core/object.h>

namespace vgc::core {

namespace {

#ifdef VGC_CORE_OBJECT_DEBUG
void printDebugInfo_(Object* obj, const char* s) {
    core::print("Object {} {}\n", core::asAddress(obj), s);
}
#else
void printDebugInfo_(Object*, const char*) {
}
#endif

// Returns whether the string s ends with the given suffix.
//
bool endsWith_(const std::string& s, const std::string& suffix) {
    size_t n1 = s.length();
    size_t n2 = suffix.length();
    if (n1 < n2) {
        return false;
    }
    else {
        for (size_t i = 0; i < n2; ++i) {
            if (s[n1 - i] != suffix[n2 - i]) {
                return false;
            }
        }
        return true;
    }
}

// Removes in-place the given suffix to the string s.
// The behavior is undefined if s doesn't end with the
// given suffix.
//
void removeSuffix_(std::string& s, const std::string& suffix) {
    size_t n2 = suffix.length();
    for (size_t i = 0; i < n2; ++i) {
        s.pop_back();
    }
}

void dumpObjectTree_(const Object* obj, std::string& out, std::string& prefix) {
    static const std::string I = "│ ";
    static const std::string T = "├ ";
    static const std::string L = "└ ";
    static const std::string W = "  ";

    out += prefix;
    out += core::toAddressString(obj);
    out += " ";
    out += obj->className();
    if (obj->isAlive()) {
        out += " [";
        out += core::toString(obj->refCount());
        out += "]";
    }
    else {
        out += " (";
        out += core::toString(obj->refCount());
        out += ")";
    }
    out += "\n";

    // Print children
    if (obj->firstChildObject()) {

        // Modify previous indent
        if (endsWith_(prefix, T)) {
            // ├ this          ├ this
            // ├  └ child  =>  │  └ child
            removeSuffix_(prefix, T);
            prefix += I;
        }
        else if (endsWith_(prefix, L)) {
            // └ this          └ this
            // └  └ child  =>     └ child
            removeSuffix_(prefix, L);
            prefix += W;
        }

        for (Object* child = obj->firstChildObject(); //
             child != nullptr;                        //
             child = child->nextSiblingObject()) {

            // Indent
            if (child->nextSiblingObject()) {
                prefix += T;
            }
            else {
                prefix += L;
            }

            // Dump child. If child itself has children, then the above
            // indent will be modified from T to I and from L to W.
            dumpObjectTree_(child, out, prefix);

            // De-indent
            if (endsWith_(prefix, T)) {
                removeSuffix_(prefix, T);
            }
            else if (endsWith_(prefix, L)) {
                removeSuffix_(prefix, L);
            }
            else if (endsWith_(prefix, I)) {
                removeSuffix_(prefix, I);
            }
            else if (endsWith_(prefix, W)) {
                removeSuffix_(prefix, W);
            }
        }
    }
}

} // namespace

bool Object::isDescendantObjectOf(const Object* other) const {
    // Fast path when other is nullptr
    if (!other) {
        return false;
    }

    // Go up from this node to the root; check if we encounter the other node
    const Object* obj = this;
    while (obj != nullptr) {
        if (obj == other) {
            return true;
        }
        obj = obj->parentObject();
    }
    return false;
}

void Object::dumpObjectTree() const {
    std::string out;
    std::string prefix;
    dumpObjectTree_(this, out, prefix);
    core::print(out);
}

void Object::onDestroyed() {
    printDebugInfo_(this, "destroyed");
}

void Object::onChildAdded(Object*, bool) {
}

void Object::onChildRemoved(Object*) {
}

void Object::onChildAdded_(Object* child, bool wasOnlyReordered) {
    if (!wasOnlyReordered) {
        ++numChildObjects_;
        setBranchSizeDirty_();
    }
    onChildAdded(child, wasOnlyReordered);
}

void Object::onChildRemoved_(Object* child) {
    --numChildObjects_;
    setBranchSizeDirty_();
    onChildRemoved(child);
}

Object::Object([[maybe_unused]] CreateKey key)
    : signalHub_(this) {

    printDebugInfo_(this, "constructed");
}

Object::~Object() {
    printDebugInfo_(this, "destructed");
}

void Object::destroyObject_() {
    destroyObjectImpl_();
}

void Object::destroyAllChildObjects_() {
    Object* x = this->firstChildObject_;
    while (x) {
        Object* next = x->nextSiblingObject_;
        x->destroyObject_();
        x = next;
    }
}

void Object::destroyChildObject_(Object* child) {
    if (!child || child->parentObject_ != this) {
        throw core::NotAChildError(child, this);
    }
    else {
        child->destroyObject_();
    }
}

void Object::appendChildObject_(Object* child) {
    insertChildObject_(nullptr, child);
}

void Object::prependChildObject_(Object* child) {
    insertChildObject_(firstChildObject(), child);
}

void Object::insertChildObject_(Object* nextSibling, Object* child) {
    // Check that child is non-nullptr
    if (!child) {
        throw core::NullError();
    }

    // Ensure that nextSibling is either null or a child of `this`
    if (nextSibling && nextSibling->parentObject_ != this) {
        throw core::NotAChildError(nextSibling, this);
    }

    // Check trivial case
    if (nextSibling == child) {
        return;
    }

    // If parent is different we have to detach and reattach.
    Object* oldParent = child->parentObject();
    bool sameParent = oldParent == this;

    // If exactly the same location, fast return.
    if (sameParent && (nextSibling == child->nextSiblingObject_)) {
        return;
    }

    ObjectPtr p;
    if (!sameParent) {

        // Detach child from current parent if any. Note that it would be safe to
        // unconditionally do `ObjectPtr p = child->removeObjectFromParent_();`, but
        // it would cause unnecessary incref and decref in the common case where the
        // given child doesn't have a parent yet.
        //
        if (oldParent) {
            ObjectPtr q = child->removeObjectFromParent_();
            p = std::move(q);
        }

        // Set parent-child relationships
        child->parentObject_ = this;

        // XXX onChildReordered instead of onChildAdded ? See below.
    }
    else {
        // Detach from current siblings
        if (child->previousSiblingObject_) {
            child->previousSiblingObject_->nextSiblingObject_ = child->nextSiblingObject_;
        }
        else {
            firstChildObject_ = child->nextSiblingObject_;
        }

        if (child->nextSiblingObject_) {
            child->nextSiblingObject_->previousSiblingObject_ =
                child->previousSiblingObject_;
        }
        else {
            lastChildObject_ = child->previousSiblingObject_;
        }
    }

    // Insert between target siblings
    child->nextSiblingObject_ = nextSibling;
    if (nextSibling) {
        child->previousSiblingObject_ = nextSibling->previousSiblingObject_;
        nextSibling->previousSiblingObject_ = child;
    }
    else {
        child->previousSiblingObject_ = lastChildObject_;
        lastChildObject_ = child;
    }
    if (child->previousSiblingObject_) {
        child->previousSiblingObject_->nextSiblingObject_ = child;
    }
    else {
        firstChildObject_ = child;
    }

    // XXX May be better to have both general and fine grained events.
    // e.g.: onChildrenChanged, onChildReordered, onChildAdded, onChildRemoved.
    onChildAdded_(child, sameParent);
}

ObjectPtr Object::removeChildObject_(Object* child) {
    if (!child || child->parentObject_ != this) {
        throw core::NotAChildError(child, this);
    }
    else {
        return child->removeObjectFromParent_();
    }
}

void Object::appendObjectToParent_(Object* parent) {
    if (parent) {
        parent->appendChildObject_(this);
    }
    else {
        removeObjectFromParent_();
    }
}

void Object::prependObjectToParent_(Object* parent) {
    if (parent) {
        parent->prependChildObject_(this);
    }
    else {
        removeObjectFromParent_();
    }
}

void Object::insertObjectToParent_(Object* parent, Object* nextSibling) {
    if (parent) {
        parent->insertChildObject_(nextSibling, this);
    }
    else {
        removeObjectFromParent_();
    }
}

ObjectPtr Object::removeObjectFromParent_() {
    Object* parent = parentObject_;
    ObjectPtr res(this);
    if (parent) {
        if (previousSiblingObject_) {
            previousSiblingObject_->nextSiblingObject_ = nextSiblingObject_;
        }
        else {
            parent->firstChildObject_ = nextSiblingObject_;
        }
        if (nextSiblingObject_) {
            nextSiblingObject_->previousSiblingObject_ = previousSiblingObject_;
        }
        else {
            parent->lastChildObject_ = previousSiblingObject_;
        }
        previousSiblingObject_ = nullptr;
        nextSiblingObject_ = nullptr;
        parentObject_ = nullptr;
        parent->onChildRemoved_(this);
    }
    return res;
}

Int Object::branchSize() const {
    updateBranchSize_();
    return branchSize_;
}

void Object::destroyObjectImpl_() {

    // Prevent infinite loops and inform others that we're about to be destroyed
    if (hasReachedStage(ObjectStage::AboutToBeDestroyed)) {
        return;
    }
    stage_ = ObjectStage::AboutToBeDestroyed;
    aboutToBeDestroyed().emit(this);

    // Recursively destroy children
    while (firstChildObject_) {
        firstChildObject_->destroyObjectImpl_();
    }
    ObjectPtr p = removeObjectFromParent_(); // refCount() becomes >= 1
    stage_ = ObjectStage::ChildrenDestroyed; // isAlive() becomes false

    // Disconnect slots, then call onDestroyed(), then disconnect signals. This
    // order ensures that the object won't receive signals in the middle of its
    // onDestroyed() call, while allowing it to still emit signals in there.
    detail::SignalHub::disconnectSlots(this);
    onDestroyed();
    detail::SignalHub::disconnectSignals(this);
    stage_ = ObjectStage::Destroyed;

    // Note 1: at the end of this scope, the ObjectPtr p is destructed, which
    // causes a call to decref(), calling `delete obj` if the refCount becomes
    // zero.

    // Note 2: in C++, constness is only meant to indicate immutability while
    // the object is alive. For example, destructors are allowed to call
    // non-const methods, and calling `delete p` is allowed even if p is a
    // `const T*`. This is why we decided that onDestroyed() should be a
    // non-const method (to be able to call non-const methods, like destructors
    // are allowed to do), and this is why we decided that this
    // destroyObjectImpl_() method should be non-const too. In turn, this is
    // why we need to perform a const-cast in the implementation of decref().
    // Interesting discussion here:
    //
    // https://stackoverflow.com/questions/755196/deleting-a-pointer-to-const-t-const
}

void Object::setBranchSizeDirty_() {
    isBranchSizeDirty_ = true;
    Object* obj = parentObject_;
    while (obj && !obj->isBranchSizeDirty_) {
        obj->isBranchSizeDirty_ = true;
        obj = obj->parentObject_;
    }
}

void Object::updateBranchSize_() const {
    if (isBranchSizeDirty_) {
        Object* self = const_cast<Object*>(this);
        self->branchSize_ = 1;

        Object* c = self->firstChildObject_;
        if (c) {
            bool firstVisit = true;
            while (c != self) {
                if (firstVisit && c->isBranchSizeDirty_) {

                    // pre-update of c
                    c->branchSize_ = 1;
                    // ...

                    if (c->firstChildObject_) {
                        c = c->firstChildObject_;
                        continue;
                    }
                }

                // can reach here only if:
                // - c is a leaf, or
                // - c was not initially dirty, or
                // - c is being visited for the second time

                // post-update of c
                c->isBranchSizeDirty_ = false;
                // ...

                // accumulate step of parent value
                c->parentObject_->branchSize_ += c->branchSize_;

                if (c->nextSiblingObject_) {
                    c = c->nextSiblingObject_;
                    firstVisit = true;
                }
                else {
                    c = c->parentObject_;
                    firstVisit = false;
                }
            }
        }

        self->isBranchSizeDirty_ = false;
    }
}

namespace detail {

// Note: implementing this create() function in the *.cpp file rather than
// inline in the *.h file is preferable, because hiding the "new" in the
// implementation avoids the following clang-tidy false positive warning:
//
//   warning: Potential leak of memory pointed to by field '_obj'
//   [clang-analyzer-cplusplus.NewDeleteLeaks]
//
ConstructibleTestObjectPtr ConstructibleTestObject::create() {
    return createObject<ConstructibleTestObject>();
}

void SignalTestObject::connectToOtherNoArgs(SignalTestObject* other) const {
    signalNoArgs().connect(other->slotNoArgs());
}

} // namespace detail

} // namespace vgc::core
