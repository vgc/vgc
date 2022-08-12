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

using schar = signed char;
using llong = long long;
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;

// Note: the fundamental integral types are:
//
// bool
//
// short
// int
// long
// long long
//
// unsigned short
// unsigned int
// unsigned long
// unsigned long long
//
// char
// signed char
// unsigned char
// wchar_t
// char8_t  (since C++20)
// char16_t (since C++11)
// char32_t (since C++11)
//
// There might also in theory be "extended integer types", but there are no
// such arcane things in the platforms/compilers we support:
//
// https://stackoverflow.com/q/13403600/what-are-extended-integer-types
//
// All the types listed above are distinct types, and all other integral types
// (e.g., size_t, int64_t) are required to be a typedef for one of those types.
// For example, size_t is typically a typedef for unsigned long or unsigned
// long long.
//
// Therefore, in our tests, we don't test, say, size_t specifically.
//
// However, we do test our custom-made typedefs, to verify that they exist (the
// fixed-width integer types are in theory optional), and that they cover the
// required range.
//
// Note that std::is_signed<bool>::value is false, that is, bool is considered
// an unsigned integer.
//

TEST(TestInt, DefineZero) {

    EXPECT_EQ(bool(0), 0);
    EXPECT_EQ(short(0), 0);
    EXPECT_EQ(int(0), 0);
    EXPECT_EQ(long(0), 0);
    EXPECT_EQ(llong(0), 0);
    EXPECT_EQ(ushort(0), 0);
    EXPECT_EQ(uint(0), 0);
    EXPECT_EQ(ulong(0), 0);
    EXPECT_EQ(ullong(0), 0);
    EXPECT_EQ(char(0), 0);
    EXPECT_EQ(schar(0), 0);
    EXPECT_EQ(uchar(0), 0);
    EXPECT_EQ(wchar_t(0), 0);
    EXPECT_EQ(char16_t(0), 0);
    EXPECT_EQ(char32_t(0), 0);
    EXPECT_EQ(Int(0), 0);
    EXPECT_EQ(Int8(0), 0);
    EXPECT_EQ(Int16(0), 0);
    EXPECT_EQ(Int32(0), 0);
    EXPECT_EQ(Int64(0), 0);
    EXPECT_EQ(UInt(0), 0);
    EXPECT_EQ(UInt8(0), 0);
    EXPECT_EQ(UInt16(0), 0);
    EXPECT_EQ(UInt32(0), 0);
    EXPECT_EQ(UInt64(0), 0);

    EXPECT_EQ(core::int_cast<bool>(0), 0);
    EXPECT_EQ(core::int_cast<short>(0), 0);
    EXPECT_EQ(core::int_cast<int>(0), 0);
    EXPECT_EQ(core::int_cast<long>(0), 0);
    EXPECT_EQ(core::int_cast<llong>(0), 0);
    EXPECT_EQ(core::int_cast<ushort>(0), 0);
    EXPECT_EQ(core::int_cast<uint>(0), 0);
    EXPECT_EQ(core::int_cast<ulong>(0), 0);
    EXPECT_EQ(core::int_cast<ullong>(0), 0);
    EXPECT_EQ(core::int_cast<char>(0), 0);
    EXPECT_EQ(core::int_cast<schar>(0), 0);
    EXPECT_EQ(core::int_cast<uchar>(0), 0);
    EXPECT_EQ(core::int_cast<wchar_t>(0), 0);
    EXPECT_EQ(core::int_cast<char16_t>(0), 0);
    EXPECT_EQ(core::int_cast<char32_t>(0), 0);
    EXPECT_EQ(core::int_cast<Int>(0), 0);
    EXPECT_EQ(core::int_cast<Int8>(0), 0);
    EXPECT_EQ(core::int_cast<Int16>(0), 0);
    EXPECT_EQ(core::int_cast<Int32>(0), 0);
    EXPECT_EQ(core::int_cast<Int64>(0), 0);
    EXPECT_EQ(core::int_cast<UInt>(0), 0);
    EXPECT_EQ(core::int_cast<UInt8>(0), 0);
    EXPECT_EQ(core::int_cast<UInt16>(0), 0);
    EXPECT_EQ(core::int_cast<UInt32>(0), 0);
    EXPECT_EQ(core::int_cast<UInt64>(0), 0);

    EXPECT_EQ(core::int_cast<bool>(0), bool(0));
    EXPECT_EQ(core::int_cast<short>(0), short(0));
    EXPECT_EQ(core::int_cast<int>(0), int(0));
    EXPECT_EQ(core::int_cast<long>(0), long(0));
    EXPECT_EQ(core::int_cast<llong>(0), llong(0));
    EXPECT_EQ(core::int_cast<ushort>(0), ushort(0));
    EXPECT_EQ(core::int_cast<uint>(0), uint(0));
    EXPECT_EQ(core::int_cast<ulong>(0), ulong(0));
    EXPECT_EQ(core::int_cast<ullong>(0), ullong(0));
    EXPECT_EQ(core::int_cast<char>(0), char(0));
    EXPECT_EQ(core::int_cast<schar>(0), schar(0));
    EXPECT_EQ(core::int_cast<uchar>(0), uchar(0));
    EXPECT_EQ(core::int_cast<wchar_t>(0), wchar_t(0));
    EXPECT_EQ(core::int_cast<char16_t>(0), char16_t(0));
    EXPECT_EQ(core::int_cast<char32_t>(0), char32_t(0));
    EXPECT_EQ(core::int_cast<Int>(0), Int(0));
    EXPECT_EQ(core::int_cast<Int8>(0), Int8(0));
    EXPECT_EQ(core::int_cast<Int16>(0), Int16(0));
    EXPECT_EQ(core::int_cast<Int32>(0), Int32(0));
    EXPECT_EQ(core::int_cast<Int64>(0), Int64(0));
    EXPECT_EQ(core::int_cast<UInt>(0), UInt(0));
    EXPECT_EQ(core::int_cast<UInt8>(0), UInt8(0));
    EXPECT_EQ(core::int_cast<UInt16>(0), UInt16(0));
    EXPECT_EQ(core::int_cast<UInt32>(0), UInt32(0));
    EXPECT_EQ(core::int_cast<UInt64>(0), UInt64(0));
}

