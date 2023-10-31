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

#ifndef VGC_TOOLS_CURRENTCOLOR_H
#define VGC_TOOLS_CURRENTCOLOR_H

#include <vgc/core/color.h>
#include <vgc/core/colors.h>
#include <vgc/tools/api.h>
#include <vgc/ui/module.h>

namespace vgc::tools {

VGC_DECLARE_OBJECT(CurrentColor);

/// \class vgc::tools::CurrentColor
/// \brief A module to specify a current color.
///
class VGC_TOOLS_API CurrentColor : public ui::Module {
private:
    VGC_OBJECT(CurrentColor, ui::Module)

protected:
    CurrentColor(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `CurrentColor` module.
    ///
    static CurrentColorPtr create(const ui::ModuleContext& context);

    /// Returns the current color.
    ///
    core::Color color() const {
        return color_;
    }

    /// Sets the current color.
    ///
    void setColor(const core::Color& color);
    VGC_SLOT(setColor)

    /// This signal is emitted whenever the current color changed.
    ///
    VGC_SIGNAL(colorChanged, (const core::Color&, color))

private:
    core::Color color_ = core::colors::black;
};

} // namespace vgc::tools

#endif // VGC_TOOLS_CURRENTCOLOR_H
