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

#ifndef VGC_UI_PANELCONTEXT_H
#define VGC_UI_PANELCONTEXT_H

#include <vgc/ui/api.h>
#include <vgc/ui/modulemanager.h>

namespace vgc::ui {

class PanelManager;

/// \class vgc::ui::PanelContext
/// \brief Provides access to various application objects that panels may need.
///
class VGC_UI_API PanelContext {
private:
    PanelContext(ModuleManager* moduleManager);

public:
    PanelContext(const PanelContext&) = delete;
    PanelContext& operator=(const PanelContext&) = delete;

    /// Returns the module manager of the application.
    ///
    ModuleManager* moduleManager() const {
        return moduleManager_;
    }

    /// Retrieves the given `TModule` module, or creates it if there is no such
    /// module yet.
    ///
    template<typename TModule>
    core::ObjPtr<TModule> importModule() const {
        return moduleManager()->importModule<TModule>();
    }

private:
    friend PanelManager;
    ModuleManager* moduleManager_ = nullptr; // PanelContext outlives ModuleManager
};

} // namespace vgc::ui

#endif // VGC_UI_PANELCONTEXT_H
