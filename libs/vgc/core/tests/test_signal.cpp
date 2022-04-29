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

using TestSignalObject = vgc::core::internal::TestSignalObject;

TEST(TestSignal, AnySignalArg)
{
    //using AnySignalArg = vgc::core::internal::AnySignalArg;
    //// Checking that AnySignalArg::IsMakeableFrom is working as expected.
    //// The goal is for AnySignalArg to not accept temporaries created on construction.
    ////
    //struct A_ {};
    //struct B_ : A_ {};
    //// upcasts are allowed
    //ASSERT_TRUE((  AnySignalArg::isMakeableFrom<       A_    , B_&              > ));
    //ASSERT_TRUE((  AnySignalArg::isMakeableFrom<       A_&   , B_&              > ));
    //ASSERT_TRUE((  AnySignalArg::isMakeableFrom< const A_&   , B_&              > ));
    //ASSERT_TRUE((  AnySignalArg::isMakeableFrom<       A_    , const B_&        > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       A_&   , const B_&        > ));
    //ASSERT_TRUE((  AnySignalArg::isMakeableFrom< const A_&   , const B_&        > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       A_    , B_&&             > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       A_&   , B_&&             > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom< const A_&   , B_&&             > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       A_    , const B_&&       > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       A_&   , const B_&&       > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom< const A_&   , const B_&&       > ));
    //// copies ar( e not allowed                  
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       int   , float&           > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       int&  , float&           > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom< const int&  , float&           > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       int   , const float&     > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       int&  , const float&     > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom< const int&  , const float&     > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       int   , float&&          > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       int&  , float&&          > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom< const int&  , float&&          > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       int   , const float&&    > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom<       int&  , const float&&    > ));
    //ASSERT_TRUE(( !AnySignalArg::isMakeableFrom< const int&  , const float&&    > ));
}

TEST(TestSignal, ConnectDisconnect)
{
    auto t = TestSignalObject::create();
    auto t2 = TestSignalObject::create();

    bool vref = false;
    /*  (1) */ t->signalIntDouble().connect(t->slotIntSlot());
    /*  (2) */ t->signalIntDouble().connect(t2->slotIntSlot());
    /*  (3) */ t->signalIntDouble().connect(t2->signalInt());
    /*  (4) */ t->signalIntDouble().connect(TestSignalObject::staticFuncInt);
    /*  (5) */ t->signalIntDouble().connect([&](int){ vref = true; });
    ASSERT_EQ(t->numConnections(), 5);

    /*  (6) */ t->signalInt().connect(TestSignalObject::staticFuncInt);
    /*  (7) */ t->signalInt().connect(t2->slotIntSlot());
    /*  (8) */ t->signalInt().connect(t2->slotIntSlot());
    /*  (9) */ t->signalInt().connect(t2->signalInt());
    /* (10) */ t->signalInt().connect(t2->signalInt());
    /* (11) */ t->signalInt().connect(t2->slotNoargsSlot());
    /* (12) */ t->signalInt().connect(t2->slotUIntSlot());
    /* (13) */ auto h = t->signalInt().connect(t->slotIntSlot());
    /* (14) */ t->signalInt().connect([&](int){ vref = true; });
    /* (15) */ t->signalInt().connect(t->slotIntSlot());
    int expected = 15;
    ASSERT_EQ(t->numConnections(), expected);

    // disconnect signal from all slots: (1), (2), (3), (4), (5)
    ASSERT_TRUE(t->signalIntDouble().disconnect());
    expected -= 5;
    ASSERT_EQ(t->numConnections(), expected);
    ASSERT_FALSE(t->signalIntDouble().disconnect());

    // disconnect signal from free function: (6)
    ASSERT_TRUE(t->signalInt().disconnect(TestSignalObject::staticFuncInt));
    expected -= 1;
    ASSERT_EQ(t->numConnections(), expected);
    ASSERT_FALSE(t->signalInt().disconnect(TestSignalObject::staticFuncInt));

    // disconnect signal by slot: (7), (8)
    ASSERT_TRUE(t->signalInt().disconnect(t2->slotIntSlot()));
    expected -= 2;
    ASSERT_EQ(t->numConnections(), expected);
    ASSERT_FALSE(t->signalInt().disconnect(t2->slotIntSlot()));

    // disconnect signal by signal-slot: (9), (10)
    ASSERT_TRUE(t->signalInt().disconnect(t2->signalInt()));
    expected -= 2;
    ASSERT_EQ(t->numConnections(), expected);
    ASSERT_FALSE(t->signalInt().disconnect(t2->signalInt()));

    // disconnect signal by receiver: (11), (12)
    ASSERT_TRUE(t->signalInt().disconnect(t2.get()));
    expected -= 2;
    ASSERT_EQ(t->numConnections(), expected);
    ASSERT_FALSE(t->signalInt().disconnect(t2.get()));

    // disconnect signal by handle: (13)
    ASSERT_TRUE(t->signalInt().disconnect(h));
    expected -= 1;
    ASSERT_EQ(t->numConnections(), expected);
    ASSERT_FALSE(t->signalInt().disconnect(h));

    // disconnect signal from all slots
    ASSERT_TRUE(t->signalInt().disconnect());
    ASSERT_EQ(t->numConnections(), 0);
    ASSERT_FALSE(t->signalInt().disconnect());
}

