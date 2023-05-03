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

#ifndef VGC_UI_SELECTTOOL_H
#define VGC_UI_SELECTTOOL_H

#include <vgc/core/array.h>
#include <vgc/core/id.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/ui/api.h>
#include <vgc/ui/canvastool.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(SelectTool);

/// \class vgc::ui::SelectTool
/// \brief A CanvasTool that implements selecting strokes.
///
class VGC_UI_API SelectTool : public CanvasTool {
private:
    VGC_OBJECT(SelectTool, CanvasTool)

protected:
    /// This is an implementation details.
    /// Please use `SketchTool::create()` instead.
    ///
    SelectTool();

public:
    /// Creates a `SelectTool`.
    ///
    static SelectToolPtr create();

protected:
    // Reimplementation of Widget virtual methods
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;

private:
    enum class SelectionMode {
        Single,
        Add,
        Remove,
        Toggle
    };
    enum class DragAction {
        Select,
        TranslateSelection,
        TranslateCandidate,
    };

    core::Array<SelectionCandidate> candidates_;
    core::Array<core::Id> selectionAtPress_;
    geometry::Vec2f cursorPositionAtPress_;
    geometry::Vec2f cursorPositionAtDragStart_;
    geometry::Vec2f cursorPositionAtLastTranslate_;
    Int timeAtPress_ = 0;
    bool isInAction_ = false;
    bool isDragging_ = false;
    DragAction dragAction_ = {};
    SelectionMode selectionMode_ = {};
    bool isAlternativeMode_ = false;
    core::Id lastSelectedId_ = -1;
    core::Id lastDeselectedId_ = -1;

    void resetActionState_();
};

} // namespace vgc::ui

#endif // VGC_UI_SELECTTOOL_H
