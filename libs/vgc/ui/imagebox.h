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

#ifndef VGC_UI_IMAGEBOX_H
#define VGC_UI_IMAGEBOX_H

#include <QImage>

#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ImageBox);

/// \class vgc::ui::ImageBox
/// \brief An image widget.
///
class VGC_UI_API ImageBox : public Widget {
private:
    VGC_OBJECT(ImageBox, Widget)

protected:
    /// This is an implementation details. Please use
    /// ImageBox::create(text) instead.
    ///
    ImageBox(std::string_view relativePath);

public:
    /// Creates a ImageBox with the given text.
    ///
    static ImageBoxPtr create(std::string_view relativePath);

    // Reimplementation of Widget virtual methods
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    graphics::GeometryViewPtr quad_;
    graphics::ImagePtr image_;
    graphics::ImageViewPtr imageView_;
    graphics::SamplerStatePtr samplerState_;

    QImage qimage_ = {};
    bool reloadImg_ = true;
    bool reload_ = true;
};

} // namespace vgc::ui

#endif // VGC_UI_IMAGEBOX_H
