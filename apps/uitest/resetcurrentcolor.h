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

#ifndef VGC_APPS_UITEST_RESETCURRENTCOLOR_H
#define VGC_APPS_UITEST_RESETCURRENTCOLOR_H

#include <vgc/ui/module.h>

namespace vgc::tools {

VGC_DECLARE_OBJECT(CurrentColor);

} // namespace vgc::tools

namespace vgc::apps::uitest {

VGC_DECLARE_OBJECT(ResetCurrentColor);

// Tests the ability to define widget-less window actions in a module
//
class ResetCurrentColor : public ui::Module {
private:
    VGC_OBJECT(ResetCurrentColor, ui::Module)

protected:
    ResetCurrentColor(CreateKey, const ui::ModuleContext& context);

public:
    static ResetCurrentColorPtr create(const ui::ModuleContext& context);

private:
    tools::CurrentColorPtr currentColor_;

    void onActionTriggered_();
    VGC_SLOT(onActionTriggered_)
};

} // namespace vgc::apps::uitest

#endif // VGC_APPS_UITEST_RESETCURRENTCOLOR_H
