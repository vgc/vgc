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
#include <vgc/core/arithmetic.h>

using namespace vgc;

TEST(TestArithmetic, IFloorAroundZeroSigned) {
    EXPECT_THROW(core::ifloor<Int8>(-129.0), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<Int8>(-128.5), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<Int8>(-128.0), -128);
    EXPECT_EQ(core::ifloor<Int8>(-127.5), -128);
    EXPECT_EQ(core::ifloor<int>(-2.0), -2);
    EXPECT_EQ(core::ifloor<int>(-1.5), -2);
    EXPECT_EQ(core::ifloor<int>(-1.0), -1);
    EXPECT_EQ(core::ifloor<int>(-0.5), -1);
    EXPECT_EQ(core::ifloor<int>(-0.0), 0);
    EXPECT_EQ(core::ifloor<int>(0.0), 0);
    EXPECT_EQ(core::ifloor<int>(0.5), 0);
    EXPECT_EQ(core::ifloor<int>(1.0), 1);
    EXPECT_EQ(core::ifloor<int>(1.5), 1);
    EXPECT_EQ(core::ifloor<int>(2.0), 2);
    EXPECT_EQ(core::ifloor<Int8>(127.0), 127);
    EXPECT_EQ(core::ifloor<Int8>(127.5), 127);
    EXPECT_THROW(core::ifloor<Int8>(128.0), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<Int8>(128.5), core::IntegerOverflowError);
}

TEST(TestArithmetic, IFloorAroundZeroUnsigned) {
    EXPECT_THROW(core::ifloor<UInt>(-1.0), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<UInt>(-0.5), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<UInt>(-0.0), 0);
    EXPECT_EQ(core::ifloor<UInt>(0.0), 0);
    EXPECT_EQ(core::ifloor<UInt>(0.5), 0);
    EXPECT_EQ(core::ifloor<UInt>(1.0), 1);
    EXPECT_EQ(core::ifloor<UInt>(1.5), 1);
    EXPECT_EQ(core::ifloor<UInt>(2.0), 2);
    EXPECT_EQ(core::ifloor<UInt8>(255.0), 255);
    EXPECT_EQ(core::ifloor<UInt8>(255.5), 255);
    EXPECT_THROW(core::ifloor<UInt8>(256.0), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<UInt8>(256.5), core::IntegerOverflowError);
}

TEST(TestArithmetic, IFloorLimitsFloat64ToInt32) {
    // Note: any int32 is exactly representable as a float64.
    double Int32Mind = static_cast<double>(core::Int32Min);
    double Int32Maxd = static_cast<double>(core::Int32Max);
    EXPECT_THROW(core::ifloor<Int32>(Int32Mind - 1.5), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<Int32>(Int32Mind - 1.0), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<Int32>(Int32Mind - 0.5), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<Int32>(Int32Mind), core::Int32Min);
    EXPECT_EQ(core::ifloor<Int32>(Int32Mind + 0.5), core::Int32Min);
    EXPECT_EQ(core::ifloor<Int32>(Int32Mind + 1.0), core::Int32Min + 1);
    EXPECT_EQ(core::ifloor<Int32>(Int32Mind + 1.5), core::Int32Min + 1);
    EXPECT_EQ(core::ifloor<Int32>(Int32Maxd - 1.5), core::Int32Max - 2);
    EXPECT_EQ(core::ifloor<Int32>(Int32Maxd - 1.0), core::Int32Max - 1);
    EXPECT_EQ(core::ifloor<Int32>(Int32Maxd - 0.5), core::Int32Max - 1);
    EXPECT_EQ(core::ifloor<Int32>(Int32Maxd), core::Int32Max);
    EXPECT_EQ(core::ifloor<Int32>(Int32Maxd + 0.5), core::Int32Max);
    EXPECT_THROW(core::ifloor<Int32>(Int32Maxd + 1.0), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<Int32>(Int32Maxd + 1.5), core::IntegerOverflowError);
}

