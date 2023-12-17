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

#include <vgc/canvas/canvasmanager.h>

#include <vgc/canvas/canvas.h>
#include <vgc/canvas/documentmanager.h>

namespace vgc::canvas {

CanvasManager::CanvasManager(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    documentManager_ = context.importModule<DocumentManager>();
    if (auto documentManager = documentManager_.lock()) {
        documentManager->currentWorkspaceChanged().connect(
            onCurrentWorkspaceChanged_Slot());
    }
}

CanvasManagerPtr CanvasManager::create(const ui::ModuleContext& context) {
    return core::createObject<CanvasManager>(context);
}

namespace {

void setCanvasWorkspace(
    CanvasWeakPtr& canvas_,
    DocumentManagerWeakPtr& documentManager_) {

    if (auto canvas = canvas_.lock()) {
        if (auto documentManager = documentManager_.lock()) {
            canvas->setWorkspace(documentManager->currentWorkspace());
        }
    }
}

} // namespace

void CanvasManager::setActiveCanvas(CanvasWeakPtr canvas) {
    if (activeCanvas_ == canvas) {
        return;
    }
    activeCanvas_ = canvas;
    setCanvasWorkspace(activeCanvas_, documentManager_);
}

void CanvasManager::onCurrentWorkspaceChanged_() {
    setCanvasWorkspace(activeCanvas_, documentManager_);
}

} // namespace vgc::canvas
