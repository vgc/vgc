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

#ifndef VGC_WORKSPACE_FACE_H
#define VGC_WORKSPACE_FACE_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/dom/element.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/vec4f.h>
#include <vgc/graphics/engine.h>
#include <vgc/vacomplex/complex.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/element.h>
#include <vgc/workspace/vertex.h>

namespace vgc::workspace {

class VGC_WORKSPACE_API FaceGraphics {
public:
    void clear() {
        clearFillGeometry();
    }

    const graphics::GeometryViewPtr& fillGeometry() const {
        return fillGeometry_;
    }

    void setFillGeometry(const graphics::GeometryViewPtr& fillGeometry) {
        fillGeometry_ = fillGeometry;
    }

    void clearFillGeometry() {
        fillGeometry_.reset();
    }

    bool hasStyle() const {
        return !isStyleDirty_;
    }

    // currently not an independent graphics object
    void setStyle() {
        isStyleDirty_ = false;
    }

    void clearStyle() {
        isStyleDirty_ = true;
    }

private:
    graphics::GeometryViewPtr fillGeometry_;

    // Style
    bool isStyleDirty_ = true;
};

class VGC_WORKSPACE_API VacFaceCellFrameData {
private:
    friend class VacFaceCell;
    friend class VacKeyFace;
    friend class VacInbetweenFace;

public:
    VacFaceCellFrameData(const core::AnimTime& t) noexcept
        : time_(t) {
    }

    void clear() {
        graphics_.clear();
        triangulation_.clear();
        isFillMeshComputed_ = false;
    }

    const core::AnimTime& time() const {
        return time_;
    }

    const core::Color& color() const {
        return color_;
    }

    const FaceGraphics& graphics() const {
        return graphics_;
    }

    bool isSelectableAt(
        const geometry::Vec2d& position,
        bool outlineOnly,
        double tol,
        double* outDistance) const;

private:
    core::AnimTime time_;
    geometry::Rect2d bbox_ = {};

    // At the time of definition Curves2d only returns an array of triangle list vertices.
    // TODO: use indexed geometry.
    core::FloatArray triangulation_;

    // style (independent stage)
    core::Color color_;
    bool isStyleDirty_ = true;

    // Note: only valid for a single engine at the moment.
    FaceGraphics graphics_;

    bool isFillMeshComputed_ = false;
    bool isComputing_ = false;
};

// wrapper to benefit from "final" specifier.
class VGC_WORKSPACE_API VacKeyFaceFrameData final : public VacFaceCellFrameData {
public:
    VacKeyFaceFrameData(const core::AnimTime& t) noexcept
        : VacFaceCellFrameData(t) {
    }
};

class VGC_WORKSPACE_API VacFaceCell : public VacElement {
private:
    friend class Workspace;
    friend class VacVertexCell;

protected:
    VacFaceCell(Workspace* workspace)
        : VacElement(workspace) {
    }

public:
    vacomplex::FaceCell* vacFaceCellNode() const {
        vacomplex::Cell* cell = vacCellUnchecked();
        return cell ? cell->toFaceCellUnchecked() : nullptr;
    }

    virtual const VacFaceCellFrameData* computeFrameDataAt(core::AnimTime t) = 0;
};

class VGC_WORKSPACE_API VacKeyFace final : public VacFaceCell {
private:
    friend class Workspace;

public:
    ~VacKeyFace() override;

    VacKeyFace(Workspace* workspace)
        : VacFaceCell(workspace)
        , frameData_({}) {
    }

    vacomplex::KeyFace* vacKeyFaceNode() const {
        vacomplex::Cell* cell = vacCellUnchecked();
        return cell ? cell->toKeyFaceUnchecked() : nullptr;
    }

    std::optional<core::StringId> domTagName() const override;

    geometry::Rect2d boundingBox(core::AnimTime t) const override;

    bool isSelectableAt(
        const geometry::Vec2d& position,
        bool outlineOnly,
        double tol,
        double* outDistance = nullptr,
        core::AnimTime t = {}) const override;

    bool isSelectableInRect(const geometry::Rect2d& rect, core::AnimTime t = {})
        const override;

    const VacFaceCellFrameData* computeFrameData();

    const VacFaceCellFrameData* computeFrameDataAt(core::AnimTime t) override;

protected:
    void onPaintPrepare(core::AnimTime t, PaintOptions flags) override;

    void onPaintDraw(
        graphics::Engine* engine,
        core::AnimTime t,
        PaintOptions flags = PaintOption::None) const override;

private:
    // used to know if cycles have changed
    std::string lastCyclesDomDescription_;
    // used to support path updates
    core::Array<Element*> cyclesElementsSequence_;

    mutable VacKeyFaceFrameData frameData_;

    ElementStatus onDependencyChanged_(Element* dependency, ChangeFlags changes) override;
    ElementStatus onDependencyRemoved_(Element* dependency) override;

    // returns whether style changed
    static bool updatePropertiesFromDom_(
        vacomplex::KeyFaceData* data,
        const dom::Element* domElement);
    static void writePropertiesToDom_(
        dom::Element* domElement,
        const vacomplex::KeyFaceData* data,
        core::ConstSpan<core::StringId> propNames);
    static void writeAllPropertiesToDom_(
        dom::Element* domElement,
        const vacomplex::KeyFaceData* data);

    ElementStatus updateFromDom_(Workspace* workspace) override;

    void updateFromVac_(vacomplex::NodeModificationFlags flags) override;

    void updateDependencies_(core::Array<Element*> sortedNewDependencies);

    bool computeStrokeStyle_();

    void dirtyStrokeStyle_(bool notifyDependentsImmediately = true);

    bool computeFillMesh_();

    void dirtyFillMesh_();

    void onUpdateError_();
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_FACE_H
