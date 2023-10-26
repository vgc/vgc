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

#ifndef VGC_UI_MODULE_H
#define VGC_UI_MODULE_H

#include <vgc/core/object.h>
#include <vgc/core/typeid.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

using ModuleId = core::TypeId;

VGC_DECLARE_OBJECT(Module);

/// \class vgc::ui::Module
/// \brief Encapsulates a piece of application functionality.
///
/// This class helps organize pieces of functionality by separating them into
/// modules that can then be dynamically created and queried at runtime. This
/// allows to reduce coupling by avoiding excessive static dependencies. It is
/// related to the concept of [dependency
/// injection](https://en.wikipedia.org/wiki/Dependency_injection).
///
/// The class `ModuleManager` is responsible for owning the created modules,
/// and ensuring that it creates at most one `Module` instance of a given
/// module type.
///
/// Example:
///
/// Let's assume that we want to implement a `ColorPanel`, which requires to
/// make signal-slot connections between the widgets of the panel and the
/// "current color" backend of an application, for example
/// `MyApplication::currentColor()`.
///
/// Without using modules, who should be responsible to make these connections?
///
/// - If `MyApplication` makes the connections, then this would typically mean
/// that `myapplication.cpp` should include `colorpanel.h`. This unfortunately
/// does not scale as the program grows, and wouldn't work for plugins.
///
/// - If `ColorPanel` makes the connections, then this would typically mean
/// that `colorpanel.cpp` should include `myapplication.h`. But what if we want
/// to use the same implementation of `ColorPanel` across different
/// applications, for example `MyOtherApplication`?
///
/// Basically, we do not want `MyApplication` and `ColorPanel` to know about
/// each other, as this would increase coupling and decrease reusability.
///
/// A possible solution to this problem could be to have an intermediate base
/// class `ColorApplication` that `MyApplication` inherits from. This would
/// allow `ColorPanel` to only depend on `ColorApplication` instead of
/// `MyApplication`, which is a bit better. However, this doesn't scale either
/// for multiple functionalities, especially if we want to avoid multiple
/// inheritance.
///
/// Using a `Module` is a more flexible solution to this problem: it avoids
/// static dependencies by creating and querying modules dynamically. Each
/// module is essentially defining an additional interface to `Application`,
/// but without having to change the `Application` class itself.
///
/// In the case of this example, we can create a `CurrentColorModule` that
/// extends any application with the concept of "current color":
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
///     VGC_SIGNAL(colorChanged, (const Color&, color))
///
///     // ...
/// };
/// ```
///
/// Then, we can implement the signal-slot connections within a constructor of
/// `ColorPanel`, by creating or querying the module from the `PanelContext`,
/// which knows about the application's `ModuleManager`:
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
///     VGC_SIGNAL(colorChanged, (const Color&, color))
///
///     // ...
/// };
/// ```
///
/// We can this that by using such module, `ColorPanel` and `MyApplication` do not statically depend
/// on each other anymore, the only static dependencies are the following:
///
/// - `MyApplication` depends on: `Application`, `CurrentColorModule`.
/// - `ColorPanel` depends on: `Application`, `Panel`, `CurrentColorModule`.
///
/// In fact, `MyApplication` doesn't even have to depend on
/// `CurrentColorModule`. Such dependency is only required if we want to
/// provide a convenient method such as `MyApplication::currentColor()`, but in
/// practice such function isn't needed: any panel or module that requires the
/// current color can instead query the `CurrentColorModule` directly, further
/// reducing coupling.
///
class VGC_UI_API Module : public core::Object {
private:
    VGC_OBJECT(Module, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

    Module(CreateKey);

public:
    /// Creates a `Module()`.
    ///
    static ModulePtr create();
};

/// Type trait for isModule<T>.
///
template<typename T, typename SFINAE = void>
struct IsModule : std::false_type {};

template<>
struct IsModule<Module, void> : std::true_type {};

template<typename T>
struct IsModule<T, core::Requires<std::is_base_of_v<Module, T>>> : std::true_type {};

/// Checks whether T is vgc::core::Module or derives from it.
///
template<typename T>
inline constexpr bool isModule = IsModule<T>::value;

/// Creates a `Module`. This function is meant to be used in the implementation
/// of `MyModule::create()` static methods
///
template<typename T, typename... Args>
core::ObjPtr<T> createModule(Args&&... args) {
    static_assert(
        isModule<T>,
        "Cannot call createModule<T>(...) because T does not inherit from "
        "vgc::ui::Module.");
    return core::createObject<T>(std::forward<Args>(args)...);

    // Note: wouldn't it be a good idea if this was implemented directly in `Object`?
    // The TypeId passed to the Object constructor could even be part of the CreateKey.
}

} // namespace vgc::ui

#endif // VGC_UI_MODULE_H
