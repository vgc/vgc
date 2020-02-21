// Copyright 2020 The VGC Developers
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

#ifndef VGC_UI_COLORPALETTE_H
#define VGC_UI_COLORPALETTE_H

#include <vgc/ui/widget.h>

namespace vgc {
namespace ui {

VGC_CORE_DECLARE_PTRS(ColorPalette);

/// \class vgc::ui::ColorPalette
/// \brief Allow users to select a color.
///
class VGC_UI_API ColorPalette : public Widget
{  
    VGC_CORE_OBJECT(ColorPalette)

public:
    /// This is an implementation details. Please use
    /// ColorPalette::create() instead.
    ///
    ColorPalette(const ConstructorKey&);

public:
    /// Creates a ColorPalette.
    ///
    static ColorPaletteSharedPtr create();

    /// Returns the current color.
    ///
    core::Color currentColor() const {
        return currentColor_;
    }

    /// Sets the current color.
    ///
    void setCurrentColor(const core::Color& color) {
        currentColor_ = color;
    }

    // reimpl
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMousePress(MouseEvent* event) override;

private:
    core::Color currentColor_;
    Int trianglesId_;
    float oldWidth_;
    float oldHeight_;
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_COLORPALETTE_H
