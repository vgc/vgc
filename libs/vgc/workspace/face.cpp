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

#include <vgc/workspace/face.h>

#include <tesselator.h> // libtess2

#include <vgc/core/span.h>
#include <vgc/workspace/edge.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

namespace {

struct DomCycleComponent {
    dom::Path path = {};
    bool direction = true;

    template<typename OStream>
    friend void write(OStream& out, const DomCycleComponent& component) {
        write(out, component.path);
        if (!component.direction) {
            write(out, '*');
        }
    }
};

template<typename IStream>
void readTo(DomCycleComponent& component, IStream& in) {
    readTo(v.path, in);
    char c = -1;
    if (in.get(c)) {
        if (c == '*') {
            component.direction = false;
        }
        else {
            in.unget();
        }
    }
}

struct DomCycle {
    core::Array<DomCycleComponent> components;

    template<typename OStream>
    friend void write(OStream& out, const DomCycle& cycle) {
        bool first = true;
        for (const DomCycleComponent& component : cycle.components) {
            if (!first) {
                write(out, ' ');
            }
            else {
                first = false;
            }
            write(out, component);
        }
    }
};

template<typename IStream>
void readTo(DomCycle& cycle, IStream& in) {
    char c = -1;
    bool got = false;
    readTo(cycle.components.emplaceLast(), in);
    core::skipWhitespaceCharacters(in);
    got = bool(in.get(c));
    while (got && dom::isValidPathFirstChar(c)) {
        in.unget();
        readTo(cycle.components.emplaceLast(), in);
        core::skipWhitespaceCharacters(in);
        got = bool(in.get(c));
    }
    if (got) {
        in.unget();
    }
}

} // namespace

bool VacFaceCellFrameData::isSelectableAt(
    const geometry::Vec2d& position,
    bool /*outlineOnly*/,
    double tol,
    double* outDistance) const {

    using Vec2f = geometry::Vec2f;
    using Vec2d = geometry::Vec2d;

    if (bbox_.isEmpty()) {
        return false;
    }

    geometry::Rect2d inflatedBbox = bbox_;
    inflatedBbox.setPMin(inflatedBbox.pMin() - Vec2d(tol, tol));
    inflatedBbox.setPMax(inflatedBbox.pMax() + Vec2d(tol, tol));
    if (!inflatedBbox.contains(position)) {
        return false;
    }

    geometry::Vec2f positionF(position);

    bool isContained = false;
    if (inflatedBbox.contains(position)) {
        for (Int i = 0; i < triangulation_.length() - 5; i += 6) {
            Vec2f v0(triangulation_[i + 0], triangulation_[i + 1]);
            Vec2f v1(triangulation_[i + 2], triangulation_[i + 3]);
            Vec2f v2(triangulation_[i + 4], triangulation_[i + 5]);
            bool b0 = (v1 - v0).det(positionF - v0) > 0;
            bool b1 = (v2 - v1).det(positionF - v1) > 0;
            if (b0 != b1) {
                continue;
            }
            bool b2 = (v0 - v2).det(positionF - v2) > 0;
            if (b2 == b0) {
                isContained = true;
                break;
            }
        }
    }

    if (isContained) {
        if (outDistance) {
            *outDistance = 0;
        }
        return true;
    }

    return false;
}

VacKeyFace::~VacKeyFace() {
}

std::optional<core::StringId> VacKeyFace::domTagName() const {
    return dom::strings::face;
}

geometry::Rect2d VacKeyFace::boundingBox(core::AnimTime t) const {
    if (frameData_.time() == t) {
        return frameData_.bbox_;
    }
    return geometry::Rect2d::empty;
}

bool VacKeyFace::isSelectableAt(
    const geometry::Vec2d& position,
    bool outlineOnly,
    double tol,
    double* outDistance,
    core::AnimTime t) const {

    if (frameData_.time() == t) {
        return frameData_.isSelectableAt(position, outlineOnly, tol, outDistance);
    }
    return false;
}

