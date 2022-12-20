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

#include <functional>

#include <vgc/dom/strings.h>
#include <vgc/topology/operations.h>
#include <vgc/topology/vac.h>
#include <vgc/workspace/logcategories.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

Workspace::Workspace(dom::DocumentPtr document)
    : document_(document) {

    document->changed().connect(onDocumentDiff());

    vac_ = topology::Vac::create();
    vac_->changed().connect(onVacDiff());

    rebuildTreeFromDom_();
    rebuildVacFromTree_();
}

/* static */
WorkspacePtr Workspace::create(dom::DocumentPtr document) {
    return WorkspacePtr(new Workspace(document));
}

//Renderable* Workspace::createRenderable(
//    graphics::Engine* engine,
//    core::AnimTime t,
//    core::Id id) {
//
//    // todo
//}

void Workspace::sync() {

    bool domChanged = document_ && (document_->versionId() != lastSyncedDomVersionId_);
    bool vacChanged = vac_->version() != lastSyncedVacVersion_;

    if (domChanged && vacChanged) {
        VGC_ERROR(
            LogVgcWorkspace,
            "Both DOM and VAC of workspace have been edited since last synchronization. "
            "Rebuilding VAC from DOM.");
        updateTreeFromDom_();
        rebuildVacFromTree_();
        return;
    }
    else if (domChanged) {
        updateTreeAndVacFromDom_();
    }
    else if (vacChanged) {
        updateTreeAndDomFromVac_();
    }
}

void Workspace::onDocumentDiff_(const dom::Diff& /*diff*/) {
    if (isDomBeingUpdated_) {
        // workspace is doing the changes, no need to see the diff.
        return;
    }

    // XXX todo:
    // 1) test for topology change, if none, only update parameters
    // 2) do a true update, identify the impacted cells to rebuild
    //    only the necessary part of the vac
    updateTreeFromDom_();
    rebuildVacFromTree_();
    return;

    //namespace ss = dom::strings;

    // first create everything new
    // processing order:
    //
    //  std::unordered_map<Element*, std::set<core::StringId>> modifiedElements_;
    //  -> invalidate and delete desynced vac parts.
    //  -> add the destroyed cells to a pending rebuild and link list.
    //
    //  core::Array<Node*> createdNodes_;
    //  -> but don't link them yet. add
    //  -> add the destroyed cells to a pending link list.
    //
    //  std::unordered_map<Element*, std::set<core::StringId>> modifiedElements_;
    //  std::set<Node*> reparentedNodes_;
    //  core::Array<Node*> removedNodes_;
    //  std::set<Node*> childrenReorderedNodes_;
    //

    //core::Array<std::unique_ptr<topology::VacCell>> createdCells;

    //for (dom::Node* n : diff.createdNodes()) {
    //    dom::Element* e = dom::Element::cast(n);
    //    if (!e) {
    //        continue;
    //    }
    //    if (e->tagName() == ss::vertex) {
    //        std::unique_ptr<topology::KeyVertex> v =
    //            topology::ops::createUnlinkedKeyVertex(e->internalId(), core::AnimTime());
    //        /*
    //            {"color", core::colors::black},
    //            {"position", geometry::Vec2d()},
    //        */
    //        topology::ops::setKeyVertexPosition(
    //            v.get(), e->getAttribute(ss::position).getVec2d());

    //        // XXX create graphics obj
    //    }
    //    else if (e->tagName() == ss::edge) {
    //        std::unique_ptr<topology::KeyEdge> v =
    //            topology::ops::createUnlinkedKeyEdge(e->internalId(), core::AnimTime());
    //        /*
    //            {"color", core::colors::black},
    //            {"positions", geometry::Vec2dArray()},
    //            {"widths", core::DoubleArray()},
    //            {"startVertex", dom::Path()},
    //            {"endVertex", dom::Path()},
    //        */

    //        // will we deal with scripts here or will dom do it itself ?
    //    }
    //    else if (e->tagName() == ss::layer) {
    //        std::unique_ptr<topology::VacGroup> g =
    //            topology::ops::createUnlinkedVacGroup(e->internalId());
    //    }
    //}

    //for (const auto& me : diff.modifiedElements()) {
    //    dom::Element* e = me.first;
    //    const std::set<core::StringId>& attrs = me.second;
    //    if (!e) {
    //        continue;
    //    }
    //    if (e->tagName() == ss::vertex) {
    //        //
    //    }
    //    else if (e->tagName() == ss::edge) {
    //        // What if startVertex or endVertex changes ?
    //        // It is not really a valid vac operation.
    //        //
    //    }
    //    else if (e->tagName() == ss::layer) {
    //        //
    //    }
    //}

    //std::set<topology::VacCell*> aliveStar;
}

void Workspace::onVacDiff_(const topology::VacDiff& /*diff*/) {
    if (isVacBeingUpdated_) {
        // workspace is doing the changes, no need to see the diff.
        return;
    }

    //
}

