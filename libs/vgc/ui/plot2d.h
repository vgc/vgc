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

#ifndef VGC_UI_PLOT2D_H
#define VGC_UI_PLOT2D_H

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/richtext.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Plot2d);

/// \class vgc::ui::Plot2d
/// \brief A Plot2d widget.
///
class VGC_UI_API Plot2d : public Widget {
private:
    VGC_OBJECT(Plot2d, Widget)

protected:
    /// This is an implementation details. Please use
    /// Button::create(text) instead.
    ///
    Plot2d();

public:
    /// Creates a Button.
    ///
    static Plot2dPtr create();

    /// Sets the data points.
    ///
    void setData(core::Array<geometry::Vec2f> points);

    void appendDataPoint(geometry::Vec2f point);

    // reimpl
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    core::Array<geometry::Vec2f> points_;
    graphics::GeometryViewPtr bgGeom_;
    graphics::GeometryViewPtr plotGridGeom_;
    graphics::GeometryViewPtr plotGeom_;
    graphics::GeometryViewPtr plotTextGeom_;
    graphics::GeometryViewPtr hintBgGeom_;
    graphics::GeometryViewPtr hintTextGeom_;
    bool dirtyBg_ = false;
    bool dirtyPlot_ = false;
    bool dirtyHint_ = false;
    bool isHovered_ = false;
    graphics::RichTextPtr maxYText_;
    graphics::RichTextPtr minYText_;
    graphics::RichTextPtr minXText_;
    graphics::RichTextPtr maxXText_;
    graphics::RichTextPtr hintText_;

};

} // namespace vgc::ui

#endif // VGC_UI_BUTTON_H
