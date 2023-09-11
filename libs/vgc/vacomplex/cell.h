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

#ifndef VGC_VACOMPLEX_CELL_H
#define VGC_VACOMPLEX_CELL_H

#include <optional>
#include <type_traits>

#include <vgc/core/animtime.h>
#include <vgc/core/id.h>
#include <vgc/core/object.h>
#include <vgc/core/templateutil.h>
#include <vgc/geometry/curve.h>
#include <vgc/geometry/mat2d.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/range1d.h>
#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/keyedgedata.h>
#include <vgc/vacomplex/transform.h>

namespace vgc::vacomplex {

namespace detail {

class Operations;

} // namespace detail

class Complex;

class Node;
class Group;
class Cell;

class VertexCell;
class EdgeCell;
class FaceCell;

class KeyCell;
class KeyVertex;
class KeyEdge;
class KeyFace;

class InbetweenCell;
class InbetweenVertex;
class InbetweenEdge;
class InbetweenFace;

class KeyEdgeData;
class FaceGeometry;

/// \enum vgc::graph::CellSpatialType
/// \brief Specifies the spatial type of a Cell.
// clang-format off
enum class CellSpatialType : UInt8 {
    Vertex = 0,
    Edge   = 1,
    Face   = 2,
};
// clang-format on

/// \enum vgc::graph::CellTemporalType
/// \brief Specifies the temporal type of a Cell.
// clang-format off
enum class CellTemporalType : UInt8 {
    Key       = 0,
    Inbetween = 1,
};
// clang-format on

/// \enum vgc::graph::CellType
/// \brief Specifies the type of a VAC Cell.
// clang-format off
enum class CellType : UInt8 {
    KeyVertex       = 0,
    KeyEdge         = 1,
    KeyFace         = 2,
    // 3 is skipped for bit masking
    InbetweenVertex = 4,
    InbetweenEdge   = 5,
    InbetweenFace   = 6,
};
// clang-format on

namespace detail {

inline constexpr CellSpatialType cellTypeToSpatialType(CellType x) {
    return static_cast<CellSpatialType>(core::toUnderlying(x) & 3);
}

inline constexpr CellTemporalType cellTypeToTemporalType(CellType x) {
    return static_cast<CellTemporalType>((core::toUnderlying(x) >> 2) & 1);
}

inline constexpr CellType vacCellTypeCombine(CellSpatialType st, CellTemporalType tt) {
    return static_cast<CellType>(
        ((core::toUnderlying(tt) << 2) | core::toUnderlying(st)));
}

template<typename Derived, typename Child>
class TreeParentBase;

template<typename Derived, typename Parent>
class TreeChildBase;

template<typename T>
class TreeChildrenIterator {
public:
    using difference_type = Int;
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using iterator_category = std::forward_iterator_tag;

    constexpr TreeChildrenIterator() noexcept = default;

    explicit TreeChildrenIterator(T* p)
        : p_(p) {
    }

    TreeChildrenIterator& operator++() {
        p_ = p_->nextSibling();
        return *this;
    }

    TreeChildrenIterator operator++(int) {
        TreeChildrenIterator cpy(*this);
        operator++();
        return cpy;
    }

    T* const& operator*() const {
        return p_;
    }

    T* const* operator->() const {
        return &p_;
    }

    bool operator==(const TreeChildrenIterator& other) const {
        return p_ == other.p_;
    }

    bool operator!=(const TreeChildrenIterator& other) const {
        return p_ != other.p_;
    }

private:
    T* p_ = nullptr;
};

// non-owning-tree child
// nothing is done automatically on destruction
template<typename Child, typename Parent = Child>
class TreeChildBase {
private:
    friend TreeParentBase<Parent, Child>;
    friend TreeChildrenIterator<TreeChildBase>;

protected:
    Child* previousSibling() const {
        return previousSibling_;
    }

    Child* nextSibling() const {
        return nextSibling_;
    }

    Parent* parent() const {
        return parent_;
    }

    void unparent();

private:
    Child* previousSibling_ = nullptr;
    Child* nextSibling_ = nullptr;
    Parent* parent_ = nullptr;
};

// non-owning-tree parent
// nothing is done automatically on destruction
template<typename Parent, typename Child = Parent>
class TreeParentBase {
private:
    using ChildBase = TreeChildBase<Child, Parent>;
    friend ChildBase;

protected:
    using Iterator = TreeChildrenIterator<Child>;

    constexpr TreeParentBase() noexcept {
        static_assert(std::is_base_of_v<ChildBase, Child>);
        static_assert(std::is_base_of_v<ChildBase, Parent>);
        static_assert(std::is_base_of_v<ChildBase, Parent>);
    }

    Child* firstChild() const {
        return firstChild_;
    }

    Child* lastChild() const {
        return lastChild_;
    }

    Iterator begin() const {
        return Iterator(firstChild_);
    }

    Iterator end() const {
        return Iterator();
    }

    Int numChildren() const {
        return numChildren_;
    }

    void resetChildrenNoUnlink() {
        numChildren_ = 0;
        firstChild_ = nullptr;
        lastChild_ = nullptr;
    }

    bool appendChild(Child* child) {
        return insertChildUnchecked(nullptr, child);
    }