// Note: in all the following tests of ifloor, we must use nextafter rather
// than directly assign a floating point literal, since the latter isn't
// reliable. Example with gcc 7.4.0:
//
//   double Int64Maxd = static_cast<double>(core::Int64Max);
//   double Int64Maxd_1 = core::nextafter(Int64Maxd);
//   double Int64Maxd_2 = 9223372036854775856.0;
//   core::print("{:.1f}\n", Int64Maxd);   // "9223372036854777808.0"
//   core::print("{:.1f}\n", Int64Maxd_1); // "9223372036854777856.0"
//   core::print("{:.1f}\n", Int64Maxd_2); // "9223372036854775808.0"
//   EXPECT_EQ(Int64Maxd_1, Int64Maxd_2);  // => Fail!
//

TEST(TestArithmetic, IFloorLimitsFloat64ToInt64) {
    double Int64Mind = static_cast<double>(core::Int64Min);
    double Int64Maxd = static_cast<double>(core::Int64Max);
    double Int64Mindb = core::nextbefore(Int64Mind);
    double Int64Minda = core::nextafter(Int64Mind);
    double Int64Maxdb = core::nextbefore(Int64Maxd);
    double Int64Maxda = core::nextafter(Int64Maxd);
    core::print("Int64Min   = {:>20}\n", core::Int64Min); // -9223372036854775808
    core::print("Int64Mindb = {:>22.1f}\n", Int64Mindb);  // -9223372036854777856.0
    core::print("Int64Mind  = {:>22.1f}\n", Int64Mind);   // -9223372036854775808.0
    core::print("Int64Minda = {:>22.1f}\n", Int64Minda);  // -9223372036854774784.0
    core::print("Int64Max   = {:>20}\n", core::Int64Max); //  9223372036854775807
    core::print("Int64Maxdb = {:>22.1f}\n", Int64Maxdb);  //  9223372036854774784.0
    core::print("Int64Maxd  = {:>22.1f}\n", Int64Maxd);   //  9223372036854775808.0
    core::print("Int64Maxda = {:>22.1f}\n", Int64Maxda);  //  9223372036854775856.0
    EXPECT_THROW(core::ifloor<Int64>(Int64Mindb), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<Int64>(Int64Mind), core::Int64Min);
    EXPECT_EQ(core::ifloor<Int64>(Int64Minda), -9223372036854774784);
    EXPECT_EQ(core::ifloor<Int64>(Int64Maxdb), 9223372036854774784);
    EXPECT_THROW(core::ifloor<Int64>(Int64Maxd), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<Int64>(Int64Maxda), core::IntegerOverflowError);
}

TEST(TestArithmetic, IFloorLimitsFloat32ToInt64) {
    float Int64Minf = static_cast<float>(core::Int64Min);
    float Int64Maxf = static_cast<float>(core::Int64Max);
    float Int64Minfb = core::nextbefore(Int64Minf);
    float Int64Minfa = core::nextafter(Int64Minf);
    float Int64Maxfb = core::nextbefore(Int64Maxf);
    float Int64Maxfa = core::nextafter(Int64Maxf);
    core::print("Int64Min   = {:>20}\n", core::Int64Min); // -9223372036854775808
    core::print("Int64Minfb = {:>22.1f}\n", Int64Minfb);  // -9223373136366403584.0
    core::print("Int64Minf  = {:>22.1f}\n", Int64Minf);   // -9223372036854775808.0
    core::print("Int64Minfa = {:>22.1f}\n", Int64Minfa);  // -9223371487098961920.0
    core::print("Int64Max   = {:>20}\n", core::Int64Max); //  9223372036854775807
    core::print("Int64Maxfb = {:>22.1f}\n", Int64Maxfb);  //  9223371487098961920.0
    core::print("Int64Maxf  = {:>22.1f}\n", Int64Maxf);   //  9223372036854775808.0
    core::print("Int64Maxfa = {:>22.1f}\n", Int64Maxfa);  //  9223373136366403584.0
    EXPECT_THROW(core::ifloor<Int64>(Int64Minfb), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<Int64>(Int64Minf), core::Int64Min);
    EXPECT_EQ(core::ifloor<Int64>(Int64Minfa), -9223371487098961920);
    EXPECT_EQ(core::ifloor<Int64>(Int64Maxfb), 9223371487098961920);
    EXPECT_THROW(core::ifloor<Int64>(Int64Maxf), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<Int64>(Int64Maxfa), core::IntegerOverflowError);
}

