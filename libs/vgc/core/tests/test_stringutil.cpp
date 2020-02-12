// Copyright 2020 The VGC Developers
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
#include <vgc/core/stringutil.h>

// For benchmarking
// TODO: Use Google Benchmark and separate benchmarks from unit tests
#include <vgc/core/stopwatch.h>

TEST(TestStringUtil, WriteChar) {
    std::string s;
    vgc::core::StringWriter sw(s);
    sw << 'a';
    vgc::core::write(sw, 'b');
    EXPECT_EQ(s, "ab");
}

TEST(TestStringUtil, WriteCString) {
    std::string s;
    vgc::core::StringWriter sw(s);
    sw << "Hello";
    vgc::core::write(sw, " World!");
    EXPECT_EQ(s, "Hello World!");
}

TEST(TestStringUtil, WriteInt8) {
    signed char   c = 'A';
    unsigned char d = 'A';
    vgc::Int8     i = 65;
    vgc::UInt8    j = 65;

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
    testWriteInteger<T>(vgc::internal::type_min<T>::value);
    testWriteInteger<T>(vgc::internal::type_max<T>::value);
}

TEST(TestStringUtil, WriteIntegers) {
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

TEST(TestStringUtil, WriteMixed) {
    vgc::Int x = 42;
    std::string s;
    vgc::core::StringWriter sw(s);
    sw << "The value of x is: " << x << "\n";
    EXPECT_EQ(s, "The value of x is: 42\n");
}

TEST(TestStringUtil, Benchmark) {
    vgc::core::Stopwatch t;
    int n = 1000000;
    std::vector<int> v;
    v.reserve(n);
    for (int i = 0; i < n ; ++i) {
        v.push_back(i);
    }

    t.restart();
    std::string s1a;
    s1a.reserve(6*n);
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
    s3a.reserve(6*n);
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
    s4a.reserve(6*n);
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
    std::cout << "StringWriter (reserved) ........... " << std::setw(10) << vgc::core::secondsToString(t1a, unit) << "\n";
    std::cout << "StringWriter (not reserved) ....... " << std::setw(10) << vgc::core::secondsToString(t1b, unit) << "\n";
    std::cout << "std::ostringstream (N/A) .......... " << std::setw(10) << vgc::core::secondsToString(t2, unit)  << "\n";
    std::cout << "toString(x) (reserved) ............ " << std::setw(10) << vgc::core::secondsToString(t3a, unit) << "\n";
    std::cout << "toString(x) (not reserved) ........ " << std::setw(10) << vgc::core::secondsToString(t3b, unit) << "\n";
    std::cout << "std::to_string(x) (reserved) ...... " << std::setw(10) << vgc::core::secondsToString(t4a, unit) << "\n";
    std::cout << "std::to_string(x) (not reserved) .. " << std::setw(10) << vgc::core::secondsToString(t4b, unit) << "\n";

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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
