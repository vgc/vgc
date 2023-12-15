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
#include <vgc/ui/panelarea.h>
#include <vgc/ui/tabbar.h>
#include <vgc/ui/tabbody.h>

namespace vgc::ui {

PanelManager::PanelManager(CreateKey key, ModuleManager* moduleManager)
    : Object(key)
    , moduleManager_(moduleManager) {
}

PanelManagerPtr PanelManager::create(ModuleManager* moduleManager) {
    return core::createObject<PanelManager>(moduleManager);
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
    if (parent) {
        // Preferred size for the PanelArea containing the panel, including
        // both the TabBar and the TabBody.
        // TODO: use preferredWidthForHeight() and vice-versa?
        PanelArea* grandParent = parent->parentArea();
        if (grandParent && grandParent->isSplit()) {
            Int mainDir = grandParent->type() == PanelAreaType::HorizontalSplit ? 0 : 1;
            geometry::Vec2f preferredSize = parent->preferredSize();
            parent->setSplitSize(preferredSize[mainDir]);
        }
    }
    info.instances_.append(panel);
    panel->aboutToBeDestroyed().connect(onPanelInstanceAboutToBeDestroyed_Slot());
    instanceToId_[panel] = id;
    return panel;
}

core::Array<Panel*> PanelManager::instances(PanelTypeId id) const {
    return getInfo_(infos_, id).instances_;
}

void PanelManager::registerPanelType_(
    std::string_view id,
    std::string_view label,
    detail::PanelFactory&& factory) {

    infos_.try_emplace(PanelTypeId(id), label, std::move(factory));
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

Widget* PanelManager::preCreatePanel_(PanelArea* parentArea) {
    if (parentArea->type() != PanelAreaType::Tabs) {
        VGC_WARNING(
            LogVgcUi, "Cannot create a Panel in a PanelArea which is not of type Tabs.");
        return nullptr;
    }
    TabBody* parentWidget = parentArea->tabBody();
    return parentWidget;
}

void PanelManager::postCreatePanel_(PanelArea* parentArea, Panel* panel) {
    parentArea->tabBar()->addTab(panel->title());
}

} // namespace vgc::ui
