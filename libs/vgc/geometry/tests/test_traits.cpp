// Copyright 2024 The VGC Developers
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
#include <vgc/geometry/mat2.h>
#include <vgc/geometry/mat3.h>
#include <vgc/geometry/mat4.h>
#include <vgc/geometry/segment2.h>
#include <vgc/geometry/traits.h>
#include <vgc/geometry/vec2.h>
#include <vgc/geometry/vec3.h>
#include <vgc/geometry/vec4.h>

using namespace vgc::geometry;

TEST(TestTraits, ScalarType) {

    // Note: double parentheses otherwise the macro thinks there are two args

    EXPECT_TRUE((std::is_same_v<scalarType<int>, int>));
    EXPECT_TRUE((std::is_same_v<scalarType<float>, float>));
    EXPECT_TRUE((std::is_same_v<scalarType<double>, double>));

    EXPECT_TRUE((std::is_same_v<scalarType<Vec2f>, float>));
    EXPECT_TRUE((std::is_same_v<scalarType<Vec3f>, float>));
    EXPECT_TRUE((std::is_same_v<scalarType<Vec4f>, float>));

    EXPECT_TRUE((std::is_same_v<scalarType<Mat2f>, float>));
    EXPECT_TRUE((std::is_same_v<scalarType<Mat3f>, float>));
    EXPECT_TRUE((std::is_same_v<scalarType<Mat4f>, float>));

    EXPECT_TRUE((std::is_same_v<scalarType<Segment2f>, float>));

    EXPECT_TRUE((std::is_same_v<scalarType<Vec2d>, double>));
    EXPECT_TRUE((std::is_same_v<scalarType<Vec3d>, double>));
    EXPECT_TRUE((std::is_same_v<scalarType<Vec4d>, double>));

    EXPECT_TRUE((std::is_same_v<scalarType<Mat2d>, double>));
    EXPECT_TRUE((std::is_same_v<scalarType<Mat3d>, double>));
    EXPECT_TRUE((std::is_same_v<scalarType<Mat4d>, double>));

    EXPECT_TRUE((std::is_same_v<scalarType<Segment2d>, double>));
}

TEST(TestTraits, Dimension) {
    EXPECT_EQ(dimension<int>, 1);
    EXPECT_EQ(dimension<float>, 1);
    EXPECT_EQ(dimension<double>, 1);

    EXPECT_EQ(dimension<Vec2f>, 2);
    EXPECT_EQ(dimension<Vec3f>, 3);
    EXPECT_EQ(dimension<Vec4f>, 4);

    EXPECT_EQ(dimension<Mat2f>, 2);
    EXPECT_EQ(dimension<Mat3f>, 3);
    EXPECT_EQ(dimension<Mat4f>, 4);

    EXPECT_EQ(dimension<Segment2f>, 2);

    EXPECT_EQ(dimension<Vec2d>, 2);
    EXPECT_EQ(dimension<Vec3d>, 3);
    EXPECT_EQ(dimension<Vec4d>, 4);

    EXPECT_EQ(dimension<Mat2d>, 2);
    EXPECT_EQ(dimension<Mat3d>, 3);
    EXPECT_EQ(dimension<Mat4d>, 4);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
