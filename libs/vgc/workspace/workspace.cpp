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
#include <memory>

#include <vgc/dom/strings.h>
#include <vgc/topology/operations.h>
#include <vgc/topology/vac.h>
#include <vgc/workspace/edge.h>
#include <vgc/workspace/layer.h>
#include <vgc/workspace/logcategories.h>
#include <vgc/workspace/vertex.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

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

class ScopedRecursiveBoolGuard {
public:
    ScopedRecursiveBoolGuard(bool& ref)
        : old_(ref)
        , ref_(ref) {

        ref_ = true;
    }

    ~ScopedRecursiveBoolGuard() {
        ref_ = old_;
    }

private:
    bool old_;
    bool& ref_;
};

} // namespace detail

namespace {

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

template<
    typename Node,
    typename TreeLinksGetter = topology::detail::TreeLinksGetter<Node>>
void visitDfs(
    Node* root,
    std::function<bool(core::TypeIdentity<Node>*, Int)> preOrderFn,
    std::function<void(core::TypeIdentity<Node>*, Int)> postOrderFn) {

    Int depth = 0;
    Node* node = root;
    while (node) {
        if (preOrderFn(node, depth)) {
            // depth first, go down
            Node* firstChild = TreeLinksGetter::firstChild(node);
            if (firstChild) {
                ++depth;
                node = firstChild;
                continue;
            }
        }
        postOrderFn(node, depth); // post-order leaf
        // breadth next
        Node* next = nullptr;
        while (node) {
            next = TreeLinksGetter::next(node);
            if (next) {
                node = next;
                break;
            }
            // go up
            Node* parent = TreeLinksGetter::parent(node);
            if (parent == root) {
                node = nullptr;
                depth = 0;
                break;
            }
            --depth;
            node = parent;
            if (node) {
                postOrderFn(node, depth); // post-order parent
                parent = TreeLinksGetter::parent(node);
            }
        }
    }
}

} // namespace

void Workspace::visitDfsPreOrder(std::function<void(Element*, Int)> preOrderFn) {
    workspace::visitDfsPreOrder<Element>(vgcElement(), std::move(preOrderFn));
}

void Workspace::visitDfs(
    std::function<bool(Element*, Int)> preOrderFn,
    std::function<void(Element*, Int)> postOrderFn) {
    workspace::visitDfs<Element>(
        vgcElement(), std::move(preOrderFn), std::move(postOrderFn));
}

Workspace::Workspace(dom::DocumentPtr document)
    : document_(document) {

    document->changed().connect(onDocumentDiff());

    vac_ = topology::Vac::create();
    vac_->changed().connect(onVacDiff());
    vac_->onNodeAboutToBeRemoved().connect(onVacNodeAboutToBeRemoved());

    rebuildTreeFromDom_();
    rebuildVacFromTree_();
}

void Workspace::onDestroyed() {

    vac_->changed().disconnect(this);
    vac_->onNodeAboutToBeRemoved().disconnect(this);
    for (const auto& p : elements_) {
        Element* e = p.second.get();
        if (e->isVacElement()) {
            static_cast<VacElement*>(e)->vacNode_ = nullptr;
        }
    }
    vac_ = nullptr;
    document_ = nullptr;

    SuperClass::onDestroyed();
}

namespace {

std::once_flag initOnceFlag;

template<typename T>
std::unique_ptr<Element>
makeUniqueElement(Workspace* workspace, dom::Element* domElement) {
    return std::make_unique<T>(workspace, domElement);
}

} // namespace

/* static */
WorkspacePtr Workspace::create(dom::DocumentPtr document) {

    namespace ds = dom::strings;

    std::call_once(initOnceFlag, []() {
        registerElementClass(ds::vgc, &makeUniqueElement<Layer>);
        registerElementClass(ds::layer, &makeUniqueElement<Layer>);
        registerElementClass(ds::vertex, &makeUniqueElement<KeyVertex>);
        //registerElementClass(ds::edge, &makeUniqueElement<KeyEdge>);
        registerElementClass(ds::edge, &makeUniqueElement<KeyEdge>);
    });

    return WorkspacePtr(new Workspace(document));
}

std::unordered_map<core::StringId, Workspace::ElementCreator>&
Workspace::elementCreators() {
    static std::unordered_map<core::StringId, Workspace::ElementCreator>* instance_ =
        new std::unordered_map<core::StringId, Workspace::ElementCreator>();
    return *instance_;
}

void Workspace::registerElementClass(
    core::StringId tagName,
    ElementCreator elementCreator) {

    elementCreators()[tagName] = elementCreator;
}

void Workspace::sync() {
    // slots do check if a rebuild is necessary
    document_->emitPendingDiff();
    vac_->emitPendingDiff();
}

