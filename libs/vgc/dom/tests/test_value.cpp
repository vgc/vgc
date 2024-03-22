// Copyright 2021 The VGC Developers
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
#include <vgc/dom/value.h>

using vgc::dom::Value;

namespace {

thread_local size_t numDefaultConstruct = 0;
thread_local size_t numCopyConstruct = 0;
thread_local size_t numMoveConstruct = 0;
thread_local size_t numCopyAssign = 0;
thread_local size_t numMoveAssign = 0;
thread_local size_t numDestruct = 0;

void clearCounters() {
    numDefaultConstruct = 0;
    numCopyConstruct = 0;
    numMoveConstruct = 0;
    numCopyAssign = 0;
    numMoveAssign = 0;
    numDestruct = 0;
}

class TestObject {
public:
    TestObject() {
        ++numDefaultConstruct;
    }

    TestObject(const TestObject&) {
        ++numCopyConstruct;
    }

    TestObject(TestObject&&) {
        ++numMoveConstruct;
    }

    TestObject& operator=(const TestObject&) {
        ++numCopyAssign;
        return *this;
    }

    TestObject& operator=(TestObject&&) {
        ++numMoveAssign;
        return *this;
    }

    ~TestObject() {
        ++numDestruct;
    }

    bool operator==(const TestObject&) const {
        return true;
    }

    bool operator!=(const TestObject&) const {
        return false;
    }

    bool operator<(const TestObject&) const {
        return false;
    }

    template<typename IStream>
    friend void readTo(TestObject&, IStream&) {
    }

    template<typename OStream>
    friend void write(OStream&, const TestObject&) {
    }
};

} // namespace

template<>
struct fmt::formatter<TestObject> : fmt::formatter<int> {
    template<typename FormatContext>
    auto format(const TestObject&, FormatContext& ctx) {
        return ctx.out();
    }
};

namespace {

class TestObject2 {
public:
    TestObject2() {
        ++numDefaultConstruct;
    }

    TestObject2(const TestObject2&) {
        ++numCopyConstruct;
    }

    TestObject2(TestObject2&&) {
        ++numMoveConstruct;
    }

    TestObject2& operator=(const TestObject2&) {
        ++numCopyAssign;
        return *this;
    }

    TestObject2& operator=(TestObject2&&) {
        ++numMoveAssign;
        return *this;
    }

    ~TestObject2() {
        ++numDestruct;
    }

    bool operator==(const TestObject2&) const {
        return true;
    }

    bool operator!=(const TestObject2&) const {
        return false;
    }

    bool operator<(const TestObject2&) const {
        return false;
    }

    template<typename IStream>
    friend void readTo(TestObject2&, IStream&) {
    }

    template<typename OStream>
    friend void write(OStream&, const TestObject2&) {
    }
};

} // namespace

template<>
struct fmt::formatter<TestObject2> : fmt::formatter<int> {
    template<typename FormatContext>
    auto format(const TestObject2&, FormatContext& ctx) {
        return ctx.out();
    }
};

TEST(TestValue, Int) {
    int i = 42;
    Value v = i;
    EXPECT_TRUE(v.has<int>());
    EXPECT_EQ(v.get<int>(), 42);
}

TEST(TestValue, DefaultConstruct) {
    Value v;
    EXPECT_TRUE(v.isNone());
}

TEST(TestValue, ConstructFromTemporary) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 2);

    clearCounters();
    {
        Value v{TestObject()};
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 2);
}

TEST(TestValue, ConstructFromRRef) {

    clearCounters();
    {
        TestObject obj;
        Value v = std::move(obj);
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);
    }
    EXPECT_EQ(numDestruct, 2);

    clearCounters();
    {
        TestObject obj;
        Value v{std::move(obj)};
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);
    }
    EXPECT_EQ(numDestruct, 2);
}

TEST(TestValue, ConstructFromCRef) {

    clearCounters();
    {
        TestObject obj;
        const TestObject& cobj = obj;
        Value v = cobj;
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);
    }
    EXPECT_EQ(numDestruct, 2);

    clearCounters();
    {
        TestObject obj;
        const TestObject& cobj = obj;
        Value v{cobj};
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);
    }
    EXPECT_EQ(numDestruct, 2);
}

TEST(TestValue, ConstructFromRef) {

    clearCounters();
    {
        TestObject obj;
        TestObject& robj = obj;
        Value v = robj;
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);
    }
    EXPECT_EQ(numDestruct, 2);

    clearCounters();
    {
        TestObject obj;
        const TestObject& robj = obj;
        Value v{robj};
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);
    }
    EXPECT_EQ(numDestruct, 2);
}

TEST(TestValue, CopyConstruct) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        Value v2 = v;
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 3);

    clearCounters();
    {
        Value v{TestObject()};
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        Value v2{v};
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 3);
}

