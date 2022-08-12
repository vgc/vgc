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
#include <memory>
#include <vgc/core/object.h>

using SignalTestObject = vgc::core::detail::SignalTestObject;

TEST(TestSignal, Disconnect) {
    auto o1 = SignalTestObject::create();
    auto o2 = SignalTestObject::create();

    bool vref = false;
    /*  (1) */ o1->signalIntFloat().connect(o1->slotInt());
    /*  (2) */ o1->signalIntFloat().connect(o2->slotInt());
    /*  (3) */ o1->signalIntFloat().connect(o2->signalInt());
    /*  (4) */ o1->signalIntFloat().connect(SignalTestObject::staticFuncInt);
    /*  (5) */ o1->signalIntFloat().connect([&](int) { vref = true; });
    ASSERT_EQ(o1->numConnections(), 5);

    /*  (6) */ o1->signalInt().connect(SignalTestObject::staticFuncInt);
    /*  (7) */ o1->signalInt().connect(o2->slotInt());
    /*  (8) */ o1->signalInt().connect(o2->slotInt());
    /*  (9) */ o1->signalInt().connect(o2->signalInt());
    /* (10) */ o1->signalInt().connect(o2->signalInt());
    /* (11) */ o1->signalInt().connect(o2->slotNoArgs());
    /* (12) */ o1->signalInt().connect(o2->slotUInt());
    /* (13) */ auto h = o1->signalInt().connect(o1->slotInt());
    /* (14) */ o1->signalInt().connect([&](int) { vref = true; });
    /* (15) */ o1->signalInt().connect(o1->slotInt());
    int expected = 15;
    ASSERT_EQ(o1->numConnections(), expected);

    // disconnect signal from all slots: (1), (2), (3), (4), (5)
    ASSERT_TRUE(o1->signalIntFloat().disconnect());
    expected -= 5;
    ASSERT_EQ(o1->numConnections(), expected);
    ASSERT_FALSE(o1->signalIntFloat().disconnect());

    // disconnect signal from free function: (6)
    ASSERT_TRUE(o1->signalInt().disconnect(SignalTestObject::staticFuncInt));
    expected -= 1;
    ASSERT_EQ(o1->numConnections(), expected);
    ASSERT_FALSE(o1->signalInt().disconnect(SignalTestObject::staticFuncInt));

    // disconnect signal by slot: (7), (8)
    ASSERT_TRUE(o1->signalInt().disconnect(o2->slotInt()));
    expected -= 2;
    ASSERT_EQ(o1->numConnections(), expected);
    ASSERT_FALSE(o1->signalInt().disconnect(o2->slotInt()));

    // disconnect signal by signal-slot: (9), (10)
    ASSERT_TRUE(o1->signalInt().disconnect(o2->signalInt()));
    expected -= 2;
    ASSERT_EQ(o1->numConnections(), expected);
    ASSERT_FALSE(o1->signalInt().disconnect(o2->signalInt()));

    // disconnect signal by receiver: (11), (12)
    ASSERT_TRUE(o1->signalInt().disconnect(o2.get()));
    expected -= 2;
    ASSERT_EQ(o1->numConnections(), expected);
    ASSERT_FALSE(o1->signalInt().disconnect(o2.get()));

    // disconnect signal by handle: (13)
    ASSERT_TRUE(o1->signalInt().disconnect(h));
    expected -= 1;
    ASSERT_EQ(o1->numConnections(), expected);
    ASSERT_FALSE(o1->signalInt().disconnect(h));

    // disconnect signal from all slots
    ASSERT_TRUE(o1->signalInt().disconnect());
    ASSERT_EQ(o1->numConnections(), 0);
    ASSERT_FALSE(o1->signalInt().disconnect());
}

TEST(TestSignal, EmitByRef) {
    auto o1 = SignalTestObject::create();
    auto o2 = SignalTestObject::create();

    o1->signalIntRef().connect(o2->slotIncIntRef());
    o1->signalIntRef().connect([](int& a) { a += 10; });
    int a = 1;
    o1->signalIntRef().emit(a);
    ASSERT_EQ(a, 1 + 1 + 10);
}

