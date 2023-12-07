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

#ifndef VGC_UI_INSPECTOR_H
#define VGC_UI_INSPECTOR_H

#include <vgc/ui/api.h>
#include <vgc/ui/module.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Inspector);

/// \class vgc::ui::Inspector
/// \brief A module to inspect widget style and computed sizes.
///
/// For now, this simply creates an action (default shortcut: Ctrl+Shift+I)
/// that prints to the console information about the hovered widget of the
/// active window.
///
/// In the future, this might be made more interactive, similar to dev tools in
/// web browsers.
///
class VGC_UI_API Inspector : public ui::Module {
private:
    VGC_OBJECT(Inspector, ui::Module)

protected:
    Inspector(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `Inspector` module.
    ///
    static InspectorSharedPtr create(const ui::ModuleContext& context);

private:
    void onInspect_();
    VGC_SLOT(onInspect_)
};

} // namespace vgc::ui

#endif // VGC_UI_INSPECTOR_H
