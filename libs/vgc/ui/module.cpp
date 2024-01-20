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

#include <vgc/ui/module.h>

#include <vgc/ui/widget.h>

namespace vgc::ui {

ModuleContext::ModuleContext(ModuleManager* moduleManager)
    : moduleManager_(moduleManager) {
}

ModuleManager::ModuleManager(CreateKey key)
    : Object(key) {
}

ModuleManagerPtr ModuleManager::create() {
    return core::createObject<ModuleManager>();
}

// Inserts (ModuleType, ModulePtr()) in the map, or retrieve the existing
// ModulePtr if the ModuleType was already in the map.
//
// Note that it's important to release this map mutex before calling
// TModule::create(), otherwise there would be a deadlock when calling
// importModule() within the constructor of another module.
//
// This is why we use a mutex for insertion into the map, and then a separate
// mutex for each module to be created.
//
ModuleManager::GetOrInsertInfo_ ModuleManager::getOrInsert_(core::ObjectType objectType) {
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto [it, inserted] = modules_.try_emplace(objectType);
    return {it->second, inserted};
}

ModulePtr ModuleManager::importModule_(core::ObjectType key, ModuleFactory factory) {

    // Retrieves an existing value from the map, or insert a default-constructed value
    //
    GetOrInsertInfo_ info = getOrInsert_(key);

    if (info.inserted) {
        // If a value was just inserted in the map, this means that this thread
        // is responsible for creating the module now. At this moment,
        // value.module is still a null ModulePtr.

        VGC_ASSERT(info.value.creationThread == std::this_thread::get_id());

        // Make other threads asking for the same module wait until this thread
        // has finished creating the module.
        //
        std::lock_guard<std::mutex> lock(info.value.creationMutex);

        // Construct the module by calling TModule::create().
        //
        // If this recursively calls importModule() for the same module,
        // this means there is a cyclic dependency (handled below).
        //
        ModuleContext context(this);
        ModulePtr res = factory(context);

        // Change the value stored in the map from null to the now-constructed
        // non-null module.
        //
        info.value.module = res;

        // Inform the world of the creation of the module and return it.
        //
        moduleCreated().emit(res.get());
        return res;
    }
    else {

        if (info.value.creationThread == std::this_thread::get_id()) {

            // No need for a mutex lock before reading info.value.module, since
            // we are the only thread potentially mutating it.

            if (!info.value.module) {

                // If this thread is the same thread as the one responsible for
                // creating the module (see above), but value.module is still a
                // null pointer, this means the module is still under
                // construction, which means this is a recursive call to
                // importModule_() in the same call stack, for the same
                // module, which means there is a cyclic dependency.
                //
                throw core::LogicError(core::format(
                    "Cyclic dependencies between modules involving {}.", key));
            }
            else {
                return info.value.module;
            }
        }
        else {
            // Wait for the creation thread to finish creating the module
            std::lock_guard<std::mutex> lock(info.value.creationMutex);

            // Return the now-created module
            return info.value.module;
        }
    }
}

Module::Module(CreateKey key, const ModuleContext& context)
    : Object(key)
    , moduleManager_(context.moduleManager()) {
}

ModulePtr Module::create(const ModuleContext& context) {
    return core::createObject<Module>(context);
}

// XXX: do we need to override onDestroyed() and call removeAction()?

// Note: removing the action ensures that the action is now a root object, and
// therefore that our ObjPtr stored in actions_ keeps it alive (unless a parent
// is later re-added... for now we assume it doesn't happen and later things
// should be more robust with true shared ownership).
//
void Module::addAction(Action* action) {
    ActionPtr actionPtr = action;
    if (Widget* widget = action->owningWidget()) {
        widget->removeAction(action);
    }
    if (!actions_.contains(action)) {
        actions_.append(std::move(actionPtr));
        actionAdded().emit(action);
    }
}

void Module::removeAction(Action* action) {
    ActionPtr actionPtr = action;
    Int i = actions_.removeAll(actionPtr);
    if (i > 0) {
        actionRemoved().emit(action);
    }
}

void Module::clearActions() {
    while (!actions_.isEmpty()) {
        removeAction(actions_.last().get());
    }
}

ModuleActionCreator::ModuleActionCreator(ui::ModuleWeakPtr module)
    : module_(std::move(module)) {
}

// Note: this must be defined in the .cpp file because we cannot
// copy an Obj[Weak]Ptr<T> with only a forward declaration of T.
//
// Note 2: On Windows, defining this function in the .cpp file isn't even
// enough: MSVC requires the full definition of T even to just accept the
// function declaration in the .h file. So for now, we include <menu.h> from
// <module.h>.
//
// XXX: Should we use some static_cast<void*> in weakIncref() so that
// ObjWeakPtr<T> can be copyable with just a forward declaration? If we do
// this, would it be still possible to prevent people to use ObjWeakPtr<T> even
// when T does not derive from Object?
//
ui::MenuWeakPtr ModuleActionCreator::menu() const {
    return menu_;
}

void ModuleActionCreator::setMenu(ui::MenuWeakPtr menu) {
    menu_ = std::move(menu);
}

void ModuleActionCreator::addSeparator() {
    if (auto menu = menu_.lock()) {
        menu->addSeparator();
    }
}

ui::Action* ModuleActionCreator::createActionAndAddToMenu_(core::StringId commandName) {
    ui::Action* action = createAction_(commandName);
    if (action) {
        addToMenu_(action);
    }
    return action;
}

ui::Action* ModuleActionCreator::createAction_(core::StringId commandName) {
    if (auto module = module_.lock()) {
        return module->createTriggerAction(commandName);
    }
    else {
        return nullptr;
    }
}

void ModuleActionCreator::addToMenu_(ui::Action* action) {
    if (auto menu = menu_.lock()) {
        menu->addItem(action);
    }
}

} // namespace vgc::ui
