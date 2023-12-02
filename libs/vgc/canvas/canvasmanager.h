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

#ifndef VGC_CANVAS_CANVASMANAGER_H
#define VGC_CANVAS_CANVASMANAGER_H

#include <vgc/canvas/api.h>
#include <vgc/dom/document.h>
#include <vgc/ui/module.h>
#include <vgc/workspace/workspace.h>

namespace vgc::canvas {

VGC_DECLARE_OBJECT(Canvas);
VGC_DECLARE_OBJECT(CanvasManager);
VGC_DECLARE_OBJECT(DocumentManager);

/// \class vgc::tools::CanvasManager
/// \brief A module to specify the active canvas.
///
/// Currently, the design is that there is one active canvas, and that it
/// automatically tracks any document changes, so that it always displays the
/// current document. This design may change in the future.
///
class VGC_CANVAS_API CanvasManager : public ui::Module {
private:
    VGC_OBJECT(CanvasManager, ui::Module)

protected:
    CanvasManager(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `CanvasManager` module.
    ///
    static CanvasManagerPtr create(const ui::ModuleContext& context);

    /// Returns the active canvas.
    ///
    CanvasWeakPtr activeCanvas() const {
        return activeCanvas_;
    }

    /// Sets the active canvas.
    ///
    void setActiveCanvas(CanvasWeakPtr canvas);

    /// This signal is emitted whenever the active canvas changed.
    ///
    VGC_SIGNAL(activeCanvasChanged, (CanvasWeakPtr, canvas))

private:
    DocumentManagerWeakPtr documentManager_;
    CanvasWeakPtr activeCanvas_;

    void onCurrentWorkspaceChanged_();
    VGC_SLOT(onCurrentWorkspaceChanged_);
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_CANVASMANAGER_H
