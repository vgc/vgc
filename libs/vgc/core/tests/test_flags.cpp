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
#include <vgc/core/arithmetic.h>
#include <vgc/core/flags.h>

using namespace vgc;

enum class MyEnum : vgc::UInt8 {
    None = 0x00,
    Foo = 0x01,
    Bar = 0x02,
    FooBar = Foo | Bar
};
VGC_DEFINE_FLAGS(MyFlags, MyEnum)

TEST(TestFlags, EnumOperators) {
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

    EXPECT_EQ((~foo).toUnderlying(), 0xfe);

    EXPECT_EQ(bool(foo & foobar), true);
    EXPECT_EQ(bool(foo & bar), false);
}

TEST(TestFlags, Operators) {
    MyFlags none = MyEnum::None;
    MyFlags foo = MyEnum::Foo;
    MyFlags bar = MyEnum::Bar;
    MyFlags foobar = MyEnum::FooBar;

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

    MyFlags x = foobar;
    x &= foo;
    EXPECT_EQ(x, foo);
    x ^= foobar;
    EXPECT_EQ(x, bar);
    x |= foo;
    EXPECT_EQ(x, foobar);

    EXPECT_EQ((~foo).toUnderlying(), 0xfe);

    EXPECT_TRUE(foo & foobar);
    EXPECT_FALSE(foo & bar);

    if (foo & bar) {
        EXPECT_TRUE(false);
    }
}

TEST(TestFlags, Methods) {
    MyFlags x(MyEnum::Foo);
    EXPECT_EQ(x, MyEnum::Foo);
    EXPECT_TRUE(x.has(MyEnum::Foo));
    EXPECT_TRUE(x.hasAll(MyEnum::Foo));
    EXPECT_FALSE(x.has(MyEnum::Bar));
    EXPECT_FALSE(x.hasAll(MyEnum::Bar));
    EXPECT_FALSE(x.has(MyEnum::FooBar));
    EXPECT_FALSE(x.hasAll(MyEnum::FooBar));
    EXPECT_TRUE(x.hasAny(MyEnum::FooBar));
    x.set(MyEnum::Bar);
    EXPECT_EQ(x, MyEnum::FooBar);
    EXPECT_TRUE(x.has(MyEnum::Foo));
    EXPECT_TRUE(x.hasAll(MyEnum::Foo));
    EXPECT_TRUE(x.has(MyEnum::Bar));
    EXPECT_TRUE(x.hasAll(MyEnum::Bar));
    EXPECT_TRUE(x.has(MyEnum::FooBar));
    EXPECT_TRUE(x.hasAll(MyEnum::FooBar));
    EXPECT_TRUE(x.hasAny(MyEnum::FooBar));
    x.unset(MyEnum::Foo);
    EXPECT_EQ(x, MyEnum::Bar);
    EXPECT_FALSE(x.has(MyEnum::Foo));
    EXPECT_FALSE(x.hasAll(MyEnum::Foo));
    EXPECT_TRUE(x.has(MyEnum::Bar));
    EXPECT_TRUE(x.hasAll(MyEnum::Bar));
    EXPECT_FALSE(x.has(MyEnum::FooBar));
    EXPECT_FALSE(x.hasAll(MyEnum::FooBar));
    EXPECT_TRUE(x.hasAny(MyEnum::FooBar));
    x.set(MyEnum::Foo);
    x.mask(MyEnum::Bar);
    EXPECT_EQ(x, MyEnum::Bar);
    x.toggle(MyEnum::Foo);
    EXPECT_EQ(x, MyEnum::FooBar);
    x.toggle(MyEnum::Foo);
    EXPECT_EQ(x, MyEnum::Bar);
    x.toggleAll();
    EXPECT_EQ(x, ~MyEnum::Bar);
    EXPECT_TRUE(x.has(MyEnum::Foo));
    EXPECT_FALSE(x.has(MyEnum::Bar));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
