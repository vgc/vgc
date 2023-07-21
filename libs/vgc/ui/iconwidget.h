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

#ifndef VGC_UI_ICONWIDGET_H
#define VGC_UI_ICONWIDGET_H

#include <vgc/graphics/icon.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(IconWidget);

/// \class vgc::ui::IconWidget
/// \brief An image widget.
///
class VGC_UI_API IconWidget : public Widget {
private:
    VGC_OBJECT(IconWidget, Widget)

protected:
    IconWidget(std::string_view svgPath);

public:
    /// Creates a IconWidget with the given text.
    ///
    static IconWidgetPtr create(std::string_view svgPath);

    // Reimplementation of Widget virtual methods
    void onResize() override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    graphics::IconPtr icon_;

    // Cache computation of icon position/size
    bool isIconGeometryDirty_ = true;
    float iconScale_ = 0;
    geometry::Vec2f iconPosition_;
};

} // namespace vgc::ui

#endif // VGC_UI_ICONWIDGET_H
