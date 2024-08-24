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

#include <unordered_set>

namespace vgc::vacomplex::detail {

core::Array<KeyCell*> Operations::simplify(
    core::Span<KeyVertex*> kvs,
    core::Span<KeyEdge*> kes,
    bool smoothJoins) {

    Complex* complex = nullptr;
    if (kvs.isEmpty()) {
        if (kes.isEmpty()) {
            return {};
        }
        complex = kes.first()->complex();
    }
    else {
        complex = kvs.first()->complex();
    }

    core::Array<KeyCell*> result;

    std::unordered_set<core::Id> resultEdgeIds;
    std::unordered_set<core::Id> resultFaceIds;

    for (KeyEdge* ke : kes) {
        UncutAtKeyEdgeResult res = uncutAtKeyEdge(ke);
        if (res.success) {
            if (res.removedKfId1) {
                resultFaceIds.erase(res.removedKfId1);
            }
            if (res.removedKfId2) {
                resultFaceIds.erase(res.removedKfId2);
            }
            if (res.resultKf) {
                resultFaceIds.insert(res.resultKf->id());
            }
        }
        else {
            // cannot uncut at edge: add it to the list of returned cells
            resultEdgeIds.insert(ke->id());
        }
    }

    for (KeyVertex* kv : kvs) {
        UncutAtKeyVertexResult res = uncutAtKeyVertex(kv, smoothJoins);
        if (res.success) {
            if (res.removedKeId1) {
                resultEdgeIds.erase(res.removedKeId1);
            }
            if (res.removedKeId2) {
                resultEdgeIds.erase(res.removedKeId2);
            }
            if (res.resultKe) {
                resultEdgeIds.insert(res.resultKe->id());
            }
            if (res.resultKf) {
                resultFaceIds.insert(res.resultKe->id());
            }
        }
        else {
            // cannot uncut at vertex: add it to the list of returned cells
            result.append(kv);
        }
    }

    for (core::Id id : resultEdgeIds) {
        Cell* cell = complex->findCell(id);
        if (cell) {
            KeyEdge* ke = cell->toKeyEdge();
            if (ke) {
                result.append(ke);
            }
        }
    }

    return result;
}

} // namespace vgc::vacomplex::detail
