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
#include <vgc/core/enum.h>

using namespace vgc;

enum class MyEnum {
    None = 0x00,
    Foo = 0x01,
    Bar = 0x02,
    FooBar = Foo | Bar
};
VGC_DEFINE_SCOPED_ENUM_FLAGS_OPERATORS(MyEnum);

TEST(TestEnum, Operators)
{
    MyEnum none = MyEnum::None;
    MyEnum foo = MyEnum::Foo;
    MyEnum bar = MyEnum::Bar;
    MyEnum foobar = MyEnum::FooBar;

    EXPECT_EQ(none | none, none);
    EXPECT_EQ(none | foo, foo);
    EXPECT_EQ(none | bar, bar);
    EXPECT_EQ(none | foobar, foobar);
    EXPECT_EQ(foo | none, foo);
    EXPECT_EQ(foo | foo, foo);
    EXPECT_EQ(foo | bar, foobar);
    EXPECT_EQ(foo | foobar, foobar);

    EXPECT_EQ(none & none, none);
    EXPECT_EQ(none & foo, none);
    EXPECT_EQ(none & bar, none);
    EXPECT_EQ(none & foobar, none);
    EXPECT_EQ(foo & none, none);
    EXPECT_EQ(foo & foo, foo);
    EXPECT_EQ(foo & bar, none);
    EXPECT_EQ(foo & foobar, foo);

    EXPECT_EQ(none ^ none, none);
    EXPECT_EQ(none ^ foo, foo);
    EXPECT_EQ(none ^ bar, bar);
    EXPECT_EQ(none ^ foobar, foobar);
    EXPECT_EQ(foo ^ none, foo);
    EXPECT_EQ(foo ^ foo, none);
    EXPECT_EQ(foo ^ bar, foobar);
    EXPECT_EQ(foo ^ foobar, bar);

    EXPECT_EQ(bool(foo & foobar), true);
    EXPECT_EQ(bool(foo & bar), false);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
