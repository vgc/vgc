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

#ifndef VGC_UI_MODULEMANAGER_H
#define VGC_UI_MODULEMANAGER_H

#include <mutex>
#include <unordered_map>

#include <vgc/core/object.h>
#include <vgc/ui/api.h>
#include <vgc/ui/module.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ModuleManager);

/// \class vgc::ui::ModuleManager
/// \brief Organize application functionality into modules.
///
/// This class makes it possible to dynamically create and retrieve `Module`
/// instances, ensuring that at most one `Module` of each module type is
/// instanciated by the manager.
///
/// Therefore, the concept of module is similar to the concept of
/// [singleton](https://en.wikipedia.org/wiki/Singleton_pattern), except that
/// instead of having a unique instance for the whole program, there is a
/// unique instance per `ModuleManager`.
///
/// In most use cases, there is only one `ModuleManager` instance, which is
/// owned by the `Application`, so each module is effectively a singleton.
///
/// However, in some cases, for example for unit-testing, it can be useful to
/// have multiple `ModuleManager` instances, and therefore there can be
/// multiple instances of the same module, each instance belonging to a
/// different `ModuleManager`.
///
/// See the documentation of `Module` for more information.
///
class VGC_UI_API ModuleManager : public core::Object {
private:
    VGC_OBJECT(ModuleManager, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

    ModuleManager(CreateKey);

public:
    /// Creates a `ModuleManager()`.
    ///
    static ModuleManagerPtr create();

    /// Retrieves the given `TModule` module, or creates it if there is no such
    /// module yet.
    ///
    template<typename TModule>
    core::ObjPtr<TModule> getOrCreateModule() {

        using TModulePtr = core::ObjPtr<TModule>;
        checkIsModule_<TModule>();

        std::lock_guard<std::mutex> lock(mutex_);

        auto [it, inserted] = modules_.try_emplace(TModule::objectType());
        if (inserted) {
            TModulePtr res = TModule::create();
            *it = res;
            return res;
        }
        else {
            return core::static_pointer_cast<TModulePtr>(*it);
        }
    }

private:
    std::mutex mutex_;
    std::unordered_map<core::ObjectType, ModulePtr> modules_;

    template<typename TModule>
    void checkIsModule_() {
        static_assert(isModule<TModule>, "TModule must inherit from vgc::ui::Module");
    }
};

} // namespace vgc::ui

#endif // VGC_UI_MODULEMANAGER_H
