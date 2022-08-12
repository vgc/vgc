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
#include <sstream>
#include <vgc/core/arithmetic.h>
#include <vgc/core/compiler.h>
#include <vgc/core/format.h>

// For benchmarking
// TODO: Use Google Benchmark and separate benchmarks from unit tests
#include <vgc/core/stopwatch.h>

TEST(TestFormat, WriteChar) {
    std::string s;
    vgc::core::StringWriter sw(s);
    sw << 'a';
    vgc::core::write(sw, 'b');
    EXPECT_EQ(s, "ab");
}

TEST(TestFormat, WriteCString) {
    std::string s;
    vgc::core::StringWriter sw(s);
    sw << "Hello";
    vgc::core::write(sw, " World!");
    EXPECT_EQ(s, "Hello World!");
}

TEST(TestFormat, WriteInt8) {
    signed char c = 'A';
    unsigned char d = 'A';
    vgc::Int8 i = 65;
    vgc::UInt8 j = 65;

    std::ostringstream oss;
    oss << c << d << i << j;
    EXPECT_EQ(oss.str(), "AAAA");

    std::string s;
    vgc::core::StringWriter sw(s);
    sw << c << d << i << j;
    EXPECT_EQ(s, "65656565");
}

template<typename T>
void testWriteInteger(T x) {
    std::string s;
    vgc::core::StringWriter sw(s);
    sw << x;
    EXPECT_EQ(s, std::to_string(x));
}

template<typename T>
void testWriteIntegers() {
    testWriteInteger<T>(0);
    testWriteInteger<T>(vgc::core::tmin<T>);
    testWriteInteger<T>(vgc::core::tmax<T>);
}

TEST(TestFormat, WriteIntegers) {
    testWriteIntegers<vgc::Int>();
    testWriteIntegers<vgc::Int8>();
    testWriteIntegers<vgc::Int16>();
    testWriteIntegers<vgc::Int32>();
    testWriteIntegers<vgc::Int64>();
    testWriteIntegers<vgc::UInt>();
    testWriteIntegers<vgc::UInt8>();
    testWriteIntegers<vgc::UInt16>();
    testWriteIntegers<vgc::UInt32>();
    testWriteIntegers<vgc::UInt64>();
}

template<typename T, VGC_REQUIRES(std::is_floating_point_v<T>)>
void writeFloat(T x, const char* expected) {
    std::string s;
    vgc::core::StringWriter sw(s);
    sw << x;
    EXPECT_EQ(s, expected);
}

#if defined(VGC_CORE_COMPILER_MSVC)
#    pragma warning(push)
#    pragma warning(disable : 4723) // potential divide by 0
#endif
template<typename T>
void writeFloatsCreatedViaDivideByZero() {
    T zero = static_cast<T>(0);
    T one = static_cast<T>(1);
    writeFloat(one / zero, "inf");
    writeFloat(-one / zero, "-inf");
    writeFloat(zero / zero, "nan");
    writeFloat(-zero / zero, "nan");
}
#if defined(VGC_CORE_COMPILER_MSVC)
#    pragma warning(pop)
#endif

template<typename T>
void writeFloats() {
    T zero = static_cast<T>(0);
    T inf = std::numeric_limits<T>::infinity();
    writeFloat(zero, "0");
    writeFloat(-zero, "0");
    writeFloat(inf, "inf");
    writeFloat(-inf, "-inf");
    writeFloatsCreatedViaDivideByZero<T>();
    writeFloat(static_cast<T>(42.0), "42");
    writeFloat(static_cast<T>(420.0), "420");
    writeFloat(static_cast<T>(1988.42), "1988.42");
    writeFloat(static_cast<T>(0.000010), "0.00001");
    writeFloat(static_cast<T>(0.0000000000004), "0");
    writeFloat(static_cast<T>(0.0000000000006), "0.000000000001");
    writeFloat(static_cast<T>(41.99999999999999), "42");
    writeFloat(static_cast<T>(-42.0), "-42");
    writeFloat(static_cast<T>(-420.0), "-420");
    writeFloat(static_cast<T>(-1988.42), "-1988.42");
    writeFloat(static_cast<T>(-0.000010), "-0.00001");
    writeFloat(static_cast<T>(-0.0000000000004), "0");
    writeFloat(static_cast<T>(-0.0000000000006), "-0.000000000001");
    writeFloat(static_cast<T>(-41.99999999999999), "-42");
}

