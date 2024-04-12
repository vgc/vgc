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

#include <unordered_map>

using vgc::core::Object;
using vgc::core::ObjectLockPtr;
using vgc::core::ObjectPtr;
using vgc::core::ObjectSharedPtr;
using vgc::core::ObjectWeakPtr;

using vgc::core::detail::ConstructibleTestObject;
using vgc::core::detail::ConstructibleTestObjectLockPtr;
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

TEST(TestObject, ChildSharedAndWeakPtr) {

    bool isRootDestructed = false;
    bool isChildDestructed = false;

    // TODO: on assignment from SharedPtr to LockPtr, throw if null.
    ConstructibleTestObjectLockPtr root =
        ConstructibleTestObject::create(&isRootDestructed);
    ASSERT_TRUE(root);

    ConstructibleTestObjectWeakPtr child = root->createChild(&isChildDestructed);
    ASSERT_EQ(child.sharedCount(), 0);
    ASSERT_EQ(child.weakCount(), 1);
    ASSERT_EQ(child.isAlive(), true);
    ASSERT_EQ(isChildDestructed, false);

    child = nullptr;
    ASSERT_EQ(child.sharedCount(), -1);
    ASSERT_EQ(child.weakCount(), -1);
    ASSERT_EQ(child.isAlive(), false);
    ASSERT_EQ(isChildDestructed, false);

    ObjectSharedPtr child2 = root->firstChildObject();
    ASSERT_EQ(child2.sharedCount(), 1);
    ASSERT_EQ(child2.weakCount(), 0);
    ASSERT_EQ(child2.isAlive(), true);
    ASSERT_EQ(isChildDestructed, false);

    child2 = nullptr;
    ASSERT_EQ(child.sharedCount(), -1);
    ASSERT_EQ(child.weakCount(), -1);
    ASSERT_EQ(child.isAlive(), false);
    ASSERT_EQ(isChildDestructed, false);

    child2 = root->firstChildObject();
    ASSERT_EQ(child2.sharedCount(), 1);
    ASSERT_EQ(child2.weakCount(), 0);
    ASSERT_EQ(child2.isAlive(), true);
    ASSERT_EQ(isChildDestructed, false);

    // Test that parent can uniquely destroy children
    root->clearChildren();
    ASSERT_EQ(child2.sharedCount(), 1); // one shared pointer
    ASSERT_EQ(child2.weakCount(), 0);
    ASSERT_EQ(child2.isAlive(), false); // but still dead
    ASSERT_EQ(isChildDestructed, false);

    child2 = nullptr;
    ASSERT_EQ(child2.sharedCount(), -1);
    ASSERT_EQ(child2.weakCount(), -1);
    ASSERT_EQ(child2.isAlive(), false);
    ASSERT_EQ(isChildDestructed, true);
}

TEST(TestObject, SharedPtrHash) {
    bool isDestructed = false;
    std::unordered_map<ConstructibleTestObjectSharedPtr, int> map;
    auto obj1 = ConstructibleTestObject::create(&isDestructed);
    auto obj2 = ConstructibleTestObject::create(&isDestructed);

    map.try_emplace(obj1, 1);
    map[obj2] = 2;
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map[obj1], 1);
    ASSERT_EQ(map[obj2], 2);

    map.erase(obj1);
    map[obj2] = 3;
    ASSERT_EQ(map.size(), 1);
    ASSERT_EQ(map.find(obj1), map.end());
    ASSERT_NE(map.find(obj2), map.end());
    ASSERT_EQ(map[obj2], 3);

    // The following must fail to compile with:
    // "no known conversion from ObjWeakPtr to ObjSharedPtr"
    //
    //   ConstructibleTestObjectWeakPtr weak2 = obj2;
    //   ASSERT_NE(map.find(weak2), map.end());
}

TEST(TestObject, WeakPtrHash) {
    bool isDestructed = false;
    std::unordered_map<ConstructibleTestObjectWeakPtr, int> map;
    auto obj1 = ConstructibleTestObject::create(&isDestructed);
    auto obj2 = ConstructibleTestObject::create(&isDestructed);
    auto obj3 = ConstructibleTestObject::create(&isDestructed);
    auto obj4 = ConstructibleTestObject::create(&isDestructed);

    ConstructibleTestObjectWeakPtr weak1 = obj1;
    ConstructibleTestObjectWeakPtr weak2 = obj2;
    ConstructibleTestObjectWeakPtr weak3 = obj3;
    ConstructibleTestObjectWeakPtr weak4 = obj4;

    map.try_emplace(weak1, 1); //
    map.try_emplace(obj2, 2);  // intentionally inserting via SharedPtr
    map[weak3] = 3;            //
    map[obj4] = 4;             // intentionally inserting via SharedPtr
    ASSERT_EQ(map.size(), 4);
    ASSERT_EQ(map[weak1], 1);
    ASSERT_EQ(map[weak2], 2);
    ASSERT_EQ(map[weak3], 3);
    ASSERT_EQ(map[weak4], 4);
    ASSERT_EQ(map[obj1], 1);
    ASSERT_EQ(map[obj2], 2);
    ASSERT_EQ(map[obj3], 3);
    ASSERT_EQ(map[obj4], 4);

    map.erase(weak1);
    map.erase(weak2);
    map.erase(obj3);
    map[weak4] = 5;
    ASSERT_EQ(map.size(), 1);
    ASSERT_EQ(map.find(weak1), map.end());
    ASSERT_EQ(map.find(weak2), map.end());
    ASSERT_EQ(map.find(weak3), map.end());
    ASSERT_NE(map.find(weak4), map.end());
    ASSERT_EQ(map[weak4], 5);
    ASSERT_EQ(map.find(obj1), map.end());
    ASSERT_EQ(map.find(obj2), map.end());
    ASSERT_EQ(map.find(obj3), map.end());
    ASSERT_NE(map.find(obj4), map.end());
    ASSERT_EQ(map[obj4], 5);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
