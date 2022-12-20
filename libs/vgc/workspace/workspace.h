// Copyright 2022 The VGC Developers
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

#ifndef VGC_WORKSPACE_WORKSPACE_H
#define VGC_WORKSPACE_WORKSPACE_H

#include <map>
#include <unordered_map>

#include <vgc/core/animtime.h>
#include <vgc/core/flags.h>
#include <vgc/core/id.h>
#include <vgc/core/object.h>
#include <vgc/dom/document.h>
#include <vgc/graphics/engine.h>
#include <vgc/topology/vac.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/element.h>

namespace vgc::workspace {

VGC_DECLARE_OBJECT(Workspace);

namespace detail {

struct VacElementLists;

} // namespace detail

/// \class vgc::workspace::Workspace
/// \brief Represents a workspace to manage, manipulate and visit a vector graphics scene.
///
class VGC_WORKSPACE_API Workspace : public core::Object {
private:
    VGC_OBJECT(Workspace, core::Object)

    Workspace(dom::DocumentPtr document);

public:
    static WorkspacePtr create(dom::DocumentPtr document);

    /*RenderObject*
    getOrCreateRenderObject(core::Id id, graphics::Engine* engine, core::AnimTime t);*/

    //RenderObject* getOrCreateRenderObject(
    //    Renderable* renderable,
    //    graphics::Engine* engine,
    //    core::AnimTime t);

    dom::Document* document() const {
        return document_.get();
    }

    const topology::Vac* vac() const {
        return vac_.get();
    }

    core::History* history() const {
        return document_.get()->history();
    }

    Element* vgcElement() const {
        return vgcElement_;
    }

    void sync();

    VGC_SIGNAL(changed);

    VGC_SLOT(onDocumentDiff, onDocumentDiff_)
    VGC_SLOT(onVacDiff, onVacDiff_)

protected:
    /*RenderObjectMap* getRenderObjectMap(graphics::Engine* engine, core::AnimTime t) {
        return renderObjectByIdByTimeByEngine_[engine][t].get();
    }*/

    void onDocumentDiff_(const dom::Diff& diff);
    void onVacDiff_(const topology::VacDiff& diff);

    void updateTreeAndVacFromDom_();
    void updateTreeAndDomFromVac_();

    void updateTreeFromDom_();

    void rebuildTreeFromDom_();
    void rebuildVacFromTree_();

    void
    fillVacElementListsUsingTagName(Element* root, detail::VacElementLists& ce) const;

private:
    std::unordered_map<core::Id, std::unique_ptr<Element>> elements_;
    Element* vgcElement_;

    Element* createElement(dom::Element* domElement, Element* parent);

    Element* getRefAttribute(
        dom::Element* domElement,
        core::StringId name,
        core::StringId tagNameFilter = {}) const;

    dom::DocumentPtr document_;
    topology::VacPtr vac_;
    bool isDomBeingUpdated_ = false;
    bool isVacBeingUpdated_ = false;
    core::Id lastSyncedDomVersionId_ = {};
    Int64 lastSyncedVacVersion_ = -1;
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_WORKSPACE_H