void Workspace::updateTreeAndVacFromDom_() {
    if (!document_ || !vac_) {
        return;
    }

    isVacBeingUpdated_ = true;
    // process dom diff
    document_->emitPendingDiff();
    // flush vac changes
    vac_->emitPendingDiff();
    isVacBeingUpdated_ = false;
}

void Workspace::updateTreeAndDomFromVac_() {
    if (!document_ || !vac_) {
        return;
    }

    isDomBeingUpdated_ = true;
    // process vac diff
    vac_->emitPendingDiff();
    // flush dom changes
    document_->emitPendingDiff();
    isDomBeingUpdated_ = false;
}

void Workspace::updateTreeFromDom_() {
    //
    rebuildTreeFromDom_();
}

// Assumes (parent == it->parent()).
template<
    typename Node,
    typename TreeLinksGetter = topology::detail::TreeLinksGetter<Node>>
void iterDfsPreOrderSkipChildren(Node*& it, Int& depth, core::TypeIdentity<Node>* root) {
    Node* next = nullptr;
    // breadth next
    while (it) {
        next = TreeLinksGetter::next(it);
        if (next) {
            it = next;
            return;
        }
        // go up
        Node* p = TreeLinksGetter::parent(it);
        it = p;
        --depth;
        if (it == root) {
            it = nullptr;
            return;
        }
        if (p) {
            p = TreeLinksGetter::parent(p);
        }
    }
}

// Assumes (parent == it->parent()).
template<
    typename Node,
    typename TreeLinksGetter = topology::detail::TreeLinksGetter<Node>>
void iterDfsPreOrder(Node*& it, Int& depth, core::TypeIdentity<Node>* root) {
    Node* next = nullptr;
    // depth first
    next = TreeLinksGetter::firstChild(it);
    if (next) {
        ++depth;
        it = next;
        return;
    }
    // breadth next
    iterDfsPreOrderSkipChildren(it, depth, root);
}

template<
    typename Node,
    typename TreeLinksGetter = topology::detail::TreeLinksGetter<Node>>
void iterDfsPreOrder(
    Node*& it,
    Int& depth,
    core::TypeIdentity<Node>* root,
    bool skipChildren) {

    if (skipChildren) {
        iterDfsPreOrderSkipChildren(it, depth, root);
    }
    else {
        iterDfsPreOrder(it, depth, root);
    }
}

template<
    typename Node,
    typename TreeLinksGetter = topology::detail::TreeLinksGetter<Node>>
void visitDfsPreOrder(Node* root, std::function<void(core::TypeIdentity<Node>*, Int)> f) {
    Node* e = root;
    Int depth = 0;
    while (e) {
        f(e, depth);
        iterDfsPreOrder(e, depth, root);
    }
}

// Updates parent.
// Assumes (parent == it->parent()).
dom::Element* rebuildTreeFromDomIter(Element* it, Element*& parent) {
    dom::Element* e = it->domElement();
    dom::Element* next = nullptr;
    // depth first
    next = e->firstChildElement();
    if (next) {
        parent = it;
        return next;
    }
    // breadth next
    while (e) {
        next = e->nextSiblingElement();
        if (next) {
            return next;
        }
        // go up
        if (!parent) {
            return nullptr;
        }
        e = parent->domElement();
        parent = parent->parent();
    }
    return next;
}

void Workspace::rebuildTreeFromDom_() {

    namespace ss = dom::strings;

    // flush dom diff
    isDomBeingUpdated_ = true;
    document_->emitPendingDiff();
    isDomBeingUpdated_ = false;

    // reset tree
    elements_.clear();
    vgcElement_ = nullptr;

    if (!document_) {
        return;
    }

    dom::Element* domVgcElement = document_->rootElement();
    if (!domVgcElement || domVgcElement->tagName() != ss::vgc) {
        return;
    }
    vgcElement_ = createElement(domVgcElement, nullptr);

    Element* p = nullptr;
    Element* e = vgcElement_;
    dom::Element* domElement = rebuildTreeFromDomIter(e, p);
    while (domElement) {
        // XXX later use a factory
        e = createElement(domElement, p);
        domElement = rebuildTreeFromDomIter(e, p);
    }
}

namespace detail {

struct VacElementLists {
    // groups are in Dfs order
    core::Array<Element*> groups;
    core::Array<Element*> keyVertices;
    core::Array<Element*> keyEdges;
    core::Array<Element*> keyFaces;
    core::Array<Element*> inbetweenVertices;
    core::Array<Element*> inbetweenEdges;
    core::Array<Element*> inbetweenFaces;
};

} // namespace detail

Element* Workspace::getRefAttribute(
    dom::Element* domElement,
    core::StringId name,
    core::StringId tagNameFilter) const {

    dom::Element* domTargetElement = domElement->getRefAttribute(name, tagNameFilter);
    if (!domTargetElement) {
        return nullptr;
    }

    auto it = elements_.find(domTargetElement->internalId());
    return it->second.get();
}