    // assumes nextSibling is nullptr or a child of this
    bool insertChildUnchecked(Child* nextSibling, Child* child) {

        Child* const newNextSibling = nextSibling;
        if (child == newNextSibling) {
            return false;
        }

        Child* const newPreviousSibling =
            newNextSibling ? newNextSibling->previousSibling_ : lastChild_;
        if (child == newPreviousSibling) {
            return false;
        }

        Parent* const oldParent = child->parent_;
        Child* const oldPreviousSibling = child->previousSibling_;
        Child* const oldNextSibling = child->nextSibling_;

        if (oldPreviousSibling) {
            oldPreviousSibling->nextSibling_ = oldNextSibling;
        }
        else if (oldParent) {
            oldParent->firstChild_ = oldNextSibling;
        }

        if (oldNextSibling) {
            oldNextSibling->previousSibling_ = oldPreviousSibling;
        }
        else if (oldParent) {
            oldParent->lastChild_ = oldPreviousSibling;
        }

        if (newPreviousSibling) {
            newPreviousSibling->nextSibling_ = child;
        }
        else {
            firstChild_ = child;
        }

        if (newNextSibling) {
            newNextSibling->previousSibling_ = child;
        }
        else {
            lastChild_ = child;
        }

        child->previousSibling_ = newPreviousSibling;
        child->nextSibling_ = newNextSibling;

        if (oldParent != this) {
            child->parent_ = static_cast<Parent*>(this);
            ++numChildren_;
            if (oldParent) {
                --(oldParent->numChildren_);
            }
        }

        return true;
    }

private:
    Child* firstChild_ = nullptr;
    Child* lastChild_ = nullptr;
    Int numChildren_ = 0;
};

template<typename Derived, typename Parent>
void TreeChildBase<Derived, Parent>::unparent() {

    Parent* const oldParent = parent_;
    Derived* const oldPreviousSibling = previousSibling_;
    Derived* const oldNextSibling = nextSibling_;

    if (oldPreviousSibling) {
        oldPreviousSibling->nextSibling_ = oldNextSibling;
        previousSibling_ = nullptr;
    }
    else if (oldParent) {
        oldParent->firstChild_ = oldNextSibling;
    }

    if (oldNextSibling) {
        oldNextSibling->previousSibling_ = oldPreviousSibling;
        nextSibling_ = nullptr;
    }
    else if (oldParent) {
        oldParent->lastChild_ = oldPreviousSibling;
    }

    if (oldParent) {
        --(oldParent->numChildren_);
        parent_ = nullptr;
    }
}

template<typename Node>
struct DefaultTreeLinksGetter {
    static Node* parent(Node* n) {
        return n->parent();
    }
    static Node* previousSibling(Node* n) {
        return n->previousSibling();
    }
    static Node* nextSibling(Node* n) {
        return n->nextSibling();
    }
    static Node* firstChild(Node* n) {
        return n->firstChild();
    }
    static Node* lastChild(Node* n) {
        return n->lastChild();
    }
};

template<typename Node, typename SFINAE = void>
struct GetTreeLinksGetter_;

template<typename Node>
struct GetTreeLinksGetter_<Node, core::RequiresValid<typename Node::TreeLinksGetter>> {
    using type = typename Node::TreeLinksGetter;
};

template<typename Derived>
class TreeNodeBase : public TreeChildBase<Derived>, public TreeParentBase<Derived> {
private:
    template<typename Node>
    friend struct DefaultTreeLinksGetter;

public:
    using TreeLinksGetter = DefaultTreeLinksGetter<Derived>;
};

template<typename Node>
using TreeLinksGetter = typename GetTreeLinksGetter_<Node>::type;

} // namespace detail

class VGC_VACOMPLEX_API Node : public detail::TreeChildBase<Node, Group> {
private:
    friend detail::Operations;

    using TreeChildBase = detail::TreeChildBase<Node, Group>;

    static constexpr UInt8 notACell = static_cast<UInt8>(-1);

protected:
    Node(core::Id id) noexcept;

    Node(core::Id id, CellType cellType) noexcept;

public:
    virtual ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    Node* previousSibling() const {
        return TreeChildBase::previousSibling();
    }

    Node* nextSibling() const {
        return TreeChildBase::nextSibling();
    }

    Group* parentGroup() const {
        return TreeChildBase::parent();
    }

    core::Id id() const {
        return id_;
    }

    inline Complex* complex() const;

    bool isCell() const {
        return cellType_ != notACell;
    }

    bool isGroup() const {
        return !isCell();
    }

    Cell* toCell() {
        return isCell() ? toCellUnchecked() : nullptr;
    }

    const Cell* toCell() const {
        return const_cast<Node*>(this)->toCell();
    }

    inline Cell* toCellUnchecked();

    const Cell* toCellUnchecked() const {
        return const_cast<Node*>(this)->toCellUnchecked();
    }

    Group* toGroup() {
        return isGroup() ? toGroupUnchecked() : nullptr;
    }

    const Group* toGroup() const {
        return const_cast<Node*>(this)->toGroup();
    }

    inline Group* toGroupUnchecked();

    const Group* toGroupUnchecked() const {
        return const_cast<Node*>(this)->toGroupUnchecked();
    }

    virtual geometry::Rect2d boundingBoxAt(core::AnimTime t) const = 0;

    void debugPrint(core::StringWriter& out);

protected:
    CellType cellTypeUnchecked() const {
        return static_cast<CellType>(cellType_);
    }

private:
    friend Complex;

    core::Id id_ = -1;
    const UInt8 cellType_;
    // used during hard/soft delete operations
    bool isBeingDeleted_ = false;
    bool canBeAtomicallyUncut_ = false;

    virtual void debugPrint_(core::StringWriter& out) = 0;
};

class VGC_VACOMPLEX_API Group : public Node, public detail::TreeParentBase<Group, Node> {
private:
    friend detail::Operations;

