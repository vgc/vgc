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

#ifndef VGC_UI_COLUMNLAYOUT_H
#define VGC_UI_COLUMNLAYOUT_H

#include <vgc/ui/flexlayout.h>

namespace vgc {
namespace ui {

VGC_DECLARE_OBJECT(ColumnLayout);

/// \class vgc::ui::ColumnLayout
/// \brief A vertical column of widgets.
///
/// This class is a convenient subclass of FlexLayout that initializes it with
/// a Column direction and NoWrap wrapping behavior.
///
/// ```cpp
/// auto column = ColumnLayout::create();
/// column->createChild<Button>();
/// column->createChild<Button>();
/// ```
///
/// Note that it is allowed to change the direction and wrapping behavior after
/// creating a ColumnLayout, although for better readability we advise to
/// directly create a FlexLayout instead of a ColumnLayout if you intend to do
/// so.
///
class VGC_UI_API ColumnLayout : public FlexLayout {
private:
    VGC_OBJECT(ColumnLayout)

protected:
    /// This is an implementation details. Please use
    /// ColumnLayout::create() instead.
    ///
    ColumnLayout();

public:
    /// Creates a ColumnLayout.
    ///
    static ColumnLayoutPtr create();
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_COLUMNLAYOUT_H