const VacFaceCellFrameData* VacKeyFace::computeFrameData() {
    bool success = computeFillMesh_();
    return success ? &frameData_ : nullptr;
}

const VacFaceCellFrameData* VacKeyFace::computeFrameDataAt(core::AnimTime t) {
    if (frameData_.time() == t) {
        return computeFrameData();
    }
    return nullptr;
}

void VacKeyFace::onPaintPrepare(core::AnimTime /*t*/, PaintOptions /*flags*/) {
    // todo, use paint options to not compute everything or with lower quality
    computeFillMesh_();
}

void VacKeyFace::onPaintDraw(
    graphics::Engine* engine,
    core::AnimTime t,
    PaintOptions flags) const {

    VacKeyFaceFrameData& data = frameData_;
    vacomplex::KeyFace* ke = vacKeyFaceNode();
    if (!ke || t != ke->time()) {
        return;
    }

    // if not already done (should we leave preparePaint_ optional?)
    const_cast<VacKeyFace*>(this)->computeFillMesh_();

    using namespace graphics;
    namespace ds = dom::strings;

    // XXX "implicit" cells' domElement would be the composite ?

    constexpr PaintOptions fillOptions = {PaintOption::Selected, PaintOption::Draft};

    // XXX todo: reuse geometry objects, create buffers separately (attributes waiting in FaceGraphics).
    FaceGraphics& graphics = frameData_.graphics_;
    core::Color color = frameData_.color_;

    bool hasNewFillGraphics = false;
    if ((flags.hasAny(fillOptions) || !flags.has(PaintOption::Outline))
        && !graphics.fillGeometry()) {

        hasNewFillGraphics = true;

        graphics.setFillGeometry(
            engine->createDynamicTriangleListView(BuiltinGeometryLayout::XY_iRGBA));

        engine->updateBufferData(
            graphics.fillGeometry()->vertexBuffer(0), //
            frameData_.triangulation_);
    }
    if (graphics.fillGeometry() && (data.hasPendingColorChange_ || hasNewFillGraphics)) {
        engine->updateBufferData(
            graphics.fillGeometry()->vertexBuffer(1), //
            core::Array<float>({color.r(), color.g(), color.b(), color.a()}));
    }

    data.hasPendingColorChange_ = false;

    if (flags.has(PaintOption::Selected)) {
        // TODO: use special shader for selected faces
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(graphics.fillGeometry());
    }
    else if (!flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(graphics.fillGeometry());
    }

    // draws nothing if non-selected and in outline mode
}

ElementStatus
VacKeyFace::onDependencyChanged_(Element* /*dependency*/, ChangeFlags changes) {
    ElementStatus status = this->status();
    if (status == ElementStatus::Ok) {
        if (changes.has(ChangeFlag::EdgePreJoinGeometry)) {
            dirtyFillMesh_();
        }
    }
    return status;
}

ElementStatus VacKeyFace::onDependencyRemoved_(Element* /*dependency*/) {
    ElementStatus status = this->status();
    if (status == ElementStatus::Ok) {
        status = ElementStatus::UnresolvedDependency;
    }
    return status;
}

