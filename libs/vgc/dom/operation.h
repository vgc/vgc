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

#ifndef VGC_DOM_DIFF_H
#define VGC_DOM_DIFF_H

#include <vgc/core/array.h>
#include <vgc/core/object.h>
#include <vgc/dom/api.h>
#include <vgc/dom/value.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Element);

/*

enum AtomicOperationKind {
    SetAttr,
    AddAttr,
    RemAttr,
    ModAttr,
    AddElem,
    RemElem,
}

AtomicOperation {
    AtomicOperationKind kind;
    Element* who;
    Value savedValue; // past value for undo, future value for redo
}

Diff {
    vector<AtomicOperation> ops;
}

UndoGroup {
    Int historyPos;
    str name;
}

Document extension {
    rotatingStack<Diff> history;
    Int historyPos;
    Document removedElemHolder;
}

History View {
    Array<str> names; // don't need more if it listens to document events.
}

*/

enum class AtomicOpKind {
    SetAttribute,
    AddAttribute,
    RemoveAttribute,
    ModifyAttribute,
    AddElement,
    RemoveElement
};

struct AtomicOperation {
    AtomicOpKind kind;
    Element* who;
    Value savedValue; // past value for undo, future value for redo
};

enum class RevertOpKind {
    Undo,
    Redo
};

struct Operation {
    std::string name;
    core::Array<AtomicOperation> ops;
    RevertOpKind revertOperationKind;
};

} // namespace vgc::dom

#endif // VGC_DOM_DIFF_H