    using TreeParentBase = detail::TreeParentBase<Group, Node>;

public:
    using Iterator = detail::TreeChildrenIterator<Node>;

    ~Group() override = default;

protected:
    explicit Group(core::Id id, Complex* complex) noexcept
        : Node(id)
        , complex_(complex) {
    }

public:
    Complex* complex() const {
        return complex_;
    }

    /// Returns bottom-most child in depth order.
    ///
    Node* firstChild() const {
        return TreeParentBase::firstChild();
    }

    /// Returns top-most child in depth order.
    ///
    Node* lastChild() const {
        return TreeParentBase::lastChild();
    }

    Int numChildren() const {
        return TreeParentBase::numChildren();
    }

    Iterator begin() const {
        return TreeParentBase::begin();
    }

    Iterator end() const {
        return TreeParentBase::end();
    }

    geometry::Rect2d boundingBoxAt(core::AnimTime t) const override {
        geometry::Rect2d result = geometry::Rect2d::empty;
        for (Node* node : *this) {
            result.uniteWith(node->boundingBoxAt(t));
        }
        return result;
    }

    const Transform& transform() const {
        return transform_;
    }

    const Transform& inverseTransform() const {
        return inverseTransform_;
    }

    const Transform& transformFromRoot() const {
        return transformFromRoot_;
    }

    Transform computeInverseTransformTo(Group* ancestor) const;

    Transform computeInverseTransformToRoot() const {
        return computeInverseTransformTo(nullptr);
    }

private:
    friend Complex;
    friend detail::Operations;

    Complex* complex_ = nullptr;

    Transform transform_;
    // to speed-up working with cells connected from different groups
    Transform inverseTransform_;
    Transform transformFromRoot_;

    void onChildrenDestroyed() {
        TreeParentBase::resetChildrenNoUnlink();
    }

    void setTransform_(const Transform& transform);
    void updateTransformFromRoot_();

    void debugPrint_(core::StringWriter& out) override;
};

template<typename T>
class CellProxy;

/// Checks whether the type `T` is a Cell type.
///
template<typename T>
inline constexpr bool isCell =
    std::disjunction_v<std::is_base_of<Cell, T>, core::IsTemplateBaseOf<CellProxy, T>>;

template<typename To, typename From, VGC_REQUIRES((isCell<To> && isCell<From>))>
constexpr To* static_cell_cast(From* p);

template<typename To, typename From, VGC_REQUIRES((isCell<To> && isCell<From>))>
constexpr To* dynamic_cell_cast(From* p);

#define VGC_VACOMPLEX_UNSAFE_CAST_METHOD_NAME(ToType) to##ToType##Unchecked
#define VGC_VACOMPLEX_SAFE_CAST_METHOD_NAME(ToType) to##ToType

#define VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(To)                                        \
    template<typename To_ = To, VGC_REQUIRES(std::is_same_v<To_, To>)>                   \
    To* VGC_VACOMPLEX_SAFE_CAST_METHOD_NAME(To)() {                                      \
        return dynamic_cell_cast<To_>(this);                                             \
    }                                                                                    \
    template<typename To_ = To, VGC_REQUIRES(std::is_same_v<To_, To>)>                   \
    const To* VGC_VACOMPLEX_SAFE_CAST_METHOD_NAME(To)() const {                          \
        return dynamic_cell_cast<const To_>(this);                                       \
    }                                                                                    \
    template<typename To_ = To, VGC_REQUIRES(std::is_same_v<To_, To>)>                   \
    constexpr To* VGC_VACOMPLEX_UNSAFE_CAST_METHOD_NAME(To)() {                          \
        return static_cell_cast<To_>(this);                                              \
    }                                                                                    \
    template<typename To_ = To, VGC_REQUIRES(std::is_same_v<To_, To>)>                   \
    constexpr const To* VGC_VACOMPLEX_UNSAFE_CAST_METHOD_NAME(To)() const {              \
        return static_cell_cast<const To_>(this);                                        \
    }

#define VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(To)                                \
    To* VGC_VACOMPLEX_SAFE_CAST_METHOD_NAME(To)() = delete;                              \
    const To* VGC_VACOMPLEX_SAFE_CAST_METHOD_NAME(To)() const = delete;                  \
    constexpr To* VGC_VACOMPLEX_UNSAFE_CAST_METHOD_NAME(To)() = delete;                  \
    constexpr const To* VGC_VACOMPLEX_UNSAFE_CAST_METHOD_NAME(To)() const = delete;

#define VGC_VACOMPLEX_DEFINE_CELL_UPCAST_METHOD(To)                                      \
    template<typename To_ = To, VGC_REQUIRES(std::is_same_v<To_, To>)>                   \
    To* VGC_VACOMPLEX_SAFE_CAST_METHOD_NAME(To)() {                                      \
        return static_cell_cast<To_>(this);                                              \
    }                                                                                    \
    template<typename To_ = To, VGC_REQUIRES(std::is_same_v<To_, To>)>                   \
    const To* VGC_VACOMPLEX_SAFE_CAST_METHOD_NAME(To)() const {                          \
        return static_cell_cast<const To_>(this);                                        \
    }                                                                                    \
    template<typename To_ = To, VGC_REQUIRES(std::is_same_v<To_, To>)>                   \
    To* VGC_VACOMPLEX_UNSAFE_CAST_METHOD_NAME(To)() {                                    \
        return static_cell_cast<To_>(this);                                              \
    }                                                                                    \
    template<typename To_ = To, VGC_REQUIRES(std::is_same_v<To_, To>)>                   \
    const To* VGC_VACOMPLEX_UNSAFE_CAST_METHOD_NAME(To)() const {                        \
        return static_cell_cast<const To_>(this);                                        \
    }

#define VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_Vertex_1 Edge
#define VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_Vertex_2 Face
#define VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_Edge_1 Vertex
#define VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_Edge_2 Face
#define VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_Face_1 Vertex
#define VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_Face_2 Edge
#define VGC_VACOMPLEX_TEMPORAL_NAME_ALTERNATIVE_Key Inbetween
#define VGC_VACOMPLEX_TEMPORAL_NAME_ALTERNATIVE_Inbetween Key

// X: Cell
// X -> Cell
// X -> [Key|Inbetween]Cell
// X -> [Vertex|Edge|Face]Cell
// X -> [Key|Inbetween][Vertex|Edge|Face]
#define VGC_VACOMPLEX_DEFINE_BASE_CELL_CAST_METHODS()                                    \
    VGC_VACOMPLEX_DEFINE_CELL_UPCAST_METHOD(Cell)                                        \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyCell)                                       \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenCell)                                 \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(VertexCell)                                    \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(EdgeCell)                                      \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(FaceCell)                                      \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyVertex)                                     \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyEdge)                                       \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyFace)                                       \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenVertex)                               \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenEdge)                                 \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenFace)