ElementStatus VacKeyFace::updateFromDom_(Workspace* workspace) {
    namespace ds = dom::strings;
    dom::Element* const domElement = this->domElement();
    if (!domElement) {
        // todo: use owning composite when it is implemented
        return ElementStatus::Ok;
    }

    // always update dependencies first

    std::string_view cyclesDescription = domElement->getAttribute(ds::cycles).getString();

    core::Array<DomCycle> domCycles;
    core::StringReader cyclesDescriptionReader(cyclesDescription);
    readTo(domCycles, cyclesDescriptionReader);

    bool hasBoundaryChanged = false;
    bool hasUnresolvedDependency = false;
    bool hasErrorInDependency = false;
    bool hasInvalidAttribute = false;

    core::Array<Element*> newDependencies = {};
    core::Array<vacomplex::KeyCycle> cycles = {};
    auto cyclesElementsSequenceIterator = cyclesElementsSequence_.begin();
    core::Array<Element*> newCyclesElementsSequence;
    for (auto& domCycle : domCycles) {
        core::Array<DomCycleComponent>& components = domCycle.components;
        bool first = true;
        bool isSteiner = false;
        bool isNotHalfedgeCycle = false;
        core::Array<vacomplex::KeyHalfedge> halfedges;
        for (auto& component : components) {
            dom::Element* domComponentElement =
                domElement->getElementFromPath(component.path);
            Element* componentElement = workspace->find(domComponentElement);
            if (cyclesElementsSequenceIterator == cyclesElementsSequence_.end()) {
                hasBoundaryChanged = true;
            }
            else {
                if (*cyclesElementsSequenceIterator != componentElement) {
                    hasBoundaryChanged = true;
                }
                ++cyclesElementsSequenceIterator;
            }
            newCyclesElementsSequence.emplaceLast(componentElement);
            if (!componentElement) {
                hasUnresolvedDependency = true;
                continue;
            }
            // add as dependency even if it is invalid in a cycle
            // this allows updating the path if the dependency is moved
            {
                // insert in sorted array
                auto it = newDependencies.begin();
                for (; it != newDependencies.end(); ++it) {
                    if (*it >= componentElement) {
                        break;
                    }
                }
                if (it == newDependencies.end() || *it != componentElement) {
                    newDependencies.emplace(it, componentElement);
                }
            }
            if (isSteiner) {
                // steiner cycle with more elements than the vertex
                hasInvalidAttribute = true;
            }
            if (isNotHalfedgeCycle) {
                continue;
            }
            VacKeyEdge* keElement = dynamic_cast<VacKeyEdge*>(componentElement);
            if (keElement) {
                workspace->updateElementFromDom(keElement);
                vacomplex::KeyEdge* ke = keElement->vacKeyEdgeNode();
                if (ke && !keElement->hasError()) {
                    halfedges.emplaceLast(ke, component.direction);
                }
                else {
                    hasErrorInDependency = true;
                    isNotHalfedgeCycle = true;
                }
            }
            else {
                isNotHalfedgeCycle = true;
                if (first) {
                    first = false;
                    VacKeyVertex* kvElement =
                        dynamic_cast<VacKeyVertex*>(componentElement);
                    if (kvElement) {
                        workspace->updateElementFromDom(kvElement);
                        vacomplex::KeyVertex* kv = kvElement->vacKeyVertexNode();
                        if (kv && !kvElement->hasError()) {
                            cycles.emplaceLast(kv);
                            isSteiner = true;
                        }
                        else {
                            hasErrorInDependency = true;
                        }
                    }
                    else {
                        hasInvalidAttribute = true;
                    }
                }
                else {
                    hasInvalidAttribute = true;
                }
            }
        }
        if (!isNotHalfedgeCycle) {
            cycles.emplaceLast(std::move(halfedges));
        }
        // add cycle separator in the form of a null element
        newCyclesElementsSequence.emplaceLast(nullptr);
        if (cyclesElementsSequenceIterator != cyclesElementsSequence_.end()) {
            ++cyclesElementsSequenceIterator;
        }
    }
    if (cyclesElementsSequenceIterator != cyclesElementsSequence_.end()) {
        hasBoundaryChanged = true;
    }
    cyclesElementsSequence_ = std::move(newCyclesElementsSequence);

    updateDependencies_(newDependencies);

    if (hasUnresolvedDependency) {
        onUpdateError_();
        return ElementStatus::UnresolvedDependency;
    }
    if (hasErrorInDependency) {
        onUpdateError_();
        return ElementStatus::ErrorInDependency;
    }
    if (hasInvalidAttribute) {
        onUpdateError_();
        return ElementStatus::InvalidAttribute;
    }

    // update group
    vacomplex::Group* parentGroup = nullptr;
    Element* parentElement = parent();
    if (parentElement) {
        workspace->updateElementFromDom(parentElement);
        vacomplex::Node* parentNode = parentElement->vacNode();
        if (parentNode) {
            // checked cast to group, could be something invalid
            parentGroup = parentNode->toGroup();
        }
    }
    if (!parentGroup) {
        onUpdateError_();
        return ElementStatus::ErrorInParent;
    }

    vacomplex::KeyFace* kf = vacKeyFaceNode();

    ChangeFlags changeFlags = {};

    if (kf && hasBoundaryChanged) {
        // must rebuild
        removeVacNode();
        kf = nullptr;
    }

    // create/rebuild/update vac node
    if (!kf) {
        kf = topology::ops::createKeyFace(cycles, parentGroup);
        if (!kf) {
            onUpdateError_();
            return ElementStatus::InvalidAttribute;
        }
        setVacNode(kf);
    }

    bool colorChanged = false;

    const auto& color = domElement->getAttribute(ds::color).getColor();
    if (frameData_.color_ != color) {
        frameData_.color_ = color;
        colorChanged = true;
        changeFlags.set(ChangeFlag::Style);
    }

    // dirty cached data
    if (hasBoundaryChanged) {
        changeFlags.set(ChangeFlag::FaceFillMesh);
        dirtyFillMesh_();
    }
    else if (colorChanged) {
        frameData_.hasPendingColorChange_ = true;
    }

    if (changeFlags) {
        notifyChangesToDependents(changeFlags);
    }

    return ElementStatus::Ok;
}

