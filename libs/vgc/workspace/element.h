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

#ifndef VGC_WORKSPACE_ELEMENT_H
#define VGC_WORKSPACE_ELEMENT_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/flags.h>
#include <vgc/core/id.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/element.h>
#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/graphics/engine.h>
#include <vgc/topology/operations.h>
#include <vgc/topology/vac.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/logcategories.h>

namespace vgc::workspace {

//
// transforms will only be available on groups, composites, text, but not cells to keep
// vac computations reasonably fast.
// A vac needs all of the transforms from its root.
//
// We want our workspace to provide a way to visit the scene for rendering.
// This brings a few questions:
//     - geometry is different depending on time, where and when should we cache it ?
//     - layers can thus be different too depending on time, when and where should we cache the textures ?
//
// If a layer is constant we probably want to keep it for other frames.
// When editing, we can keep the composition of contiguous sequences of elements if their blend mode allows it.
//
// A previewer would cache its end frames directly, but a re-render after changing a few items should be reasonably fast.
//
// We also want to be able to draw different times simultaneously, it means synchronized cache or should we copy ?
//
// We don't know at what speed geometry can be generated for a given frame, maybe we could cache based on perf (time / size) ?
// scripted values for instance may be worth caching.
//
// We can identify some reusable buffers (all times or specific time but all renderers):
//    mesh vertices
//    mesh indices
//    parametrization
//    outline strip
//    color buffers
//    gradient params (gradient palette ?)
//    effect params (effect group ?)
//

// Editable component (control points..)
struct Component {};

/// \enum vgc::ui::PaintOption
/// \brief Specifies element paint options.
///
enum class PaintOption : UInt64 {
    None = 0x00,
    Draft = 0x02,
    Selected = 0x04,
    Outline = 0x08,
};
VGC_DEFINE_FLAGS(PaintOptions, PaintOption)

enum class ElementFlag : UInt16 {
    None = 0x00,
    // these will be in the schema too
    VisibleInRender = 0x01,
    VisibleInEditor = 0x02,
    Locked = 0x04,
    Implicit = 0x08,
};
VGC_DEFINE_FLAGS(ElementFlags, ElementFlag)

enum class ElementStatus : Int8 {
    Ok,
    InvalidAttribute,
    UnresolvedDependency,
    ErrorInDependency,
    ErrorInParent,
};

constexpr bool operator!(const ElementStatus& status) noexcept {
    return status != ElementStatus::Ok;
}

class Workspace;
class VacElement;

class VGC_WORKSPACE_API Element : public topology::detail::TreeNodeBase<Element> {
private:
    friend Workspace;
    friend VacElement;

    using Base = topology::detail::TreeNodeBase<Element>;

protected:
    Element(Workspace* workspace, dom::Element* domElement)
        : workspace_(workspace)
        , domElement_(domElement) {
    }

public:
    virtual ~Element();

public:
    using ChildrenIterator = Base::Iterator;

    core::Id id() const {
        return id_;
    }

    // the returned pointer can be dangling if the workspace is not synced with its dom
    dom::Element* domElement() const {
        return domElement_;
    }

    bool isVgcElement() const {
        return false;
    }

    bool isVacElement() const {
        return isVacElement_;
    }

    inline VacElement* toVacElement();
    inline const VacElement* toVacElement() const;

    inline vacomplex::Node* vacNode() const;

    core::StringId tagName() const {
        return domElement_->tagName();
    }

    ElementFlags flags() const {
        return flags_;
    }

    ElementStatus status() const {
        return status_;
    }

    bool hasError() const {
        return !status_;
    }

    bool hasPendingUpdate() const {
        return hasPendingUpdate_;
    }

    const Workspace* workspace() const {
        return workspace_;
    }

    Element* parent() const {
        return Base::parent();
    }

    inline VacElement* parentVacElement() const;

    Element* previous() const {
        return Base::previous();
    }

    Element* next() const {
        return Base::next();
    }

    VacElement* nextVacElement() const {
        Element* e = next();
        return findFirstSiblingVacElement_(e);
    }

