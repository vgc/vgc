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

#include <vgc/vacomplex/detail/operations.h>

namespace vgc::vacomplex::detail {

// Note: Uncut does not yet support incident inbetween cells. As a
// workaround, we do nothing, as if uncutting here isn't possible, even
// though maybe in theory it is. In the future, we should handle the cases
// where uncutting is actually possible despite the presence of incident
// inbetween cells.
Operations::UncutAtKeyVertexInfo_ Operations::prepareUncutAtKeyVertex_(KeyVertex* kv) {
    UncutAtKeyVertexInfo_ result = {};

    for (Cell* starCell : kv->star()) {
        switch (starCell->cellType()) {
        case CellType::KeyEdge: {
            KeyEdge* ke = starCell->toKeyEdgeUnchecked();
            if (ke->isStartVertex(kv)) {
                if (!result.khe1.edge()) {
                    result.khe1 = KeyHalfedge(ke, false);
                }
                else if (!result.khe2.edge()) {
                    result.khe2 = KeyHalfedge(ke, true);
                }
                else {
                    // Cannot uncut if kv is used more than twice as edge vertex.
                    return result;
                }
            }
            if (ke->isEndVertex(kv)) {
                if (!result.khe1.edge()) {
                    result.khe1 = KeyHalfedge(ke, true);
                }
                else if (!result.khe2.edge()) {
                    result.khe2 = KeyHalfedge(ke, false);
                }
                else {
                    // Cannot uncut if kv is used more than twice as edge vertex.
                    return result;
                }
            }
            break;
        }
        case CellType::KeyFace: {
            KeyFace* kf = starCell->toKeyFaceUnchecked();
            Int cycleIndex = -1;
            for (const KeyCycle& cycle : kf->cycles()) {
                ++cycleIndex;
                if (cycle.steinerVertex() == kv) {
                    if (result.kf) {
                        // Cannot uncut if kv is used more than once as steiner vertex.
                        return result;
                    }
                    result.kf = kf;
                    result.cycleIndex = cycleIndex;
                }
            }
            break;
        }
        case CellType::InbetweenVertex: {
            //InbetweenVertex* iv = starCell->toInbetweenVertexUnchecked();
            // Currently not supported.
            return result;
            break;
        }
        default:
            break;
        }
    }

    if (result.khe1.edge()) {
        if (!result.kf && result.khe2.edge()) {
            if (result.khe1.edge() != result.khe2.edge()) {
                // If edges are different:
                // (inverse op: cut open edge)
                //
                //                     ┌─←─┐
                //                     │   C
                // o ───A──→ X ───B──→ o ──┘
                //
                // Uncutting at X means replacing the chain AB by D.
                // Thus the cycle B*A*ABC would become D*DC but
                // the cycle B*BC would not be representable anymore.
                //
                // In other words, we want the edges to always be used
                // consecutively in the cycles they are part of.
                //
                for (Cell* starCell : kv->star()) {
                    KeyFace* kf = starCell->toKeyFace();
                    if (!kf) {
                        continue;
                    }
                    for (const KeyCycle& cycle : kf->cycles()) {
                        if (cycle.steinerVertex()) {
                            continue;
                        }
                        KeyEdge* previousKe = cycle.halfedges().last().edge();
                        for (const KeyHalfedge& khe : cycle.halfedges()) {
                            if (khe.startVertex() == kv) {
                                if (khe.edge() == previousKe) {
                                    // Cannot uncut if kv is used as a u-turn in cycle.
                                    return result;
                                }
                            }
                            previousKe = khe.edge();
                        }
                    }
                }
                result.isValid = true;
            }
            else {
                // (inverse op: cut closed edge)
                // the only incident edge is a loop, and we don't
                // want kv to be used as a u-turn in any cycle.
                for (Cell* starCell : kv->star()) {
                    KeyFace* kf = starCell->toKeyFace();
                    if (!kf) {
                        continue;
                    }
                    for (const KeyCycle& cycle : kf->cycles()) {
                        if (cycle.steinerVertex()) {
                            continue;
                        }
                        if (cycle.halfedges().first().edge() != result.khe1.edge()) {
                            continue;
                        }
                        // All edges in this cycle are equal to result.khe1.edge().
                        // We require them to be in the same direction (no u-turn).
                        bool direction = cycle.halfedges().first().direction();
                        for (const KeyHalfedge& khe : cycle.halfedges()) {
                            if (khe.direction() != direction) {
                                // Cannot uncut if kv is used as a u-turn in cycle.
                                return result;
                            }
                        }
                    }
                }
                result.isValid = true;
            }
        }
    }
    else if (result.kf) {
        // (inverse op: cut face at vertex)
        result.isValid = true;
    }

    return result;
}

Operations::UncutAtKeyEdgeInfo_ Operations::prepareUncutAtKeyEdge_(KeyEdge* ke) {
    UncutAtKeyEdgeInfo_ result = {};

    for (Cell* starCell : ke->star()) {
        switch (starCell->cellType()) {
        case CellType::KeyFace: {
            KeyFace* kf = starCell->toKeyFaceUnchecked();
            Int cycleIndex = -1;
            for (const KeyCycle& cycle : kf->cycles()) {
                ++cycleIndex;
                if (cycle.steinerVertex()) {
                    continue;
                }
                Int componentIndex = -1;
                for (const KeyHalfedge& khe : cycle.halfedges()) {
                    ++componentIndex;
                    if (khe.edge() != ke) {
                        continue;
                    }
                    if (!result.kf1) {
                        result.kf1 = kf;
                        result.cycleIndex1 = cycleIndex;
                        result.componentIndex1 = componentIndex;
                    }
                    else if (!result.kf2) {
                        result.kf2 = kf;
                        result.cycleIndex2 = cycleIndex;
                        result.componentIndex2 = componentIndex;
                    }
                    else {
                        // Cannot uncut if used more than twice as face cycle component.
                        return result;
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }

    if (result.kf1 && result.kf2) {
        result.isValid = true;
    }

    return result;
}

UncutAtKeyVertexResult
Operations::uncutAtKeyVertex(KeyVertex* targetKv, bool smoothJoin) {

    UncutAtKeyVertexResult result = {};

    UncutAtKeyVertexInfo_ info = prepareUncutAtKeyVertex_(targetKv);
    if (!info.isValid) {
        return result;
    }

    KeyEdge* newKe = nullptr;

    if (info.kf) {
        // Remove Steiner vertex from face.
        //
        //       o-----------o                     o-----------o
        //       |      v    |     uncutAt(v)      |           |
        //       |     o     |    ------------>    |           |
        //       |  f        |                     |  f        |
        //       o-----------o                     o-----------o
        //
        info.kf->cycles_.removeAt(info.cycleIndex);
        removeFromBoundary_(info.kf, targetKv);
        result.resultKf = info.kf;
    }
    else if (info.khe1.edge() == info.khe2.edge()) {

        // Transform open edge into closed edge.
        //
        //             v
        //       .-----o-----.                     .-----------.
        //       |           |     uncutAt(v)      |           |
        //       |e          |    ------------>    |e'         |
        //       |           |                     |           |
        //       '-----------'                     '-----------'
        //
        //        open edge e                      closed edge e'
        // (startVertex == endVertex)
        //
        // XXX Do not create a new edge, but instead modify it in-place?
        //     This would be similar to uncut at edge that splits one cycle
        //     into two cycles in a face, without creating a new face:
        //
        //     o------o------o                     o------o------o
        //     |      |e     |                     |             |
        //     |   o--o--o   |     uncutAt(e)      |   o--o--o   |
        //     |   |     | f |    ------------>    |   |     | f |
        //     |   o-----o   |                     |   o-----o   |
        //     |             |                     |             |
        //     o-------------o                     o-------------o
        //
        KeyEdge* oldKe = info.khe1.edge();

        KeyEdgeData newData = oldKe->data();
        newData.closeStroke(smoothJoin);

        // Create new edge
        newKe = createKeyClosedEdge(
            std::move(newData), oldKe->parentGroup(), oldKe->nextSibling());
        result.resultKe = newKe;

        // Substitute all usages of old edge by new edge
        // XXX: Can we actually call substituteEdge_() here?
        //      cf doc "Substitutes open with open or closed with closed"
        //      and comment "It assumes end vertices are the same"
        KeyHalfedge oldKhe(oldKe, true);
        KeyHalfedge newKhe(newKe, true);
        substituteEdge_(oldKhe, newKhe);

        // Since substitute expects end vertices to be the same,
        // it didn't remove our targetKv from its star's boundary.
        // So we do it manually here.
        for (Cell* cell : targetKv->star().copy()) {
            removeFromBoundary_(cell, targetKv);
        }

        // Delete old edge
        result.removedKeId1 = oldKe->id();
        hardDelete(oldKe);
    }
    else {
        // Compute new edge data as concatenation of old edges
        KeyEdgeData& ked1 = info.khe1.edge()->data();
        KeyEdgeData& ked2 = info.khe2.edge()->data();
        KeyVertex* kv1 = info.khe1.startVertex();
        KeyVertex* kv2 = info.khe2.endVertex();
        bool dir1 = info.khe1.direction();
        bool dir2 = info.khe2.direction();
        KeyEdgeData concatData;
        KeyHalfedgeData khd1(&ked1, dir1);
        KeyHalfedgeData khd2(&ked2, dir2);
        concatData = ked1.fromConcatStep(khd1, khd2, smoothJoin);

        // Determine where to insert new edge
        std::array<Node*, 2> kes = {info.khe1.edge(), info.khe2.edge()};
        Node* bottomMostEdge = findBottomMost(kes);
        Group* parentGroup = bottomMostEdge->parentGroup();
        Node* nextSibling = bottomMostEdge;

        // Create new edge e
        newKe =
            createKeyOpenEdge(kv1, kv2, std::move(concatData), parentGroup, nextSibling);
        result.resultKe = newKe;

        // Substitute all usages of (e1, e2) by e in incident faces.
        //
        // Note that we already know that the uncut is possible, which mean
        // that the face cycles never uses e1 or e2 independently, but always
        // both consecutively. In particular, we do not need to iterate on both
        // the star of e1 and e2, since they have the same star.
        //
        for (Cell* starCell : info.khe1.edge()->star().copy()) {
            KeyFace* kf = starCell->toKeyFace();
            if (!kf) {
                continue;
            }
            for (KeyCycle& cycle : kf->cycles_) {
                if (cycle.steinerVertex()) {
                    continue;
                }
                auto it = cycle.halfedges_.begin();
                while (it != cycle.halfedges_.end()) {
                    KeyHalfedge& khe = *it;
                    if (khe.edge() == info.khe1.edge()) {
                        bool dir = khe.direction() == info.khe1.direction();
                        khe = KeyHalfedge(newKe, dir);
                        ++it;
                    }
                    else if (khe.edge() == info.khe2.edge()) {
                        it = cycle.halfedges_.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
                VGC_ASSERT(cycle.isValid());
            }

            removeFromBoundary_(kf, info.khe1.edge());
            removeFromBoundary_(kf, info.khe2.edge());
            removeFromBoundary_(kf, targetKv);
            addToBoundary_(kf, newKe);
        }

        // Delete old edges
        result.removedKeId1 = info.khe1.edge()->id();
        result.removedKeId2 = info.khe2.edge()->id();
        hardDelete(info.khe1.edge());
        hardDelete(info.khe2.edge());
    }

    VGC_ASSERT(targetKv->star().isEmpty());
    hardDelete(targetKv);

    result.success = true;
    return result;
}

UncutAtKeyEdgeResult Operations::uncutAtKeyEdge(KeyEdge* targetKe) {

    UncutAtKeyEdgeResult result = {};

    UncutAtKeyEdgeInfo_ info = prepareUncutAtKeyEdge_(targetKe);
    if (!info.isValid) {
        return result;
    }

    if (targetKe->isClosed()) {
        if (info.kf1 == info.kf2) {
            // This case corresponds to removing a closed edge used twice by
            // the same face, e.g., in a cut-torus, cut-Klein bottle or
            // cut-Möbius strip.
            //
            // This doesn't make much sense in the context of vector graphics,
            // but it makes sense topologically so we support it anyway.

            KeyFace* kf = info.kf1;
            result.resultKf = kf;

            // Remove all the cycles using the closed edge. This removes:
            // - Two cycles in the case of a torus or Klein bottle,
            // - One cycle (using the edge twice) in the case of a Möbius strip.
            //
            kf->cycles_.removeIf([targetKe](const KeyCycle& cycle) {
                return !cycle.steinerVertex()
                       && cycle.halfedges().first().edge() == targetKe;
            });
            removeFromBoundary_(kf, targetKe);
        }
        else {
            // This case corresponds to removing a closed edge used once by
            // two different faces, e.g.:
            //
            //     o-------------o                     o-------------o
            //     |     e       |                     |             |
            //     |   .----.    |     uncutAt(e)      |             |
            //     |   | f1 | f2 |    ------------>    |      f      |
            //     |   '----'    |                     |             |
            //     |             |                     |             |
            //     o-------------o                     o-------------o

            // Compute cycles of new face. These are the same as all
            // the cycles from f1 and f2, except the input closed edge.
            core::Array<KeyCycle> newCycles = {};
            for (KeyCycle& cycle : info.kf1->cycles_) {
                if (cycle.steinerVertex()) {
                    newCycles.append(cycle);
                }
                else if (cycle.halfedges_.first().edge() != targetKe) {
                    newCycles.append(cycle);
                }
            }
            for (KeyCycle& cycle : info.kf2->cycles_) {
                if (cycle.steinerVertex()) {
                    newCycles.append(cycle);
                }
                else if (cycle.halfedges_.first().edge() != targetKe) {
                    newCycles.append(cycle);
                }
            }

            // Determine where to insert new face
            std::array<Node*, 2> kfs = {info.kf1, info.kf2};
            Node* bottomMostFace = findBottomMost(kfs);
            Group* parentGroup = bottomMostFace->parentGroup();
            Node* nextSibling = bottomMostFace;

            // Create new face
            KeyFace* newKf =
                createKeyFace(std::move(newCycles), parentGroup, nextSibling);
            result.resultKf = newKf;

            // Set data of the new face as concatenation of old faces
            KeyFaceData::assignFromConcatStep(
                newKf->data(), info.kf1->data(), info.kf2->data());

            // Delete old faces
            result.removedKfId1 = info.kf1->id();
            result.removedKfId2 = info.kf2->id();
            hardDelete(info.kf1);
            hardDelete(info.kf2);
        }
    }
    else { // key open edge
        if (info.kf1 == info.kf2) {
            KeyFace* kf = info.kf1;
            result.resultKf = kf;

            if (info.cycleIndex1 == info.cycleIndex2) {
                KeyCycle& cycle = kf->cycles_[info.cycleIndex1];
                Int i1 = info.componentIndex1;
                Int i2 = info.componentIndex2;
                KeyPath p1 = cycle.subPath(i1 + 1, i2);
                KeyPath p2 = cycle.subPath(i2 + 1, i1);
                bool d1 = cycle.halfedges_[i1].direction();
                bool d2 = cycle.halfedges_[i2].direction();

                if (d1 == d2) {
                    // Splice cycle into another cycle (Möbius strip):
                    //
                    //     o-----o---o                      o-----o---o
                    //     |     |e  |                      |         |
                    //     |   o-o-------o     uncutAt(e)   |   o-o-------o
                    //     |   |     | f |    ------------> |   |     | f |
                    //     |   o-----o   |                  |   o-----o   |
                    //     |             |                  |             |
                    //     o-------------o                  o-------------o
                    //
                    p1.extendReversed(p2);
                    kf->cycles_.append(KeyCycle(std::move(p1)));
                }
                else {
                    // Split cycle into two cycles:
                    //
                    //     o-----o-----o                     o-----o-----o
                    //     |     |e    |                     |           |
                    //     |   o-o-o   |     uncutAt(e)      |   o-o-o   |
                    //     |   |   | f |    ------------>    |   |   | f |
                    //     |   o---o   |                     |   o---o   |
                    //     |           |                     |           |
                    //     o-----------o                     o-----------o
                    //
                    kf->cycles_.append(KeyCycle(std::move(p1)));
                    kf->cycles_.append(KeyCycle(std::move(p2)));
                }
                kf->cycles_.removeAt(info.cycleIndex1);
                removeFromBoundary_(kf, targetKe);
            }
            else {
                // Splice two cycles of the same face into one cycle.
                //
                // Topologically, this corresponds to creating a torus with one
                // hole, starting from a torus with two holes that share a
                // common edge.
                //
                //    _____________          ___________          ___________
                //   ╱             ╲        ╱           ╲        ╱           ╲
                //  ╱      ___  f   ╲      ╱     ___  f  ╲      ╱     ___  f  ╲
                // (      (   )      ) -> (     (   )     ) -> (     (   )     )
                //  ╲  o---o o---o  ╱      ╲  o---o---o  ╱      ╲  o---o---o  ╱
                //   ╲ | e1| |e2 | ╱        ╲ |   |e  | ╱        ╲ |       | ╱
                //     o---o o---o    glue    o---o---o    uncut   o---o---o
                //                  (e1, e2)                (e)
                //
                //    Cylinder with       Torus with 2 holes       Torus with
                //   2 distinct holes    sharing common edge e      one hole

                // Compute new spliced cycle
                KeyCycle& cycle1 = kf->cycles_[info.cycleIndex1];
                KeyCycle& cycle2 = kf->cycles_[info.cycleIndex2];
                Int i1 = info.componentIndex1;
                Int i2 = info.componentIndex2;
                KeyPath p1 = cycle1.subPath(i1 + 1, i1);
                KeyPath p2 = cycle2.subPath(i2 + 1, i2);
                bool d1 = cycle1.halfedges_[i1].direction();
                bool d2 = cycle2.halfedges_[i2].direction();
                if (d1 == d2) {
                    p1.extendReversed(p2);
                }
                else {
                    p1.extend(p2);
                }
                KeyCycle newCycle(std::move(p1));

                // Add the new cycle
                kf->cycles_.append(std::move(newCycle));

                // Remove the old cycles
                auto indices = std::minmax(info.cycleIndex1, info.cycleIndex2);
                kf->cycles_.removeAt(indices.second);
                kf->cycles_.removeAt(indices.first);
                removeFromBoundary_(kf, targetKe);
            }
        }
        else { // f1 != f2

            // Splice two cycles of different faces into one cycle, merging the
            // two faces f1 and f2 into one new face.
            //
            // o--------o--------o                 o--------o--------o
            // |        |        |   uncutAt(e)    |                 |
            // |   f1   |e  f2   |  ------------>  |        f        |
            // |        |        |                 |                 |
            // o--------o--------o                 o--------o--------o

            // Compute new spliced cycle
            KeyFace* kf1 = info.kf1;
            KeyFace* kf2 = info.kf2;
            KeyCycle& cycle1 = kf1->cycles_[info.cycleIndex1];
            KeyCycle& cycle2 = kf2->cycles_[info.cycleIndex2];
            Int i1 = info.componentIndex1;
            Int i2 = info.componentIndex2;
            KeyPath p1 = cycle1.subPath(i1 + 1, i1);
            KeyPath p2 = cycle2.subPath(i2 + 1, i2);
            bool d1 = cycle1.halfedges_[i1].direction();
            bool d2 = cycle2.halfedges_[i2].direction();
            if (d1 == d2) {
                p1.extendReversed(p2);
            }
            else {
                p1.extend(p2);
            }
            KeyCycle newCycle(std::move(p1));

            // Compute cycles of new face. These are the same as all the cycles
            // from f1 and f2, except that we remove the two old cycles that
            // were using e, and add the new spliced cycle and
            //
            core::Array<KeyCycle> newCycles;
            for (Int j = 0; j < kf1->cycles_.length(); ++j) {
                if (j != info.cycleIndex1) {
                    newCycles.append(kf1->cycles_[j]);
                }
            }
            for (Int j = 0; j < kf2->cycles_.length(); ++j) {
                if (j != info.cycleIndex2) {
                    newCycles.append(kf2->cycles_[j]);
                }
            }
            newCycles.append(std::move(newCycle));

            // Determine where to insert new face
            std::array<Node*, 2> kfs = {info.kf1, info.kf2};
            Node* bottomMostFace = findBottomMost(kfs);
            Group* parentGroup = bottomMostFace->parentGroup();
            Node* nextSibling = bottomMostFace;

            // Create new face
            KeyFace* newKf =
                createKeyFace(std::move(newCycles), parentGroup, nextSibling);
            result.resultKf = newKf;

            // Set data of the new face as concatenation of old faces
            KeyFaceData::assignFromConcatStep(
                newKf->data(), info.kf1->data(), info.kf2->data());

            // Delete old faces
            result.removedKfId1 = info.kf1->id();
            result.removedKfId2 = info.kf2->id();
            hardDelete(info.kf1);
            hardDelete(info.kf2);
        }
    }

    VGC_ASSERT(targetKe->star().isEmpty());
    hardDelete(targetKe);

    result.success = true;
    return result;
}

} // namespace vgc::vacomplex::detail