void Workspace::rebuildFromDom() {
    // disable update-on-diff
    detail::ScopedRecursiveBoolGuard bgDom(isDomBeingUpdated_);
    detail::ScopedRecursiveBoolGuard bgVac(isVacBeingUpdated_);
    // flush dom diffs
    document_->emitPendingDiff();
    // rebuild
    rebuildTreeFromDom_();
    rebuildVacFromTree_();
    // flush vac diffs
    vac_->emitPendingDiff();
}

Element* Workspace::getElementFromPathAttribute(
    dom::Element* domElement,
    core::StringId attrName,
    core::StringId tagNameFilter) const {

    dom::Element* domTargetElement =
        domElement->getElementFromPathAttribute(attrName, tagNameFilter)
            .value_or(nullptr);
    if (!domTargetElement) {
        return nullptr;
    }

    auto it = elements_.find(domTargetElement->internalId());
    return it->second.get();
}

void Workspace::onDocumentDiff_(const dom::Diff& diff) {
    if (isDomBeingUpdated_) {
        // workspace is doing the changes, no need to see the diff.
        return;
    }

    bool vacChanged = vac_->version() != lastSyncedVacVersion_;
    if (vacChanged) {
        VGC_ERROR(
            LogVgcWorkspace,
            "Both DOM and VAC of workspace have been edited since last synchronization. "
            "Rebuilding VAC from DOM.");
        rebuildFromDom();
        return;
    }

    updateTreeAndVacFromDom_(diff);
}

void Workspace::onVacDiff_(const topology::VacDiff& diff) {
    if (isVacBeingUpdated_) {
        // workspace is doing the changes, no need to see the diff.
        return;
    }

    bool domChanged = document_ && (document_->versionId() != lastSyncedDomVersionId_);
    if (domChanged) {
        VGC_ERROR(
            LogVgcWorkspace,
            "Both DOM and VAC of workspace have been edited since last synchronization. "
            "Rebuilding VAC from DOM.");
        rebuildFromDom();
        return;
    }

    updateTreeAndDomFromVac_(diff);
}

Element* Workspace::createAppendElement_(dom::Element* domElement, Element* parent) {
    if (!domElement) {
        return nullptr;
    }

    std::unique_ptr<Element> u = {};
    auto& creators = elementCreators();
    auto it = creators.find(domElement->tagName());
    if (it != creators.end()) {
        u = it->second(this, domElement);
        if (!u) {
            VGC_ERROR(
                LogVgcWorkspace,
                "Element creator for \"{}\" failed to create the element.",
                domElement->tagName());
            // XXX throw or fallback to UnsupportedElement or nullptr ?
            u = std::make_unique<UnsupportedElement>(this, domElement);
        }
    }
    else {
        u = std::make_unique<UnsupportedElement>(this, domElement);
    }

    Element* e = u.get();
    const auto& p = elements_.emplace(domElement->internalId(), std::move(u));
    if (!p.second) {
        // XXX should probably throw
        return nullptr;
    }

    e->id_ = domElement->internalId();
    if (parent) {
        parent->appendChild(e);
    }

    return e;
}

bool Workspace::updateElementFromDom(Element* element) {
    if (element->isBeingUpdated_) {
        VGC_ERROR(LogVgcWorkspace, "Cyclic update dependency detected.");
        return false;
    }
    // if not already up-to-date
    if (!element->isInSyncWithDom_
        || element->error_ == ElementError::UnresolvedDependency) {

        element->isBeingUpdated_ = true;
        const ElementError result = element->updateFromDom_(this);

        if (result != element->error_) {
            if (element->error_ == ElementError::UnresolvedDependency) {
                elementsWithDependencyErrors_.removeOne(element);
            }
            switch (result) {
            case ElementError::UnresolvedDependency:
                elementsWithDependencyErrors_.emplaceLast(element);
                break;
            case ElementError::InvalidAttribute:
            case ElementError::None:
                elementsOutOfSync_.removeOne(element);
                break;
            }
        }

        element->error_ = result;
        element->isBeingUpdated_ = false;
        element->isInSyncWithDom_ = true;
    }
    return true;
}

void Workspace::onVacNodeAboutToBeRemoved_(topology::VacNode* node) {
    auto it = elements_.find(node->id());
    if (it != elements_.end()) {
        Element* element = it->second.get();
        if (element->isVacElement()) {
            static_cast<VacElement*>(element)->vacNode_ = nullptr;
            elementsOutOfSync_.emplaceLast(element);
        }
    }
}

void Workspace::fillVacElementListsUsingTagName_(
    Element* root,
    detail::VacElementLists& ce) const {

    namespace ds = dom::strings;

    Element* e = root->firstChild();
    Int depth = 1;

    while (e) {
        bool skipChildren = true;

        core::StringId tagName = e->domElement_->tagName();
        if (tagName == ds::vertex) {
            ce.keyVertices.append(e);
        }
        else if (tagName == ds::edge) {
            ce.keyEdges.append(e);
        }
        else if (tagName == ds::layer) {
            ce.groups.append(e);
            skipChildren = false;
        }

        iterDfsPreOrder(e, depth, root, skipChildren);
    }
}

