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

#ifndef VGC_UI_PAINTBUCKETTOOL_H
#define VGC_UI_PAINTBUCKETTOOL_H

#include <vgc/ui/api.h>
#include <vgc/ui/canvastool.h>
#include <vgc/ui/cursor.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(PaintBucketTool);

/// \class vgc::ui::SketchTool
/// \brief A CanvasTool that implements sketching strokes.
///
class VGC_UI_API PaintBucketTool : public CanvasTool {
private:
    VGC_OBJECT(PaintBucketTool, CanvasTool)

protected:
    /// This is an implementation details.
    /// Please use `PaintBucketTool::create()` instead.
    ///
    PaintBucketTool();

public:
    /// Creates a `SketchTool`.
    ///
    static PaintBucketToolPtr create();

protected:
    // Reimplementation of CanvasTool virtual methods
    ui::WidgetPtr createOptionsWidget() const override;

    // Reimplementation of Widget virtual methods
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

private:
    core::Array<topology::KeyCycle> paintCandidateCycles_;
    core::FloatArray paintCandidatePendingTriangles_;
    graphics::GeometryViewPtr paintCandidateFillGeometry_;
    bool hasPaintCandidate_ = false;

    /*
    // temporary method
    void onColorChanged_(const core::Color& color);
    // temporary method
    void clearPaintCandidate_();
*/
};

} // namespace vgc::ui

#endif // VGC_UI_PAINTBUCKETTOOL_H