TEST(TestArithmetic, IFloorLimitsFloat32ToInt32) {
    float Int32Minf = static_cast<float>(core::Int32Min);
    float Int32Maxf = static_cast<float>(core::Int32Max);
    float Int32Minfb = core::nextbefore(Int32Minf);
    float Int32Minfa = core::nextafter(Int32Minf);
    float Int32Maxfb = core::nextbefore(Int32Maxf);
    float Int32Maxfa = core::nextafter(Int32Maxf);
    core::print("Int32Min   = {:>20}\n", core::Int32Min); // -2147483648
    core::print("Int32Minfb = {:>22.1f}\n", Int32Minfb);  // -2147483904.0
    core::print("Int32Minf  = {:>22.1f}\n", Int32Minf);   // -2147483648.0
    core::print("Int32Minfa = {:>22.1f}\n", Int32Minfa);  // -2147483520.0
    core::print("Int32Max   = {:>20}\n", core::Int32Max); //  2147483647
    core::print("Int32Maxfb = {:>22.1f}\n", Int32Maxfb);  //  2147483520.0
    core::print("Int32Maxf  = {:>22.1f}\n", Int32Maxf);   //  2147483648.0
    core::print("Int32Maxfa = {:>22.1f}\n", Int32Maxfa);  //  2147483904.0
    EXPECT_THROW(core::ifloor<Int32>(Int32Minfb), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<Int32>(Int32Minf), core::Int32Min);
    EXPECT_EQ(core::ifloor<Int32>(Int32Minfa), -2147483520);
    EXPECT_EQ(core::ifloor<Int32>(Int32Maxfb), 2147483520);
    EXPECT_THROW(core::ifloor<Int32>(Int32Maxf), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<Int32>(Int32Maxfa), core::IntegerOverflowError);
}

TEST(TestArithmetic, IFloorLimitsFloat64ToUInt32) {
    // Note: any uint32 is exactly representable as a float64.
    double UInt32Mind = static_cast<double>(core::UInt32Min);
    double UInt32Maxd = static_cast<double>(core::UInt32Max);
    EXPECT_THROW(core::ifloor<UInt32>(UInt32Mind - 1.5), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<UInt32>(UInt32Mind - 1.0), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<UInt32>(UInt32Mind - 0.5), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Mind), core::UInt32Min);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Mind + 0.5), core::UInt32Min);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Mind + 1.0), core::UInt32Min + 1);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Mind + 1.5), core::UInt32Min + 1);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Maxd - 1.5), core::UInt32Max - 2);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Maxd - 1.0), core::UInt32Max - 1);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Maxd - 0.5), core::UInt32Max - 1);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Maxd), core::UInt32Max);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Maxd + 0.5), core::UInt32Max);
    EXPECT_THROW(core::ifloor<UInt32>(UInt32Maxd + 1.0), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<UInt32>(UInt32Maxd + 1.5), core::IntegerOverflowError);
}

