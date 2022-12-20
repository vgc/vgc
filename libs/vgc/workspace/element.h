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
#include <vgc/topology/vac.h>
#include <vgc/workspace/api.h>

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
    Selected = 0x01,
    Outline = 0x02,
    Draft = 0x04,
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

class VGC_WORKSPACE_API Element : public topology::detail::TreeNodeBase<Element> {
private:
    friend class Workspace;

    using Base = topology::detail::TreeNodeBase<Element>;

public:
    virtual ~Element() = default;

    Element(dom::Element* domElement)
        : domElement_(domElement)
        , vacNode_(nullptr) {
    }

public:
    Element* parent() const {
        return Base::parent();
    }

    core::Id id() const {
        return id_;
    }

    dom::Element* domElement() const {
        return domElement_;
    }

    topology::VacNode* vacNode() const {
        return vacNode_;
    }

    core::StringId tagName() const {
        return domElement_->tagName();
    }

    ElementFlags flags() const {
        return flags_;
    }

    Element* previous() const {
        return Base::previous();
    }

    Element* next() const {
        return Base::next();
    }

    Element* nextVacElement() const {
        Element* e = next();
        return findFirstSiblingVacElement_(e);
    }

    /// Returns bottom-most child in depth order.
    ///
    Element* firstChild() const {
        return Base::firstChild();
    }

    Element* firstChildVacElement() const {
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

    void paint(
        graphics::Engine* engine,
        core::AnimTime t = {},
        PaintOptions flags = PaintOption::None) {

        paint_(engine, t, flags);
    }

protected:
    virtual geometry::Rect2d boundingBox();

    virtual void onDomElementChanged();
    virtual void onVacNodeChanged();

    virtual void prepareForFrame_(core::AnimTime t = {});

    virtual void paint_(
        graphics::Engine* engine,
        core::AnimTime t = {},
        PaintOptions flags = PaintOption::None);

private:
    // uniquely identifies an element
    // if vacNode_ != nullptr then vacNode_->id() == id_.
    core::Id id_ = -1;

    dom::Element* domElement_ = nullptr;
    topology::VacNode* vacNode_ = nullptr;

    ElementFlags flags_;

    static Element* findFirstSiblingVacElement_(Element* start) {
        topology::VacNode* n = nullptr;
        Element* e = start;
        if (e) {
            n = e->vacNode();
            while (!n && e) {
                e = e->next();
                n = e->vacNode();
            }
        }
        return e;
    }
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_ELEMENT_H