TEST(TestFormat, WriteFloats) {
    writeFloats<float>();

    writeFloat(0.1234567890123456f, "0.123457");
    writeFloat(0.012345601f, "0.0123456");
    writeFloat(0.012345641f, "0.0123456");
    //  writeFloat(0.012345651f, unspecified);
    writeFloat(0.012345661f, "0.0123457");
    writeFloat(0.012345691f, "0.0123457");
    writeFloat(0.012345991f, "0.012346");
    writeFloat(0.012349991f, "0.01235");
    writeFloat(0.012399991f, "0.0124");
    writeFloat(0.012999991f, "0.013");
    writeFloat(0.019999991f, "0.02");
    writeFloat(0.099999991f, "0.1");
    writeFloat(0.999999991f, "1");
    writeFloat(12345601.f, "12345600");
    writeFloat(12345641.f, "12345600");
    //  writeFloat(12345651.f,  unspecified);
    writeFloat(12345661.f, "12345700");
    writeFloat(12345691.f, "12345700");
    writeFloat(12345991.f, "12346000");
    writeFloat(12349991.f, "12350000");
    writeFloat(12399991.f, "12400000");
    writeFloat(12999991.f, "13000000");
    writeFloat(19999991.f, "20000000");
    writeFloat(99999991.f, "100000000");
    writeFloat(1234.5601f, "1234.56");
    writeFloat(1234.5641f, "1234.56");
    //  writeFloat(1234.5651f,  unspecified);
    writeFloat(1234.5661f, "1234.57");
    writeFloat(1234.5691f, "1234.57");
    writeFloat(1234.5991f, "1234.6");
    writeFloat(1234.9991f, "1235");
    writeFloat(1239.9991f, "1240");
    writeFloat(1299.9991f, "1300");
    writeFloat(1999.9991f, "2000");
    writeFloat(9999.9991f, "10000");

    writeFloat(-0.1234567890123456f, "-0.123457");
    writeFloat(-0.012345601f, "-0.0123456");
    writeFloat(-0.012345641f, "-0.0123456");
    //  writeFloat(-0.012345651f, unspecified);
    writeFloat(-0.012345661f, "-0.0123457");
    writeFloat(-0.012345691f, "-0.0123457");
    writeFloat(-0.012345991f, "-0.012346");
    writeFloat(-0.012349991f, "-0.01235");
    writeFloat(-0.012399991f, "-0.0124");
    writeFloat(-0.012999991f, "-0.013");
    writeFloat(-0.019999991f, "-0.02");
    writeFloat(-0.099999991f, "-0.1");
    writeFloat(-0.999999991f, "-1");
    writeFloat(-12345601.f, "-12345600");
    writeFloat(-12345641.f, "-12345600");
    //  writeFloat(-12345651.f,  unspecified);
    writeFloat(-12345661.f, "-12345700");
    writeFloat(-12345691.f, "-12345700");
    writeFloat(-12345991.f, "-12346000");
    writeFloat(-12349991.f, "-12350000");
    writeFloat(-12399991.f, "-12400000");
    writeFloat(-12999991.f, "-13000000");
    writeFloat(-19999991.f, "-20000000");
    writeFloat(-99999991.f, "-100000000");
    writeFloat(-1234.5601f, "-1234.56");
    writeFloat(-1234.5641f, "-1234.56");
    //  writeFloat(-1234.5651f,  unspecified);
    writeFloat(-1234.5661f, "-1234.57");
    writeFloat(-1234.5691f, "-1234.57");
    writeFloat(-1234.5991f, "-1234.6");
    writeFloat(-1234.9991f, "-1235");
    writeFloat(-1239.9991f, "-1240");
    writeFloat(-1299.9991f, "-1300");
    writeFloat(-1999.9991f, "-2000");
    writeFloat(-9999.9991f, "-10000");
}

TEST(TestFormat, WriteDoubles) {
    writeFloats<double>();

    writeFloat(0.1234567890123456, "0.123456789012");
    writeFloat(0.1234567890124, "0.123456789012");
    //  writeFloat(0.1234567890125,      unspecified);
    writeFloat(0.1234567890126, "0.123456789013");
    writeFloat(0.9999999999994, "0.999999999999");
    writeFloat(0.9999999999996, "1");
    writeFloat(1234567890.123456789, "1234567890.12346");
    writeFloat(999999999999999., "999999999999999");
    writeFloat(9999999999999994., "9999999999999990");
    writeFloat(9999999999999996., "10000000000000000");

    writeFloat(-0.1234567890123456, "-0.123456789012");
    writeFloat(-0.1234567890124, "-0.123456789012");
    //  writeFloat(-0.1234567890125,      unspecified);
    writeFloat(-0.1234567890126, "-0.123456789013");
    writeFloat(-0.9999999999994, "-0.999999999999");
    writeFloat(-0.9999999999996, "-1");
    writeFloat(-1234567890.123456789, "-1234567890.12346");
    writeFloat(-999999999999999., "-999999999999999");
    writeFloat(-9999999999999994., "-9999999999999990");
    writeFloat(-9999999999999996., "-10000000000000000");
}

