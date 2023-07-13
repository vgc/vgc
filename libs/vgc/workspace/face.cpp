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

#include <vgc/core/span.h>
#include <vgc/geometry/triangle2f.h>
#include <vgc/workspace/colors.h>
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
    readTo(component.path, in);
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
    bool outlineOnly,
    double tol,
    double* outDistance) const {

    if (outlineOnly) {
        return false;
    }

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

    bool isContained = false;

    Vec2f positionF(position);
    geometry::Triangle2f triangle;
    for (Int i = 0; i < triangulation_.length() - 5; i += 6) {
        triangle.setA(triangulation_[i + 0], triangulation_[i + 1]);
        triangle.setB(triangulation_[i + 2], triangulation_[i + 3]);
        triangle.setC(triangulation_[i + 4], triangulation_[i + 5]);
        if (triangle.contains(positionF)) {
            isContained = true;
            break;
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
        const_cast<VacKeyFace*>(this)->computeFillMesh_();
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

bool VacKeyFace::isSelectableInRect(const geometry::Rect2d& rect, core::AnimTime t)
    const {

    vacomplex::KeyFace* kf = vacKeyFaceNode();
    if (kf && frameData_.time() == t) {
        if (frameData_.bbox_.isEmpty() || !frameData_.bbox_.intersects(rect)) {
            return false;
        }
        for (const vacomplex::KeyCycle& cycle : kf->cycles()) {
            if (cycle.steinerVertex()) {
                if (rect.contains(cycle.steinerVertex()->position())) {
                    return true;
                }
            }
            for (const vacomplex::KeyHalfedge& khe : cycle.halfedges()) {
                VacElement* edgeElement = workspace()->findVacElement(khe.edge());
                if (edgeElement->isSelectableInRect(rect, t)) {
                    return true;
                }
            }
        }
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
        const core::Color& c = colors::selection;
        core::FloatArray bufferData = {c.r(), c.g(), c.b(), c.a()};
        const BufferPtr& buffer = graphics.fillGeometry()->vertexBuffer(1);
        engine->updateBufferData(buffer, std::move(bufferData));
        engine->setProgram(graphics::BuiltinProgram::SimplePreview);
        engine->draw(graphics.fillGeometry());
        data.hasPendingColorChange_ = true;
        // For now we share the same GeometryView for PaintOption::Selected and
        // PaintOption::None, so we set hasPendingColorChange_ to true so that
        // in the next onPaintDraw(PaintOption::None), we go again in the code
        // path updating the face color.
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

    // create/rebuild/update VAC node
    if (!kf) {
        kf = vacomplex::ops::createKeyFace(cycles, parentGroup);
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

void VacKeyFace::updateFromVac_(vacomplex::NodeModificationFlags flags) {
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

    using vacomplex::NodeModificationFlag;

    bool boundaryChanged = flags.has(NodeModificationFlag::BoundaryChanged);
    if (boundaryChanged) {

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

        // Update dependencies_
        core::Array<Element*> newDependencies;
        vacomplex::CellRangeView boundary = kf->boundary();
        newDependencies.reserve(boundary.length());
        for (vacomplex::Cell* cell : boundary) {
            if (VacElement* element = workspace()->findVacElement(cell)) {
                newDependencies.append(element);
            }
        }
        std::sort(newDependencies.begin(), newDependencies.end());
        updateDependencies_(newDependencies);
    }

    if (boundaryChanged || flags.has(NodeModificationFlag::BoundaryMeshChanged)) {
        dirtyFillMesh_();
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

    using namespace vacomplex;
    using geometry::Vec2d;
    using geometry::Vec2f;

    KeyFace* kf = vacKeyFaceNode();
    if (!kf) {
        return false;
    }

    data.isComputing_ = true;
    data.bbox_ = geometry::Rect2d::empty;

    vacomplex::detail::computeKeyFaceFillTriangles(
        kf->cycles(), data.triangulation_, geometry::WindingRule::Odd);

    if (data.triangulation_.reservedLength() > data.triangulation_.length() * 3) {
        data.triangulation_.shrinkToFit();
    }

    core::Span<geometry::Vec2f> points(
        reinterpret_cast<geometry::Vec2f*>(data.triangulation_.data()),
        data.triangulation_.length() / 2);
    for (const geometry::Vec2f& point : points) {
        data.bbox_.uniteWith(geometry::Vec2d(point));
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
