// Copyright 2023 The VGC Developers
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

#include <vgc/geometry/tesselator.h>

#include <type_traits> // is_same

#include <tesselator.h> // libtess2

namespace vgc::geometry {

namespace {

TESStesselator* getTess(void* tess) {
    return static_cast<TESStesselator*>(tess);
}

} // namespace

Tesselator::Tesselator() {
    TESSalloc* alloc = nullptr; // default allocator
    tess_ = static_cast<void*>(tessNewTess(alloc));
}

Tesselator::~Tesselator() {
    tessDeleteTess(getTess(tess_));
}

void Tesselator::addContour(core::ConstSpan<float> coords) {

    static_assert(std::is_same_v<float, TESSreal>);

    int vertexSize = 2;
    if (coords.size() > 4) { // ignore contour if 2 points or less
        tessAddContour(
            getTess(tess_),
            vertexSize,
            coords.data(),
            sizeof(TESSreal) * vertexSize,
            core::int_cast<int>(coords.length() / 2));
    }
}

void Tesselator::addContour(core::ConstSpan<double> coords) {
    buffer_.assign(coords);
    addContour(buffer_);
}

namespace {

int getTessWindingRule(WindingRule windingRule) {
    switch (windingRule) {
    case WindingRule::Odd:
        return TESS_WINDING_ODD;
    case WindingRule::NonZero:
        return TESS_WINDING_NONZERO;
    case WindingRule::Positive:
        return TESS_WINDING_POSITIVE;
    case WindingRule::Negative:
        return TESS_WINDING_NEGATIVE;
    }
    return TESS_WINDING_NONZERO;
}

template<typename TFloat>
void tesselate_(
    core::Array<TFloat>& data,
    WindingRule windingRule,
    TESStesselator* tess) {

    int vertexSize = 2; // Number of coordinates per vertex (must be 2 or 3)
    int windingRule_ = getTessWindingRule(windingRule); // Winding rule
    int elementType = TESS_POLYGONS;
    int maxPolySize = 3;                 // Triangles only
    const TESSreal normal[] = {0, 0, 1}; // Normal for 2D points is the Z unit vector

    int success =
        tessTesselate(tess, windingRule_, elementType, maxPolySize, vertexSize, normal);

    if (success) {
        const TESSreal* vertices = tessGetVertices(tess);
        const TESSindex* polygons = tessGetElements(tess);
        const int numPolygons = tessGetElementCount(tess);
        Int numOutputVertices = 0;
        for (int i = 0; i < numPolygons; ++i) {
            const TESSindex* p = &polygons[i * maxPolySize];
            int polySize = maxPolySize;
            while (p[polySize - 1] == TESS_UNDEF) {
                --polySize;
            }
            numOutputVertices += 6 * (polySize - 2);
        }
        data.reserve(data.length() + numOutputVertices);
        for (int i = 0; i < numPolygons; ++i) {
            const TESSindex* p = &polygons[i * maxPolySize];
            int polySize = maxPolySize;
            while (p[polySize - 1] == TESS_UNDEF) {
                --polySize;
            }
            for (int j = 0; j < polySize - 2; ++j) { // triangle fan
                const TESSreal* v1 = &vertices[p[j] * vertexSize];
                const TESSreal* v2 = &vertices[p[j + 1] * vertexSize];
                const TESSreal* v3 = &vertices[p[j + 2] * vertexSize];
                data.append(core::narrow_cast<TFloat>(v1[0]));
                data.append(core::narrow_cast<TFloat>(v1[1]));
                data.append(core::narrow_cast<TFloat>(v2[0]));
                data.append(core::narrow_cast<TFloat>(v2[1]));
                data.append(core::narrow_cast<TFloat>(v3[0]));
                data.append(core::narrow_cast<TFloat>(v3[1]));
            }
        }
    }
    else {
        // TODO: error reporting?
    }
}

} // namespace

void Tesselator::tesselate(core::Array<float>& data, WindingRule windingRule) {
    tesselate_(data, windingRule, getTess(tess_));
}

void Tesselator::tesselate(core::Array<double>& data, WindingRule windingRule) {
    tesselate_(data, windingRule, getTess(tess_));
}

} // namespace vgc::geometry