// X: Key|Inbetween
// XCell -> Cell (inherited)
// XCell -> X[Vertex|Edge|Face]
// XCell -> [Vertex|Edge|Face]Cell
#define VGC_VACOMPLEX_DEFINE_TEMPORAL_CELL_CAST_METHODS(Name)                            \
    /* toCell() inherited */                                                             \
    VGC_VACOMPLEX_DEFINE_CELL_UPCAST_METHOD(Name##Cell)                                  \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(VertexCell)                                    \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(EdgeCell)                                      \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(FaceCell)                                      \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(Name##Vertex)                                  \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(Name##Edge)                                    \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(Name##Face)                                    \
    VGC_VACOMPLEX_DEFINE_TEMPORAL_CELL_CAST_DELETED_METHODS_(                            \
        VGC_VACOMPLEX_TEMPORAL_NAME_ALTERNATIVE_##Name)
#define VGC_VACOMPLEX_DEFINE_TEMPORAL_CELL_CAST_DELETED_METHODS_(OtherName)              \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(OtherName, Cell))           \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(OtherName, Vertex))         \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(OtherName, Edge))           \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(OtherName, Face))

// X: Vertex|Edge|Face
// XCell -> Cell
// XCell -> [Key|Inbetween]Cell
// XCell -> [Key|Inbetween]X
#define VGC_VACOMPLEX_DEFINE_SPATIAL_CELL_CAST_METHODS(Name)                             \
    /* toCell() inherited */                                                             \
    VGC_VACOMPLEX_DEFINE_CELL_UPCAST_METHOD(Name##Cell)                                  \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyCell)                                       \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenCell)                                 \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(Key##Name)                                     \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(Inbetween##Name)                               \
    VGC_VACOMPLEX_DEFINE_SPATIAL_CELL_CAST_DELETED_METHODS_(                             \
        VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_##Name##_1)                               \
    VGC_VACOMPLEX_DEFINE_SPATIAL_CELL_CAST_DELETED_METHODS_(                             \
        VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_##Name##_2)
#define VGC_VACOMPLEX_DEFINE_SPATIAL_CELL_CAST_DELETED_METHODS_(OtherName)               \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(OtherName, Cell))           \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(Key, OtherName))            \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(Inbetween, OtherName))

// T: [Key|Inbetween]
// S: [Vertex|Edge|Face]
// X: TS
// X -> Cell (inherited)
// X -> TCell
// X -> SCell
#define VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(TName, SName)              \
    /* toCell() inherited */                                                             \
    using TName##Cell::toKeyCell;                                                        \
    using TName##Cell::toKeyCellUnchecked;                                               \
    using TName##Cell::toInbetweenCell;                                                  \
    using TName##Cell::toInbetweenCellUnchecked;                                         \
    using SName##Cell::toVertexCell;                                                     \
    using SName##Cell::toVertexCellUnchecked;                                            \
    using SName##Cell::toEdgeCell;                                                       \
    using SName##Cell::toEdgeCellUnchecked;                                              \
    using SName##Cell::toFaceCell;                                                       \
    using SName##Cell::toFaceCellUnchecked;                                              \
    VGC_VACOMPLEX_DEFINE_CELL_UPCAST_METHOD(TName##SName)                                \
    VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_DELETED_METHODS_(                      \
        TName,                                                                           \
        VGC_VACOMPLEX_TEMPORAL_NAME_ALTERNATIVE_##TName,                                 \
        SName,                                                                           \
        VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_##SName##_1,                              \
        VGC_VACOMPLEX_SPATIAL_NAME_ALTERNATIVE_##SName##_2)
#define VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_DELETED_METHODS_(                  \
    TName, OtherTName, SName, OtherSName1, OtherSName2)                                  \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(TName, OtherSName1))        \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(TName, OtherSName2))        \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(OtherTName, Vertex))        \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(OtherTName, Edge))          \
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD_DELETED(VGC_PP_CAT(OtherTName, Face))

// boundaries:
//  key vertex  -> none
//  key edge    -> 2 key vertices
//  key face    -> N key vertices, key edges
//  ib vertex   -> 2 key vertices
//  ib edge     -> N key vertices, ib vertices, key edges
//  ib face     -> N key faces, ib edges
//
// stars:
//  key vertex  -> ib vertices, key edges, ib edges, key faces, ib faces
//  key edge    ->
//  key face    ->
//  ib vertex   ->
//  ib edge     ->
//  ib face     ->
//
// additional repr:
//  key face    -> cycle    = half key edges
//  ib edge     -> path     = half key edges
//              -> animvtx  = ib vertices
//  ib face     -> animcycl = planar graph with key verts, ib verts, key edges, ib edges
//

class CellRangeView {
private:
    using Container = core::Array<Cell*>;

    friend Cell;

    CellRangeView(const Container& container)
        : container_(container) {
    }

public:
    using ConstIterator = Container::const_iterator;

    ConstIterator begin() const {
        return container_.begin();
    }

    ConstIterator end() const {
        return container_.end();
    }

    Int length() const {
        return container_.length();
    }

    bool isEmpty() const {
        return container_.isEmpty();
    }

    core::Array<Cell*> copy() const {
        return container_;
    }

private:
    const Container& container_;
};

class VGC_VACOMPLEX_API Cell : public Node {
private:
    friend detail::Operations;

    friend KeyCell;
    friend InbetweenCell;

    friend VertexCell;
    friend EdgeCell;
    friend FaceCell;

    Cell();

protected:
    Cell(
        core::Id id,
        CellSpatialType spatialType,
        CellTemporalType temporalType) noexcept;

public:
    ~Cell() override = default;

    Complex* complex() const;

    Cell* previousSiblingCell() const {
        Node* node = previousSibling();
        while (node) {
            if (node->isCell()) {
                return node->toCellUnchecked();
            }
            node = node->previousSibling();
        }
        return nullptr;
    }

    Cell* nextSiblingCell() const {
        Node* node = nextSibling();
        while (node) {
            if (node->isCell()) {
                return node->toCellUnchecked();
            }
            node = node->nextSibling();
        }
        return nullptr;
    }

    /// Returns the cell type of this `Cell`.
    ///
    CellType cellType() const {
        return cellTypeUnchecked();
    }

    CellSpatialType spatialType() const {
        return detail::cellTypeToSpatialType(cellType());
    }

    CellTemporalType temporalType() const {
        return detail::cellTypeToTemporalType(cellType());
    }

    bool isKeyCell() const {
        return detail::cellTypeToTemporalType(cellType()) == CellTemporalType::Key;
    }

    bool isInbetweenCell() const {
        return detail::cellTypeToTemporalType(cellType()) == CellTemporalType::Inbetween;
    }

    virtual bool existsAt(core::AnimTime t) const = 0;

    geometry::Rect2d boundingBoxAt(core::AnimTime t) const override = 0;

    CellRangeView star() const {
        return CellRangeView(star_);
    }

    CellRangeView boundary() const {
        return CellRangeView(boundary_);
    }

    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(VertexCell)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(EdgeCell)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(FaceCell)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyCell)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenCell)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyVertex)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyEdge)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(KeyFace)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenVertex)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenEdge)
    VGC_VACOMPLEX_DEFINE_CELL_CAST_METHOD(InbetweenFace)