TEST(TestValue, MoveConstruct) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        // Note: the tests below assumes no Small-Value Optimization, in which
        // case moving the Value does not even move the TestObject, since it is
        // heap allocated and we are just re-assigning pointers. With SVO, then
        // we would have to actually move the TestObject, so there would be an
        // extra numMoveConstruct and numDestruct.
        //
        Value v2 = std::move(v);
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 2);

    clearCounters();
    {
        Value v{TestObject()};
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        // Assumes no SVO
        Value v2{std::move(v)};
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 2);
}

// Preliminary sanity check that compiliers don't try to optimize an assign
// into a construct, otherwise the other tests do not actually test
// assignments.
//
TEST(TestValue, AssignNotConstruct) {

    clearCounters();
    {
        TestObject obj;
        obj = TestObject();
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 1);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 2);
}

TEST(TestValue, AssignFromTemp) {

    clearCounters();
    {
        Value v;
        v = TestObject(); // 1st temporary
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        // Test when v already has a TestObject.
        //
        // This means that there is an opportunity for moveAssign rather than
        // destruct + moveConsstruct. However, we do not implement that yet: it
        // requires a smart operator=(T&&) that does something special when the
        // current type is the same as the assigned type.
        //
        v = TestObject(); // 2nd temporary
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0); // no moveAssign in current impl
        EXPECT_EQ(numDestruct, 3);   // 1st + 2nd temporary + v's previous data
    }
    EXPECT_EQ(numDestruct, 4);
}

TEST(TestValue, AssignFromRRef) {

    clearCounters();
    {
        Value v;
        TestObject obj;
        v = std::move(obj);
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);

        // Test when v already has a TestObject.
        TestObject obj2;
        v = std::move(obj2);
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0); // no moveAssign in current impl
        EXPECT_EQ(numDestruct, 1);   // v's previous data
    }
    EXPECT_EQ(numDestruct, 4);
}

TEST(TestValue, AssignFromCRef) {

    clearCounters();
    {
        Value v;
        TestObject obj;
        const TestObject& cobj = obj;
        v = cobj;
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);

        // Test when v already has a TestObject.
        v = cobj;
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 2);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0); // no copyAssign in current impl
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1); // v's previous data
    }
    EXPECT_EQ(numDestruct, 3);
}

TEST(TestValue, AssignFromRef) {

    clearCounters();
    {
        Value v;
        TestObject obj;
        TestObject& cobj = obj;
        v = cobj;
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 0);

        // Test when v already has a TestObject.
        v = cobj;
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 2);
        EXPECT_EQ(numMoveConstruct, 0);
        EXPECT_EQ(numCopyAssign, 0); // no copyAssign in current impl
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1); // v's previous data
    }
    EXPECT_EQ(numDestruct, 3);
}

TEST(TestValue, CopyAssignDifferentTypes) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        Value v2 = TestObject2();
        EXPECT_TRUE(v2.has<TestObject2>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 2);

        v2 = v;
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 3);
    }
    EXPECT_EQ(numDestruct, 5);
}

TEST(TestValue, CopyAssignSameTypes) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        Value v2 = TestObject();
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 2);

        v2 = v;
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 1);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0); // no copyAssign in current impl
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 3);
    }
    EXPECT_EQ(numDestruct, 5);
}

TEST(TestValue, CopyAssignSelf) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        const Value& v2 = v; // Avoids triggering warning if we do `v = v` directly
        v = v2;
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 2);
}

TEST(TestValue, MoveAssignDifferentTypes) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        Value v2 = TestObject2();
        EXPECT_TRUE(v2.has<TestObject2>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 2);

        // This assumes no SVO
        v2 = std::move(v);
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 3);
    }
    EXPECT_EQ(numDestruct, 4);
}

TEST(TestValue, MoveAssignSameTypes) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        Value v2 = TestObject();
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 2);

        // This assumes no SVO
        v2 = std::move(v);
        EXPECT_TRUE(v2.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 2);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 2);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 3);
    }
    EXPECT_EQ(numDestruct, 4);
}

TEST(TestValue, MoveAssignSelf) {

    clearCounters();
    {
        Value v = TestObject();
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);

        Value& v2 = v;
        v = std::move(v2);
        EXPECT_TRUE(v.has<TestObject>());
        EXPECT_EQ(numDefaultConstruct, 1);
        EXPECT_EQ(numCopyConstruct, 0);
        EXPECT_EQ(numMoveConstruct, 1);
        EXPECT_EQ(numCopyAssign, 0);
        EXPECT_EQ(numMoveAssign, 0);
        EXPECT_EQ(numDestruct, 1);
    }
    EXPECT_EQ(numDestruct, 2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
