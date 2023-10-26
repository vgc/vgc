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

#include <vgc/core/object.h>
#include <vgc/core/typeid.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ModuleManager);

/// \class vgc::ui::ModuleManager
/// \brief Organize application functionality into modules.
///
/// This class makes it possible to dynamically create and retrieve `Module`
/// instances, ensuring that at most one `Module` of each `ModuleId` is
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

    // TODO

private:
    using ModuleId = core::TypeId;
};

} // namespace vgc::ui

#endif // VGC_UI_MODULEMANAGER_H