TEST(TestArithmetic, IFloorLimitsFloat64ToUInt64) {
    double UInt64Mind = static_cast<double>(core::UInt64Min);
    double UInt64Maxd = static_cast<double>(core::UInt64Max);
    double UInt64Mindb = core::nextbefore(UInt64Mind);
    double UInt64Minda = core::nextafter(UInt64Mind);
    double UInt64Maxdb = core::nextbefore(UInt64Maxd);
    double UInt64Maxda = core::nextafter(UInt64Maxd);
    core::print("UInt64Min   = {:>20}\n", core::UInt64Min); //  0
    core::print("UInt64Mindb = {:>28.2e}\n", UInt64Mindb);  // -4.94e-324
    core::print("UInt64Mind  = {:>22.1f}\n", UInt64Mind);   //  0.0
    core::print("UInt64Minda = {:>28.2e}\n", UInt64Minda);  //  4.94e-324
    core::print("UInt64Max   = {:>20}\n", core::UInt64Max); //  18446744073709551615
    core::print("UInt64Maxdb = {:>22.1f}\n", UInt64Maxdb);  //  18446744073709549568.0
    core::print("UInt64Maxd  = {:>22.1f}\n", UInt64Maxd);   //  18446744073709551616.0
    core::print("UInt64Maxda = {:>22.1f}\n", UInt64Maxda);  //  18446744073709555712.0
    EXPECT_THROW(core::ifloor<UInt64>(UInt64Mindb), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<UInt64>(UInt64Mind), core::UInt64Min);
    EXPECT_EQ(core::ifloor<UInt64>(UInt64Minda), core::UInt64Min);
    EXPECT_EQ(core::ifloor<UInt64>(UInt64Maxdb), 18446744073709549568ULL);
    EXPECT_THROW(core::ifloor<UInt64>(UInt64Maxd), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<UInt64>(UInt64Maxda), core::IntegerOverflowError);
}

TEST(TestArithmetic, IFloorLimitsFloat32ToUInt64) {
    float UInt64Minf = static_cast<float>(core::UInt64Min);
    float UInt64Maxf = static_cast<float>(core::UInt64Max);
    float UInt64Minfb = core::nextbefore(UInt64Minf);
    float UInt64Minfa = core::nextafter(UInt64Minf);
    float UInt64Maxfb = core::nextbefore(UInt64Maxf);
    float UInt64Maxfa = core::nextafter(UInt64Maxf);
    core::print("UInt64Min   = {:>20}\n", core::UInt64Min); //  0
    core::print("UInt64Minfb = {:>27.2e}\n", UInt64Minfb);  // -1.40e-45
    core::print("UInt64Minf  = {:>22.1f}\n", UInt64Minf);   //  0.0
    core::print("UInt64Minfa = {:>27.2e}\n", UInt64Minfa);  //  1.40e-45
    core::print("UInt64Max   = {:>20}\n", core::UInt64Max); //  18446744073709551615
    core::print("UInt64Maxfb = {:>22.1f}\n", UInt64Maxfb);  //  18446742974197923840.0
    core::print("UInt64Maxf  = {:>22.1f}\n", UInt64Maxf);   //  18446744073709551616.0
    core::print("UInt64Maxfa = {:>22.1f}\n", UInt64Maxfa);  //  18446746272732807168.0
    EXPECT_THROW(core::ifloor<UInt64>(UInt64Minfb), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<UInt64>(UInt64Minf), core::UInt64Min);
    EXPECT_EQ(core::ifloor<UInt64>(UInt64Minfa), core::UInt64Min);
    EXPECT_EQ(core::ifloor<UInt64>(UInt64Maxfb), 18446742974197923840ULL);
    EXPECT_THROW(core::ifloor<UInt64>(UInt64Maxf), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<UInt64>(UInt64Maxfa), core::IntegerOverflowError);
}

