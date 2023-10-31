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

#include <vgc/ui/modulemanager.h>

#include <vgc/ui/modulecontext.h>

namespace vgc::ui {

namespace detail {

// Note: make_unique doesn't have a Deleter template argument, so we do the
// `new` explicitly. This is symmetric with doing the `delete` explicitly.
//
// See: https://stackoverflow.com/questions/21788066/using-stdmake-unique-with-a-custom-deleter
//
ModuleContextPtr ModuleContextAccess::createContext(ModuleManager* moduleManager) {
    return ModuleContextPtr(new ModuleContext(moduleManager));
}

void ModuleContextAccess::destroyContext(ModuleContext* p) {
    delete p;
}

} // namespace detail

ModuleManager::ModuleManager(CreateKey key)
    : Object(key) {
}

ModuleManagerPtr ModuleManager::create() {
    return core::createObject<ModuleManager>();
}

} // namespace vgc::ui