TEST(TestSignal, SignalToSignal) {
    auto o1 = SignalTestObject::create();
    auto o2 = SignalTestObject::create();
    auto o3 = SignalTestObject::create();

    o1->signalInt().connect(o2->signalInt());
    o2->signalInt().connect(o3->slotInt());
    o1->signalInt().emit(42);
    ASSERT_EQ(o3->sumInt, 42);

    o1->signalIntRef().connect(o2->signalIntRef());
    o2->signalIntRef().connect(o3->slotIncIntRef());
    int a = 41;
    o1->signalIntRef().emit(a);
    ASSERT_EQ(a, 42);
}

TEST(TestSignal, SameSlot) {
    auto o1 = SignalTestObject::create();
    auto o2 = SignalTestObject::create();

    o1->signalInt().connect(o2->slotInt());
    o1->signalInt().connect(o2->slotInt());
    o1->signalInt().connect(o2->slotInt());

    o1->signalInt().emit(2);
    ASSERT_EQ(o2->sumInt, 3 * 2);
}

namespace {

void getTheAnswer(int& a) {
    a = 42;
}

} // namespace
TEST(TestSignal, SignalToFreeFunc) {
    auto o1 = SignalTestObject::create();

    o1->sfnIntCalled = false;
    o1->signalInt().connect(o1->staticFuncInt);
    o1->signalInt().emit(42);
    ASSERT_TRUE(o1->sfnIntCalled);

    int theAnswer = 0;
    o1->signalIntRef().connect(getTheAnswer);
    o1->signalIntRef().emit(theAnswer);
    ASSERT_EQ(theAnswer, 42);
}

TEST(TestSignal, SignalToLambda) {
    auto o1 = SignalTestObject::create();

    int a = 0;
    o1->signalInt().connect([&](int b) { a = b; });

    o1->signalInt().emit(42);
    ASSERT_EQ(a, 42);
}

TEST(TestSignal, TruncateArgs) {
    auto o1 = SignalTestObject::create();
    auto o2 = SignalTestObject::create();

    o1->signalIntFloatBool().connect(o2->slotIntFloat());
    o1->signalIntFloatBool().connect(o2->slotInt());
    o1->signalIntFloatBool().connect(o2->slotNoArgs());

    o1->signalIntFloatBool().emit(4, 10.5f, false);
    ASSERT_EQ(o2->sumInt, 4 * 2);
    ASSERT_FLOAT_EQ(o2->sumFloat, 10.5f);
    ASSERT_EQ(o2->slotNoargsCallCount, 1);
}

TEST(TestSignal, SlotWithConvertibleArgs) {
    auto o1 = SignalTestObject::create();
    auto o2 = SignalTestObject::create();

    o1->signalIntFloat().connect(o2->slotUInt());

// C4244 = conversion from 'const int' to 'float', possible loss of data
#if defined(VGC_CORE_COMPILER_MSVC)
#    pragma warning(push)
#    pragma warning(disable : 4244)
#endif
    o1->signalIntFloat().connect(o2->slotFloat());
#if defined(VGC_CORE_COMPILER_MSVC)
#    pragma warning(pop)
#endif

    o1->signalIntFloat().emit(42, 1.f);
    ASSERT_EQ(o2->sumInt, 42);
    ASSERT_FLOAT_EQ(o2->sumFloat, 42.f);
}

TEST(TestSignal, CrossModuleSignals) {
    // The idea of this test is to call connect() inside a shared lib,
    // then call emit() outside of the shared lib. The risk is that
    // connect() adds to the SignalHub a SignalId which is different
    // from the SignalId passed by the emit() call, in which case it
    // wouldn't match any connection and the slot wouldn't be called.
    auto o1 = SignalTestObject::create();
    auto o2 = SignalTestObject::create();
    o1->connectToOtherNoArgs(o2.get());
    o1->signalNoArgs().emit();
    EXPECT_EQ(o2->slotNoargsCallCount, 1);
}

int main(int argc, char** argv) {
    // Mess up with ID generation for CrossModuleSignals test.
    // The idea is to check whether a call to  basically attempt to get
    const int biggerThanNumSignalsInCore = 1000;
    for (int i = 0; i < biggerThanNumSignalsInCore; ++i) {
        vgc::core::detail::genFunctionId();
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
