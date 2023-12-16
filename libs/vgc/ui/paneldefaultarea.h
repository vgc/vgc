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

#ifndef VGC_UI_PANELDEFAULTAREA_H
#define VGC_UI_PANELDEFAULTAREA_H

#include <vgc/core/enum.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

/// \enum vgc::ui::PanelDefaultArea
/// \brief Specifies where a Panel should be opened by default.
///
enum PanelDefaultArea {
    Left,
    Right
};

VGC_UI_API
VGC_DECLARE_ENUM(PanelDefaultArea)

} // namespace vgc::ui

#endif // VGC_UI_PANELDEFAULTAREA_H
