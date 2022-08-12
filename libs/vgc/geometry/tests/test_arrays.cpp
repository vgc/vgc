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

#include <gtest/gtest.h>
#include <vgc/core/stopwatch.h>
#include <vgc/geometry/vec2d.h>

using vgc::Int;
using vgc::core::Stopwatch;
using vgc::geometry::Vec2d;
using vgc::geometry::Vec2dArray;

TEST(TestArrays, Resize) {
    Vec2dArray a = {{1, 2}, {3, 4}};
    Vec2dArray b = {{1, 2}};
    Vec2dArray c = {{1, 2}, {0, 0}};
    a.removeLast();
    EXPECT_EQ(a, b);
    a.resize(2);
    EXPECT_EQ(a, c);
}

TEST(TestArrays, ResizeNoInit) {
    Vec2dArray a = {{1, 2}, {3, 4}};
    Vec2dArray b = {{1, 2}};
    Vec2dArray c = {{1, 2}, {3, 4}};
    a.removeLast();
    EXPECT_EQ(a, b);
    a.resizeNoInit(2);
    EXPECT_EQ(a, c);
}

#ifndef VGC_DEBUG_BUILD

namespace {

void measureResizePerf(
    Vec2dArray& array,
    void (Vec2dArray::*resizeFn)(Int),
    Int n,
    double& elapsed,
    bool isWarmup) {

    vgc::core::Stopwatch s;
    (array.*resizeFn)(0);
    EXPECT_EQ(array.length(), 0); // Ensures that resize(0) isn't optimized out
    (array.*resizeFn)(n);
    EXPECT_EQ(array[n - 1], Vec2d()); // Ensures that resize(n) isn't optimized out
    if (!isWarmup) {
        elapsed += s.elapsed();
    }
}

} // namespace

TEST(TestArrays, ResizeNoInitPerf) {
    Int k = 1000;
    Int n = 100000;
    Vec2dArray a(n);
    double elapsedInit = 0;
    double elapsedNoInit = 0;
    Int numWarmupIterations = k / 10;
    for (Int i = 0; i < k; ++i) {
        bool isWarmup = (k < numWarmupIterations);
        measureResizePerf(a, &Vec2dArray::resize, n, elapsedInit, isWarmup);
        measureResizePerf(a, &Vec2dArray::resizeNoInit, n, elapsedNoInit, isWarmup);
    }
    vgc::core::print("Resize with init    = {:.7f} sec.\n", elapsedInit);
    vgc::core::print("Resize without init = {:.7f} sec.\n", elapsedNoInit);
    EXPECT_LT(elapsedNoInit, 0.01 * elapsedInit);

    // Performance results on DELL Precision 3561, i7-11850H, 32GB RAM:
    //    Resize with init    = 0.0343690 sec.
    //    Resize without init = 0.0000265 sec.
}

#endif // VGC_DEBUG_BUILD

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
