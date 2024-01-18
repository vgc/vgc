// Copyright 2023 The VGC Developers
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

#include <vgc/canvas/documentmanager.h>

#include <vgc/workspace/workspace.h>

namespace vgc::canvas {

DocumentManager::DocumentManager(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {
}

DocumentManagerPtr DocumentManager::create(const ui::ModuleContext& context) {
    return core::createObject<DocumentManager>(context);
}

void DocumentManager::setCurrentWorkspace(workspace::WorkspaceSharedPtr workspace) {
    if (currentWorkspace_ == workspace) {
        return;
    }
    currentWorkspace_ = workspace;
    currentWorkspaceChanged().emit(workspace);
}

dom::DocumentWeakPtr DocumentManager::currentDocument() const {
    if (auto workspace = currentWorkspace().lock()) {
        return workspace->document();
    }
    else {
        return nullptr;
    }
}

} // namespace vgc::canvas
