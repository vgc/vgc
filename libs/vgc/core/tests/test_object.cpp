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
#include <vgc/core/object.h>

using vgc::core::Object;
using vgc::core::ObjectPtr;
using vgc::core::detail::ConstructibleTestObject;
using vgc::core::detail::ConstructibleTestObjectPtr;
using vgc::core::detail::ConstructibleTestObjectSharedPtr;
using vgc::core::detail::ConstructibleTestObjectWeakPtr;

TEST(TestObject, StaticUnqualifiedName) {
    ASSERT_EQ(Object::staticObjectType().unqualifiedName(), "Object");
    ASSERT_EQ(
        ConstructibleTestObject::staticObjectType().unqualifiedName(),
        "ConstructibleTestObject");

    ConstructibleTestObjectPtr derived = ConstructibleTestObject::create();
    ObjectPtr base = vgc::core::static_pointer_cast<Object>(derived);
    ASSERT_EQ(base->staticObjectType().unqualifiedName(), "Object");
    ASSERT_EQ(derived->staticObjectType().unqualifiedName(), "ConstructibleTestObject");
}

TEST(TestObject, UnqualifiedName) {
    ConstructibleTestObjectPtr derived = ConstructibleTestObject::create();
    ObjectPtr base = vgc::core::static_pointer_cast<Object>(derived);
    ASSERT_EQ(base->objectType().unqualifiedName(), "ConstructibleTestObject");
    ASSERT_EQ(derived->objectType().unqualifiedName(), "ConstructibleTestObject");
}

TEST(TestObject, Format) {
    ConstructibleTestObjectPtr obj = ConstructibleTestObject::create();
    Object* parent = obj->parentObject();

    std::string objAddress = vgc::core::format("{}", fmt::ptr(obj.get()));
    ASSERT_GT(objAddress.size(), 2);
    ASSERT_EQ(objAddress.substr(0, 2), "0x");

    std::string s = vgc::core::format("The parent of {} is {}", ptr(obj), ptr(parent));

    std::string expectedResult = "The parent of <ConstructibleTestObject @ ";
    expectedResult += objAddress;
    expectedResult += "> is <Null Object>";

    ASSERT_EQ(s, expectedResult);
}

TEST(TestObject, RootSharedAndWeakPtr) {

    bool isDestructed = false;

    ConstructibleTestObjectSharedPtr sp = ConstructibleTestObject::create(&isDestructed);
    ASSERT_EQ(sp.sharedCount(), 1);
    ASSERT_EQ(sp.weakCount(), 0);
    ASSERT_EQ(sp.isAlive(), true);
    ASSERT_EQ(isDestructed, false);

    ConstructibleTestObjectSharedPtr sp2 = sp;
    ASSERT_EQ(sp.sharedCount(), 2);
    ASSERT_EQ(sp.weakCount(), 0);
    ASSERT_EQ(sp.isAlive(), true);
    ASSERT_EQ(sp2.sharedCount(), 2);
    ASSERT_EQ(sp2.weakCount(), 0);
    ASSERT_EQ(sp2.isAlive(), true);
    ASSERT_EQ(isDestructed, false);

    sp = nullptr;
    ASSERT_EQ(sp.sharedCount(), -1);
    ASSERT_EQ(sp.weakCount(), -1);
    ASSERT_EQ(sp.isAlive(), false);
    ASSERT_EQ(sp2.sharedCount(), 1);
    ASSERT_EQ(sp2.weakCount(), 0);
    ASSERT_EQ(sp2.isAlive(), true);
    ASSERT_EQ(isDestructed, false);

    ConstructibleTestObjectWeakPtr wp = sp2;
    ASSERT_EQ(sp2.sharedCount(), 1);
    ASSERT_EQ(sp2.weakCount(), 1);
    ASSERT_EQ(sp2.isAlive(), true);
    ASSERT_EQ(wp.sharedCount(), 1);
    ASSERT_EQ(wp.weakCount(), 1);
    ASSERT_EQ(wp.isAlive(), true);
    ASSERT_EQ(isDestructed, false);

    ASSERT_EQ(bool(wp.lock()), true);

    sp2 = nullptr;
    ASSERT_EQ(sp2.sharedCount(), -1);
    ASSERT_EQ(sp2.weakCount(), -1);
    ASSERT_EQ(sp2.isAlive(), false);
    ASSERT_EQ(wp.sharedCount(), 0);
    ASSERT_EQ(wp.weakCount(), 1);
    ASSERT_EQ(wp.isAlive(), false);
    ASSERT_EQ(isDestructed, false);

    ASSERT_EQ(bool(wp.lock()), false);

    wp = nullptr;
    ASSERT_EQ(wp.sharedCount(), -1);
    ASSERT_EQ(wp.weakCount(), -1);
    ASSERT_EQ(wp.isAlive(), false);
    ASSERT_EQ(isDestructed, true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