// TODO: DefineMin/DefineMax

// Note: "upcast" (resp. "downcast") in this context means int-casting to an
// integer type of greater (resp. lesser) rank. It is obviously unrelated to
// inheritance. "eqcast" means casting to the same type, which should be a
// no-op.

TEST(TestInt, EqCastZero) {

    EXPECT_EQ(core::int_cast<bool>(bool(0)), bool(0));
    EXPECT_EQ(core::int_cast<short>(short(0)), short(0));
    EXPECT_EQ(core::int_cast<int>(int(0)), int(0));
    EXPECT_EQ(core::int_cast<long>(long(0)), long(0));
    EXPECT_EQ(core::int_cast<llong>(llong(0)), llong(0));
    EXPECT_EQ(core::int_cast<ushort>(ushort(0)), ushort(0));
    EXPECT_EQ(core::int_cast<uint>(uint(0)), uint(0));
    EXPECT_EQ(core::int_cast<ulong>(ulong(0)), ulong(0));
    EXPECT_EQ(core::int_cast<ullong>(ullong(0)), ullong(0));
    EXPECT_EQ(core::int_cast<char>(char(0)), char(0));
    EXPECT_EQ(core::int_cast<schar>(schar(0)), schar(0));
    EXPECT_EQ(core::int_cast<uchar>(uchar(0)), uchar(0));
    EXPECT_EQ(core::int_cast<wchar_t>(wchar_t(0)), wchar_t(0));
    EXPECT_EQ(core::int_cast<char16_t>(char16_t(0)), char16_t(0));
    EXPECT_EQ(core::int_cast<char32_t>(char32_t(0)), char32_t(0));
    EXPECT_EQ(core::int_cast<Int>(Int(0)), Int(0));
    EXPECT_EQ(core::int_cast<Int8>(Int8(0)), Int8(0));
    EXPECT_EQ(core::int_cast<Int16>(Int16(0)), Int16(0));
    EXPECT_EQ(core::int_cast<Int32>(Int32(0)), Int32(0));
    EXPECT_EQ(core::int_cast<Int64>(Int64(0)), Int64(0));
    EXPECT_EQ(core::int_cast<UInt>(UInt(0)), UInt(0));
    EXPECT_EQ(core::int_cast<UInt8>(UInt8(0)), UInt8(0));
    EXPECT_EQ(core::int_cast<UInt16>(UInt16(0)), UInt16(0));
    EXPECT_EQ(core::int_cast<UInt32>(UInt32(0)), UInt32(0));
    EXPECT_EQ(core::int_cast<UInt64>(UInt64(0)), UInt64(0));
}

