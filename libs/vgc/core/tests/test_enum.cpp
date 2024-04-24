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
#include <vgc/core/enum.h>

namespace {

enum UnscopedFoo {
    A,
    B
};

enum UnscopedBar {
    C,
    D
};

enum class ScopedFoo {
    E,
    F
};

enum class ScopedBar {
    G,
    H
};

} // namespace

TEST(TestEnumValue, Empty) {
    vgc::core::EnumValue v;
    EXPECT_TRUE(v.isEmpty());
}

TEST(TestEnumValue, UnscopedEnum) {

    {
        vgc::core::EnumValue v(A);
        EXPECT_FALSE(v.isEmpty());
        EXPECT_TRUE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_FALSE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<UnscopedFoo>(), A);
        EXPECT_EQ(v.getUnchecked<UnscopedFoo>(), A);
    }

    {
        vgc::core::EnumValue v(A);
        EXPECT_FALSE(v.isEmpty());
        EXPECT_TRUE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_FALSE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<UnscopedFoo>(), A);
        EXPECT_EQ(v.getUnchecked<UnscopedFoo>(), A);
    }

    {
        vgc::core::EnumValue v{A};
        EXPECT_FALSE(v.isEmpty());
        EXPECT_TRUE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_FALSE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<UnscopedFoo>(), A);
        EXPECT_EQ(v.getUnchecked<UnscopedFoo>(), A);
    }
}

TEST(TestEnumValue, ScopedEnum) {

    {
        vgc::core::EnumValue v(ScopedFoo::E);
        EXPECT_FALSE(v.isEmpty());
        EXPECT_FALSE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_TRUE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<ScopedFoo>(), ScopedFoo::E);
        EXPECT_EQ(v.getUnchecked<ScopedFoo>(), ScopedFoo::E);
    }

    {
        vgc::core::EnumValue v(ScopedFoo::E);
        EXPECT_FALSE(v.isEmpty());
        EXPECT_FALSE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_TRUE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<ScopedFoo>(), ScopedFoo::E);
        EXPECT_EQ(v.getUnchecked<ScopedFoo>(), ScopedFoo::E);
    }

    {
        vgc::core::EnumValue v{ScopedFoo::E};
        EXPECT_FALSE(v.isEmpty());
        EXPECT_FALSE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_TRUE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<ScopedFoo>(), ScopedFoo::E);
        EXPECT_EQ(v.getUnchecked<ScopedFoo>(), ScopedFoo::E);
    }
}

TEST(TestEnumValue, Assignment) {

    vgc::core::EnumValue v1 = ScopedFoo::E;
    EXPECT_EQ(v1.get<ScopedFoo>(), ScopedFoo::E);

    vgc::core::EnumValue v2 = ScopedBar::G;
    EXPECT_EQ(v2.get<ScopedBar>(), ScopedBar::G);

    v1 = v2;
    EXPECT_EQ(v1.get<ScopedBar>(), ScopedBar::G);

    v1 = A;
    EXPECT_EQ(v1.get<UnscopedFoo>(), UnscopedFoo::A);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