TEST(TestArithmetic, IFloorLimitsFloat32ToUInt32) {
    float UInt32Minf = static_cast<float>(core::UInt32Min);
    float UInt32Maxf = static_cast<float>(core::UInt32Max);
    float UInt32Minfb = core::nextbefore(UInt32Minf);
    float UInt32Minfa = core::nextafter(UInt32Minf);
    float UInt32Maxfb = core::nextbefore(UInt32Maxf);
    float UInt32Maxfa = core::nextafter(UInt32Maxf);
    core::print("UInt32Min   = {:>20}\n", core::UInt32Min); //  0
    core::print("UInt32Minfb = {:>27.2e}\n", UInt32Minfb);  // -1.40e-45
    core::print("UInt32Minf  = {:>22.1f}\n", UInt32Minf);   //  0.0
    core::print("UInt32Minfa = {:>27.2e}\n", UInt32Minfa);  //  1.40e-45
    core::print("UInt32Max   = {:>20}\n", core::UInt32Max); //  4294967295
    core::print("UInt32Maxfb = {:>22.1f}\n", UInt32Maxfb);  //  4294967040.0
    core::print("UInt32Maxf  = {:>22.1f}\n", UInt32Maxf);   //  4294967296.0
    core::print("UInt32Maxfa = {:>22.1f}\n", UInt32Maxfa);  //  4294967808.0
    EXPECT_THROW(core::ifloor<UInt32>(UInt32Minfb), core::IntegerOverflowError);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Minf), core::UInt32Min);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Minfa), core::UInt32Min);
    EXPECT_EQ(core::ifloor<UInt32>(UInt32Maxfb), 4294967040);
    EXPECT_THROW(core::ifloor<UInt32>(UInt32Maxf), core::IntegerOverflowError);
    EXPECT_THROW(core::ifloor<UInt32>(UInt32Maxfa), core::IntegerOverflowError);
}

TEST(TestArithmetic, Typedefs) {
    EXPECT_EQ(core::Int8Max, 127);
    EXPECT_EQ(core::Int16Max, 32767);
    EXPECT_EQ(core::Int32Max, 2147483647LL);
    EXPECT_EQ(core::Int64Max, 9223372036854775807LL);
    EXPECT_EQ(core::UInt8Max, 255);
    EXPECT_EQ(core::UInt16Max, 65535);
    EXPECT_EQ(core::UInt32Max, 4294967295ULL);
    EXPECT_EQ(core::UInt64Max, 18446744073709551615ULL);
    EXPECT_EQ(core::FloatMax, FLT_MAX);
    EXPECT_EQ(core::DoubleMax, DBL_MAX);

    EXPECT_EQ(core::Int8Min, -128);
    EXPECT_EQ(core::Int16Min, -32768);
    EXPECT_EQ(core::Int32Min, -2147483648LL);
    EXPECT_EQ(core::Int64Min, -9223372036854775807LL - 1);
    // Note: we can't write -92...08LL directly because it's interpreted as
    // <minus> <92...08LL> and 92...08 is too big to be represented as LL
    // so it's interpreted as ULL instead. See -Wimplicitly-unsigned-literal
    EXPECT_EQ(core::UInt8Min, 0);
    EXPECT_EQ(core::UInt16Min, 0);
    EXPECT_EQ(core::UInt32Min, 0);
    EXPECT_EQ(core::UInt64Min, 0);
    EXPECT_EQ(core::FloatMin, -FLT_MAX);
    EXPECT_EQ(core::DoubleMin, -DBL_MAX);

    EXPECT_GT(core::FloatSmallestNormal, 0.0f);
    EXPECT_GT(core::DoubleSmallestNormal, 0.0);
    EXPECT_LT(core::FloatSmallestNormal, 2e-38f);
    EXPECT_LT(core::DoubleSmallestNormal, 3e-308);
    EXPECT_EQ(core::FloatSmallestNormal, FLT_MIN);
    EXPECT_EQ(core::DoubleSmallestNormal, DBL_MIN);
    EXPECT_EQ(core::FloatInfinity, HUGE_VALF);
    EXPECT_EQ(core::DoubleInfinity, HUGE_VAL);

#ifdef VGC_CORE_USE_32BIT_INT
    EXPECT_EQ(core::IntMax, core::Int32Max);
    EXPECT_EQ(core::IntMin, core::Int32Min);
    EXPECT_EQ(core::UIntMax, core::UInt32Max);
    EXPECT_EQ(core::UIntMin, core::UInt32Min);
#else
    EXPECT_EQ(core::IntMax, core::Int64Max);
    EXPECT_EQ(core::IntMin, core::Int64Min);
    EXPECT_EQ(core::UIntMax, core::UInt64Max);
    EXPECT_EQ(core::UIntMin, core::UInt64Min);
#endif
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
