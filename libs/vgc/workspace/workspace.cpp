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

#include <vgc/workspace/workspace.h>

#include <functional>
#include <memory>

#include <vgc/core/boolguard.h>
#include <vgc/dom/strings.h>
#include <vgc/vacomplex/complex.h>
#include <vgc/vacomplex/operations.h>
#include <vgc/workspace/edge.h>
#include <vgc/workspace/face.h>
#include <vgc/workspace/freehandedgegeometry.h>
#include <vgc/workspace/layer.h>
#include <vgc/workspace/logcategories.h>
#include <vgc/workspace/vertex.h>

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

} // namespace detail

namespace {

// Assumes (parent == it->parent()).
template<
    typename Node,
    typename TreeLinksGetter = vacomplex::detail::TreeLinksGetter<Node>>
void iterDfsPreOrderSkipChildren(Node*& it, Int& depth, core::TypeIdentity<Node>* root) {
    Node* next = nullptr;
    // breadth next
    while (it) {
        next = TreeLinksGetter::nextSibling(it);
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
    typename TreeLinksGetter = vacomplex::detail::TreeLinksGetter<Node>>
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
    typename TreeLinksGetter = vacomplex::detail::TreeLinksGetter<Node>>
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
    typename TreeLinksGetter = vacomplex::detail::TreeLinksGetter<Node>>
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
    typename TreeLinksGetter = vacomplex::detail::TreeLinksGetter<Node>>
void visitDfs(
    Node* root,
    const std::function<bool(core::TypeIdentity<Node>*, Int)>& preOrderFn,
    const std::function<void(core::TypeIdentity<Node>*, Int)>& postOrderFn) {

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
            next = TreeLinksGetter::nextSibling(node);
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

Workspace::Workspace(CreateKey key, dom::DocumentPtr document)
    : Object(key)
    , document_(document) {

    document->changed().connect(onDocumentDiff());

    vac_ = vacomplex::Complex::create();
    vac_->nodesChanged().connect(onVacNodesChanged());

    rebuildFromDom();
}

void Workspace::onDestroyed() {

    elementByVacInternalId_.clear();
    for (const auto& p : elements_) {
        Element* e = p.second.get();
        VacElement* ve = e->toVacElement();
        if (ve) {
            // the whole VAC is cleared afterwards
            ve->vacNode_ = nullptr;
        }
        while (!e->dependents_.isEmpty()) {
            Element* dependent = e->dependents_.pop();
            dependent->dependencies_.removeOne(e);
            dependent->onDependencyRemoved_(e);
            e->onDependentElementRemoved_(dependent);
        }
    }
    elements_.clear();
    rootVacElement_ = nullptr;
    elementsWithError_.clear();
    elementsToUpdateFromDom_.clear();
    vac_ = nullptr;
    document_ = nullptr;

    SuperClass::onDestroyed();
}

namespace {

std::once_flag initOnceFlag;

template<typename T>
std::unique_ptr<Element> makeUniqueElement(Workspace* workspace) {
    return std::make_unique<T>(workspace);
}

} // namespace

WorkspacePtr Workspace::create(dom::DocumentPtr document) {

    namespace ds = dom::strings;

    std::call_once(initOnceFlag, []() {
        registerElementClass_(ds::vgc, &makeUniqueElement<Layer>);
        registerElementClass_(ds::layer, &makeUniqueElement<Layer>);
        registerElementClass_(ds::vertex, &makeUniqueElement<VacKeyVertex>);
        registerElementClass_(ds::edge, &makeUniqueElement<VacKeyEdge>);
        registerElementClass_(ds::face, &makeUniqueElement<VacKeyFace>);
    });

    return core::createObject<Workspace>(document);
}

void Workspace::registerElementClass_(
    core::StringId tagName,
    ElementCreator elementCreator) {

    elementCreators_()[tagName] = elementCreator;
}

void Workspace::sync() {
    document_->emitPendingDiff();
}

void Workspace::rebuildFromDom() {
    if (!document_) {
        return;
    }
    rebuildWorkspaceTreeFromDom_();
    rebuildVacFromWorkspaceTree_();
    lastSyncedDomVersionId_ = document_->versionId();
    changed().emit();
}

bool Workspace::updateElementFromDom(Element* element) {
    if (element->isBeingUpdated_) {
        VGC_ERROR(LogVgcWorkspace, "Cyclic update dependency detected.");
        return false;
    }
    if (element->hasPendingUpdate_) {
        element->isBeingUpdated_ = true;
        const ElementStatus oldStatus = element->status_;
        const ElementStatus newStatus = element->updateFromDom_(this);

        if (!newStatus) {
            if (oldStatus == ElementStatus::Ok) {
                elementsWithError_.emplaceLast(element);
            }
        }
        else if (!oldStatus) {
            elementsWithError_.removeOne(element);
        }

        element->status_ = newStatus;
        element->isBeingUpdated_ = false;
        clearPendingUpdateFromDom_(element);
    }
    return true;
}

std::optional<Element*> Workspace::getElementFromPathAttribute(
    const dom::Element* domElement,
    core::StringId attrName,
    core::StringId tagNameFilter) const {

    std::optional<dom::Element*> domTargetElement =
        domElement->getElementFromPathAttribute(attrName, tagNameFilter);

    if (!domTargetElement.has_value()) {
        return std::nullopt;
    }

    dom::Element* domTargetElementValue = domTargetElement.value();
    if (!domTargetElementValue) {
        return nullptr;
    }

    auto it = elements_.find(domTargetElementValue->internalId());
    if (it == elements_.end()) {
        return nullptr;
    }

    return {it->second.get()};
}

void Workspace::visitDepthFirstPreOrder(
    const std::function<void(Element*, Int)>& preOrderFn) {
    workspace::visitDfsPreOrder<Element>(vgcElement(), preOrderFn);
}

void Workspace::visitDepthFirst(
    const std::function<bool(Element*, Int)>& preOrderFn,
    const std::function<void(Element*, Int)>& postOrderFn) {
    workspace::visitDfs<Element>(vgcElement(), preOrderFn, postOrderFn);
}

namespace {

// Assumes `!samples.isEmpty()` and `numSamples >= 2`.
core::Array<geometry::Vec2d> computeApproximateUniformSamplingPositions(
    const geometry::StrokeSample2dArray& samples,
    Int numSamples) {

    core::Array<geometry::Vec2d> result;
    result.reserve(numSamples);
    result.append(samples.first().position());
    double l = samples.last().s();
    if (l > 0) {
        double deltaS = l / (numSamples - 1);
        double targetS = deltaS;
        const geometry::StrokeSample2d* s0 = &samples[0];
        for (const geometry::StrokeSample2d& s1 : samples) {
            double ds = s1.s() - s0->s();
            if (ds > 0 && targetS <= s1.s()) {
                double t = (targetS - s0->s()) / ds;
                result.append(t * s1.position() + (1 - t) * s0->position());
                targetS += deltaS;
            }
            s0 = &s1;
        }
    }
    while (result.length() < numSamples) {
        result.append(samples.last().position());
    }

    return result;
}

} // namespace

core::Id Workspace::glue(core::Span<core::Id> elementIds) {
    core::Id resultId = -1;

    // Open history group
    static core::StringId commandId = core::StringId("workspace.glue");
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = this->history();
    if (history) {
        undoGroup = history->createUndoGroup(commandId);
    }

    core::Array<vacomplex::KeyVertex*> kvs;
    core::Array<vacomplex::KeyEdge*> openKes;
    core::Array<vacomplex::KeyEdge*> closedKes;
    bool hasOtherCells = false;
    bool hasNonVacElements = false;
    for (core::Id id : elementIds) {
        workspace::Element* element = find(id);
        if (!element) {
            continue;
        }
        vacomplex::Node* node = element->vacNode();
        if (!node || !node->isCell()) {
            hasNonVacElements = true;
            continue;
        }
        vacomplex::Cell* cell = node->toCellUnchecked();
        switch (cell->cellType()) {
        case vacomplex::CellType::KeyVertex: {
            kvs.append(cell->toKeyVertexUnchecked());
            break;
        }
        case vacomplex::CellType::KeyEdge: {
            vacomplex::KeyEdge* ke = cell->toKeyEdgeUnchecked();
            if (ke->isClosed()) {
                closedKes.append(ke);
            }
            else {
                openKes.append(ke);
            }
            break;
        }
        default:
            hasOtherCells = true;
            break;
        }
    }

    if (hasOtherCells || hasNonVacElements) {
        // do not glue
    }
    else if (!kvs.isEmpty()) {
        if (openKes.isEmpty() && closedKes.isEmpty()) {
            geometry::Vec2d cog = {};
            for (vacomplex::KeyVertex* kv : kvs) {
                cog += kv->position();
            }
            cog /= core::narrow_cast<double>(kvs.length());

            vacomplex::Cell* result = vacomplex::ops::glueKeyVertices(kvs, cog);
            Element* e = this->findVacElement(result);
            if (e) {
                resultId = e->id();
            }
        }
    }
    else if (!openKes.isEmpty()) {
        if (closedKes.isEmpty() && openKes.length() == 2) {

            // sample both edges
            vacomplex::KeyEdge* ke0 = openKes[0];
            vacomplex::KeyEdge* ke1 = openKes[1];

            // TODO: use operation source in onVacNodesChanged_ to do the color blend
            // Note: at least 1 sample is guaranteed.
            bool isEdge0Longer = ke0->sampling().samples().last().s()
                                 > ke1->sampling().samples().last().s();
            VacKeyEdge* colorEdge = dynamic_cast<VacKeyEdge*>(
                this->findVacElement(isEdge0Longer ? ke0 : ke1));
            core::Color color =
                colorEdge
                    ? colorEdge
                          ->computeFrameData(VacEdgeComputationStage::PreJoinGeometry)
                          ->color()
                    : core::Color{};

            const geometry::StrokeSample2dArray& samples0 = ke0->sampling().samples();
            geometry::StrokeSample2dArray samples1 = ke1->sampling().samples();

            // Detect which edge direction should be used for gluing.
            // Here, we handle the simple cases where the two edges already share
            // at least one vertex.
            vacomplex::KeyVertex* ke00 = ke0->startVertex();
            vacomplex::KeyVertex* ke01 = ke0->endVertex();
            vacomplex::KeyVertex* ke10 = ke1->startVertex();
            vacomplex::KeyVertex* ke11 = ke1->endVertex();
            bool isAnyLoop = (ke00 == ke01) || (ke10 == ke11);
            bool isBestDirectionKnown = false;
            bool reverse1 = false;
            if (!isAnyLoop) {
                if ((ke00 == ke10) || (ke01 == ke11)) {
                    // If the two edges have the same start vertex or the same
                    // end vertex, we glue them in their intrinsic direction.
                    reverse1 = false;
                    isBestDirectionKnown = true;
                }
                else if ((ke00 == ke11) || (ke01 == ke10)) {
                    // If the start (resp. end) vertex of ke0 is equal to the
                    // end (resp. start) vertex of ke1, we want to glue them in reverse.
                    reverse1 = true;
                    isBestDirectionKnown = true;
                }
            }

            core::Array<FreehandEdgePoint> newPoints;

            if (samples0.length() < 2) {
                Int n = samples1.length();
                newPoints.resizeNoInit(n);
                FreehandEdgePoint fep0(samples0.first());
                for (Int i = 0; i < n; ++i) {
                    newPoints[i] = fep0.average(samples1[i]);
                }
            }
            else if (samples1.length() < 2) {
                Int n = samples0.length();
                newPoints.resizeNoInit(n);
                FreehandEdgePoint fep1(samples1.first());
                for (Int i = 0; i < n; ++i) {
                    newPoints[i] = fep1.average(samples1[i]);
                }
            }
            else {
                double l0 = samples0.last().s();
                double l1 = samples1.last().s();

                Int n0 = samples0.length();
                Int n1 = samples1.length();
                Int n = std::max<Int>(0, n0 - 2) + std::max<Int>(0, n1 - 2) + 2;

                // If we don't know yet which direction to use for gluing, we
                // detect the best direction now by choosing the one minimizing
                // the squared distance between the curves.
                //
                if (!isBestDirectionKnown && l0 > 0 && l1 > 0) {
                    core::Array<geometry::Vec2d> us0 =
                        computeApproximateUniformSamplingPositions(samples0, 10);
                    core::Array<geometry::Vec2d> us1 =
                        computeApproximateUniformSamplingPositions(samples1, 10);

                    // test best halfedge orientation
                    double costA = 0;
                    double costB = 0;

                    Int m = us0.length();
                    for (Int i = 0; i < m; ++i) {
                        costA += (us0[i] - us1[i]).squaredLength();
                        costB += (us0[i] - us1[m - 1 - i]).squaredLength();
                    }

                    // reverse samples1 if necessary
                    if (costB < costA) {
                        reverse1 = true;
                    }
                }

                // Now, compute an interpolation between the two curve with the given direction.

                if (reverse1) {
                    geometry::StrokeSample2dArray reversedSamples1(n1, core::noInit);
                    for (Int i = 0; i < n1; ++i) {
                        reversedSamples1[i] = samples1[n1 - 1 - i];
                        reversedSamples1[i].setS(l1 - reversedSamples1[i].s());
                    }
                    samples1 = std::move(reversedSamples1);
                }

                FreehandEdgePoint p0a = samples0.first();
                FreehandEdgePoint p1a = samples1.first();

                core::Array<FreehandEdgePoint> points0;
                points0.reserve(n);
                points0.append(p0a);

                core::Array<FreehandEdgePoint> points1;
                points1.reserve(n);
                points1.append(p1a);

                double u0a = 0;
                Int i0 = std::min<Int>(1, n0);
                double u1a = 0;
                Int i1 = std::min<Int>(1, n1);

                for (Int i = 1; i < n - 1; ++i) {
                    const geometry::StrokeSample2d& currentSample0 = samples0[i0];
                    const geometry::StrokeSample2d& currentSample1 = samples1[i1];
                    double u0b = currentSample0.s() / l0;
                    double u1b = currentSample1.s() / l1;
                    FreehandEdgePoint p0 = currentSample0;
                    FreehandEdgePoint p1 = currentSample1;
                    bool canIterate0 = i0 < n0 - 1;
                    bool canIterate1 = i1 < n1 - 1;
                    if (canIterate1 && (u0b > u1b || !canIterate0)) {
                        double t = (u1b - u0a) / (u0b - u0a);
                        points0.append(p0a.lerp(p0, t));
                        points1.append(p1);
                        u1a = u1b;
                        p1a = p1;
                        ++i1;
                    }
                    else if (canIterate0) {
                        double t = (u0b - u1a) / (u1b - u1a);
                        points0.append(p0);
                        points1.append(p1a.lerp(p1, t));
                        u0a = u0b;
                        p0a = p0;
                        ++i0;
                    }
                    else {
                        // shouldn't happen if `n` is correct.
                        break;
                    }
                }

                points0.emplaceLast(samples0.last());
                points1.emplaceLast(samples1.last());

                newPoints.resizeNoInit(n);
                for (Int i = 0; i < n; ++i) {
                    newPoints[i] = points0[i].average(points1[i]);
                }
            }

            double maxWidth = 0;
            for (Int i = 0; i < newPoints.length(); ++i) {
                double w = newPoints[i].width();
                if (w > maxWidth) {
                    maxWidth = w;
                }
            }

            // tolerance = 5% of max width
            // TODO: what if maxWidth = 0? Better heuristic for tolerance?
            std::shared_ptr<vacomplex::KeyEdgeGeometry> newGeometry =
                FreehandEdgeGeometry::createFromPoints(newPoints, false, maxWidth * 0.05);

            core::Array<vacomplex::KeyHalfedge> halfedges(
                {{ke0, true}, {ke1, !reverse1}});

            vacomplex::Cell* result = vacomplex::ops::glueKeyOpenEdges(
                halfedges,
                std::move(newGeometry),
                newPoints.first().position(),
                newPoints.last().position());
            Element* e = this->findVacElement(result);
            if (e) {
                e->domElement()->setAttribute(dom::strings::color, color);
                resultId = e->id();
            }
        }
    }
    else if (!closedKes.isEmpty()) {
        if (closedKes.length() == 2) {

            // sample both edges
            vacomplex::KeyEdge* ke0 = closedKes[0];
            vacomplex::KeyEdge* ke1 = closedKes[1];

            // TODO: use operation source in onVacNodesChanged_ to do the color blend
            bool isEdge0Longer = ke0->sampling().samples().last().s()
                                 > ke1->sampling().samples().last().s();
            VacKeyEdge* colorEdge = dynamic_cast<VacKeyEdge*>(
                this->findVacElement(isEdge0Longer ? ke0 : ke1));
            core::Color color =
                colorEdge
                    ? colorEdge
                          ->computeFrameData(VacEdgeComputationStage::PreJoinGeometry)
                          ->color()
                    : core::Color{};

            const geometry::StrokeSample2dArray& samples0 = ke0->sampling().samples();
            geometry::StrokeSample2dArray samples1 = ke1->sampling().samples();

            if (samples0.length() >= 2 && samples1.length() >= 2) {
                double l0 = samples0.last().s();
                double l1 = samples1.last().s();

                Int n0 = samples0.length();
                Int n1 = samples1.length();
                bool reverse1 = false;
                double offset1 = 0;

                if (l0 > 0 && l1 > 0) {
                    constexpr Int numSamplesForCost = 10;
                    constexpr Int factorToNumSamplesForShift = 10;
                    constexpr Int numSamples =
                        numSamplesForCost * factorToNumSamplesForShift;
                    double deltaS1 = l1 / numSamples;

                    core::Array<geometry::Vec2d> us0 =
                        computeApproximateUniformSamplingPositions(
                            samples0, numSamples + 1);
                    core::Array<geometry::Vec2d> us1 =
                        computeApproximateUniformSamplingPositions(
                            samples1, numSamples + 1);
                    // since it is closed, first and last are the same
                    us0.removeLast();
                    us1.removeLast();

                    double bestCost = core::DoubleInfinity;

                    Int m = us1.length();
                    // For every shift of edge 1 uniform sampling
                    for (Int i = 0; i < numSamples; ++i) {
                        double costA = 0;
                        double costB = 0;
                        // Compute distance using less samples
                        for (Int j = 0; j < numSamplesForCost; ++j) {
                            Int k = j * factorToNumSamplesForShift;
                            costA += (us0[k] - us1[(k + i) % m]).squaredLength();
                            costB +=
                                (us0[k] - us1[(m - 1 - k + i + m) % m]).squaredLength();
                        }
                        if (costA < bestCost) {
                            offset1 = deltaS1 * i;
                            reverse1 = false;
                            bestCost = costA;
                        }
                        if (costB < bestCost) {
                            offset1 = deltaS1 * i;
                            reverse1 = true;
                            bestCost = costB;
                        }
                    }
                }

                FreehandEdgePoint p0a = samples0.first();
                FreehandEdgePoint p1a = samples1.first();

                // rotate samples1 by offset1
                if (offset1 > 0) {
                    Int m = 0;
                    for (; m < n1; ++m) {
                        if (samples1[m].s() >= offset1) {
                            break;
                        }
                    }
                    const geometry::StrokeSample2d& s0 =
                        samples1[std::max<Int>(0, m - 1)];
                    const geometry::StrokeSample2d& s1 =
                        samples1[std::min<Int>(m, n1 - 1)];
                    double ds = s1.s() - s0.s();
                    if (ds > 0) {
                        double t = (offset1 - s0.s()) / ds;
                        p1a = FreehandEdgePoint(s0).lerp(FreehandEdgePoint(s1), t);
                    }
                    else {
                        p1a = s1;
                    }
                    std::rotate(samples1.begin(), samples1.begin() + m, samples1.end());
                    Int mr = n1 - m;
                    for (Int i = 0; i < mr; ++i) {
                        samples1[i].setS(samples1[i].s() - offset1);
                    }
                    for (Int i = mr; i < n1; ++i) {
                        samples1[i].setS(samples1[i].s() - offset1 + l1);
                    }
                }

                // reverse samples1 if necessary
                if (reverse1) {
                    geometry::StrokeSample2dArray reversedSamples1(n1, core::noInit);
                    for (Int i = 0; i < n1; ++i) {
                        reversedSamples1[i] = samples1[n1 - 1 - i];
                        reversedSamples1[i].setS(l1 - reversedSamples1[i].s());
                    }
                    samples1 = std::move(reversedSamples1);
                }

                Int n = std::max<Int>(0, n0 - 2) + std::max<Int>(0, n1 - 2) + 1;

                core::Array<FreehandEdgePoint> points0;
                points0.reserve(n);
                points0.append(p0a);

                core::Array<FreehandEdgePoint> points1;
                points1.reserve(n);
                points1.append(p1a);

                double u0a = 0;
                Int i0 = std::min<Int>(1, n0);
                double u1a = 0;
                Int i1 = std::min<Int>(1, n1);

                for (Int i = 1; i < n; ++i) {
                    const geometry::StrokeSample2d& currentSample0 = samples0[i0];
                    const geometry::StrokeSample2d& currentSample1 = samples1[i1];
                    double u0b = currentSample0.s() / l0;
                    double u1b = currentSample1.s() / l1;
                    FreehandEdgePoint p0 = currentSample0;
                    FreehandEdgePoint p1 = currentSample1;
                    bool canIterate0 = i0 < n0 - 1;
                    bool canIterate1 = i1 < n1 - 1;
                    if (canIterate1 && (u0b > u1b || !canIterate0)) {
                        double t = (u1b - u0a) / (u0b - u0a);
                        points0.append(p0a.lerp(p0, t));
                        points1.append(p1);
                        u1a = u1b;
                        p1a = p1;
                        ++i1;
                    }
                    else if (canIterate0) {
                        double t = (u0b - u1a) / (u1b - u1a);
                        points0.append(p0);
                        points1.append(p1a.lerp(p1, t));
                        u0a = u0b;
                        p0a = p0;
                        ++i0;
                    }
                    else {
                        // shouldn't happen if `n` is correct.
                        break;
                    }
                }

                core::Array<vacomplex::KeyHalfedge> halfedges(
                    {{ke0, true}, {ke1, !reverse1}});

                core::Array<FreehandEdgePoint> newPoints(n, core::noInit);
                for (Int i = 0; i < n; ++i) {
                    newPoints[i] = points0[i].average(points1[i]);
                }

                double maxWidth = 0;
                for (Int i = 0; i < n; ++i) {
                    double w = newPoints[i].width();
                    if (w > maxWidth) {
                        maxWidth = w;
                    }
                }

                // tolerance = 5% of max width
                std::shared_ptr<vacomplex::KeyEdgeGeometry> newGeometry =
                    FreehandEdgeGeometry::createFromPoints(
                        newPoints, true, maxWidth * 0.05);

                vacomplex::Cell* result =
                    vacomplex::ops::glueKeyClosedEdges(halfedges, std::move(newGeometry));

                Element* e = this->findVacElement(result);
                if (e) {
                    e->domElement()->setAttribute(dom::strings::color, color);
                    resultId = e->id();
                }
            }
            else {
                // TODO: warning ?
            }
        }
    }

    sync();

    // Close history group
    if (undoGroup) {
        undoGroup->close();
    }

    return resultId;
}

core::Array<core::Id> Workspace::unglue(core::Span<core::Id> elementIds) {
    core::Array<core::Id> result;

    struct TargetEdge {
        vacomplex::KeyEdge* ke;
        core::Color color;
    };

    core::Array<vacomplex::KeyVertex*> kvs;
    core::Array<TargetEdge> edges;
    for (core::Id id : elementIds) {
        workspace::Element* element = find(id);
        if (!element) {
            continue;
        }
        vacomplex::Node* node = element->vacNode();
        if (!node || !node->isCell()) {
            continue;
        }
        vacomplex::Cell* cell = node->toCellUnchecked();
        switch (cell->cellType()) {
        case vacomplex::CellType::KeyVertex: {
            kvs.append(cell->toKeyVertexUnchecked());
            break;
        }
        case vacomplex::CellType::KeyEdge: {
            vacomplex::KeyEdge* ke = cell->toKeyEdgeUnchecked();
            core::Color color(1, 0, 0);
            dom::Element* sourceDomElement = element->domElement();
            if (sourceDomElement) {
                color = sourceDomElement->getAttribute(dom::strings::color).getColor();
            }
            else {
                // TODO: warn ?
            }
            edges.append(TargetEdge{ke, color});
            break;
        }
        default:
            break;
        }
    }

    if (kvs.isEmpty() && edges.isEmpty()) {
        return result;
    }

    // Open history group
    static core::StringId commandId = core::StringId("workspace.unglue");
    core::UndoGroup* undoGroup = nullptr;
    core::History* history = this->history();
    if (history) {
        undoGroup = history->createUndoGroup(commandId);
    }

    for (const TargetEdge& targetEdge : edges) {
        // TODO: use operation source in onVacNodesChanged_ to do the color copy
        vacomplex::KeyEdge* targetKe = targetEdge.ke;
        core::Array<vacomplex::KeyEdge*> ungluedKeyEdges =
            vacomplex::ops::unglueKeyEdges(targetKe);

        for (vacomplex::KeyEdge* ke : ungluedKeyEdges) {
            Element* e = this->findVacElement(ke);
            if (e) {
                result.append(e->id());
                dom::Element* domElement = e->domElement();
                if (domElement) {
                    domElement->setAttribute(dom::strings::color, targetEdge.color);
                }
                else {
                    // TODO: warn ?
                }
            }
        }
    }

    for (vacomplex::KeyVertex* targetKv : kvs) {
        std::unordered_map<core::Id, core::Color> starEdgeColors;
        for (vacomplex::Cell* cell : targetKv->star()) {
            vacomplex::KeyEdge* ke = cell->toKeyEdge();
            if (ke) {
                VacKeyEdge* e = dynamic_cast<VacKeyEdge*>(findVacElement(ke->id()));
                if (e && e->domElement()) {
                    starEdgeColors[ke->id()] =
                        e->domElement()->getAttribute(dom::strings::color).getColor();
                }
            }
        }

        core::Array<std::pair<core::Id, core::Array<vacomplex::KeyEdge*>>>
            ungluedKeyEdges;
        core::Array<vacomplex::KeyVertex*> ungluedKeyVertices =
            vacomplex::ops::unglueKeyVertices(targetKv, ungluedKeyEdges);

        // TODO: use operation source in onVacNodesChanged_ to do the color copy
        for (const auto& entry : ungluedKeyEdges) {
            core::Id id = entry.first;
            const core::Array<vacomplex::KeyEdge*>& kes = entry.second;
            auto it = starEdgeColors.find(id);
            core::Color color =
                it != starEdgeColors.end() ? it->second : core::Color(1, 0, 0);
            for (vacomplex::KeyEdge* ke : kes) {
                Element* e = this->findVacElement(ke);
                if (e) {
                    //result.append(e->id());
                    dom::Element* domElement = e->domElement();
                    if (domElement) {
                        domElement->setAttribute(dom::strings::color, color);
                    }
                    else {
                        // TODO: warn ?
                    }
                }
            }
        }
        for (vacomplex::KeyVertex* kv : ungluedKeyVertices) {
            Element* e = this->findVacElement(kv);
            if (e) {
                result.append(e->id());
            }
        }
    }

    sync();

    // Close history group
    if (undoGroup) {
        undoGroup->close();
    }

    return result;
}

dom::DocumentPtr Workspace::copy(core::Span<core::Id> elementIds) {

    core::Array<Element*> elements;
    for (core::Id id : elementIds) {
        Element* element = find(id);
        if (!element || elements.contains(element)) {
            continue;
        }
        elements.append(element);
    }

    // get full dependency chain
    core::Array<Element*> elementsToCopy;
    while (!elements.isEmpty()) {
        Element* element = elements.pop();
        elementsToCopy.append(element);
        for (Element* dependency : element->dependencies()) {
            if (!elements.contains(dependency) && !elementsToCopy.contains(dependency)) {
                elements.append(dependency);
            }
        }
    }

    core::Array<dom::Node*> domNodesToCopy;
    domNodesToCopy.reserve(elementsToCopy.length());
    for (Element* e : elementsToCopy) {
        dom::Element* domElement = e->domElement();
        // TODO: what to do if domElement is null?
        domNodesToCopy.append(domElement);
    }

    return dom::Document::copy(domNodesToCopy);
}

void Workspace::paste(dom::DocumentPtr document) {
    // TODO: use active group element
    document_->paste(document, document_->rootElement());
    sync();
}

std::unordered_map<core::StringId, Workspace::ElementCreator>&
Workspace::elementCreators_() {
    static std::unordered_map<core::StringId, Workspace::ElementCreator>* instance_ =
        new std::unordered_map<core::StringId, Workspace::ElementCreator>();
    return *instance_;
}

void Workspace::removeElement_(Element* element) {
    VGC_ASSERT(element);
    bool removed = removeElement_(element->id());
    VGC_ASSERT(removed);
}

bool Workspace::removeElement_(core::Id id) {

    // Find the element with the given ID. Fast return if not found.
    //
    auto it = elements_.find(id);
    if (it == elements_.end()) {
        return false;
    }
    Element* element = it->second.get();

    // Update parent-child relationship between elements.
    //
    element->unparent();
    if (rootVacElement_ == element) {
        rootVacElement_ = nullptr;
    }

    // Remove from error list and pending update lists.
    //
    if (element->hasError()) {
        elementsWithError_.removeOne(element);
    }
    if (element->hasPendingUpdate()) {
        elementsToUpdateFromDom_.removeOne(element);
    }

    // Remove from the elements_ map.
    //
    // Note: we temporarily keep a unique_ptr before calling erase() since the
    // element's destructor can indirectly use elements_ via callbacks (e.g.
    // onVacNodeDestroyed_).
    //
    std::unique_ptr<Element> uptr = std::move(it->second);
    elements_.erase(it);

    // Update dependencies and execute callbacks.
    //
    while (!element->dependents_.isEmpty()) {
        Element* dependent = element->dependents_.pop();
        dependent->dependencies_.removeOne(element);
        dependent->onDependencyRemoved_(element);
        element->onDependentElementRemoved_(dependent);
    }

    // Finally destruct the element.
    //
    uptr.reset();
    return true;
}

void Workspace::clearElements_() {
    // Note: elements_.clear() indirectly calls onVacNodeDestroyed_() and thus fills
    // elementsToUpdateFromDom_. So it is important to clear the latter after the former.
    for (const auto& p : elements_) {
        Element* e = p.second.get();
        while (!e->dependents_.isEmpty()) {
            Element* dependent = e->dependents_.pop();
            dependent->dependencies_.removeOne(e);
            dependent->onDependencyRemoved_(e);
            e->onDependentElementRemoved_(dependent);
        }
    }
    elements_.clear();
    rootVacElement_ = nullptr;
    elementsWithError_.clear();
    elementsToUpdateFromDom_.clear();
}

void Workspace::setPendingUpdateFromDom_(Element* element) {
    if (!element->hasPendingUpdate_) {
        element->hasPendingUpdate_ = true;
        elementsToUpdateFromDom_.emplaceLast(element);
    }
}

void Workspace::clearPendingUpdateFromDom_(Element* element) {
    if (element->hasPendingUpdate_) {
        element->hasPendingUpdate_ = false;
        elementsToUpdateFromDom_.removeOne(element);
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

void Workspace::debugPrintWorkspaceTree_() {
    visitDepthFirstPreOrder([](Element* e, Int depth) {
        VGC_DEBUG(
            LogVgcWorkspace,
            "{:>{}}<{} id=\"{}\">",
            "",
            depth * 2,
            e->tagName(),
            e->id());
    });
}

void Workspace::preUpdateDomFromVac_() {

    // Check whether the Workspace is properly synchronized with the DOM before
    // we attempt to update the DOM from the VAC.
    //
    if (document_->hasPendingDiff()) {
        VGC_ERROR(
            LogVgcWorkspace,
            "The topological complex has been edited while not being up to date with "
            "the latest changes in the document: the two may now be out of sync. "
            "This is probably caused by a missing document.emitPendingDiff().");
        flushDomDiff_();
        // TODO: rebuild from DOM instead of ignoring the pending diffs?
    }

    // Note: There is no need to do something like `isUpdatingDomFromVac =
    // true` here, because no signal is emitted from the DOM between
    // preUpdateDomFromVac_() and postUpdateDomFromVac_. Indeed, we only
    // call document->emitPendingDiff() in postUpdateDomFromVac_(), not
    // before.
}

void Workspace::postUpdateDomFromVac_() {

    // Now that we're done updating the DOM from the VAC, we emit the pending
    // DOM diff but ignore them. Indeed, the Workspace is the author of the
    // pending diff so there is no need to process them.
    //
    flushDomDiff_();

    // Inform the world that the Workspace content has changed.
    //
    changed().emit();

    // TODO: delay for batch VAC-to-DOM updates?
}

void Workspace::updateElementFromVac_(
    VacElement* element,
    vacomplex::NodeModificationFlags flags) {

    element->updateFromVac_(flags);
    // Note: VAC always has a correct state so this element should
    //       be errorless after being updated from VAC.
    element->status_ = ElementStatus::Ok;
}

void Workspace::rebuildDomFromWorkspaceTree_() {
    // todo later
    throw core::RuntimeError("not implemented");
}

void Workspace::onVacNodesChanged_(const vacomplex::ComplexDiff& diff) {

    if (isUpdatingVacFromDom_) {
        // If a VAC node has been destroyed as a side-effect of updating the VAC
        // from the DOM, it can mean that some Workspace elements should now
        // be mark as corrupted.
        //
        // Example scenario:
        //
        // 1. User deletes a <vertex> element in the DOM Editor, despite
        //    an existing <edge> element using it as end vertex.
        //
        // 2. This modifies the dom::Document, calls doc->emitPendingDiff(),
        //    which in turn calls Workspace::onDocumentDiff().
        //
        // 3. The Workspace retrieves the corresponding vertexElement Workspace element
        //    and vertexNode VAC Node.
        //
        // 4. The Workspace destroys and unregisters the vertexElement.
        //
        // 5. The Workspace calls ops::hardDelete(vertexNode)
        //
        // 6. The VAC diff contains destroyed node ids:
        //    - vertexNodeId  -> destroyed explicitly:     OK
        //    - edgeNodeId    -> destroyed as side-effect: now corrupted
        //
        for (const auto& info : diff.destroyedNodes()) {
            VacElement* vacElement = findVacElement(info.nodeId());
            if (vacElement && vacElement->vacNode_) {
                // If we find the element despite being updating VAC from DOM,
                // this means the element is now corrupted, so mark it as such.
                //
                elementByVacInternalId_.erase(vacElement->vacNode_->id());
                vacElement->vacNode_ = nullptr;
                setPendingUpdateFromDom_(vacElement);

                // TODO: only clear graphics and actually append to corrupt list
            }
        }
        return;
    }

    // Process destroyed vac nodes.
    // Corresponding workspace element are kept until the end of this function but
    // their vacNode_ pointer must be set to null.
    //
    for (const auto& info : diff.destroyedNodes()) {
        VacElement* vacElement = findVacElement(info.nodeId());
        if (vacElement) {
            vacElement->vacNode_ = nullptr;
        }
    }

    preUpdateDomFromVac_();

    // Process created vac nodes
    //
    core::Array<VacElement*> createdElements;
    createdElements.reserve(diff.createdNodes().length());
    for (const auto& info : diff.createdNodes()) {
        vacomplex::Node* node = info.node();

        // TODO: check id conflict

        // TODO: add constructors expecting operationSourceNodes

        // Create the workspace element
        std::unique_ptr<Element> u = {};
        if (node->isGroup()) {
            //vacomplex::Group* group = node->toGroup();
            u = makeUniqueElement<Layer>(this);
        }
        else {
            vacomplex::Cell* cell = node->toCell();
            switch (cell->cellType()) {
            case vacomplex::CellType::KeyVertex:
                u = makeUniqueElement<VacKeyVertex>(this);
                break;
            case vacomplex::CellType::KeyEdge:
                u = makeUniqueElement<VacKeyEdge>(this);
                break;
            case vacomplex::CellType::KeyFace:
                u = makeUniqueElement<VacKeyFace>(this);
                break;
            case vacomplex::CellType::InbetweenVertex:
            case vacomplex::CellType::InbetweenEdge:
            case vacomplex::CellType::InbetweenFace:
                break;
            }
        }
        if (!u) {
            // TODO: error ?
            continue;
        }

        VacElement* element = u->toVacElement();
        if (!element) {
            // TODO: error ?
            continue;
        }

        // By default insert is as last child of root.
        // There should be a NodeInsertionInfo in the diff that will
        // let us move it to its final destination later in this function.
        rootVacElement_->appendChild(element);

        // Create the DOM element (as last child of root too)

        dom::ElementPtr domElement = dom::Element::create(
            document_->rootElement(), element->domTagName().value(), nullptr);
        const core::Id id = domElement->internalId();

        const auto& p = elements_.emplace(id, std::move(u));
        if (!p.second) {
            // TODO: throw ?
            continue;
        }

        element->domElement_ = domElement.get();
        element->id_ = id;
        element->setVacNode(node);

        createdElements.append(element);
    }

    // Process transient vac nodes
    //
    for (const auto& info : diff.transientNodes()) {
        core::Id nodeId = info.nodeId();

        // TODO: check id conflict

        // TODO: add constructors expecting operationSourceNodes

        // Create the workspace element
        std::unique_ptr<Element> u = makeUniqueElement<TransientVacElement>(this);

        VacElement* element = u->toVacElement();
        if (!element) {
            // TODO: error ?
            continue;
        }

        // By default insert is as last child of root.
        // There should be a NodeInsertionInfo in the diff that will
        // let us move it to its final destination later in this function.
        rootVacElement_->appendChild(element);

        dom::ElementPtr domElement =
            dom::Element::create(document_->rootElement(), "transient", nullptr);
        const core::Id id = domElement->internalId();

        const auto& p = elements_.emplace(id, std::move(u));
        if (!p.second) {
            // TODO: throw ?
            continue;
        }

        element->domElement_ = domElement.get();
        element->id_ = id;
        element->status_ = ElementStatus::Ok;

        // Note: One must not forget to remove it manually when destroying
        //       the transient element before leaving this function.
        elementByVacInternalId_.emplace(nodeId, element);
    }

    // Process insertions
    for (const auto& info : diff.insertions()) {
        VacElement* vacElement = findVacElement(info.nodeId());
        if (!vacElement) {
            VGC_ERROR(
                LogVgcWorkspace,
                "No matching workspace::VacElement found for "
                "nodeInsertionInfo->nodeId().");
            continue;
        }
        dom::Element* domElement = vacElement->domElement();

        VacElement* vacParentElement = findVacElement(info.newParentId());
        if (!vacParentElement) {
            VGC_ERROR(
                LogVgcWorkspace,
                "No matching workspace::VacElement found for "
                "nodeInsertionInfo->newParentId().");
            continue;
        }
        dom::Element* domParentElement = vacParentElement->domElement();

        switch (info.type()) {
        case vacomplex::NodeInsertionType::BeforeSibling: {
            VacElement* vacSiblingElement = findVacElement(info.newSiblingId());
            if (!vacSiblingElement) {
                VGC_ERROR(
                    LogVgcWorkspace,
                    "No matching workspace::VacElement found for "
                    "nodeInsertionInfo->newSiblingId().");
                continue;
            }
            dom::Element* domSiblingElement = vacSiblingElement->domElement();
            vacParentElement->insertChildUnchecked(vacSiblingElement, vacElement);
            domParentElement->insertChild(domSiblingElement, domElement);
            break;
        }
        case vacomplex::NodeInsertionType::AfterSibling: {
            VacElement* vacSiblingElement = findVacElement(info.newSiblingId());
            if (!vacSiblingElement) {
                VGC_ERROR(
                    LogVgcWorkspace,
                    "No matching workspace::VacElement found for "
                    "nodeInsertionInfo->newSiblingId().");
                continue;
            }
            dom::Element* domSiblingElement = vacSiblingElement->domElement();
            vacParentElement->insertChildUnchecked(
                vacSiblingElement->nextSibling(), vacElement);
            domParentElement->insertChild(domSiblingElement->nextSibling(), domElement);
            break;
        }
        case vacomplex::NodeInsertionType::FirstChild: {
            vacParentElement->insertChildUnchecked(
                vacParentElement->firstChild(), vacElement);
            domParentElement->insertChild(domParentElement->firstChild(), domElement);
            break;
        }
        case vacomplex::NodeInsertionType::LastChild: {
            vacParentElement->insertChildUnchecked(nullptr, vacElement);
            domParentElement->insertChild(nullptr, domElement);
            break;
        }
        }
    }

    // Update created vac nodes
    //
    for (VacElement* element : createdElements) {
        updateElementFromVac_(element, vacomplex::NodeModificationFlag::All);
    }

    // Process modified vac nodes
    //
    for (const auto& info : diff.modifiedNodes()) {
        VacElement* vacElement = findVacElement(info.nodeId());
        if (!vacElement) {
            VGC_ERROR(LogVgcWorkspace, "Unexpected vacomplex::Node");
            // TODO: recover from error by creating the Cell in workspace and DOM ?
            continue;
        }
        updateElementFromVac_(vacElement, info.flags());
    }

    // Process destroyed vac nodes
    //
    // XXX What if node is the root node?
    //
    // Currently, in the Workspace(document) constructor, we first create a
    // vacomplex::Complex (which creates a root node), then call
    // rebuildFromDom(), which first clears the Complex (destructing the root
    // node), then creates the nodes including the root node. Therefore, this
    // function is called in the Workspace(document) constructor, which is
    // probably not ideal.
    //
    // Get Workspace element corresponding to the destroyed VAC node. Nothing
    // to do if there is no such element.
    //
    // Note: transient vac nodes are included.
    //
    for (const auto& info : diff.destroyedNodes()) {
        VacElement* vacElement = findVacElement(info.nodeId());
        if (vacElement) {
            // Delete the corresponding DOM element if any.
            //
            dom::Element* domElement = vacElement->domElement();
            if (domElement) {
                domElement->remove();
            }

            // Delete the Workspace element. We need to set its vacNode_ to
            // nullptr before removal since the destructor of the VacElement
            // will hardDelete() its vacNode_ if any.
            //
            elementByVacInternalId_.erase(info.nodeId());
            vacElement->vacNode_ = nullptr;
            removeElement_(vacElement);
        }
    }

    postUpdateDomFromVac_();
}

void Workspace::onDocumentDiff_(const dom::Diff& diff) {
    if (numDocumentDiffToSkip_ > 0) {
        --numDocumentDiffToSkip_;
    }
    else {
        updateVacFromDom_(diff);
    }
}

// Flushing ensures that the DOM doesn't contain pending diff, by emitting
// but ignoring them.
//
void Workspace::flushDomDiff_() {
    if (document_->hasPendingDiff()) {
        ++numDocumentDiffToSkip_;
        document_->emitPendingDiff();
    }
}

// This method creates the workspace element corresponding to a given
// DOM element, but without initializing it yet (that is, it doesn't
// create the corresponding VAC element)
//
// Initialization is perform later, by calling updateElementFromDom(element).
//
Element*
Workspace::createAppendElementFromDom_(dom::Element* domElement, Element* parent) {
    if (!domElement) {
        return nullptr;
    }

    std::unique_ptr<Element> createdElementPtr = {};
    auto& creators = elementCreators_();
    auto it = creators.find(domElement->tagName());
    if (it != creators.end()) {
        auto& creator = it->second;
        createdElementPtr = std::invoke(creator, this);
        if (!createdElementPtr) {
            VGC_ERROR(
                LogVgcWorkspace,
                "Element creator for \"{}\" failed to create the element.",
                domElement->tagName());
            // XXX throw or fallback to UnsupportedElement or nullptr ?
            createdElementPtr = std::make_unique<UnsupportedElement>(this);
        }
    }
    else {
        createdElementPtr = std::make_unique<UnsupportedElement>(this);
    }

    core::Id id = domElement->internalId();
    Element* createdElement = createdElementPtr.get();
    bool emplaced = elements_.try_emplace(id, std::move(createdElementPtr)).second;
    if (!emplaced) {
        // TODO: should probably throw
        return nullptr;
    }
    createdElement->domElement_ = domElement;
    createdElement->id_ = id;

    if (parent) {
        parent->appendChild(createdElement);
    }

    setPendingUpdateFromDom_(createdElement);

    return createdElement;
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

void Workspace::rebuildWorkspaceTreeFromDom_() {

    VGC_ASSERT(document_);
    VGC_ASSERT(vac_);

    // reset tree
    clearElements_();

    // reset VAC
    {
        core::BoolGuard bgVac(isUpdatingVacFromDom_);
        vac_->clear();
        //vac_->emitPendingDiff();
    }

    flushDomDiff_();

    dom::Element* domVgcElement = document_->rootElement();
    if (!domVgcElement || domVgcElement->tagName() != dom::strings::vgc) {
        return;
    }

    Element* vgcElement = createAppendElementFromDom_(domVgcElement, nullptr);
    VGC_ASSERT(vgcElement);
    rootVacElement_ = vgcElement->toVacElement();
    VGC_ASSERT(rootVacElement_);

    Element* p = nullptr;
    Element* e = rootVacElement_;
    dom::Element* domElement = rebuildTreeFromDomIter(e, p);
    while (domElement) {
        e = createAppendElementFromDom_(domElement, p);
        domElement = rebuildTreeFromDomIter(e, p);
    }

    // children should already be in the correct order.
}

void Workspace::rebuildVacFromWorkspaceTree_() {

    VGC_ASSERT(rootVacElement_);
    VGC_ASSERT(vac_);

    core::BoolGuard bgVac(isUpdatingVacFromDom_);

    // reset vac
    vac_->clear();
    vac_->resetRoot();
    rootVacElement_->setVacNode(vac_->rootGroup());

    Element* root = rootVacElement_;
    Element* element = root->firstChild();
    Int depth = 1;
    while (element) {
        updateElementFromDom(element);
        iterDfsPreOrder(element, depth, root);
    }

    updateVacChildrenOrder_();
}

void Workspace::updateVacChildrenOrder_() {
    // todo: sync children order in all groups
    Element* root = rootVacElement_;
    Element* element = root;

    Int depth = 0;
    while (element) {
        vacomplex::Node* node = element->vacNode();
        if (node) {
            if (node->isGroup()) {
                vacomplex::Group* group = static_cast<vacomplex::Group*>(node);
                // synchronize first common child
                VacElement* childVacElement = element->firstChildVacElement();
                while (childVacElement) {
                    if (childVacElement->vacNode()) {
                        vacomplex::ops::moveToGroup(
                            childVacElement->vacNode(), group, group->firstChild());
                        break;
                    }
                    childVacElement = childVacElement->nextSiblingVacElement();
                }
            }

            if (node->parentGroup()) {
                // synchronize next sibling element's node as node's next sibling
                VacElement* nextSiblingVacElement = element->nextSiblingVacElement();
                while (nextSiblingVacElement) {
                    vacomplex::Node* nextSiblingElementNode =
                        nextSiblingVacElement->vacNode();
                    if (nextSiblingElementNode) {
                        vacomplex::ops::moveToGroup(
                            nextSiblingElementNode,
                            node->parentGroup(),
                            node->nextSibling());
                        break;
                    }
                    nextSiblingVacElement =
                        nextSiblingVacElement->nextSiblingVacElement();
                }
            }
        }

        iterDfsPreOrder(element, depth, root);
    }
}

//bool Workspace::haveKeyEdgeBoundaryPathsChanged_(Element* e) {
//    vacomplex::Node* node = e->vacNode();
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
//    vacomplex::Node* node = vacNode();
//    vacomplex::KeyEdge* kv = node ? node->toCellUnchecked()->toKeyEdgeUnchecked() : nullptr;
//    if (!kv) {
//        return false;
//    }
//
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

void Workspace::updateVacFromDom_(const dom::Diff& diff) {

    VGC_ASSERT(document_);

    core::BoolGuard bgVac(isUpdatingVacFromDom_);

    // impl goal: we want to keep as much cached data as possible.
    //            we want the VAC to be valid -> using only its operators
    //            limits bugs to their implementation.

    bool hasModifiedPaths =
        (diff.removedNodes().length() > 0) || (diff.reparentedNodes().size() > 0);
    bool hasNewPaths = (diff.createdNodes().length() > 0);

    std::set<Element*> parentsToOrderSync;

    // First, we remove what has to be removed.
    //
    // Note that if `element` is a VacElement, then:
    // - removeElement_(element) calls vacomplex::ops::hardDelete(node),
    // - which may destroy other VAC nodes,
    // - which will invoke onVacNodeDestroyed(otherNodeId).
    //
    for (dom::Node* node : diff.removedNodes()) {
        dom::Element* domElement = dom::Element::cast(node);
        if (!domElement) {
            continue;
        }
        Element* element = find(domElement);
        if (element) {
            Element* parent = element->parent();
            VGC_ASSERT(parent);
            // reparent children to element's parent
            for (Element* child : *element) {
                parent->appendChild(child);
            }
            removeElement_(element);
        }
    }

    // Create new elements, but for now uninitialized and in a potentially
    // incorrect child ordering.
    //
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
            //            and <vgc> element should already exist.
            continue;
        }
        createAppendElementFromDom_(domElement, parent);
        parentsToOrderSync.insert(parent);
    }

    // Collect all parents with reordered children.
    //
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

    // Update children order between workspace elements.
    //
    for (Element* element : parentsToOrderSync) {
        Element* child = element->firstChild();
        dom::Element* domChild = element->domElement()->firstChildElement();
        while (domChild) {
            if (!child || child->domElement() != domChild) {
                Element* missingChild = find(domChild);
                while (!missingChild) {
                    domChild = domChild->nextSiblingElement();
                    if (!domChild) {
                        break;
                    }
                    missingChild = find(domChild);
                }
                if (!domChild) {
                    break;
                }
                element->insertChildUnchecked(child, missingChild);
                child = missingChild;
            }
            child = child->nextSibling();
            domChild = domChild->nextSiblingElement();
        }
    }

    if (hasNewPaths || hasModifiedPaths) {
        // Flag all elements with error for update.
        for (Element* element : elementsWithError_) {
            setPendingUpdateFromDom_(element);
        }
    }

    if (hasModifiedPaths) {
        // Update everything for now.
        // TODO: An element dependent on a Path should have it in its dependencies
        // so we could force path reevaluation to the dependents of an element that moved,
        // as well as errored elements.
        Element* root = rootVacElement_;
        Element* element = root;
        Int depth = 0;
        while (element) {
            setPendingUpdateFromDom_(element);
            iterDfsPreOrder(element, depth, root);
        }
    }
    else {
        // Otherwise we update the elements flagged as modified
        for (const auto& it : diff.modifiedElements()) {
            Element* element = find(it.first);
            // If the element has already an update pending it will be
            // taken care of in the update loop further below.
            if (element) {
                setPendingUpdateFromDom_(element);
                // TODO: pass the set of modified attributes ids to the Element
            }
        }
    }

    // Now that all workspace elements are created (or removed), we finally
    // update all the elements by calling their updateFromDom() virtual method,
    // which creates their corresponding VAC node if any, and transfers all the
    // attributes from the DOM element to the workspace element and/or VAC
    // node.
    //
    while (!elementsToUpdateFromDom_.isEmpty()) {
        // There is no need to pop the element since updateElementFromDom is
        // in charge of removing it from the list when updated.
        Element* element = elementsToUpdateFromDom_.last();
        updateElementFromDom(element);
    }

    // Update children order between VAC nodes.
    //
    updateVacChildrenOrder_();

    // Update Version ID and notify that the workspace changed.
    //
    lastSyncedDomVersionId_ = document_->versionId();
    changed().emit();
}

//void Workspace::updateTreeAndDomFromVac_(const vacomplex::Diff& /*diff*/) {
//    if (!document_) {
//        VGC_ERROR(LogVgcWorkspace, "DOM is null.")
//        return;
//    }
//
//    detail::ScopedTemporaryBoolSet bgDom(isCreatingDomElementsFromVac_);
//
//    // todo later
//    throw core::RuntimeError("not implemented");
//}

} // namespace vgc::workspace