    /// Returns bottom-most child in depth order.
    ///
    Element* firstChild() const {
        return Base::firstChild();
    }

    VacElement* firstChildVacElement() const {
        Element* e = firstChild();
        return findFirstSiblingVacElement_(e);
    }

    /// Returns top-most child in depth order.
    ///
    Element* lastChild() const {
        return Base::lastChild();
    }

    Int numChildren() const {
        return Base::numChildren();
    }

    ChildrenIterator begin() const {
        return Base::begin();
    }

    ChildrenIterator end() const {
        return Base::end();
    }

    void paint(
        graphics::Engine* engine,
        core::AnimTime t = {},
        PaintOptions flags = PaintOption::None) const {

        paint_(engine, t, flags);
    }

    virtual geometry::Rect2d boundingBox(core::AnimTime t = {}) const;

protected:
    virtual ElementStatus updateFromDom_(Workspace* workspace);

    void addDependency(Element* dependency);
    /// Both `oldDependency` and `newDependency` can be null. Returns true if `oldDependency != newDependency`, false otherwise.
    bool replaceDependency(Element* oldDependency, Element* newDependency);
    void removeDependency(Element* dependency);
    void clearDependencies();

    virtual void onDependencyBeingDestroyed_(Element* dependency);

    /// dependent may be being destroyed, only use its pointer as key.
    virtual void onDependentElementRemoved_(Element* dependent);
    virtual void onDependentElementAdded_(Element* dependent);

    virtual void
    preparePaint_(core::AnimTime t = {}, PaintOptions flags = PaintOption::None);

    virtual void paint_(
        graphics::Engine* engine,
        core::AnimTime t = {},
        PaintOptions flags = PaintOption::None) const;

private:
    const Workspace* workspace_;
    // uniquely identifies an element
    // if vacNode_ != nullptr then vacNode_->id() == id_.
    core::Id id_ = -1;

    // this pointer is not safe to use when tree is not synced with dom
    dom::Element* domElement_;

    ElementFlags flags_;
    bool isVacElement_ = false;

    bool hasPendingUpdate_ = true;
    bool isBeingUpdated_ = false;
    ElementStatus status_ = ElementStatus::Ok;

    core::Array<Element*> dependencies_;
    core::Array<Element*> dependents_;

    static VacElement* findFirstSiblingVacElement_(Element* start);
};

class VGC_WORKSPACE_API UnsupportedElement final : public Element {
private:
    friend class Workspace;

public:
    ~UnsupportedElement() override = default;

    UnsupportedElement(Workspace* workspace, dom::Element* domElement)
        : Element(workspace, domElement) {
    }
};

class VGC_WORKSPACE_API VacElement : public Element {
private:
    friend class Workspace;

public:
    ~VacElement() override;

    VacElement(Workspace* workspace, dom::Element* domElement)
        : Element(workspace, domElement)
        , vacNode_(nullptr) {

        isVacElement_ = true;
    }

public:
    // the returned pointer can be dangling if the workspace is not synced with its vac
    vacomplex::Node* vacNode() const {
        return vacNode_;
    }

protected:
    vacomplex::Cell* vacCellUnchecked() const {
        return vacNode_->toCellUnchecked();
    }

    void removeVacNode();
    void setVacNode(vacomplex::Node* vacNode);

    virtual void onVacNodeRemoved_();

private:
    // this pointer is not safe to use when tree is not synced with vac
    vacomplex::Node* vacNode_ = nullptr;
};

vacomplex::Node* Element::vacNode() const {
    return isVacElement() ? static_cast<const VacElement*>(this)->vacNode() : nullptr;
}

VacElement* Element::parentVacElement() const {
    Element* e = parent();
    return (e && e->isVacElement()) ? static_cast<VacElement*>(e) : nullptr;
}

VacElement* Element::toVacElement() {
    return isVacElement_ ? static_cast<VacElement*>(this) : nullptr;
}

const VacElement* Element::toVacElement() const {
    return isVacElement_ ? static_cast<const VacElement*>(this) : nullptr;
}

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_ELEMENT_H
