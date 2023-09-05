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

#ifndef VGC_TOOLS_SCULPT_H
#define VGC_TOOLS_SCULPT_H

#include <vgc/canvas/canvastool.h>
#include <vgc/core/array.h>
#include <vgc/core/id.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/tools/api.h>
#include <vgc/ui/action.h>
#include <vgc/vacomplex/keyedgedata.h>

namespace vgc::tools {

VGC_DECLARE_OBJECT(Sculpt);

/// \class vgc::tools::Sculpt
/// \brief A CanvasTool that implements sculpting strokes.
///
class VGC_TOOLS_API Sculpt : public canvas::CanvasTool {
private:
    VGC_OBJECT(Sculpt, canvas::CanvasTool)

protected:
    /// This is an implementation details.
    /// Please use `Sculpt::create()` instead.
    ///
    Sculpt(CreateKey);

public:
    /// Creates a `Sculpt`.
    ///
    static SculptPtr create();

    core::Id candidateId() const {
        return candidateId_;
    }

    void setActionCircleCenter(const geometry::Vec2d& circleCenter) {
        actionCircleCenter_ = circleCenter;
        if (isActionCircleEnabled_) {
            requestRepaint();
        }
    }

    void setActionCircleEnabled(bool enabled) {
        if (enabled != isActionCircleEnabled_) {
            isActionCircleEnabled_ = enabled;
            requestRepaint();
        }
    }

    void dirtyActionCircle() {
        requestRepaint();
    }

protected:
    // Reimplementation of CanvasTool virtual methods
    ui::WidgetPtr createOptionsWidget() const override;

    // Reimplementation of Widget virtual methods
    void onMouseHover(ui::MouseHoverEvent* event) override;
    void onMouseLeave() override;

    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

private:
    core::Id candidateId_ = -1;
    geometry::Vec2d candidateClosestPoint_ = {};
    geometry::Vec2f cursorPosition_ = {};
    geometry::Vec2d actionCircleCenter_ = {};
    bool isActionCircleEnabled_ = false;

    graphics::GeometryViewPtr circleGeometry_;
    graphics::GeometryViewPtr pointGeometry_;
};

} // namespace vgc::tools

#endif // VGC_TOOLS_SCULPT_H
