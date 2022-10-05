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

namespace vgc::foo {

enum class MyEnum {
    MyValue,
    MyOtherValue
};

// clang-format off

enum class LongEnum {
    V1 = 1, V2, V3, V4, V5, V6, V7, V8, V9,
    V10, V11, V12, V13, V14, V15, V16, V17, V18, V19,
    V20, V21, V22, V23, V24, V25, V26, V27, V28, V29,
    V30, V31, V32, V33, V34, V35, V36, V37, V38, V39,
    V40, V41, V42, V43, V44, V45, V46, V47, V48, V49,
    V50, V51, V52, V53, V54, V55, V56, V57, V58, V59,
    V60, V61, V62, V63, V64, V65, V66, V67, V68, V69,
    V70, V71, V72, V73, V74, V75, V76, V77, V78, V79,
    V80, V81, V82, V83, V84, V85, V86, V87, V88, V89,
    V90, V91, V92, V93, V94, V95, V96, V97, V98, V99,
    V100, V101, V102, V103, V104, V105, V106, V107, V108,
    V109, V110, V111, V112, V113, V114, V115, V116, V117,
    V118, V119, V120, V121, V122
};

// clang-format on

} // namespace vgc::foo

VGC_DECLARE_SCOPED_ENUM_FORMATTER((vgc, foo), MyEnum)

VGC_DEFINE_SCOPED_ENUM_FORMATTER(
    (vgc, foo),
    MyEnum,
    (MyValue, "My Value"),
    (MyOtherValue, "My Other Value"))

VGC_DECLARE_SCOPED_ENUM_FORMATTER((vgc, foo), LongEnum)

// clang-format off

VGC_DEFINE_SCOPED_ENUM_FORMATTER(
    (vgc, foo),
    LongEnum,
    (V1, "v1"), (V2, "v2"), (V3, "v3"), (V4, "v4"), (V5, "v5"), (V6, "v6"), (V7, "v7"), (V8, "v8"), (V9, "v9"),
    (V10, "v10"), (V11, "v11"), (V12, "v12"), (V13, "v13"), (V14, "v14"), (V15, "v15"), (V16, "v16"), (V17, "v17"), (V18, "v18"), (V19, "v19"),
    (V20, "v20"), (V21, "v21"), (V22, "v22"), (V23, "v23"), (V24, "v24"), (V25, "v25"), (V26, "v26"), (V27, "v27"), (V28, "v28"), (V29, "v29"),
    (V30, "v30"), (V31, "v31"), (V32, "v32"), (V33, "v33"), (V34, "v34"), (V35, "v35"), (V36, "v36"), (V37, "v37"), (V38, "v38"), (V39, "v39"),
    (V40, "v40"), (V41, "v41"), (V42, "v42"), (V43, "v43"), (V44, "v44"), (V45, "v45"), (V46, "v46"), (V47, "v47"), (V48, "v48"), (V49, "v49"),
    (V50, "v50"), (V51, "v51"), (V52, "v52"), (V53, "v53"), (V54, "v54"), (V55, "v55"), (V56, "v56"), (V57, "v57"), (V58, "v58"), (V59, "v59"),
    (V60, "v60"), (V61, "v61"), (V62, "v62"), (V63, "v63"), (V64, "v64"), (V65, "v65"), (V66, "v66"), (V67, "v67"), (V68, "v68"), (V69, "v69"),
    (V70, "v70"), (V71, "v71"), (V72, "v72"), (V73, "v73"), (V74, "v74"), (V75, "v75"), (V76, "v76"), (V77, "v77"), (V78, "v78"), (V79, "v79"),
    (V80, "v80"), (V81, "v81"), (V82, "v82"), (V83, "v83"), (V84, "v84"), (V85, "v85"), (V86, "v86"), (V87, "v87"), (V88, "v88"), (V89, "v89"),
    (V90, "v90"), (V91, "v91"), (V92, "v92"), (V93, "v93"), (V94, "v94"), (V95, "v95"), (V96, "v96"), (V97, "v97"), (V98, "v98"), (V99, "v99"),
    (V100, "v100"), (V101, "v101"), (V102, "v102"), (V103, "v103"), (V104, "v104"), (V105, "v105"), (V106, "v106"), (V107, "v107"), (V108, "v108"), (V109, "v109"),
    (V110, "v110"), (V111, "v111"), (V112, "v112"), (V113, "v113"), (V114, "v114"), (V115, "v115"), (V116, "v116"), (V117, "v117"), (V118, "v118"), (V119, "v119"),
    (V120, "v120"), (V121, "v121"), (V122, "v122"))

// clang-format on

TEST(TestFormat, Enum) {
    std::string s1 = vgc::core::format("{:-^29}", vgc::foo::MyEnum::MyValue);
    std::string_view s2 = enumeratorStrings(vgc::foo::MyEnum::MyOtherValue).prettyName();
    std::string_view s3 = enumeratorStrings(vgc::foo::LongEnum::V1).prettyName();
    std::string_view s4 = enumeratorStrings(vgc::foo::LongEnum::V122).prettyName();
    EXPECT_EQ(s1, "--vgc::foo::MyEnum::MyValue--");
    EXPECT_EQ(s2, "My Other Value");
    EXPECT_EQ(s3, "v1");
    EXPECT_EQ(s4, "v122");
    for (int i = 2; i < 122; ++i) {
        std::string v = "v";
        std::string_view prettyName =
            enumeratorStrings(static_cast<vgc::foo::LongEnum>(i)).prettyName();
        EXPECT_EQ(prettyName, vgc::core::format("v{}", i));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