TEST(TestInt, UpCastZero) {

    // TODO: complete this
    EXPECT_EQ(core::int_cast<Int16>(Int8(0)), Int16(0));
}

TEST(TestInt, UpCastMax) {

    // TODO: complete this, use vgc::IntMin / vgc::IntMax constants
    EXPECT_EQ(core::int_cast<Int16>(Int8(127)), Int16(127));
}

TEST(TestInt, UpCastMin) {

    // TODO: complete this, use vgc::IntMin / vgc::IntMax constants
    EXPECT_EQ(core::int_cast<Int16>(Int8(-128)), Int16(-128));
}

TEST(TestInt, DownCastZero) {

    // TODO: complete this
    EXPECT_EQ(core::int_cast<Int8>(Int16(0)), Int8(0));
}

TEST(TestInt, DownCastMax) {

    // TODO: complete this, use vgc::IntMin / vgc::IntMax constants
    EXPECT_THROW(core::int_cast<Int8>(Int16(32767)), core::IntegerOverflowError);
}

TEST(TestInt, DownCastMinSigned) {

    // TODO: complete this, use vgc::IntMin / vgc::IntMax constants
    EXPECT_THROW(core::int_cast<Int8>(Int16(-32768)), core::IntegerOverflowError);
}

TEST(TestInt, DownCastMinUnsigned) {

    // TODO: complete this
    EXPECT_EQ(core::int_cast<Int8>(Int16(0)), 0);
}

TEST(TestInt, MiscNoThrow) {

    // TODO: classify and complete
    EXPECT_EQ(core::int_cast<UInt8>(Int8(127)), UInt8(127));
    EXPECT_EQ(core::int_cast<UInt8>(Int16(255)), UInt8(255));
    EXPECT_EQ(core::int_cast<UInt32>(Int32(2147483647)), UInt32(2147483647));
    EXPECT_EQ(core::int_cast<UInt32>(Int64(4294967295)), UInt32(4294967295));
    EXPECT_EQ(core::int_cast<Int8>(UInt8(127)), Int8(127));
    EXPECT_EQ(core::int_cast<Int8>(Int16(127)), Int8(127));
    EXPECT_EQ(core::int_cast<UInt8>(UInt16(255)), UInt8(255));
}

TEST(TestInt, MiscThrow) {

    // TODO: classify and complete
    EXPECT_THROW(core::int_cast<UInt8>(Int8(-1)), core::NegativeIntegerError);
    EXPECT_THROW(core::int_cast<UInt8>(Int16(256)), core::IntegerOverflowError);
    EXPECT_THROW(core::int_cast<UInt32>(Int32(-1)), core::NegativeIntegerError);
    EXPECT_THROW(core::int_cast<UInt32>(Int64(4294967296)), core::IntegerOverflowError);
    EXPECT_THROW(core::int_cast<Int8>(UInt8(128)), core::IntegerOverflowError);
    EXPECT_THROW(core::int_cast<Int8>(Int16(128)), core::IntegerOverflowError);
    EXPECT_THROW(core::int_cast<UInt8>(UInt16(256)), core::IntegerOverflowError);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
