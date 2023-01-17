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

    void onDestroyed() override;

public:
    using ElementCreator = std::unique_ptr<Element> (*)(Workspace*, dom::Element*);

    static WorkspacePtr create(dom::DocumentPtr document);

    static void
    registerElementClass(core::StringId tagName, ElementCreator elementCreator);

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

    Element* find(core::Id id) const {
        auto it = elements_.find(id);
        return it != elements_.end() ? it->second.get() : nullptr;
    }

    Element* find(const dom::Element* e) const {
        return e ? find(e->internalId()) : nullptr;
    }

    void sync();
    void rebuildFromDom();
    //void rebuildFromVac();

    bool updateElementFromDom(Element* element);

    Element* getElementFromPathAttribute(
        dom::Element* domElement,
        core::StringId attrName,
        core::StringId tagNameFilter = {}) const;

    void visitDfsPreOrder(std::function<void(Element*, Int)> preOrderFn);
    void visitDfs(
        std::function<bool(Element*, Int)> preOrderFn,
        std::function<void(Element*, Int)> postOrderFn);

    VGC_SIGNAL(changed);

    VGC_SLOT(onDocumentDiff, onDocumentDiff_);
    VGC_SLOT(onVacDiff, onVacDiff_);
    VGC_SLOT(onVacNodeAboutToBeRemoved, onVacNodeAboutToBeRemoved_);

protected:
    virtual void onDocumentDiff_(const dom::Diff& diff);
    virtual void onVacDiff_(const topology::VacDiff& diff);

private:
    static std::unordered_map<core::StringId, ElementCreator>& elementCreators();

    std::unordered_map<core::Id, std::unique_ptr<Element>> elements_;
    core::Array<Element*> elementsWithDependencyErrors_;
    core::Array<Element*> elementsOutOfSync_;

    VacElement* vgcElement_;

    Element* createAppendElement_(dom::Element* domElement, Element* parent);

    void onVacNodeAboutToBeRemoved_(topology::VacNode* node);

    void
    fillVacElementListsUsingTagName_(Element* root, detail::VacElementLists& ce) const;

    dom::DocumentPtr document_;
    topology::VacPtr vac_;
    bool isDomBeingUpdated_ = false;
    bool isVacBeingUpdated_ = false;
    core::Id lastSyncedDomVersionId_ = {};
    Int64 lastSyncedVacVersion_ = -1;

    void rebuildTreeFromDom_();
    void rebuildDomFromTree_();

    void rebuildTreeFromVac_();
    void rebuildVacFromTree_();

    //void rebuildVacGroup_(Element* e);
    //void rebuildKeyVertex_(Element* e);
    //bool rebuildKeyEdge_(Element* e, bool force = false);

    //bool haveKeyEdgeBoundaryPathsChanged_(Element* e);

    void updateVacHierarchyFromTree_();

    void updateTreeAndVacFromDom_(const dom::Diff& diff);
    void updateTreeAndDomFromVac_(const topology::VacDiff& diff);

    void debugPrintTree_();
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_WORKSPACE_H
