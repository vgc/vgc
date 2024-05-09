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

// Note: for testing purposes, we intentionally do not place the classes below in an unnamed namespace.

enum GlobalUnscopedFoo {
    GA,
    GB
};

enum class GlobalScopedFoo {
    E,
    F
};

namespace foo {

enum UnscopedFoo {
    A,
    B
};

enum class ScopedFoo {
    E,
    F
};

enum class RegisteredFoo {
    HelloWorld
};

VGC_DEFINE_ENUM(
    RegisteredFoo, //
    (HelloWorld, "Hello, world!"))

} // namespace foo

TEST(TestEnumType, Name) {
    EXPECT_EQ(vgc::core::enumType<UnscopedFoo>().shortName(), "UnscopedFoo");
    //EXPECT_EQ(vgc::core::enumType<UnscopedFoo>().fullName(), "(anonymous namespace)::UnscopedFoo");

    EXPECT_EQ(vgc::core::enumType<ScopedFoo>().shortName(), "ScopedFoo");
    //EXPECT_EQ(vgc::core::enumType<ScopedFoo>().fullName(), "(anonymous namespace)::ScopedFoo");

    EXPECT_EQ(vgc::core::enumType<GlobalUnscopedFoo>().shortName(), "GlobalUnscopedFoo");
    EXPECT_EQ(vgc::core::enumType<GlobalUnscopedFoo>().fullName(), "GlobalUnscopedFoo");

    EXPECT_EQ(vgc::core::enumType<GlobalScopedFoo>().shortName(), "GlobalScopedFoo");
    EXPECT_EQ(vgc::core::enumType<GlobalScopedFoo>().fullName(), "GlobalScopedFoo");

    EXPECT_EQ(vgc::core::enumType<foo::UnscopedFoo>().shortName(), "UnscopedFoo");
    EXPECT_EQ(vgc::core::enumType<foo::UnscopedFoo>().fullName(), "foo::UnscopedFoo");

    EXPECT_EQ(vgc::core::enumType<foo::ScopedFoo>().shortName(), "ScopedFoo");
    EXPECT_EQ(vgc::core::enumType<foo::ScopedFoo>().fullName(), "foo::ScopedFoo");

    EXPECT_EQ(vgc::core::enumType<foo::RegisteredFoo>().shortName(), "RegisteredFoo");
    EXPECT_EQ(vgc::core::enumType<foo::RegisteredFoo>().fullName(), "foo::RegisteredFoo");
}

TEST(TestEnumValue, UnscopedEnum) {

    {
        vgc::core::EnumValue v(A);
        EXPECT_TRUE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_FALSE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<UnscopedFoo>(), A);
        EXPECT_EQ(v.getUnchecked<UnscopedFoo>(), A);
    }

    {
        vgc::core::EnumValue v(A);
        EXPECT_TRUE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_FALSE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<UnscopedFoo>(), A);
        EXPECT_EQ(v.getUnchecked<UnscopedFoo>(), A);
    }

    {
        vgc::core::EnumValue v{A};
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
        EXPECT_FALSE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_TRUE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<ScopedFoo>(), ScopedFoo::E);
        EXPECT_EQ(v.getUnchecked<ScopedFoo>(), ScopedFoo::E);
    }

    {
        vgc::core::EnumValue v(ScopedFoo::E);
        EXPECT_FALSE(v.has<UnscopedFoo>());
        EXPECT_FALSE(v.has<UnscopedBar>());
        EXPECT_TRUE(v.has<ScopedFoo>());
        EXPECT_FALSE(v.has<ScopedBar>());
        EXPECT_EQ(v.get<ScopedFoo>(), ScopedFoo::E);
        EXPECT_EQ(v.getUnchecked<ScopedFoo>(), ScopedFoo::E);
    }

    {
        vgc::core::EnumValue v{ScopedFoo::E};
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

TEST(TestEnumValue, Names) {

    vgc::core::EnumValue v = foo::RegisteredFoo::HelloWorld;
    EXPECT_EQ(v.shortName(), "HelloWorld");
    EXPECT_EQ(v.fullName(), "foo::RegisteredFoo::HelloWorld");
    EXPECT_EQ(v.prettyName(), "Hello, world!");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
