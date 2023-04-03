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
#include <vgc/core/span.h>
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
/// This implementation is still preliminary and the API may change significantly.
///
class VGC_WORKSPACE_API Workspace : public core::Object {
private:
    VGC_OBJECT(Workspace, core::Object)

    Workspace(dom::DocumentPtr document);

    void onDestroyed() override;

public:
    using ElementCreator = std::unique_ptr<Element> (*)(Workspace*);

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

    Element* find(core::Id elementId) const {
        auto it = elements_.find(elementId);
        return it != elements_.end() ? it->second.get() : nullptr;
    }

    Element* find(const dom::Element* element) const {
        return element ? find(element->internalId()) : nullptr;
    }

    VacElement* findVacElement(core::Id nodeId) const {
        auto it = elementByVacInternalId_.find(nodeId);
        return it != elementByVacInternalId_.end() ? it->second : nullptr;
    }

    VacElement* findVacElement(const vacomplex::Node* node) const {
        return node ? findVacElement(node->id()) : nullptr;
    }

    void sync();
    void rebuildFromDom();
    //void rebuildFromVac();

    bool updateElementFromDom(Element* element);

    Element* getElementFromPathAttribute(
        dom::Element* domElement,
        core::StringId attrName,
        core::StringId tagNameFilter = {}) const;

    void visitDepthFirstPreOrder(const std::function<void(Element*, Int)>& preOrderFn);
    void visitDepthFirst(
        const std::function<bool(Element*, Int)>& preOrderFn,
        const std::function<void(Element*, Int)>& postOrderFn);

    VGC_SIGNAL(changed);

    // updates from dom are deferred
    VGC_SLOT(onDocumentDiff, onDocumentDiff_);
    // updates from vac are direct (after each atomic operation)
    VGC_SLOT(onVacNodeAboutToBeRemoved, onVacNodeAboutToBeRemoved_);
    VGC_SLOT(onVacNodeCreated, onVacNodeCreated_);
    VGC_SLOT(onVacNodeMoved, onVacNodeMoved_);
    VGC_SLOT(onVacCellModified, onVacCellModified_);

private:
    friend VacElement;
    static std::unordered_map<core::StringId, ElementCreator>& elementCreators_();

    // This is the <vgc> element (the root).
    VacElement* vgcElement_;
    std::unordered_map<core::Id, std::unique_ptr<Element>> elements_;
    std::unordered_map<core::Id, VacElement*> elementByVacInternalId_;
    core::Array<Element*> elementsWithError_;
    core::Array<Element*> elementsToUpdateFromDom_;

    void removeElement_(Element* element);
    void removeElement_(core::Id id);
    void clearElements_();

    void
    fillVacElementListsUsingTagName_(Element* root, detail::VacElementLists& ce) const;

    dom::DocumentPtr document_;
    vacomplex::ComplexPtr vac_;

    void debugPrintTree_();

    // ---------------
    // VAC -> DOM Sync

    bool isCreatingDomElementsFromVac_ = false;

    void preUpdateDomFromVac_();
    void postUpdateDomFromVac_();

    void rebuildDomFromTree_();

    void onVacNodeAboutToBeRemoved_(vacomplex::Node* node);
    void onVacNodeCreated_(
        vacomplex::Node* node,
        core::Span<vacomplex::Node*> operationSourceNodes);
    void onVacNodeMoved_(vacomplex::Node* node);
    void onVacCellModified_(vacomplex::Cell* cell);

    // ---------------
    // DOM -> VAC Sync

    bool isCreatingVacElementsFromDom_ = false;
    bool shouldSkipNextDomDiff_ = false;
    core::Id lastSyncedDomVersionId_ = {};

    Element* createAppendElementFromDom_(dom::Element* domElement, Element* parent);

    void rebuildTreeFromDom_();
    void rebuildVacFromTree_();

    void updateVacHierarchyFromTree_();
    void updateTreeAndVacFromDom_(const dom::Diff& diff);

    void onDocumentDiff_(const dom::Diff& diff);
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_WORKSPACE_H
