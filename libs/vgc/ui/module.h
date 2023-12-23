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

#include <functional> // function
#include <mutex>
#include <thread>
#include <unordered_map>

#include <vgc/core/object.h>
#include <vgc/core/objectarray.h>
#include <vgc/ui/action.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

using ActionPtrArrayView = core::ObjPtrArrayView<Action>;

VGC_DECLARE_OBJECT(Module);
VGC_DECLARE_OBJECT(ModuleManager);

class ModuleContext;

/// Type trait for isModule<T>.
///
template<typename T, typename SFINAE = void>
struct IsModule : std::false_type {};

template<typename T>
struct IsModule<T, core::Requires<std::is_base_of_v<Module, T>>> : std::true_type {};

/// Checks whether T is vgc::core::Module or derives from it.
///
template<typename T>
inline constexpr bool isModule = IsModule<T>::value;

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
        ModuleSharedPtr module;

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

/// \class vgc::ui::ModuleContext
/// \brief Provides access to various application objects that modules may need.
///
class VGC_UI_API ModuleContext {
private:
    ModuleContext(ModuleManager* moduleManager);

public:
    ModuleContext(const ModuleContext&) = delete;
    ModuleContext& operator=(const ModuleContext&) = delete;

    /// Returns the module manager related to this module context.
    ///
    ModuleManagerWeakPtr moduleManager() const {
        return moduleManager_;
    }

    /// Retrieves the given `TModule` module, or creates it if there is no such
    /// module yet.
    ///
    /// Returns a null pointer if the module couldn't be imported, for example if
    /// the `moduleManager()` has already been destroyed.
    ///
    template<typename TModule>
    core::ObjPtr<TModule> importModule() const {
        if (auto moduleManager = moduleManager_.lock()) {
            return moduleManager->importModule<TModule>();
        }
        else {
            return {};
        }
    }

private:
    friend ModuleManager; // For accessing the constructor
    ModuleManagerWeakPtr moduleManager_;
};

VGC_DECLARE_OBJECT(Module);
VGC_DECLARE_OBJECT(ModuleManager);

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
/// Each `Module` subclass must have a `Module::create(const ModuleContext&
/// context)` static function. This function is called by the `ModuleManager`
/// whenever the module should be instantiated.
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
/// In the case of this example, we can create a `CurrentColor` module that
/// extends any application with the concept of "current color":
///
/// ```cpp
/// class CurrentColor : public Module {
/// private:
///     VGC_OBJECT(CurrentColor, Module)
///
///     CurrentColor(CreateKey key);
///
/// public:
///     CurrentColorPtr create();
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
///         CurrentColorPtr module = context.importModule<CurrentColor>();
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
/// We can see that by using such module, `ColorPanel` and `MyApplication` do
/// not statically depend on each other anymore, the only static dependencies
/// are the following:
///
/// - `MyApplication` depends on: `Application`, `CurrentColor`.
/// - `ColorPanel` depends on: `Application`, `Panel`, `CurrentColor`.
///
/// In fact, `MyApplication` doesn't even have to depend on `CurrentColor`.
/// Such dependency is only required if we want to provide a convenient method
/// such as `MyApplication::currentColor()`, but in practice such function
/// isn't needed: any panel or module that requires the current color can
/// instead query the `CurrentColor` module directly, further reducing
/// coupling.
///
class VGC_UI_API Module : public core::Object {
private:
    VGC_OBJECT(Module, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    Module(CreateKey, const ModuleContext& context);

public:
    /// Creates a `Module()`.
    ///
    static ModulePtr create(const ModuleContext& context);

    /// Returns the list of actions of this module.
    ///
    ActionPtrArrayView actions() const {
        return actions_;
    }

    /// Creates an action of type `TAction`, adds it to this module, and
    /// returns the action.
    ///
    template<typename TAction, typename... Args>
    TAction* createAction(Args&&... args) {
        core::ObjPtr<TAction> action = TAction::create(std::forward<Args>(args)...);
        TAction* action_ = action.get();
        addAction(action_);
        return action_;
    }

    /// Adds the given `action` to the list of actions of this module.
    ///
    /// The module takes ownership of the action.
    ///
    /// If the action previously had a parent object, it is first removed
    /// from this parent.
    ///
    void addAction(Action* action);

    /// Removes the given `action` from the list of actions of this module.
    ///
    void removeAction(Action* action);

    /// Clears the list of actions of this module.
    ///
    void clearActions();

    /// Creates an action of type `ActionType::Trigger`, adds it to this
    /// module, and returns the action.
    ///
    template<typename... Args>
    Action* createTriggerAction(Args&&... args) {
        return createAction<Action>(std::forward<Args>(args)...);
    }

    /// This signal is emitted whenever an action is added to this module.
    ///
    VGC_SIGNAL(actionAdded, (Action*, addedAction))

    /// This signal is emitted whenever an action is removed from this module.
    ///
    VGC_SIGNAL(actionRemoved, (Action*, removedAction))

    /// Returns the module manager that manages this module.
    ///
    ModuleManagerWeakPtr moduleManager() const {
        return moduleManager_;
    }

    /// Retrieves the given `TModule` module, or creates it if there is no such
    /// module yet.
    ///
    /// Returns a null pointer if the module couldn't be imported, for example if
    /// the `moduleManager()` has already been destroyed.
    ///
    template<typename TModule>
    core::ObjPtr<TModule> importModule() const {
        if (auto moduleManager = moduleManager_.lock()) {
            return moduleManager->importModule<TModule>();
        }
        else {
            return {};
        }
    }

private:
    // Note: in the Widget class, actions were implemented as child objects of
    // the widget (technically, grand-child objects, since there is the
    // intermediate ActionList object). This was done to guarantee that they
    // only had one owner, and enabled our smart pointer system to keep-alive
    // the owner as long as we have a pointer to the child (but this is not
    // done anymore, as it was a performance problem).
    //
    // Therefore, we now consider that it is in general more flexible and works
    // better with Python to simply use shared ownership and keep them
    // independent root objects that do not assume that they have a parent.
    // Therefore, this is what we do in the Module class (which was implemented
    // after the Widget class). We store actions as independent root objects.
    // In the future, we'll update Widget to do the same.
    //
    core::Array<ActionPtr> actions_;

    ModuleManagerWeakPtr moduleManager_;
};

} // namespace vgc::ui

#endif // VGC_UI_MODULE_H