void VacKeyFace::updateFromVac_(vacomplex::NodeDiffFlags diffs) {
    namespace ds = dom::strings;
    vacomplex::KeyFace* kf = vacKeyFaceNode();
    if (!kf) {
        if (status() != ElementStatus::Ok) {
            // Element is already corrupt, no need to fail loudly.
            return;
        }
        // TODO: error or throw ?
        return;
    }

    dom::Element* const domElement = this->domElement();
    if (!domElement) {
        // TODO: use owning composite when implemented
        return;
    }

    using vacomplex::NodeDiffFlag;
    if (diffs.hasAny({NodeDiffFlag::BoundaryChanged, NodeDiffFlag::Created})) {
        dirtyFillMesh_();
        // rebuild cycles attribute
        core::Array<DomCycle> domCycles;
        cyclesElementsSequence_.clear();
        for (auto& cycle : kf->cycles()) {
            DomCycle& domCycle = domCycles.emplaceLast();
            vacomplex::KeyVertex* steinerVertex = cycle.steinerVertex();
            if (steinerVertex) {
                // XXX what if a pointer is nullptr here ?
                VacElement* componentElement = workspace()->findVacElement(steinerVertex);
                cyclesElementsSequence_.emplaceLast(componentElement);
                dom::Element* domComponentElement = componentElement->domElement();
                // XXX maybe it had a relative path before ?
                domCycle.components.emplaceLast(
                    DomCycleComponent{domComponentElement->getPathFromId(), false});
            }
            else {
                for (auto& he : cycle.halfedges()) {
                    // XXX what if a pointer is nullptr here ?
                    VacElement* componentElement = workspace()->findVacElement(he.edge());
                    cyclesElementsSequence_.emplaceLast(componentElement);
                    dom::Element* domComponentElement = componentElement->domElement();
                    // XXX maybe it had a relative path before ?
                    domCycle.components.emplaceLast(DomCycleComponent{
                        domComponentElement->getPathFromId(), he.direction()});
                }
            }
            // add cycle separator in the form of a null element
            cyclesElementsSequence_.emplaceLast(nullptr);
        }
        std::string cyclesDescription;
        core::StringWriter cyclesDescriptionWriter(cyclesDescription);
        write(cyclesDescriptionWriter, domCycles);
        domElement->setAttribute(ds::cycles, std::move(cyclesDescription));
    }
}

