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

#include <functional> // function
#include <memory>     // unique_ptr
#include <mutex>
#include <thread>
#include <unordered_map>

#include <vgc/core/object.h>
#include <vgc/ui/api.h>
#include <vgc/ui/module.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ModuleManager);

class ModuleContext;

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
    /// Note: it is not allowed to have cyclic dependencies between modules'
    /// contructor, such as:
    ///
    /// - Module1's constructor calling importModule<Module2>(), and
    /// - Module2's constructor calling importModule<Module1>()
    ///
    /// Indeed, modules are essentially global objects, and it makes no sense
    /// for global objects to have their construction mutually depend
    /// on each other.
    ///
    /// A workaround can be to defer calling `importModule()` until after
    /// a given module is constructed, via a 2-step initialization or
    /// lazy-initialization approach.
    ///
    template<typename TModule>
    core::ObjPtr<TModule> importModule() {
        checkIsModule_<TModule>();
        core::ObjectType key = TModule::staticObjectType();
        auto factory = [](const ModuleContext& context) -> ModulePtr {
            return TModule::create(context);
        };
        ModulePtr module = importModule_(key, factory);
        return core::static_pointer_cast<TModule>(module);
    }

    /// This signal is emitted when a module is created.
    ///
    VGC_SIGNAL(moduleCreated, (Module*, module))

private:
    struct Value_ {
        ModulePtr module;

        // Enable having two threads concurrently asking for the same module,
        // while detecting cyclic dependencies between module construction
        std::thread::id creationThread;
        std::mutex creationMutex;

        Value_()
            : creationThread(std::this_thread::get_id()) {
        }
    };
    std::mutex mapMutex_; // Prevent inserting concurrently in the map
    std::unordered_map<core::ObjectType, Value_> modules_;

    struct GetOrInsertInfo_ {
        Value_& value;
        bool inserted;
    };
    GetOrInsertInfo_ getOrInsert_(core::ObjectType objectType);

    template<typename TModule>
    static void checkIsModule_() {
        static_assert(isModule<TModule>, "TModule must inherit from vgc::ui::Module");
    }

    // non-templated version to avoid bloating the .h
    using ModuleFactory = std::function<ModulePtr(const ModuleContext& context)>;
    ModulePtr importModule_(core::ObjectType key, ModuleFactory factory);
};

} // namespace vgc::ui

#endif // VGC_UI_MODULEMANAGER_H
