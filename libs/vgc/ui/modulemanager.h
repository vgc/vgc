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
#include <vgc/ui/api.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(ModuleManager);

/// \class vgc::ui::ModuleManager
/// \brief Organize application functionality into modules
///
/// This class helps organize application functionality by separating them into
/// modules that can then be dynamically created and queried at runtime.
///
/// In essence, a module is similar to a singleton, but the difference its that
/// it is tied to a `ModuleManager`: each module manager ensures that it
/// contains at most one instance of any module. In most use cases, there is
/// only one `ModuleManager` instance (owned by the `Application`), so the
/// modules are effectively singletons. But in other cases, for example
/// unit-testing, it can be useful to have different `ModuleManager` instances,
/// and therefore there can be several instances of the same module, each
/// belonging to a different manager.
///
/// Related concept: [Dependency injection](https://en.wikipedia.org/wiki/Dependency_injection).
///
/// Example:
///
/// Let's assume that we want to implement a `ColorPanel`, which requires to
/// make signal-slot connections between the widgets of the panel and the
/// "current color" backend of the application, such as
/// `MyApplication::currentColor()`.
///
/// Without using modules, who should be responsible to make these connections?
///
/// - If `MyApplication` makes the connections, then this would typically mean
/// that `myapplication.cpp` should include `colorpanel.h`. This unfortunately
/// does not scale as the program grows, and wouldn't work for plugins.
///
/// - If `ColorPanel` makes the connections, then this would typically mean that
/// `colorpanel.cpp` should include `myapplication.h`. But what if we want to
/// use the same implementation of `ColorPanel` across different applications?
///
/// Basically, we do not want `MyApplication` and `ColorPanel` to know about
/// each other.
///
/// Using a `Module` is a solution to this problem. We can create a
/// `CurrentColorModule` that encapsulates the concept of "current color",
/// independently from any application:
///
/// ```cpp
/// class CurrentColorModule : public Module {
/// private:
///     VGC_OBJECT(CurrentColorModule, Module)
///
///     CurrentColorModule(CreateKey key);
///
/// public:
///     CurrentColorModulePtr create();
///
///     Color color() const;
///     void setColor(const Color& color);
///     VGC_SLOT(setColor)
///     VGC_SIGNAL(colorChanged)
///
///     // ...
/// };
/// ```
///
/// Then, we can implement the signal-slot connections within a constructor of `ColorPanel`,
/// by querying the module from the `PanelContext` (which internally stores a pointer to
/// the application's `ModuleManager`):
///
///
/// ```cpp
/// class ColorPanel : public Panel {
/// private:
///     VGC_OBJECT(ColorPanel, Panel)
///
///     ColorPanel(CreateKey key, const PanelContext& context)
///         : Panel(key) {
///
///         CurrentColorModulePtr module = context.getOrCreateModule<CurrentColorModule>();
///         module->colorChanged().connect(this->setColorSlot());
///         this->colorChanged().connect(module->setColorSlot());
///     }
///
/// public:
///     ColorPanelPtr create(const PanelContext& context);
///
///     Color color() const;
///     void setColor(const Color& color);
///     VGC_SLOT(setColor)
///     VGC_SIGNAL(colorChanged)
///
///     // ...
/// };
/// ```
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
};

} // namespace vgc::ui

#endif // VGC_UI_MODULEMANAGER_H
