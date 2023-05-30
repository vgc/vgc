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

#ifndef VGC_TOOLS_PAINTBUCKET_H
#define VGC_TOOLS_PAINTBUCKET_H

#include <vgc/canvas/canvastool.h>
#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/graphics/geometryview.h>
#include <vgc/tools/api.h>
#include <vgc/vacomplex/keycycle.h>

namespace vgc::tools {

VGC_DECLARE_OBJECT(PaintBucket);

/// \class vgc::tools::PaintBucketTool
/// \brief Implementation of the "paint bucket" tool, creating faces on click.
///
class VGC_TOOLS_API PaintBucket : public canvas::CanvasTool {
private:
    VGC_OBJECT(PaintBucket, canvas::CanvasTool)

protected:
    /// This is an implementation details.
    /// Please use `PaintBucketTool::create()` instead.
    ///
    PaintBucket();

public:
    /// Creates a `SketchTool`.
    ///
    static PaintBucketPtr create();

    /// Returns the color of the tool.
    ///
    core::Color color() const {
        return color_;
    }

    /// Sets the color of the tool.
    ///
    void setColor(const core::Color& color);

protected:
    // Reimplementation of CanvasTool virtual methods
    ui::WidgetPtr createOptionsWidget() const override;

    // Reimplementation of Widget virtual methods
    void onMouseHover(ui::MouseHoverEvent* event) override;
    bool onMouseMove(ui::MouseMoveEvent* event) override;
    bool onMousePress(ui::MousePressEvent* event) override;
    bool onMouseRelease(ui::MouseReleaseEvent* event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

private:
    // Tool color
    core::Color color_;

    // Face candidate
    core::Array<vacomplex::KeyCycle> faceCandidateCycles_;
    bool hasFaceCandidate_() const {
        return !faceCandidateCycles_.isEmpty();
    }
    void clearFaceCandidate_();
    void updateFaceCandidate_(const geometry::Vec2d& worldPosition);
    VGC_SLOT(clearFaceCandidateSlot_, clearFaceCandidate_)

    // Graphics Data
    bool isFaceCandidateGraphicsDirty_ = true;
    core::FloatArray faceCandidateTriangles_;
    graphics::GeometryViewPtr faceCandidateFillGeometry_;
};

} // namespace vgc::tools

#endif // VGC_TOOLS_PAINTBUCKET_H
