// Copyright 2025 The VGC Developers
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
#include <vgc/geometry/polyline.h>
#include <vgc/geometry/vec2.h>

using vgc::core::Array;
using vgc::geometry::Vec2d;
using vgc::geometry::Vec2dArray;
namespace polyline = vgc::geometry::polyline;
namespace c20 = vgc::c20;

namespace {

struct Data {
    Vec2d point;
};

Vec2d proj(const Data& d) {
    return d.point;
}

} // namespace

TEST(TestPolyline, PointType) {
    static_assert(std::is_same_v<polyline::pointType<Vec2dArray>, Vec2d>);
    static_assert(
        std::is_same_v<polyline::pointType<Array<Data>, decltype(proj)>, Vec2d>);
}

TEST(TestPolyline, ScalarType) {
    static_assert(std::is_same_v<polyline::scalarType<Vec2dArray>, double>);
    static_assert(
        std::is_same_v<polyline::scalarType<Array<Data>, decltype(proj)>, double>);
}

TEST(TestPolyline, Length) {
    {
        Vec2dArray p = {{0, 0}, {0, 1}, {1, 1}};
        EXPECT_EQ(polyline::length(p), 2);
    }
    {
        Array<Data> p = {{{0, 0}}, {{0, 1}}, {{1, 1}}};
        EXPECT_EQ(polyline::length(p, proj), 2);
        EXPECT_EQ(polyline::length(p | c20::views::transform(proj)), 2);
        EXPECT_EQ(polyline::length(p, [](auto d) { return d.point; }), 2);
        EXPECT_EQ(
            polyline::length(p | c20::views::transform([](auto d) { return d.point; })),
            2);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
