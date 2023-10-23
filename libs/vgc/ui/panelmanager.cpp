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

#include <vgc/ui/panelmanager.h>

#include <vgc/ui/panel.h>

namespace vgc::ui {

PanelManager::PanelManager(CreateKey key)
    : Object(key) {
}

PanelManagerPtr PanelManager::create() {
    return core::createObject<PanelManager>();
}

PanelTypeId PanelManager::registerPanelType(
    std::string_view id_,
    std::string_view label,
    PanelFactory&& factory) {

    PanelTypeId id(id_);
    infos_.try_emplace(id, label, std::move(factory));
    return id;
}

core::Array<PanelTypeId> PanelManager::registeredPanelTypeIds() const {
    core::Array<PanelTypeId> res;
    for (auto& [id, info] : infos_) {
        std::ignore = info;
        res.append(id);
    }
    return res;
}

bool PanelManager::isRegistered(PanelTypeId id) const {
    return infos_.find(id) != infos_.end();
}

namespace {

core::IndexError noRegisteredPanelError_(PanelTypeId id) {
    throw core::IndexError(core::format("No registered panel with ID `{}`", id));
}

const detail::PanelTypeInfo&
getInfo_(const detail::PanelTypeInfoMap& infos, PanelTypeId id) {
    auto it = infos.find(id);
    if (it != infos.end()) {
        return it->second;
    }
    else {
        throw noRegisteredPanelError_(id);
    }
}

detail::PanelTypeInfo& getInfo_(detail::PanelTypeInfoMap& infos, PanelTypeId id) {
    auto it = infos.find(id);
    if (it != infos.end()) {
        return it->second;
    }
    else {
        throw noRegisteredPanelError_(id);
    }
}

} // namespace

std::string_view PanelManager::label(PanelTypeId id) const {
    return getInfo_(infos_, id).label_;
}

Panel* PanelManager::createPanelInstance(PanelTypeId id, PanelArea* parent) {
    detail::PanelTypeInfo& info = getInfo_(infos_, id);
    Panel* panel = info.factory_(parent);
    info.instances_.append(panel);
    panel->aboutToBeDestroyed().connect(onPanelInstanceAboutToBeDestroyed_Slot());
    instanceToId_[panel] = id;
    return panel;
}

core::Array<Panel*> PanelManager::instances(PanelTypeId id) const {
    return getInfo_(infos_, id).instances_;
}

void PanelManager::onPanelInstanceAboutToBeDestroyed_(Object* object) {
    object->aboutToBeDestroyed().disconnect(onPanelInstanceAboutToBeDestroyed_Slot());
    auto it = instanceToId_.find(object);
    if (it == instanceToId_.end()) {
        VGC_WARNING(LogVgcUi, "Unregistered panel instance about to be destroyed");
        return;
    }
    PanelTypeId id = it->second;
    detail::PanelTypeInfo& info = getInfo_(infos_, id);
    info.instances_.removeAll(static_cast<Panel*>(object));
}

} // namespace vgc::ui