protected:
    void onMeshQueried() const {
        hasMeshBeenQueriedSinceLastDirtyEvent_ = true;
    }

    virtual void dirtyMesh();
    virtual bool updateGeometryFromBoundary();

private:
    // Assumes `oldVertex` is in boundary.
    //
    virtual void substituteKeyVertex_(KeyVertex* oldVertex, KeyVertex* newVertex) = 0;

    // Assumes old edge is in boundary, `oldHalfedge != newHalfedge`,
    // and end vertices match.
    //
    virtual void substituteKeyEdge_(
        const class KeyHalfedge& oldHalfedge,
        const class KeyHalfedge& newHalfedge) = 0;

private:
    core::Array<Cell*> star_;
    core::Array<Cell*> boundary_;

    // This flag is used to not signal NodeModificationFlag::MeshChanged
    // multiple times if no dependent nodes nor the user has queried
    // the new mesh. It should be set to true (either directly
    // or indirectly) in all mesh getters.
    mutable bool hasMeshBeenQueriedSinceLastDirtyEvent_ = false;
};

inline Complex* Node::complex() const {
    return isCell() ? toCellUnchecked()->complex() : toGroupUnchecked()->complex();
}

inline Cell* Node::toCellUnchecked() {
    return static_cast<Cell*>(this);
}

inline Group* Node::toGroupUnchecked() {
    return static_cast<Group*>(this);
}

template<typename T>
class CellProxy {
protected:
    constexpr CellProxy() noexcept = default;
    virtual ~CellProxy() = default;

public:
    constexpr Cell* cell();

    constexpr const Cell* cell() const {
        return const_cast<CellProxy*>(this)->cell();
    }

    Complex* complex() const {
        return cell()->complex();
    }

    CellType cellType() const {
        return cell()->cellType();
    }

    CellSpatialType spatialType() const {
        return cell()->spatialType();
    }

    CellTemporalType temporalType() const {
        return cell()->temporalType();
    }
};

// Note: CellProxy is first in the layout, allowing for direct reinterpretation
//       of KeyCell* and InbetweenCell* to any final type.
//       However a Cell* needs to be shifted when casting to any final type.
template<typename SpatialCell, typename TemporalCell>
class SpatioTemporalCell : public TemporalCell, public SpatialCell {
public:
    static_assert(std::is_base_of_v<Cell, SpatialCell>);
    static_assert(std::is_base_of_v<CellProxy<TemporalCell>, TemporalCell>);

