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

#ifndef VGC_TOOLS_TOPOLOGY_H
#define VGC_TOOLS_TOPOLOGY_H

#include <vgc/tools/api.h>
#include <vgc/ui/command.h>
#include <vgc/ui/module.h>

VGC_DECLARE_OBJECT(vgc::canvas, CanvasManager);

namespace vgc::tools {

namespace commands {

// Window commands

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(softDelete)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(hardDelete)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(glue)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(explode)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(simplify)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(cutFaceWithEdge)

// Mouse click commands

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(cutWithVertex)

} // namespace commands

VGC_DECLARE_OBJECT(TopologyModule);

/// \class vgc::tools::TopologyModule
/// \brief A module to import all topology-related actions (glue, etc.).
///
class VGC_TOOLS_API TopologyModule : public ui::Module {
private:
    VGC_OBJECT(TopologyModule, ui::Module)

protected:
    TopologyModule(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `TopologyModule` module.
    ///
    static TopologyModulePtr create(const ui::ModuleContext& context);

private:
    canvas::CanvasManagerWeakPtr canvasManager_;

    void onSoftDelete_();
    VGC_SLOT(onSoftDelete_)

    void onHardDelete_();
    VGC_SLOT(onHardDelete_)

    void onGlue_();
    VGC_SLOT(onGlue_)

    void onExplode_();
    VGC_SLOT(onExplode_)

    void onSimplify_();
    VGC_SLOT(onSimplify_)

    void onCutFaceWithEdge_();
    VGC_SLOT(onCutFaceWithEdge_)
};

} // namespace vgc::tools

#endif // VGC_TOOLS_TOPOLOGY_H