namespace {

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

} // namespace

void Workspace::rebuildTreeFromDom_() {

    namespace ds = dom::strings;

    // reset tree
    elements_.clear();
    vgcElement_ = nullptr;

    // reset vac
    {
        detail::ScopedRecursiveBoolGuard bgVac(isVacBeingUpdated_);
        vac_->clear();
        vac_->emitPendingDiff();
        lastSyncedVacVersion_ = -1;
    }

    if (!document_) {
        return;
    }

    // flush dom diff
    {
        detail::ScopedRecursiveBoolGuard bgDom(isDomBeingUpdated_);
        document_->emitPendingDiff();
    }

    dom::Element* domVgcElement = document_->rootElement();
    if (!domVgcElement || domVgcElement->tagName() != ds::vgc) {
        return;
    }

    Element* vgcElement = createAppendElement_(domVgcElement, nullptr);
    VGC_ASSERT(vgcElement->isVacElement());
    vgcElement_ = static_cast<VacElement*>(vgcElement);

    Element* p = nullptr;
    Element* e = vgcElement_;
    dom::Element* domElement = rebuildTreeFromDomIter(e, p);
    while (domElement) {
        e = createAppendElement_(domElement, p);
        domElement = rebuildTreeFromDomIter(e, p);
    }

    // children should already be in the correct order.
}

void Workspace::rebuildDomFromTree_() {
    // todo later
    throw core::RuntimeError("not implemented");
}

void Workspace::rebuildTreeFromVac_() {
    // todo later
    throw core::RuntimeError("not implemented");
}

void Workspace::rebuildVacFromTree_() {
    if (!document_ || !vgcElement_) {
        return;
    }

    //namespace ds = dom::strings;

    detail::ScopedRecursiveBoolGuard bgVac(isVacBeingUpdated_);

    // reset vac
    vac_->clear();
    vac_->emitPendingDiff();
    lastSyncedVacVersion_ = -1;

    vgcElement_->vacNode_ = vac_->rootGroup();

    // all workspace elements vac node pointers should be null
    // thanks to `onVacNodeAboutToBeRemoved`

    //detail::VacElementLists ce;
    //fillVacElementListsUsingTagName_(vgcElement_, ce);
    //
    //for (Element* e : ce.groups) {
    //    e->updateFromDom();
    //}
    //
    //for (Element* e : ce.keyVertices) {
    //    e->updateFromDom();
    //}
    //
    //for (Element* e : ce.keyEdges) {
    //    e->updateFromDom();
    //}

    Element* root = vgcElement_;
    Element* element = root->firstChild();
    Int depth = 1;
    while (element) {
        updateElementFromDom(element);
        iterDfsPreOrder(element, depth, root);
    }

    updateVacHierarchyFromTree_();

    lastSyncedDomVersionId_ = document_->versionId();
    lastSyncedVacVersion_ = vac_->version();
    changed().emit();
}

void Workspace::updateVacHierarchyFromTree_() {
    // todo: sync children order in all groups
    Element* root = vgcElement_;
    Element* e = root;
    Int depth = 0;
    while (e) {
        topology::VacNode* node = e->vacNode();
        if (!node) {
            continue;
        }

        if (node->isGroup()) {
            VacElement* child = e->firstChildVacElement();
            if (child) {
                topology::VacGroup* g = static_cast<topology::VacGroup*>(node);
                topology::ops::moveToGroup(child->vacNode(), g, g->firstChild());
            }
        }

        if (e->parent()) {
            VacElement* next = e->nextVacElement();
            topology::ops::moveToGroup(
                node, node->parentGroup(), (next ? next->vacNode() : nullptr));
        }

        iterDfsPreOrder(e, depth, root);
    }
}

//bool Workspace::haveKeyEdgeBoundaryPathsChanged_(Element* e) {
//    topology::VacNode* node = e->vacNode();
//    if (!node) {
//        return false;
//    }
//
//    namespace ds = dom::strings;
//    dom::Element* const domElem = e->domElement();
//
//    Element* ev0 = getElementFromPathAttribute(domElem, ds::startvertex, ds::vertex);
//    Element* ev1 = getElementFromPathAttribute(domElem, ds::endvertex, ds::vertex);
//
//    topology::KeyEdge* kv = node->toCellUnchecked()->toKeyEdgeUnchecked();
//    if (ev0) {
//        if (ev0->vacNode() != kv->startVertex()) {
//            return true;
//        }
//    }
//    else if (kv->startVertex()) {
//        return true;
//    }
//    if (ev1) {
//        if (ev1->vacNode() != kv->endVertex()) {
//            return true;
//        }
//    }
//    else if (kv->endVertex()) {
//        return true;
//    }
//
//    return false;
//}

