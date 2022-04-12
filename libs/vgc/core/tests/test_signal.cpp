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

TEST(TestSignal, All)
{
    auto t = vgc::core::internal::TestSignalObject::create();

    t->signalIntDouble().connect(t->slotIntSlot());
    t->signalIntDouble().disconnect(t->slotIntSlot());
    t->signalIntDouble().emit(12, 2.);
    t->signalIntDouble().emit(11, 0.5);
    ASSERT_EQ(t->sumInt, 0);
    ASSERT_FLOAT_EQ(t->sumDouble, 0);

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

    t->selfConnectIntDouble();
    t->signalIntDouble().emit(21, 4.);
    // slotIntDouble, slotInt, and slotUInt should be called
    ASSERT_EQ(t->sumInt, 21 * 3);
    ASSERT_FLOAT_EQ(t->sumDouble, 4.);
    ASSERT_TRUE(t->sfnIntCalled);
    ASSERT_TRUE(t->fnIntDoubleCalled);
    ASSERT_TRUE(t->fnUIntCalled);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