    template<typename... TemporalCellArgs>
    SpatioTemporalCell(core::Id id, TemporalCellArgs&&... args) noexcept
        : TemporalCell(std::forward<TemporalCellArgs>(args)...)
        , SpatialCell(id, TemporalCell::temporalType()) {
    }

    using Cell::cellType;
    using Cell::complex;
    using SpatialCell::spatialType;
    using TemporalCell::temporalType;

    static constexpr CellType cellType() {
        return detail::vacCellTypeCombine(spatialType(), temporalType());
    }

    bool existsAt(core::AnimTime t) const final {
        return TemporalCell::existsAt(t);
    }

    virtual geometry::Rect2d boundingBoxAt(core::AnimTime t) const = 0;
};

class VGC_VACOMPLEX_API KeyCell : public CellProxy<KeyCell> {
private:
    friend detail::Operations;

protected:
    explicit constexpr KeyCell(const core::AnimTime& time) noexcept
        : CellProxy<KeyCell>()
        , time_(time) {
    }

public:
    static constexpr CellTemporalType temporalType() {
        return CellTemporalType::Key;
    }

    constexpr core::AnimTime time() const {
        return time_;
    }

    bool existsAt(core::AnimTime t) const {
        return t == time_;
    }

    virtual geometry::Rect2d boundingBox() const = 0;

    geometry::Rect2d boundingBoxAt(core::AnimTime t) const {
        if (existsAt(t)) {
            return boundingBox();
        }
        return geometry::Rect2d::empty;
    }

    VGC_VACOMPLEX_DEFINE_TEMPORAL_CELL_CAST_METHODS(Key)

private:
    core::AnimTime time_;
};

class VGC_VACOMPLEX_API InbetweenCell : public CellProxy<InbetweenCell> {
private:
    friend detail::Operations;

protected:
    explicit constexpr InbetweenCell() noexcept
        : CellProxy<InbetweenCell>() {
    }

public:
    static constexpr CellTemporalType temporalType() {
        return CellTemporalType::Inbetween;
    }

    // virtual api

    bool existsAt(core::AnimTime t) const {
        return timeRange_.contains(t);
    }

    virtual geometry::Rect2d boundingBoxAt(core::AnimTime t) const = 0;

    VGC_VACOMPLEX_DEFINE_TEMPORAL_CELL_CAST_METHODS(Inbetween)

private:
    core::AnimTimeRange timeRange_;
};

class VGC_VACOMPLEX_API VertexCell : public Cell {
private:
    friend detail::Operations;

protected:
    VertexCell(core::Id id, CellTemporalType temporalType) noexcept
        : Cell(id, spatialType(), temporalType) {
    }

public:
    static constexpr CellSpatialType spatialType() {
        return CellSpatialType::Vertex;
    }

    VGC_VACOMPLEX_DEFINE_SPATIAL_CELL_CAST_METHODS(Vertex)

    virtual geometry::Vec2d position(core::AnimTime t) const = 0;
};

class VGC_VACOMPLEX_API EdgeCell : public Cell {
private:
    friend detail::Operations;

protected:
    EdgeCell(core::Id id, CellTemporalType temporalType) noexcept
        : Cell(id, spatialType(), temporalType) {
    }

public:
    static constexpr CellSpatialType spatialType() {
        return CellSpatialType::Edge;
    }

    VGC_VACOMPLEX_DEFINE_SPATIAL_CELL_CAST_METHODS(Edge)

    // virtual api

    virtual bool isStartVertex(VertexCell* v) const = 0;
    virtual bool isEndVertex(VertexCell* v) const = 0;
    virtual bool isClosed() const = 0;

    // note: Looks best to return an object so that we can change its impl
    //       if we want to share the data. The straight forward implementation
    //       is to not cache this result in the cell, otherwise we'd have to manage
    //       a cache array in inbetween cells.
    //virtual EdgeGeometry computeSamplingAt(core::AnimTime /*t*/) = 0;
};

class VGC_VACOMPLEX_API FaceCell : public Cell {
private:
    friend detail::Operations;

protected:
    FaceCell(core::Id id, CellTemporalType temporalType) noexcept
        : Cell(id, spatialType(), temporalType) {
    }

public:
    static constexpr CellSpatialType spatialType() {
        return CellSpatialType::Face;
    }

    VGC_VACOMPLEX_DEFINE_SPATIAL_CELL_CAST_METHODS(Face)

    // virtual api
};

class VGC_VACOMPLEX_API VertexUsage {
    // def depends on how we'll represent keyface boundaries
};

namespace detail {

struct DummySpatialCell : Cell {};

template<typename T, typename SFINAE = void>
struct CellSpatialTraits {
    static constexpr bool hasSpatialType = false;
};

template<typename T>
struct CellSpatialTraits<
    T,
    core::Requires<std::disjunction_v<
        std::is_base_of<VertexCell, T>,
        std::is_base_of<EdgeCell, T>,
        std::is_base_of<FaceCell, T>>>> {

    static constexpr bool hasSpatialType = true;
    static constexpr CellSpatialType spatialType = T::spatialType();
};

template<typename T, typename SFINAE = void>
struct CellTemporalTraits {
    static constexpr bool hasTemporalType = false;
};

template<typename T>
struct CellTemporalTraits<
    T,
    core::Requires<std::disjunction_v<
        std::is_base_of<KeyCell, T>,
        std::is_base_of<InbetweenCell, T>>>> {

    static constexpr bool hasTemporalType = true;
    static constexpr CellTemporalType temporalType = T::temporalType();
    using TemporalAny = SpatioTemporalCell<DummySpatialCell, T>;
};

} // namespace detail