TEST(TestSignal, All)
{
    auto t = TestSignalObject::create();
    auto t2 = TestSignalObject::create();

    t2->signalIntDoubleBool().connect(t->signalIntDouble());

    t->signalIntDouble().connect(t->slotIntSlot());
    t->signalIntDouble().disconnect(t->slotIntSlot());
    t->signalIntDouble().emit(12, 2.);
    t->signalIntDouble().emit(11, 0.5);
    ASSERT_EQ(t->sumInt, 0);
    ASSERT_FLOAT_EQ(t->sumDouble, 0);

    {
        auto h = t->signalIntDouble().connect(t->slotIntSlot());
        t2->signalIntDoubleBool().emit(12, 2., false);
        t2->signalIntDoubleBool().emit(11, 0.5, true);
        ASSERT_EQ(t->sumInt, 23);
        ASSERT_FLOAT_EQ(t->sumDouble, 0);
        t->signalIntDouble().disconnect(h);
        t->sumInt = 0;
        t->sumDouble = 0;
    }

    {
        auto h = t->signalIntDouble().connect(t->slotIntSlot());
        t->signalIntDouble().emit(12, 2.);
        t->signalIntDouble().emit(11, 0.5);
        ASSERT_EQ(t->sumInt, 23);
        ASSERT_FLOAT_EQ(t->sumDouble, 0);
        t->signalIntDouble().disconnect(h);
        t->sumInt = 0;
        t->sumDouble = 0;
        t->signalIntDouble().emit(12, 2.);
        t->signalIntDouble().emit(11, 0.5);
        ASSERT_EQ(t->sumInt, 0);
        ASSERT_FLOAT_EQ(t->sumDouble, 0);
    }

    bool fnIntDoubleCalled = false;
    bool fnUIntCalled = false;
    t->signalIntDouble().connect(t->slotIntDoubleSlot());
    t->signalIntDouble().connect(t->slotIntSlot());
    t->signalIntDouble().connect(t->slotUIntSlot());
    t->signalIntDouble().connect(TestSignalObject::staticFuncInt);
    t->signalIntDouble().connect([&](int, double) { fnIntDoubleCalled = true; } );
    t->signalIntDouble().connect(std::function<void(unsigned int)>([&](unsigned int) { fnUIntCalled = true; }));
    t->signalIntDouble().emit(21, 4.);
    // slotIntDouble, slotInt, and slotUInt should be called
    ASSERT_EQ(t->sumInt, 21 * 3);
    ASSERT_FLOAT_EQ(t->sumDouble, 4.);
    ASSERT_TRUE(t->sfnIntCalled);
    ASSERT_TRUE(fnIntDoubleCalled);
    ASSERT_TRUE(fnUIntCalled);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