void Workspace::rebuildVacFromTree_() {

    namespace ss = dom::strings;

    if (!document_ || !vac_ || !vgcElement_) {
        return;
    }
    isVacBeingUpdated_ = true;

    vac_->clear();
    // now, all workspace elements have a dangling vac node pointer.
    for (const std::pair<const core::Id, std::unique_ptr<Element>>& p : elements_) {
        p.second->vacNode_ = nullptr;
    }

    detail::VacElementLists ce;
    fillVacElementListsUsingTagName(vgcElement_, ce);

    vgcElement_->vacNode_ = vac_->rootGroup();

    for (Element* e : ce.groups) {
        dom::Element* const domElem = e->domElement();

        topology::VacGroup* g = topology::ops::createVacGroup(
            domElem->internalId(), e->parent()->vacNode()->toGroupUnchecked());
        e->vacNode_ = g;

        // todo: set attributes
        // ...
    }

    std::set<Element*> parentsToOrderSync;

    for (Element* e : ce.keyVertices) {
        dom::Element* const domElem = e->domElement();

        topology::KeyVertex* kv = topology::ops::createKeyVertex(
            domElem->internalId(), e->parent()->vacNode()->toGroupUnchecked());
        e->vacNode_ = kv;

        const auto& position = domElem->getAttribute(ss::position).getVec2d();
        topology::ops::setKeyVertexPosition(kv, position);
    }

    for (Element* e : ce.keyEdges) {
        dom::Element* const domElem = e->domElement();

        Element* ev0 = getRefAttribute(domElem, ss::startvertex, ss::vertex);
        Element* ev1 = getRefAttribute(domElem, ss::endvertex, ss::vertex);
        if (!ev0) {
            ev0 = ev1;
        }
        if (!ev1) {
            ev1 = ev0;
        }

        topology::KeyEdge* ke = nullptr;
        if (ev0) {
            topology::KeyVertex* v0 =
                ev0->vacNode()->toCellUnchecked()->toKeyVertexUnchecked();
            topology::KeyVertex* v1 =
                ev0->vacNode()->toCellUnchecked()->toKeyVertexUnchecked();
            ke = topology::ops::createKeyEdge(
                domElem->internalId(),
                e->parent()->vacNode()->toGroupUnchecked(),
                v0,
                v1);
        }
        else {
            ke = topology::ops::createKeyClosedEdge(
                domElem->internalId(), e->parent()->vacNode()->toGroupUnchecked());
        }
        e->vacNode_ = ke;

        const auto& points = domElem->getAttribute(ss::positions).getVec2dArray();
        topology::ops::setKeyEdgeCurvePoints(ke, points);
        const auto& widths = domElem->getAttribute(ss::widths).getDoubleArray();
        topology::ops::setKeyEdgeCurveWidths(ke, widths);
    }

    // todo: sync children order in all groups
    Element* e = vgcElement_;
    Int depth = 0;
    while (e) {
        topology::VacNode* node = e->vacNode();
        if (!node) {
            continue;
        }

        if (node->isGroup()) {
            Element* child = e->firstChildVacElement();
            if (child) {
                topology::VacGroup* g = static_cast<topology::VacGroup*>(node);
                topology::ops::moveToGroup(child->vacNode(), g, g->firstChild());
            }
        }

        if (e->parent()) {
            Element* next = e->nextVacElement();
            topology::ops::moveToGroup(
                node, node->parentGroup(), (next ? next->vacNode() : nullptr));
        }

        iterDfsPreOrder(e, depth, vgcElement_);
    }

    // debug print
    //visitDfsPreOrder(vgcElement_, [](Element* e, Int depth) {
    //    VGC_DEBUG_TMP("{:>{}}<{} id=\"{}\">", "", depth * 2, e->tagName(), e->id());
    //});

    lastSyncedDomVersionId_ = document_->versionId();
    lastSyncedVacVersion_ = vac_->version();
}

void Workspace::fillVacElementListsUsingTagName(
    Element* root,
    detail::VacElementLists& ce) const {

    namespace ss = dom::strings;

    Element* e = root->firstChild();
    Int depth = 1;

    while (e) {
        bool skipChildren = true;

        core::StringId tagName = e->domElement_->tagName();
        if (tagName == ss::vertex) {
            ce.keyVertices.append(e);
        }
        else if (tagName == ss::edge) {
            ce.keyEdges.append(e);
        }
        else if (tagName == ss::layer) {
            ce.groups.append(e);
            skipChildren = false;
        }

        iterDfsPreOrder(e, depth, root, skipChildren);
    }
}

Element* Workspace::createElement(dom::Element* domElement, Element* parent) {
    if (!domElement) {
        return nullptr;
    }
    const auto& p = elements_.emplace(
        domElement->internalId(), std::make_unique<Element>(domElement));
    if (!p.second) {
        // XXX should probably throw
        return nullptr;
    }
    Element* e = p.first->second.get();
    e->id_ = domElement->internalId();
    if (parent) {
        parent->appendChild(e);
    }
    return e;
}

} // namespace vgc::workspace
