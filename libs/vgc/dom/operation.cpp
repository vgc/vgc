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

void CreateNodeOperation::undo(Document* doc)
{
}

void CreateNodeOperation::redo(Document* doc)
{
}

void RemoveNodeOperation::undo(Document* doc)
{
}

void RemoveNodeOperation::redo(Document* doc)
{
}

void MoveNodeOperation::undo(Document* doc)
{
}

void MoveNodeOperation::redo(Document* doc)
{
}

void CreateAuthoredAttributeOperation::undo(Document* doc)
{
}

void CreateAuthoredAttributeOperation::redo(Document* doc)
{
}

void RemoveAuthoredAttributeOperation::undo(Document* doc)
{
}

void RemoveAuthoredAttributeOperation::redo(Document* doc)
{
}

void ChangeAuthoredAttributeOperation::undo(Document* doc)
{
}

void ChangeAuthoredAttributeOperation::redo(Document* doc)
{
}

namespace {

OperationIndex lastId = 0;

} // namespace

OperationIndex genOperationIndex() {
    // XXX make this thread-safe ?
    return ++lastId;
}

} // namespace vgc::dom