template<typename T, typename SFINAE = void>
struct CellTraits : detail::CellSpatialTraits<T>, detail::CellTemporalTraits<T> {
    static constexpr bool hasSpatioTemporalType = false;
};

template<typename T>
struct CellTraits<
    T,
    core::Requires<
        detail::CellSpatialTraits<T>::hasSpatialType
        && detail::CellTemporalTraits<T>::hasTemporalType>>
    : detail::CellSpatialTraits<T>, detail::CellTemporalTraits<T> {

    static constexpr bool hasSpatioTemporalType = true;
    static constexpr CellType cellType = detail::vacCellTypeCombine(
        detail::CellSpatialTraits<T>::spatialType,
        detail::CellTemporalTraits<T>::temporalType);
};

template<typename T>
constexpr Cell* CellProxy<T>::cell() {
    return static_cast<Cell*>(static_cast<typename CellTraits<T>::TemporalAny*>(this));
}

// Note: with C++20, ADL will work with explicitly instanciated template functions.
// In other words, in C++17, if you're not already in the vacomplex namespace, you have to write:
//     `vacomplex::static_cell_cast<topology::KeyVertex>(cell)`
//
// In C++20, you can more simply write:
//     `static_cell_cast<vacomplex::KeyVertex>(cell)`
//
// As an alternative, you can always write:
//     `cell->toKeyVertexUnchecked()`
//
template<typename To, typename From, VGC_FORWARDED_REQUIRES((isCell<To> && isCell<From>))>
constexpr To* static_cell_cast(From* p) {

    // Check const to non-const
    if constexpr (std::is_const_v<From> && !std::is_const_v<To>) {
        static_assert(
            core::dependentFalse<To>,
            "Invalid static_cell_cast from const to non-const.");
    }

    // Check To and From are complete
    VGC_ASSERT_TYPE_IS_COMPLETE(To, "Invalid static_cell_cast to incomplete type To.");
    VGC_ASSERT_TYPE_IS_COMPLETE(
        From, "Invalid static_cell_cast from incomplete type From.");

    // Retrieve non-const To and From
    using NcTo = std::remove_cv_t<To>;
    using NcFrom = std::remove_cv_t<From>;

    using ToTraits = CellTraits<NcTo>;
    using FromTraits = CellTraits<NcFrom>;

    if constexpr (std::is_convertible_v<From*, To*>) {
        // trivial: identity or upcast
        return p;
    }
    else if constexpr (std::is_base_of_v<NcFrom, NcTo>) {
        // trivial: downcast
        return static_cast<To*>(p);
    }
    else if constexpr (FromTraits::hasSpatialType || std::is_same_v<NcFrom, Cell>) {
        static_assert(
            !FromTraits::hasTemporalType,
            "Logic error in static_cell_cast: "
            "spatio-temporal type `From` should have been handled by upcast.");
        // check invalid cast: spatial type conflict
        if constexpr (ToTraits::hasSpatialType) {
            static_assert(
                !ToTraits::hasTemporalType,
                "Logic error in static_cell_cast: "
                "spatio-temporal type `To` should have been handled by downcast.");
            static_assert(
                ToTraits::spatialType == FromTraits::spatialType,
                "Invalid static_cell_cast with confliting spatial types.");
        }
        // NcFrom is in {VertexCell, EdgeCell, FaceCell, VaCell}
        // NcTo is in {KeyCell, InbetweenCell}
        return static_cast<To*>(static_cast<typename ToTraits::TemporalAny*>(
            static_cast<Cell*>(const_cast<NcFrom*>(p))));
    }
    else if constexpr (FromTraits::hasTemporalType) {
        static_assert(
            !FromTraits::hasSpatialType,
            "Logic error in static_cell_cast: "
            "spatio-temporal type `From` should have been handled by upcast.");
        // check invalid cast: temporal type conflict
        if constexpr (ToTraits::hasTemporalType) {
            static_assert(
                !ToTraits::hasSpatialType,
                "Logic error in static_cell_cast: "
                "spatio-temporal type `To` should have been handled by downcast.");
            static_assert(
                ToTraits::temporalType == FromTraits::temporalType,
                "Invalid static_cell_cast with confliting temporal types.");
        }
        // NcFrom is in {KeyCell, InbetweenCell}
        // NcTo is in {VertexCell, EdgeCell, FaceCell, Cell}
        return static_cast<To*>(static_cast<Cell*>(
            static_cast<typename ToTraits::TemporalAny*>(const_cast<NcFrom*>(p))));
    }
    else {
        static_assert(
            core::dependentFalse<To>,
            "Logic error in static_cell_cast: case not handled.");
    }
}

