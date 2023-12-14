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

#ifndef VGC_VACOMPLEX_OPERATIONS_H
#define VGC_VACOMPLEX_OPERATIONS_H

#include <vgc/core/animtime.h>
#include <vgc/core/enum.h>
#include <vgc/core/id.h>
#include <vgc/core/span.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/complex.h>
#include <vgc/vacomplex/exceptions.h>
#include <vgc/vacomplex/keycycle.h>
#include <vgc/vacomplex/keyedge.h>
#include <vgc/vacomplex/keyface.h>
#include <vgc/vacomplex/keyvertex.h>

namespace vgc::vacomplex {

class VGC_VACOMPLEX_API CutEdgeResult {
public:
    constexpr CutEdgeResult() noexcept = default;

    CutEdgeResult(KeyEdge* edge1, KeyVertex* vertex, KeyEdge* edge2) noexcept
        : vertex_(vertex)
        , edge1_(edge1)
        , edge2_(edge2) {
    }

    KeyVertex* vertex() const {
        return vertex_;
    }

    void setVertex(KeyVertex* vertex) {
        vertex_ = vertex;
    }

    KeyEdge* edge1() const {
        return edge1_;
    }

    void setEdge1(KeyEdge* edge1) {
        edge1_ = edge1;
    }

    KeyEdge* edge2() const {
        return edge2_;
    }

    void setEdge2(KeyEdge* edge2) {
        edge2_ = edge2;
    }

private:
    KeyVertex* vertex_ = nullptr;
    KeyEdge* edge1_ = nullptr;
    KeyEdge* edge2_ = nullptr;
};

class VGC_VACOMPLEX_API CutFaceResult {
public:
    constexpr CutFaceResult() noexcept = default;

    CutFaceResult(KeyFace* face1, KeyEdge* edge, KeyFace* face2) noexcept
        : edge_(edge)
        , face1_(face1)
        , face2_(face2) {
    }

    KeyEdge* edge() const {
        return edge_;
    }

    void setEdge(KeyEdge* edge) {
        edge_ = edge;
    }

    KeyFace* face1() const {
        return face1_;
    }

    void setFace1(KeyFace* face1) {
        face1_ = face1;
    }

    KeyFace* face2() const {
        return face2_;
    }

