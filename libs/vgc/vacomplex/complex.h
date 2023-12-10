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

#ifndef VGC_VACOMPLEX_COMPLEX_H
#define VGC_VACOMPLEX_COMPLEX_H

#include <unordered_map>

#include <vgc/core/array.h>
#include <vgc/core/id.h>
#include <vgc/core/object.h>
#include <vgc/core/span.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/complexdiff.h>

namespace vgc::vacomplex {

VGC_DECLARE_OBJECT(Complex);

/// \class vgc::vacomplex::Complex
/// \brief Represents a VAC.
///
class VGC_VACOMPLEX_API Complex : public core::Object {
private:
    VGC_OBJECT(Complex, core::Object)

protected:
    Complex(CreateKey);

    void onDestroyed() override;

public:
    static ComplexPtr create();

    void clear();

    Group* resetRoot();

    Group* rootGroup() const {
        return root_;
    }

    Node* find(core::Id id) const;

    Cell* findCell(core::Id id) const;

    Group* findGroup(core::Id id) const;

    bool containsNode(core::Id id) const;

    /// Actual type is implementation detail. Only assume forward range.
    using VertexRange = core::Array<VertexCell*>;

    /// Actual type is implementation detail. Only assume forward range.
    using EdgeRange = core::Array<EdgeCell*>;

    /// Actual type is implementation detail. Only assume forward range.
    using FaceRange = core::Array<FaceCell*>;

    VertexRange vertices() const;
    EdgeRange edges() const;
    FaceRange faces() const;

    VertexRange vertices(core::AnimTime t) const;
    EdgeRange edges(core::AnimTime t) const;
    FaceRange faces(core::AnimTime t) const;

    // An increasing version seems enough, we don't need it to match document version.

    Int64 version() const {
        return version_;
    }

    /// Returns whether someone is currently modifying this complex.
    ///
    bool isOperationInProgress() const {
        return numOperationsInProgress_ > 0;
    }

    /// Prints the tree of nodes of the Complex for debug purposes.
    ///
    void debugPrint();

    VGC_SIGNAL(nodesChanged, (const ComplexDiff&, diff))

private:
    friend detail::Operations;
    using NodePtrMap = std::unordered_map<core::Id, std::unique_ptr<Node>>;

    // Container storing and owning all the nodes in the Complex.
    NodePtrMap nodes_;

    // Non-owning pointer to the root Group.
    // Note that the root Group is also in nodes_.
    Group* root_;

    // Version control
    Int64 version_ = 0;

    // Guard against recursion when calling clear() / resetRoot()
    bool isBeingCleared_ = false;
    Int numOperationsInProgress_ = 0;

    // Stores the diff of operations that have taken place
    // and not yet emitted.
    ComplexDiff opDiff_ = {};

    // This set is used in the implementation of some operations to check later
    // whether a given cell is still alive. Any cell added to this set will
    // be automatically removed from the set when the cell is deleted.
    //
    core::Array<Cell*> temporaryCellSet_;
};

/// Returns which node among the given `nodes`, if any, is the top-most among
/// the children of `group`. Top-most means that it appears last in the list of
/// children, and is therefore drawn last, potentially occluding previous
/// siblings.
///
/// Returns `nullptr` if the `group` does not contain any of the nodes in
/// `nodes`.
///
/// \sa `bottomMostInGroup()`,
///     `topMostInGroupAbove()`, `bottomMostInGroupBelow()`.
///
VGC_VACOMPLEX_API
Node* topMostInGroup(Group* group, core::ConstSpan<Node*> nodes);

/// Returns which node among the given `nodes`, if any, is the bottom-most
/// among the children of `group`. Bottom-most means that it appears first in
/// the list of children, and is therefore drawn first, potentially occluded
/// by next siblings.
///
/// Returns `nullptr` if the `group` does not contain any of the nodes in
/// `nodes`.
///
/// \sa `topMostInGroup()`,
///     `topMostInGroupAbove()`, `bottomMostInGroupBelow()`.
///
VGC_VACOMPLEX_API
Node* bottomMostInGroup(Group* group, core::ConstSpan<Node*> nodes);

/// Returns which node among the given `nodes`, if any, is the top-most among
/// the next siblings of `node`. Top-most means that it appears last in the
/// list of children, and is therefore drawn last, potentially occluding
/// previous siblings.
///
/// Returns `nullptr` if the next siblings of `node` do not contain any of the
/// nodes in `nodes`.
///
/// \sa `topMostInGroup()`, `bottomMostInGroup()`,
///     `bottomMostInGroupBelow()`.
///
VGC_VACOMPLEX_API
Node* topMostInGroupAbove(Node* node, core::ConstSpan<Node*> nodes);

/// Returns which node among the given `nodes`, if any, is the bottom-most
/// among the previous siblings of `node`. Bottom-most means that it appears
/// first in the list of children, and is therefore drawn first, potentially
/// occluded by next siblings.
///
/// Returns `nullptr` if the previous siblings of `node` do not contain any of
/// the nodes in `nodes`.
///
/// \sa `topMostInGroup()`, `bottomMostInGroup()`,
///     `topMostInGroupAbove()`.
///
VGC_VACOMPLEX_API
Node* bottomMostInGroupBelow(Node* node, core::ConstSpan<Node*> nodes);

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_COMPLEX_H