// Note: with C++20, ADL will work with explicitly instanciated template functions.
template<typename To, typename From, VGC_FORWARDED_REQUIRES((isCell<To> && isCell<From>))>
constexpr To* dynamic_cell_cast(From* p) {

    // Check const to non-const
    if constexpr (std::is_const_v<From> && !std::is_const_v<To>) {
        static_assert(
            core::dependentFalse<To>,
            "Invalid dynamic_cell_cast from const to non-const.");
    }

    // Check To and From are complete
    VGC_ASSERT_TYPE_IS_COMPLETE(To, "Invalid dynamic_cell_cast to incomplete type To.");
    VGC_ASSERT_TYPE_IS_COMPLETE(
        From, "Invalid dynamic_cell_cast from incomplete type From.");

    // Retrieve non-const To and From
    using NcTo = std::remove_cv_t<To>;
    using NcFrom = std::remove_cv_t<From>;

    using ToTraits = CellTraits<NcTo>;
    using FromTraits = CellTraits<NcFrom>;

    if constexpr (std::is_convertible_v<From*, To*>) {
        // trivial: identity or upcast
        return p;
    }
    else if constexpr (ToTraits::hasSpatioTemporalType) {
        // NcTo is in {KeyVertex, KeyEdge, KeyFace,
        //             InbetweenVertex, InbetweenEdge, InbetweenFace}
        // check invalid cast: spatial type conflict
        if constexpr (FromTraits::hasSpatialType) {
            static_assert(
                ToTraits::spatialType == FromTraits::spatialType,
                "Invalid dynamic_cell_cast with confliting spatial types.");
        }
        // check invalid cast: temporal type conflict
        if constexpr (FromTraits::hasTemporalType) {
            static_assert(
                ToTraits::temporalType == FromTraits::temporalType,
                "Invalid dynamic_cell_cast with confliting temporal types.");
        }
        if (p->cellType() == ToTraits::cellType) {
            return static_cast<To*>(p);
        }
        return nullptr;
    }
    else if constexpr (ToTraits::hasSpatialType) {
        // NcTo is in {VertexCell, EdgeCell, FaceCell}
        // check invalid cast: spatial type conflict
        if constexpr (FromTraits::hasSpatialType) {
            static_assert(
                ToTraits::spatialType == FromTraits::spatialType,
                "Invalid dynamic_cell_cast with confliting spatial types.");
        }
        if (p->spatialType() == ToTraits::spatialType) {
            if constexpr (FromTraits::hasTemporalType && !FromTraits::hasSpatialType) {
                // NcFrom is in {KeyCell, InbetweenCell}
                // example: KeyCell -> TemporalAny -> Cell -> VertexCell
                return static_cast<To*>(
                    static_cast<Cell*>(static_cast<typename FromTraits::TemporalAny*>(
                        const_cast<NcFrom*>(p))));
            }
            else {
                // NcFrom is in {VertexCell, EdgeCell, FaceCell, Cell}
                return static_cast<To*>(p);
            }
        }
        return nullptr;
    }
    else if constexpr (ToTraits::hasTemporalType) {
        // NcTo is in {KeyCell, InbetweenCell}
        // check invalid cast: temporal type conflict
        if constexpr (FromTraits::hasTemporalType) {
            static_assert(
                ToTraits::temporalType == FromTraits::temporalType,
                "Invalid dynamic_cell_cast with confliting temporal types.");
        }
        if (p->temporalType() == ToTraits::temporalType) {
            if constexpr (!FromTraits::hasTemporalType) {
                // NcFrom is in {VertexCell, EdgeCell, FaceCell, Cell}
                // example: VertexCell -> Cell -> TemporalAny -> KeyCell
                return static_cast<To*>(static_cast<typename ToTraits::TemporalAny*>(
                    static_cast<Cell*>(const_cast<NcFrom*>(p))));
            }
            else {
                // NcFrom is in {KeyCell, InbetweenCell}
                return static_cast<To*>(p);
            }
        }
        return nullptr;
    }
    else if constexpr (std::is_same_v<NcTo, Cell>) {
        // NcTo is in {Cell}
        // NcFrom is in {KeyCell, InbetweenCell}
        static_assert(
            FromTraits::hasTemporalType && !FromTraits::hasSpatialType,
            "Logic error in dynamic_cell_cast: unexpected code path.");
        return static_cast<To*>(
            static_cast<typename FromTraits::TemporalAny*>(const_cast<NcFrom*>(p)));
    }
    else {
        static_assert(
            core::dependentFalse<To>,
            "Logic error in dynamic_cell_cast: case not handled.");
    }
}

#define VGC_VACOMPLEX_DEFINE_CAST(To)                                                    \
    template<typename From>                                                              \
    constexpr To* to##To(From* p) {                                                      \
        return dynamic_cell_cast<To, From>(p);                                           \
    }                                                                                    \
    template<typename From>                                                              \
    constexpr To* to##To##Unchecked(From* p) {                                           \
        return static_cell_cast<To, From>(p);                                            \
    }
VGC_VACOMPLEX_DEFINE_CAST(Cell)
VGC_VACOMPLEX_DEFINE_CAST(VertexCell)
VGC_VACOMPLEX_DEFINE_CAST(EdgeCell)
VGC_VACOMPLEX_DEFINE_CAST(FaceCell)
VGC_VACOMPLEX_DEFINE_CAST(KeyCell)
VGC_VACOMPLEX_DEFINE_CAST(KeyVertex)
VGC_VACOMPLEX_DEFINE_CAST(KeyEdge)
VGC_VACOMPLEX_DEFINE_CAST(KeyFace)
VGC_VACOMPLEX_DEFINE_CAST(InbetweenCell)
VGC_VACOMPLEX_DEFINE_CAST(InbetweenVertex)
VGC_VACOMPLEX_DEFINE_CAST(InbetweenEdge)
VGC_VACOMPLEX_DEFINE_CAST(InbetweenFace)
#undef VGC_VACOMPLEX_DEFINE_CAST

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_CELL_H
