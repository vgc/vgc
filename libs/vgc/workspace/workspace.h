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
/// \brief High-level interface to manipulate and render a vector graphics document.
///
/// A vector graphics document can be described as a `dom::Document`, providing
/// a simple low-level representation which is very useful for serialization,
/// undo/redo, or low-level editing in a DOM editor.
///
/// However, the DOM representation by itself does not provide any means to
/// render the scene, nor convenient methods to edit the underlying topological
/// objects described in the DOM. For such use cases, you can use a
/// `Workspace`.
///
/// A workspace takes as input a given `dom::Document` and creates two other
/// parallel tree-like structures which are all kept synchronized:
///
/// 1. A topological complex (`vacomplex::Complex`), representing the explicit
///    or implicit vertices, edges, and faces described in the document.
///
/// 2. A workspace tree, unifying both the topological complex and the DOM document.
///
/// By visiting the workspace tree, you can iterate not only on all the
/// elements in the DOM (including those not in the topological complex, e.g.,
/// text), but also on all the elements in the topological complex (including
/// those not in the DOM, e.g., implicit vertices, edges, and faces).
///
/// The elements in the workspace tree (`workspace::Element`) store pointers to
/// their corresponding `dom::Element` (if any), and their corresponding
/// `vacomplex::Node` (if any).
///
/// The elements in the workspace tree also store all the graphics resources
/// required to render the vector graphics document. These graphics resources
/// are computed from the base geometry provided by `vacomplex::Complex`, on
/// top of which is applied styling and compositing. For example, the workspace
/// is responsible for the computation of edge joins.
///
class VGC_WORKSPACE_API Workspace : public core::Object {
private:
    VGC_OBJECT(Workspace, core::Object)

    Workspace(dom::DocumentPtr document);

    void onDestroyed() override;

public:
    static WorkspacePtr create(dom::DocumentPtr document);

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

    // Factory methods to create a workspace element from a DOM tag name.
    //
    // An ElementCreator is a function taking a `Workspace*` as input and
    // returning an std::unique_ptr<Element>. This might be publicized later
    // for extensibility, but should then be adapted to allow interoperability
    // with Python.
    //
    using ElementCreator = std::unique_ptr<Element> (*)(Workspace*);

    static std::unordered_map<core::StringId, ElementCreator>& elementCreators_();

    static void
    registerElementClass_(core::StringId tagName, ElementCreator elementCreator);

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
