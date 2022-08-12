// Copyright 2021 The VGC Developers
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

#include <vgc/ui/flex.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Column);

/// \class vgc::ui::Column
/// \brief A vertical column of widgets.
///
/// This class is a convenient subclass of Flex that initializes it with
/// a Column direction and NoWrap wrapping behavior.
///
/// ```cpp
/// auto column = Column::create();
/// column->createChild<Button>();
/// column->createChild<Button>();
/// ```
///
/// Note that it is allowed to change the direction and wrapping behavior after
/// creating a Column, although for better readability we advise to
/// directly create a Flex instead of a Column if you intend to do
/// so.
///
class VGC_UI_API Column : public Flex {
private:
    VGC_OBJECT(Column, Flex)

protected:
    /// This is an implementation details. Please use
    /// Column::create() instead.
    ///
    Column();

public:
    /// Creates a Column.
    ///
    static ColumnPtr create();
};

} // namespace vgc::ui

#endif // VGC_UI_COLUMNLAYOUT_H
