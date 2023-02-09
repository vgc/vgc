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

#include <vgc/workspace/element.h>

#include <vgc/dom/document.h>
#include <vgc/dom/element.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

Element::~Element() {
    clearDependencies();
    for (Element* dependent : dependents_) {
        dependent->onDependencyBeingDestroyed_(this);
        dependent->dependencies_.removeOne(this);
    }
    dependents_.clear();
}

geometry::Rect2d Element::boundingBox(core::AnimTime /*t*/) const {
    return geometry::Rect2d::empty;
}

void Element::addDependency(Element* element) {
    if (element && !dependencies_.contains(element)) {
        dependencies_.emplaceLast(element);
        element->dependents_.emplaceLast(this);
        element->onDependentElementAdded_(this);
    }
}

bool Element::replaceDependency(Element* oldDependency, Element* newDependency) {
    if (oldDependency != newDependency) {
        removeDependency(oldDependency);
        addDependency(newDependency);
        return true;
    }
    return false;
}

void Element::removeDependency(Element* element) {
    if (element && dependencies_.removeOne(element)) {
        element->dependents_.removeOne(this);
        element->onDependentElementRemoved_(this);
    }
}

void Element::clearDependencies() {
    for (Element* dep : dependencies_) {
        dep->dependents_.removeOne(this);
        dep->onDependentElementRemoved_(this);
    }
    dependencies_.clear();
}

void Element::onDependencyBeingDestroyed_(Element* /*dependency*/) {
}

void Element::onDependentElementRemoved_(Element* /*dependent*/) {
}

void Element::onDependentElementAdded_(Element* /*dependent*/) {
}

ElementStatus Element::updateFromDom_(Workspace* /*workspace*/) {
    return ElementStatus::Ok;
}

void Element::preparePaint_(core::AnimTime /*t*/, PaintOptions /*flags*/) {
}

void Element::paint_(
    graphics::Engine* /*engine*/,
    core::AnimTime /*t*/,
    PaintOptions /*flags*/) const {

    // XXX make it pure virtual once the factory is in.
}

VacElement* Element::findFirstSiblingVacElement_(Element* start) {
    Element* e = start;
    while (e && !e->isVacElement()) {
        e = e->next();
    }
    return static_cast<VacElement*>(e);
}

VacElement::~VacElement() {
    removeVacNode();
}

void VacElement::removeVacNode() {
    if (vacNode_) {
        // We set vacNode_ to null before calling removeNode because
        // the workspace would consider it as an external event otherwise
        // and would both call onVacNodeRemoved_ and schedule this element for update.
        vacomplex::Node* tmp = vacNode_;
        vacNode_ = nullptr;
        topology::ops::removeNode(tmp, false);
        onVacNodeRemoved_();
    }
}

void VacElement::setVacNode(vacomplex::Node* vacNode) {
    removeVacNode();
    vacNode_ = vacNode;
}

void VacElement::onVacNodeRemoved_() {
}

} // namespace vgc::workspace