TEST(TestFormat, WriteMixed) {
    vgc::Int x = 42;
    std::string s;
    vgc::core::StringWriter sw(s);
    sw << "The value of x is: " << x << "\n";
    EXPECT_EQ(s, "The value of x is: 42\n");
}

TEST(TestFormat, WriteVariadic) {
    int x = 42;
    double y = 1.5;
    std::string s;
    vgc::core::StringWriter out(s);
    vgc::core::write(out, '(', x, ", ", y, ')');
    EXPECT_EQ(s, "(42, 1.5)");
}

TEST(TestFormat, Benchmark) {
    vgc::core::Stopwatch t;
    int n = 1000000;
    std::vector<int> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) {
        v.push_back(i);
    }

    t.restart();
    std::string s1a;
    s1a.reserve(6 * n);
    vgc::core::StringWriter sw1a(s1a);
    for (int x : v) {
        sw1a << x;
    }
    double t1a = t.elapsed();

    t.restart();
    std::string s1b;
    vgc::core::StringWriter sw1b(s1b);
    for (int x : v) {
        sw1b << x;
    }
    double t1b = t.elapsed();

    t.restart();
    std::ostringstream oss;
    for (int x : v) {
        oss << x;
    }
    std::string s2 = oss.str();
    double t2 = t.elapsed();

    t.restart();
    std::string s3a;
    s3a.reserve(6 * n);
    for (int x : v) {
        s3a += vgc::core::toString(x);
    }
    double t3a = t.elapsed();

    t.restart();
    std::string s3b;
    for (int x : v) {
        s3b += vgc::core::toString(x);
    }
    double t3b = t.elapsed();

    t.restart();
    std::string s4a;
    s4a.reserve(6 * n);
    for (int x : v) {
        s4a += std::to_string(x);
    }
    double t4a = t.elapsed();

    t.restart();
    std::string s4b;
    for (int x : v) {
        s4b += std::to_string(x);
    }
    double t4b = t.elapsed();

    // Check that we get the same result
    EXPECT_EQ(s1a, s1b);
    EXPECT_EQ(s1a, s2);
    EXPECT_EQ(s1a, s3a);
    EXPECT_EQ(s1a, s3b);
    EXPECT_EQ(s1a, s4a);
    EXPECT_EQ(s1a, s4b);

    // Print timings. These normally don't show up if the test
    // succeeds, but you can manually run the test binary.
    auto unit = vgc::core::TimeUnit::Microseconds;
    std::cout << "StringWriter (reserved) ........... " << std::setw(10)
              << vgc::core::secondsToString(t1a, unit) << "\n";
    std::cout << "StringWriter (not reserved) ....... " << std::setw(10)
              << vgc::core::secondsToString(t1b, unit) << "\n";
    std::cout << "std::ostringstream (N/A) .......... " << std::setw(10)
              << vgc::core::secondsToString(t2, unit) << "\n";
    std::cout << "toString(x) (reserved) ............ " << std::setw(10)
              << vgc::core::secondsToString(t3a, unit) << "\n";
    std::cout << "toString(x) (not reserved) ........ " << std::setw(10)
              << vgc::core::secondsToString(t3b, unit) << "\n";
    std::cout << "std::to_string(x) (reserved) ...... " << std::setw(10)
              << vgc::core::secondsToString(t4a, unit) << "\n";
    std::cout << "std::to_string(x) (not reserved) .. " << std::setw(10)
              << vgc::core::secondsToString(t4b, unit) << "\n";

    // Example output (Intel i7-7700K, 32GM RAM, Ubuntu 18.04, gcc 7.4.0 with -O3 -DNDEBUG):
    //
    // StringWriter (reserved) ...........   11085µs
    // StringWriter (not reserved) .......   13456µs
    // std::ostringstream (N/A) ..........   30572µs
    // toString(x) (reserved) ............   14672µs
    // toString(x) (not reserved) ........   16127µs
    // std::to_string(x) (reserved) ......   56987µs
    // std::to_string(x) (not reserved) ..   60112µs
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
