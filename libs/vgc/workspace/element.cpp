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
#include <vgc/workspace/logcategories.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

Element::~Element() {
    clearDependencies();
}

std::optional<core::StringId> Element::domTagName() const {
    return {};
}

geometry::Rect2d Element::boundingBox(core::AnimTime /*t*/) const {
    return geometry::Rect2d::empty;
}

bool Element::isSelectableAt(
    const geometry::Vec2d& /*pos*/,
    bool /*outlineOnly*/,
    double /*tol*/,
    double* /*outDistance*/,
    core::AnimTime /*t*/) const {

    return false;
}

bool Element::isSelectableInRect(const geometry::Rect2d& /*rect*/, core::AnimTime /*t*/)
    const {

    return false;
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
        onDependencyRemoved_(element);
        element->dependents_.removeOne(this);
        element->onDependentElementRemoved_(this);
    }
}

void Element::clearDependencies() {
    while (!dependencies_.isEmpty()) {
        Element* dep = dependencies_.pop();
        onDependencyRemoved_(dep);
        dep->dependents_.removeOne(this);
        dep->onDependentElementRemoved_(this);
    }
}

void Element::notifyChangesToDependents(ChangeFlags changes) {
    for (Element* e : dependents_) {
        e->onDependencyChanged_(this, changes);
    }
}

void Element::onPaintPrepare(core::AnimTime /*t*/, PaintOptions /*flags*/) {
}

void Element::onPaintDraw(
    graphics::Engine* /*engine*/,
    core::AnimTime /*t*/,
    PaintOptions /*flags*/) const {

    // XXX make it pure virtual once the factory is in.
}

VacElement* Element::findFirstSiblingVacElement_(Element* start) {
    Element* e = start;
    while (e && !e->isVacElement()) {
        e = e->nextSibling();
    }
    return static_cast<VacElement*>(e);
}

VacElement* Element::findFirstSiblingVacElementReverse_(Element* start) {
    Element* e = start;
    while (e && !e->isVacElement()) {
        e = e->previousSibling();
    }
    return static_cast<VacElement*>(e);
}

ElementStatus Element::updateFromDom_(Workspace* /*workspace*/) {
    return ElementStatus::Ok;
}

ElementStatus
Element::onDependencyChanged_(Element* /*dependency*/, ChangeFlags /*changes*/) {
    return status_;
}

ElementStatus Element::onDependencyRemoved_(Element* /*dependency*/) {
    // child classes typically have to invalidate data when a dependency is removed
    ElementStatus status = this->status();
    if (status == ElementStatus::Ok) {
        status = ElementStatus::UnresolvedDependency;
    }
    return status;
}

void Element::onDependencyMoved_(Element* /*dependency*/) {
    // child classes typically have to update paths when a dependency moves
}

void Element::onDependentElementRemoved_(Element* /*dependent*/) {
}

void Element::onDependentElementAdded_(Element* /*dependent*/) {
}

VacElement::~VacElement() {
    hardDeleteVacNode();
}

void VacElement::hardDeleteVacNode() {
    if (vacNode()) {

        // We first disconnect the link between the workspace item and the VAC node,
        // otherwise hard-deleting the VAC node would recursively cause the workspace
        // item to be deleted.
        //
        vacomplex::Node* vacNode = this->vacNode();
        unsetVacNode();

        bool deleteIsolatedVertices = false;
        vacomplex::ops::hardDelete(vacNode, deleteIsolatedVertices);
    }
}

void VacElement::unsetVacNode() {
    if (vacNode_) {
        unsetVacNode(vacNode_->id());
    }
}

// This version must be used whenever the vacNode_ is already destroyed, and
// therefore we cannot call vacNode_->id().
//
void VacElement::unsetVacNode(core::Id vacNodeId) {
    if (vacNode_) {
        workspace()->elementByVacInternalId_.erase(vacNodeId);
        vacNode_ = nullptr;
    }
}

// Invariants that we need to enforce in setVacNode() and unsetVacNode():
// - findVacElement(workspaceItem->vacNode()->id()) == workspaceItem;
// - findVacElement(vacNode->id())->vacNode() == vacNode;
//
void VacElement::setVacNode(vacomplex::Node* vacNode) {

    // Nothing to do if already set to the same VAC node.
    //
    if (this->vacNode() == vacNode) {
        return;
    }

    // Unset and delete any already previously set VAC node: we do not want to
    // keep alive VAC nodes which are not tracked by Workspace.
    //
    hardDeleteVacNode();

    // Enforce invariant: no two workspace items should point to the same VAC node.
    //
    if (workspace()->findVacElement(vacNode)) {
        VGC_WARNING(
            LogVgcWorkspace,
            "Cannot set a VAC node to a workspace item when this VAC node is already set "
            "to another workspace item.");
        return;
    }

    if (vacNode) {
        workspace()->elementByVacInternalId_.emplace(vacNode->id(), this);
    }
    vacNode_ = vacNode;
}

} // namespace vgc::workspace