void Workspace::updateTreeAndVacFromDom_(const dom::Diff& diff) {
    if (!document_) {
        return;
    }

    detail::ScopedRecursiveBoolGuard bgVac(isVacBeingUpdated_);

    VGC_ASSERT(vac_->version() == lastSyncedVacVersion_);

    namespace ds = dom::strings;

    // impl goal: we want to keep as much cached data as possible.
    //            we want the vac to be valid -> using only its operators
    //            limits bugs to their implementation.

    bool needsFullUpdate =
        (diff.removedNodes().length() > 0) || (diff.reparentedNodes().size() > 0);

    std::set<Element*> parentsToOrderSync;

    // first we remove what has to be removed
    // this can remove dependent vac nodes (star).
    for (dom::Node* node : diff.removedNodes()) {
        dom::Element* domElement = dom::Element::cast(node);
        if (!domElement) {
            continue;
        }
        Element* element = find(domElement);
        if (element) {
            Element* parent = element->parent();
            VGC_ASSERT(parent);
            for (Element* child : *element) {
                parent->appendChild(child);
            }
            element->unlink();
            elements_.erase(domElement->internalId());
        }
    }

    // create new elements
    for (dom::Node* node : diff.createdNodes()) {
        dom::Element* domElement = dom::Element::cast(node);
        if (!domElement) {
            continue;
        }
        dom::Element* domParentElement = domElement->parentElement();
        if (!domParentElement) {
            continue;
        }
        Element* parent = find(domParentElement);
        if (!parent) {
            // XXX warn ? createdNodes should be in valid build order
            //            and vgc element should already exist.
            continue;
        }
        // will be reordered afterwards
        Element* element = createAppendElement_(domElement, parent);
        element->isInSyncWithDom_ = false;
        elementsOutOfSync_.emplaceLast(element);
        parentsToOrderSync.insert(parent);
    }

    for (dom::Node* node : diff.reparentedNodes()) {
        dom::Element* domElement = dom::Element::cast(node);
        if (!domElement) {
            continue;
        }
        dom::Element* domParentElement = domElement->parentElement();
        if (!domParentElement) {
            continue;
        }
        Element* parent = find(domParentElement);
        if (parent) {
            parentsToOrderSync.insert(parent);
        }
    }

    for (dom::Node* node : diff.childrenReorderedNodes()) {
        dom::Element* domElement = dom::Element::cast(node);
        if (!domElement) {
            continue;
        }
        Element* element = find(domElement);
        if (!element) {
            // XXX error ?
            continue;
        }
        parentsToOrderSync.insert(element);
    }

    // update tree hierarchy from dom
    for (Element* element : parentsToOrderSync) {
        Element* child = element->firstChild();
        dom::Element* domChild = element->domElement()->firstChildElement();
        while (domChild) {
            if (!child || child->domElement() != domChild) {
                Element* firstChild = find(domChild);
                VGC_ASSERT(firstChild);
                element->insertChildUnchecked(child, firstChild);
                child = firstChild;
            }
            child = child->nextVacElement();
            domChild = domChild->nextSiblingElement();
        }
    }

    // update everything (paths may have changed.. etc)
    if (needsFullUpdate) {
        Element* root = vgcElement_;
        Element* element = root;
        Int depth = 0;
        while (element) {
            updateElementFromDom(element);
            iterDfsPreOrder(element, depth, root);
        }
    }
    else {
        for (const auto& it : diff.modifiedElements()) {
            Element* element = find(it.first);
            if (element) {
                element->isInSyncWithDom_ = false;
                elementsOutOfSync_.emplaceLast(element);
            }
        }
        for (Element* element : elementsOutOfSync_) {
            updateElementFromDom(element);
        }
    }

    updateVacHierarchyFromTree_();

    lastSyncedDomVersionId_ = document_->versionId();
    lastSyncedVacVersion_ = vac_->version();
    changed().emit();
}

void Workspace::updateTreeAndDomFromVac_(const topology::VacDiff& /*diff*/) {
    if (!document_) {
        VGC_ERROR(LogVgcWorkspace, "DOM is null.")
        return;
    }

    detail::ScopedRecursiveBoolGuard bgDom(isDomBeingUpdated_);

    // todo later
    throw core::RuntimeError("not implemented");
}

void Workspace::debugPrintTree_() {
    visitDfsPreOrder([](Element* e, Int depth) {
        VGC_DEBUG_TMP("{:>{}}<{} id=\"{}\">", "", depth * 2, e->tagName(), e->id());
    });
}

} // namespace vgc::workspace