void VacKeyFace::updateDependencies_(core::Array<Element*> sortedNewDependencies) {
    core::Array<Element*> oldDependencies = dependencies();
    std::sort(oldDependencies.begin(), oldDependencies.end());
    {
        // TODO: needs testing
        auto oldDependenciesIt = oldDependencies.begin();
        for (Element* newDependency : sortedNewDependencies) {
            bool found = false;
            for (; oldDependenciesIt != oldDependencies.end(); ++oldDependenciesIt) {
                if (*oldDependenciesIt >= newDependency) {
                    if (*oldDependenciesIt == newDependency) {
                        found = true;
                    }
                    ++oldDependenciesIt;
                    break;
                }
                removeDependency(*oldDependenciesIt);
            }
            if (!found) {
                addDependency(newDependency);
            }
        }
    }
}

bool VacKeyFace::computeFillMesh_() {
    VacKeyFaceFrameData& data = frameData_;
    if (data.isFillMeshComputed_ || data.isComputing_) {
        return false;
    }
    VGC_ASSERT(!data.isComputing_);

    using namespace topology;
    using geometry::Vec2d;
    using geometry::Vec2f;

    KeyFace* kf = vacKeyFaceNode();
    if (!kf) {
        return false;
    }

    data.isComputing_ = true;
    data.bbox_ = geometry::Rect2d::empty;

    geometry::Curves2d curves2d;

    for (const vacomplex::KeyCycle& cycle : kf->cycles()) {
        KeyVertex* kv = cycle.steinerVertex();
        if (kv) {
            Vec2d p = kv->position();
            curves2d.moveTo(p[0], p[1]);
        }
        else {
            bool isFirst = true;
            for (const KeyHalfedge& khe : cycle.halfedges()) {
                const KeyEdge* ke = khe.edge();
                Element* ve = workspace()->findVacElement(ke->id());
                VacKeyEdge* vke = dynamic_cast<VacKeyEdge*>(ve);
                const VacEdgeCellFrameData* edgeData = nullptr;
                if (vke) {
                    edgeData =
                        vke->computeFrameData(VacEdgeComputationStage::PreJoinGeometry);
                }
                if (edgeData) {
                    data.bbox_.uniteWith(edgeData->boundingBox());
                    const geometry::CurveSampleArray& samples =
                        edgeData->preJoinSamples();
                    if (khe.direction()) {
                        for (const geometry::CurveSample& s : samples) {
                            Vec2d p = s.position();
                            if (isFirst) {
                                curves2d.moveTo(p[0], p[1]);
                                isFirst = false;
                            }
                            else {
                                curves2d.lineTo(p[0], p[1]);
                            }
                        }
                    }
                    else {
                        for (auto it = samples.rbegin(); it != samples.rend(); ++it) {
                            const geometry::CurveSample& s = *it;
                            Vec2d p = s.position();
                            if (isFirst) {
                                curves2d.moveTo(p[0], p[1]);
                                isFirst = false;
                            }
                            else {
                                curves2d.lineTo(p[0], p[1]);
                            }
                        }
                    }
                }
                else {
                    // TODO: handle error
                }
            }
        }
        curves2d.close();
    }

    data.triangulation_.clear();
    auto params = geometry::Curves2dSampleParams::adaptive();
    curves2d.fill(data.triangulation_, params, geometry::WindingRule::Odd);

    if (data.triangulation_.reservedLength() > data.triangulation_.length() * 3) {
        data.triangulation_.shrinkToFit();
    }

    data.isFillMeshComputed_ = true;
    data.isComputing_ = false;
    return true;
}

void VacKeyFace::dirtyFillMesh_() {
    if (frameData_.isFillMeshComputed_) {
        frameData_.clear();
        notifyChangesToDependents(ChangeFlag::FaceFillMesh);
    }
}

void VacKeyFace::onUpdateError_() {
    removeVacNode();
}

} // namespace vgc::workspace
