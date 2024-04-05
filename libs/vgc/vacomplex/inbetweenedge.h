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

#ifndef VGC_VACOMPLEX_INBETWEENEDGE_H
#define VGC_VACOMPLEX_INBETWEENEDGE_H

#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cell.h>

namespace vgc::vacomplex {

class KeyHalfedge;

class VGC_VACOMPLEX_API InbetweenEdge final
    : public SpatioTemporalCell<EdgeCell, InbetweenCell> {

private:
    friend detail::Operations;

    explicit InbetweenEdge(core::Id id) noexcept
        : SpatioTemporalCell(id) {
    }

public:
    VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Inbetween, Edge)

    bool isStartVertex(const VertexCell* v) const override;
    bool isEndVertex(const VertexCell* v) const override;
    bool isClosed() const override;

    std::shared_ptr<const geometry::StrokeSampling2d>
    strokeSamplingShared(core::AnimTime t) const override {
        VGC_UNUSED(t);
        return nullptr;
    }

    geometry::Rect2d boundingBoxAt(core::AnimTime t) const override;

private:
    void substituteKeyVertex_(KeyVertex* oldVertex, KeyVertex* newVertex) override;

    void substituteKeyEdge_(
        const KeyHalfedge& oldHalfedge,
        const KeyHalfedge& newHalfedge) override;

    void debugPrint_(core::StringWriter& out) override;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_INBETWEENEDGE_H