    void setFace2(KeyFace* face2) {
        face2_ = face2;
    }

private:
    KeyEdge* edge_ = nullptr;
    KeyFace* face1_ = nullptr;
    KeyFace* face2_ = nullptr;
};

enum class OneCycleCutPolicy {
    Auto,
    Disk,
    Mobius, // non-planar non-orientable.
    Torus,  // non-planar orientable (e.g.: once-punctured torus).
};

VGC_VACOMPLEX_API
VGC_DECLARE_ENUM(OneCycleCutPolicy)

enum class TwoCycleCutPolicy {
    Auto,
    ReverseNone,
    ReverseStart,
    ReverseEnd,
    ReverseBoth
};

VGC_VACOMPLEX_API
VGC_DECLARE_ENUM(TwoCycleCutPolicy)

/// \class vgc::vacomplex::ScopedOperationsGroup
/// \brief Encloses multiple operations as one group for performance.
///
/// Whenever operations are performed on a `Complex`, the complex is in charge
/// of ensuring that geometric constraints are satisfied, for example by
/// snapping the geometry of an edge to its end vertices.
///
/// By instanciating a `ScopedOperationsGroup`, you can indicate that a
/// sequence of multiple operations is incoming, and that such constraint
/// enforcements must be delayed until the `ScopedOperationsGroup` is
/// destructed.
///
// TODO: In fact, some operations require to access edge geometry, such as
// `cutFaceWithEdge`, in order to compute heuristics on which cut to perform.
// This means that we should also in fact enforce the constraints whenever the
// edge geometry is accessed, or the ScopedOperationsGroup is destructed,
// whichever happens first.
//
class VGC_VACOMPLEX_API ScopedOperationsGroup {
public:
    ScopedOperationsGroup(Complex* complex);
    ~ScopedOperationsGroup();

private:
    std::unique_ptr<detail::Operations> group_;
};

namespace ops {

// TODO: impl the exceptions mentioned in comments, verify preconditions checks and throws.
// Indeed functions in detail::Operations do not throw and are unchecked.

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr.
///
VGC_VACOMPLEX_API
Group* createGroup(Group* parentGroup, Node* nextSibling = nullptr);

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr.
///
VGC_VACOMPLEX_API
KeyVertex* createKeyVertex(
    const geometry::Vec2d& position,
    Group* parentGroup,
    Node* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup`, `startVertex`, or `endVertex` is nullptr.
///
VGC_VACOMPLEX_API
KeyEdge* createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    KeyEdgeData&& geometry,
    Group* parentGroup,
    Node* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr.
///
VGC_VACOMPLEX_API
KeyEdge* createKeyClosedEdge(
    KeyEdgeData&& geometry,
    Group* parentGroup,
    Node* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr or one of the given `cycles` is not valid.
///
VGC_VACOMPLEX_API
KeyFace* createKeyFace(
    core::Array<KeyCycle> cycles,
    Group* parentGroup,
    Node* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr or one of the given `cycles` is not valid.
///
VGC_VACOMPLEX_API
KeyFace* createKeyFace(
    KeyCycle cycle,
    Group* parentGroup,
    Node* nextSibling = nullptr,
    core::AnimTime t = {});

// Post-condition: node is deleted, except if node is the root.
VGC_VACOMPLEX_API
void hardDelete(Node* node, bool deleteIsolatedVertices);

VGC_VACOMPLEX_API
void softDelete(core::ConstSpan<Node*> nodes, bool deleteIsolatedVertices);

VGC_VACOMPLEX_API
core::Array<KeyCell*>
simplify(core::Span<KeyVertex*> kvs, core::Span<KeyEdge*> kes, bool smoothJoins);

VGC_VACOMPLEX_API
KeyVertex* glueKeyVertices(core::Span<KeyVertex*> kvs, const geometry::Vec2d& position);

VGC_VACOMPLEX_API
KeyEdge* glueKeyOpenEdges(core::Span<KeyHalfedge> khs);

VGC_VACOMPLEX_API
KeyEdge* glueKeyOpenEdges(core::Span<KeyEdge*> kes);

VGC_VACOMPLEX_API
KeyEdge* glueKeyClosedEdges(core::Span<KeyHalfedge> khs);

VGC_VACOMPLEX_API
KeyEdge* glueKeyClosedEdges(core::Span<KeyEdge*> kes);

VGC_VACOMPLEX_API
core::Array<KeyEdge*> unglueKeyEdges(KeyEdge* ke);

// ungluedKeyEdges is temporary and will probably be replaced by a more generic
// container to support more types of unglued cells.
//
VGC_VACOMPLEX_API
core::Array<KeyVertex*> unglueKeyVertices(
    KeyVertex* kv,
    core::Array<std::pair<core::Id, core::Array<KeyEdge*>>>& ungluedKeyEdges);

// TODO: Functions to reinterpret visually similar cycles (with given winding rule).
//       [AA] (winding = 2/-2) looks like [A, A*] (winding = 0) with ODD winding rule.
//       [BACA] looks like [BA, C*A*] with ODD winding rule.
//       In other winding rules, the result may or may not be similar so it has
//       to be checked for the given geometric data (can be done using a planar graph).

VGC_VACOMPLEX_API
CutEdgeResult cutEdge(KeyEdge* ke, const geometry::CurveParameter& parameter);

VGC_VACOMPLEX_API
CutFaceResult cutGlueFace(
    KeyFace* kf,
    const KeyEdge* ke,
    OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto,
    TwoCycleCutPolicy twoCycleCutPolicy = TwoCycleCutPolicy::Auto);

VGC_VACOMPLEX_API
CutFaceResult cutGlueFace(
    KeyFace* kf,
    const KeyHalfedge& khe,
    KeyFaceVertexUsageIndex startIndex,
    KeyFaceVertexUsageIndex endIndex,
    OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto,
    TwoCycleCutPolicy twoCycleCutPolicy = TwoCycleCutPolicy::Auto);

VGC_VACOMPLEX_API
CutFaceResult cutFaceWithClosedEdge(
    KeyFace* kf,
    KeyEdgeData&& data,
    OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto);

VGC_VACOMPLEX_API
CutFaceResult cutFaceWithOpenEdge(
    KeyFace* kf,
    KeyEdgeData&& data,
    KeyFaceVertexUsageIndex startIndex,
    KeyFaceVertexUsageIndex endIndex,
    OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto,
    TwoCycleCutPolicy twoCycleCutPolicy = TwoCycleCutPolicy::Auto);

VGC_VACOMPLEX_API
CutFaceResult cutFaceWithOpenEdge(
    KeyFace* kf,
    KeyEdgeData&& data,
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto,
    TwoCycleCutPolicy twoCycleCutPolicy = TwoCycleCutPolicy::Auto);

VGC_VACOMPLEX_API
void cutGlueFaceWithVertex(KeyFace* kf, KeyVertex* kv);

VGC_VACOMPLEX_API
KeyVertex* cutFaceWithVertex(KeyFace* kf, const geometry::Vec2d& position);

/// Performs an atomic simplification at the given `KeyVertex`, if possible.
///
/// Such atomic simplification is possible in the following cases:
///
/// - The vertex has exactly two incident open edges, each of them using the
///   vertex exactly once. In this case, the simplification consists in
///   concatenating the two open edges into a single open edge.
///
/// - The vertex has exactly one incident open edge, using the vertex both
///   as start vertex and end vertex. In this case, the simplification consists
///   in converting the open edge into a closed edge.
///
/// - The vertex has no incident edges, and exactly one incident face, using
///   the vertex exactly once as a Steiner vertex. In this case, the
///   simplification consists in removing the Steiner cycle.
///
/// In all three above cases, the given `KeyVertex` is then deleted.
///
/// If an atomic simplification is not possible then nothing happens (the
/// vertex is not deleted), and this function returns `nullptr`.
///
// TODO: what about when there are inbetween cells incident to the key vertex?
//
VGC_VACOMPLEX_API
Cell* uncutAtKeyVertex(KeyVertex* kv, bool smoothJoin);

/// Performs an atomic simplification at the given `KeyEdge`, if possible.
///
/// If the edge is an open edge, such atomic simplification is possible in the
/// following cases:
///
/// - The edge has exactly two incident faces, each of them using the
///   edge exactly once. In this case, the simplification consists in
///   concatenating the two faces into a single face, splicing the two
///   original cycles into a single cycle.
///
/// - The edge has exactly one incident face, using the edge exactly
///   twice, from two different cycles. In this case, the simplification
///   consists in splicing the two cycles together into a single cycle.
///   Topologically, this corresponds to creating a torus (or surface of
///   higher genus).
///
/// - The edge has exactly one incident face, using the edge exactly twice,
///   within the same cycle and in opposite directions. In this case, the
///   simplification consists in splitting the cycle into two separate cycles.
///
/// - The edge has exactly one incident face, using the edge exactly twice,
///   within the same cycle and in the same direction. In this case, the
///   simplification consists in splicing the cycle to itself into a new
///   single cycle. Topologically, this corresponds to creating a Möbius
///   strip (or surface of higher genus).
///
/// If the edge is a closed edge, such atomic simplification is possible in the
/// following cases:
///
/// - The edge has exactly two incident faces, each of them using the
///   edge exactly once. In this case, the simplification consists in
///   concatenating the two faces into a single face, removing the
///   usages of the closed edge.
///
/// - The edge has exactly one incident face, using the edge exactly
///   twice (either from the same cycle or two different cycles).
///   In this case, the simplification consists in removing the
///   usages of the closed edge. Topologically, this corresponds to
///   creating a torus or a Möbius strip (or surface of higher genus).
///
/// In all cases above, the given `KeyEdge` is then deleted.
///
/// If an atomic simplification is not possible then nothing happens (the edge
/// is not deleted), and this function returns `nullptr`.
///
// TODO: what about when there are inbetween cells incident to the key edge?
//
VGC_VACOMPLEX_API
Cell* uncutAtKeyEdge(KeyEdge* ke);

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
// XXX should check if node belongs to same VAC.
VGC_VACOMPLEX_API
void moveToGroup(Node* node, Group* parentGroup, Node* nextSibling = nullptr);

VGC_VACOMPLEX_API
void moveBelowBoundary(Node* node);

/// Raises the given `nodes` (and their boundary) above the bottom-most node
/// (and its boundary) that is above all `nodes` and overlaps the `nodes`.
///
/// The nodes must belong to the same group, otherwise `LogicError()` is thrown.
///
VGC_VACOMPLEX_API
void raise(core::ConstSpan<Node*> nodes, core::AnimTime t);

/// Lowers the given `nodes` (and their star) below the top-most node (and its
/// star) that is below all `nodes` and overlaps at least one of the `nodes`.
///
/// The nodes must belong to the same group, otherwise `LogicError()` is thrown.
///
VGC_VACOMPLEX_API
void lower(core::ConstSpan<Node*> nodes, core::AnimTime t);

// TODO: move to `keyVertex->data().setPosition()`, similarly to
// `keyEdge->data().setStroke()`.
//
VGC_VACOMPLEX_API
void setKeyVertexPosition(KeyVertex* vertex, const geometry::Vec2d& pos);

} // namespace ops
} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_OPERATIONS_H
