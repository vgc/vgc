// Copyright 2024 The VGC Developers
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

#ifndef VGC_TOOLS_ORDER_H
#define VGC_TOOLS_ORDER_H

#include <vgc/tools/api.h>
#include <vgc/ui/command.h>
#include <vgc/ui/module.h>

VGC_DECLARE_OBJECT(vgc::canvas, DocumentManager);

namespace vgc::tools {

namespace commands {

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(bringForward)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(sendBackward)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(bringToFront)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(sendToBack)

} // namespace commands

VGC_DECLARE_OBJECT(OrderModule);

/// \class vgc::tools::OrderModule
/// \brief A module to import all order-related actions (bring forward, etc.).
///
class VGC_TOOLS_API OrderModule : public ui::Module {
private:
    VGC_OBJECT(OrderModule, ui::Module)

protected:
    OrderModule(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `OrderModule` module.
    ///
    static OrderModulePtr create(const ui::ModuleContext& context);

private:
    canvas::DocumentManagerWeakPtr documentManager_;

    void onBringForward_();
    VGC_SLOT(onBringForward_)

    void onSendBackward_();
    VGC_SLOT(onSendBackward_)

    void onBringToFront_();
    VGC_SLOT(onBringToFront_)

    void onSendToBack_();
    VGC_SLOT(onSendToBack_)
};

} // namespace vgc::tools

#endif // VGC_TOOLS_ORDER_H
