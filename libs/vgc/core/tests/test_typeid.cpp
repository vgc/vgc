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

#include <vgc/core/typeid.h>

using vgc::core::TypeId;
using vgc::core::typeId;
using vgc::core::detail::typeIdInt;
using vgc::core::detail::TypeIdTestClass;
using vgc::core::detail::typeIdTestClass;

class Foo {};
struct Bar {};

enum class FooEnum {
};

enum BarEnum {
};

namespace foo {

class Foo {};
struct Bar {};

enum class FooEnum {
};

enum BarEnum {
};

} // namespace foo

TEST(TestTypeId, Name) {

    EXPECT_EQ(typeId<int>().name(), "int");

    EXPECT_EQ(typeId<Foo>().name(), "Foo");
    EXPECT_EQ(typeId<Bar>().name(), "Bar");
    EXPECT_EQ(typeId<FooEnum>().name(), "FooEnum");
    EXPECT_EQ(typeId<BarEnum>().name(), "BarEnum");

    EXPECT_EQ(typeId<foo::Foo>().name(), "Foo");
    EXPECT_EQ(typeId<foo::Bar>().name(), "Bar");
    EXPECT_EQ(typeId<foo::FooEnum>().name(), "FooEnum");
    EXPECT_EQ(typeId<foo::BarEnum>().name(), "BarEnum");
}

TEST(TestTypeId, FullName) {

    EXPECT_EQ(typeId<int>().fullName(), "int");

    EXPECT_EQ(typeId<Foo>().fullName(), "Foo");
    EXPECT_EQ(typeId<Bar>().fullName(), "Bar");
    EXPECT_EQ(typeId<FooEnum>().fullName(), "FooEnum");
    EXPECT_EQ(typeId<BarEnum>().fullName(), "BarEnum");

    EXPECT_EQ(typeId<foo::Foo>().fullName(), "foo::Foo");
    EXPECT_EQ(typeId<foo::Bar>().fullName(), "foo::Bar");
    EXPECT_EQ(typeId<foo::FooEnum>().fullName(), "foo::FooEnum");
    EXPECT_EQ(typeId<foo::BarEnum>().fullName(), "foo::BarEnum");
}

TEST(TestTypeId, Equal) {

    EXPECT_EQ(typeId<void>(), typeId<void>());

    EXPECT_EQ(typeId<bool>(), typeId<bool>());

    EXPECT_EQ(typeId<char>(), typeId<char>());
    EXPECT_EQ(typeId<signed char>(), typeId<signed char>());
    EXPECT_EQ(typeId<unsigned char>(), typeId<unsigned char>());

    EXPECT_EQ(typeId<short>(), typeId<short>());
    EXPECT_EQ(typeId<int>(), typeId<int>());
    EXPECT_EQ(typeId<long int>(), typeId<long int>());
    EXPECT_EQ(typeId<long long int>(), typeId<long long int>());

    EXPECT_EQ(typeId<unsigned short>(), typeId<unsigned short>());
    EXPECT_EQ(typeId<unsigned int>(), typeId<unsigned int>());
    EXPECT_EQ(typeId<unsigned long int>(), typeId<unsigned long int>());
    EXPECT_EQ(typeId<unsigned long long int>(), typeId<unsigned long long int>());

    EXPECT_EQ(typeId<float>(), typeId<float>());
    EXPECT_EQ(typeId<double>(), typeId<double>());
    EXPECT_EQ(typeId<long double>(), typeId<long double>());

    EXPECT_EQ(typeId<Foo>(), typeId<Foo>());
    EXPECT_EQ(typeId<Bar>(), typeId<Bar>());

    EXPECT_EQ(typeIdInt(), typeIdInt());
    EXPECT_EQ(typeIdTestClass(), typeIdTestClass());
}

TEST(TestTypeId, NotEqual) {

    EXPECT_NE(typeId<void>(), typeId<bool>());

    EXPECT_NE(typeId<char>(), typeId<signed char>());
    EXPECT_NE(typeId<char>(), typeId<unsigned char>());
    EXPECT_NE(typeId<signed char>(), typeId<unsigned char>());

    EXPECT_NE(typeId<char>(), typeId<short>());
    EXPECT_NE(typeId<short>(), typeId<int>());
    EXPECT_NE(typeId<int>(), typeId<long int>());
    EXPECT_NE(typeId<long int>(), typeId<long long int>());

    EXPECT_NE(typeId<float>(), typeId<double>());
    EXPECT_NE(typeId<double>(), typeId<long double>());

    EXPECT_NE(typeId<int>(), typeId<Foo>());
    EXPECT_NE(typeId<Foo>(), typeId<Bar>());
    EXPECT_NE(typeIdInt(), typeIdTestClass());
}

TEST(TestTypeId, EqualAcrossDllBoundaries) {
    EXPECT_EQ(typeId<int>(), typeIdInt());
    EXPECT_EQ(typeId<TypeIdTestClass>(), typeIdTestClass());
}

TEST(TestTypeId, NotEqualAcrossDllBoundaries) {
    EXPECT_NE(typeId<int>(), typeIdTestClass());
    EXPECT_NE(typeId<TypeIdTestClass>(), typeIdInt());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
