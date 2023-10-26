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
#include <vgc/core/enum.h>
#include <vgc/core/format.h>

// For benchmarking
// TODO: Use Google Benchmark and separate benchmarks from unit tests
#include <vgc/core/stopwatch.h>

TEST(TestFormat, Format) {
    double x = 12;
    double y = 42;
    std::string s = vgc::core::format("position = ({}, {})", x, y);
    EXPECT_EQ(s, "position = (12, 42)");
}

TEST(TestFormat, FormatTo) {
    double x = 12;
    double y = 42;
    std::string out = "the position is: ";
    vgc::core::formatTo(std::back_inserter(out), "({}, {})", x, y);
    EXPECT_EQ(out, "the position is: (12, 42)");
}

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
VGC_DECLARE_ENUM(MyEnum)

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
    V100, V101, V102, V103, V104, V105, V106, V107, V108, V109,
    V110, V111, V112, V113, V114, V115, V116, V117, V118, V119,
    V120, V121, V122
};
VGC_DECLARE_ENUM(LongEnum)

enum class VeryLongEnum {
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
    V100, V101, V102, V103, V104, V105, V106, V107, V108, V109,
    V110, V111, V112, V113, V114, V115, V116, V117, V118, V119,
    V120, V121, V122, V123, V124, V125, V126, V127, V128, V129,
    V130, V131, V132, V133, V134, V135, V136, V137, V138, V139,
    V140, V141, V142, V143, V144, V145, V146, V147, V148, V149,
    V150, V151, V152, V153, V154, V155, V156, V157, V158, V159,
    V160, V161, V162, V163, V164, V165, V166, V167, V168, V169,
    V170, V171, V172, V173, V174, V175, V176, V177, V178, V179,
    V180, V181, V182, V183, V184, V185, V186, V187, V188, V189,
    V190, V191, V192, V193, V194, V195, V196, V197, V198, V199,
    V200
};

VGC_DECLARE_ENUM(VeryLongEnum)

// clang-format on

VGC_DEFINE_ENUM(MyEnum, (MyValue, "My Value"), (MyOtherValue, "My Other Value"))

// clang-format off

VGC_DEFINE_ENUM(
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

VGC_DEFINE_ENUM_BEGIN(VeryLongEnum)
    VGC_ENUM_ITEM(V1, "v1")
    VGC_ENUM_ITEM(V2, "v2")
    VGC_ENUM_ITEM(V3, "v3")
    VGC_ENUM_ITEM(V4, "v4")
    VGC_ENUM_ITEM(V5, "v5")
    VGC_ENUM_ITEM(V6, "v6")
    VGC_ENUM_ITEM(V7, "v7")
    VGC_ENUM_ITEM(V8, "v8")
    VGC_ENUM_ITEM(V9, "v9")
    VGC_ENUM_ITEM(V10, "v10")
    VGC_ENUM_ITEM(V11, "v11")
    VGC_ENUM_ITEM(V12, "v12")
    VGC_ENUM_ITEM(V13, "v13")
    VGC_ENUM_ITEM(V14, "v14")
    VGC_ENUM_ITEM(V15, "v15")
    VGC_ENUM_ITEM(V16, "v16")
    VGC_ENUM_ITEM(V17, "v17")
    VGC_ENUM_ITEM(V18, "v18")
    VGC_ENUM_ITEM(V19, "v19")
    VGC_ENUM_ITEM(V20, "v20")
    VGC_ENUM_ITEM(V21, "v21")
    VGC_ENUM_ITEM(V22, "v22")
    VGC_ENUM_ITEM(V23, "v23")
    VGC_ENUM_ITEM(V24, "v24")
    VGC_ENUM_ITEM(V25, "v25")
    VGC_ENUM_ITEM(V26, "v26")
    VGC_ENUM_ITEM(V27, "v27")
    VGC_ENUM_ITEM(V28, "v28")
    VGC_ENUM_ITEM(V29, "v29")
    VGC_ENUM_ITEM(V30, "v30")
    VGC_ENUM_ITEM(V31, "v31")
    VGC_ENUM_ITEM(V32, "v32")
    VGC_ENUM_ITEM(V33, "v33")
    VGC_ENUM_ITEM(V34, "v34")
    VGC_ENUM_ITEM(V35, "v35")
    VGC_ENUM_ITEM(V36, "v36")
    VGC_ENUM_ITEM(V37, "v37")
    VGC_ENUM_ITEM(V38, "v38")
    VGC_ENUM_ITEM(V39, "v39")
    VGC_ENUM_ITEM(V40, "v40")
    VGC_ENUM_ITEM(V41, "v41")
    VGC_ENUM_ITEM(V42, "v42")
    VGC_ENUM_ITEM(V43, "v43")
    VGC_ENUM_ITEM(V44, "v44")
    VGC_ENUM_ITEM(V45, "v45")
    VGC_ENUM_ITEM(V46, "v46")
    VGC_ENUM_ITEM(V47, "v47")
    VGC_ENUM_ITEM(V48, "v48")
    VGC_ENUM_ITEM(V49, "v49")
    VGC_ENUM_ITEM(V50, "v50")
    VGC_ENUM_ITEM(V51, "v51")
    VGC_ENUM_ITEM(V52, "v52")
    VGC_ENUM_ITEM(V53, "v53")
    VGC_ENUM_ITEM(V54, "v54")
    VGC_ENUM_ITEM(V55, "v55")
    VGC_ENUM_ITEM(V56, "v56")
    VGC_ENUM_ITEM(V57, "v57")
    VGC_ENUM_ITEM(V58, "v58")
    VGC_ENUM_ITEM(V59, "v59")
    VGC_ENUM_ITEM(V60, "v60")
    VGC_ENUM_ITEM(V61, "v61")
    VGC_ENUM_ITEM(V62, "v62")
    VGC_ENUM_ITEM(V63, "v63")
    VGC_ENUM_ITEM(V64, "v64")
    VGC_ENUM_ITEM(V65, "v65")
    VGC_ENUM_ITEM(V66, "v66")
    VGC_ENUM_ITEM(V67, "v67")
    VGC_ENUM_ITEM(V68, "v68")
    VGC_ENUM_ITEM(V69, "v69")
    VGC_ENUM_ITEM(V70, "v70")
    VGC_ENUM_ITEM(V71, "v71")
    VGC_ENUM_ITEM(V72, "v72")
    VGC_ENUM_ITEM(V73, "v73")
    VGC_ENUM_ITEM(V74, "v74")
    VGC_ENUM_ITEM(V75, "v75")
    VGC_ENUM_ITEM(V76, "v76")
    VGC_ENUM_ITEM(V77, "v77")
    VGC_ENUM_ITEM(V78, "v78")
    VGC_ENUM_ITEM(V79, "v79")
    VGC_ENUM_ITEM(V80, "v80")
    VGC_ENUM_ITEM(V81, "v81")
    VGC_ENUM_ITEM(V82, "v82")
    VGC_ENUM_ITEM(V83, "v83")
    VGC_ENUM_ITEM(V84, "v84")
    VGC_ENUM_ITEM(V85, "v85")
    VGC_ENUM_ITEM(V86, "v86")
    VGC_ENUM_ITEM(V87, "v87")
    VGC_ENUM_ITEM(V88, "v88")
    VGC_ENUM_ITEM(V89, "v89")
    VGC_ENUM_ITEM(V90, "v90")
    VGC_ENUM_ITEM(V91, "v91")
    VGC_ENUM_ITEM(V92, "v92")
    VGC_ENUM_ITEM(V93, "v93")
    VGC_ENUM_ITEM(V94, "v94")
    VGC_ENUM_ITEM(V95, "v95")
    VGC_ENUM_ITEM(V96, "v96")
    VGC_ENUM_ITEM(V97, "v97")
    VGC_ENUM_ITEM(V98, "v98")
    VGC_ENUM_ITEM(V99, "v99")
    VGC_ENUM_ITEM(V100, "v100")
    VGC_ENUM_ITEM(V101, "v101")
    VGC_ENUM_ITEM(V102, "v102")
    VGC_ENUM_ITEM(V103, "v103")
    VGC_ENUM_ITEM(V104, "v104")
    VGC_ENUM_ITEM(V105, "v105")
    VGC_ENUM_ITEM(V106, "v106")
    VGC_ENUM_ITEM(V107, "v107")
    VGC_ENUM_ITEM(V108, "v108")
    VGC_ENUM_ITEM(V109, "v109")
    VGC_ENUM_ITEM(V110, "v110")
    VGC_ENUM_ITEM(V111, "v111")
    VGC_ENUM_ITEM(V112, "v112")
    VGC_ENUM_ITEM(V113, "v113")
    VGC_ENUM_ITEM(V114, "v114")
    VGC_ENUM_ITEM(V115, "v115")
    VGC_ENUM_ITEM(V116, "v116")
    VGC_ENUM_ITEM(V117, "v117")
    VGC_ENUM_ITEM(V118, "v118")
    VGC_ENUM_ITEM(V119, "v119")
    VGC_ENUM_ITEM(V120, "v120")
    VGC_ENUM_ITEM(V121, "v121")
    VGC_ENUM_ITEM(V122, "v122")
    VGC_ENUM_ITEM(V123, "v123")
    VGC_ENUM_ITEM(V124, "v124")
    VGC_ENUM_ITEM(V125, "v125")
    VGC_ENUM_ITEM(V126, "v126")
    VGC_ENUM_ITEM(V127, "v127")
    VGC_ENUM_ITEM(V128, "v128")
    VGC_ENUM_ITEM(V129, "v129")
    VGC_ENUM_ITEM(V130, "v130")
    VGC_ENUM_ITEM(V131, "v131")
    VGC_ENUM_ITEM(V132, "v132")
    VGC_ENUM_ITEM(V133, "v133")
    VGC_ENUM_ITEM(V134, "v134")
    VGC_ENUM_ITEM(V135, "v135")
    VGC_ENUM_ITEM(V136, "v136")
    VGC_ENUM_ITEM(V137, "v137")
    VGC_ENUM_ITEM(V138, "v138")
    VGC_ENUM_ITEM(V139, "v139")
    VGC_ENUM_ITEM(V140, "v140")
    VGC_ENUM_ITEM(V141, "v141")
    VGC_ENUM_ITEM(V142, "v142")
    VGC_ENUM_ITEM(V143, "v143")
    VGC_ENUM_ITEM(V144, "v144")
    VGC_ENUM_ITEM(V145, "v145")
    VGC_ENUM_ITEM(V146, "v146")
    VGC_ENUM_ITEM(V147, "v147")
    VGC_ENUM_ITEM(V148, "v148")
    VGC_ENUM_ITEM(V149, "v149")
    VGC_ENUM_ITEM(V150, "v150")
    VGC_ENUM_ITEM(V151, "v151")
    VGC_ENUM_ITEM(V152, "v152")
    VGC_ENUM_ITEM(V153, "v153")
    VGC_ENUM_ITEM(V154, "v154")
    VGC_ENUM_ITEM(V155, "v155")
    VGC_ENUM_ITEM(V156, "v156")
    VGC_ENUM_ITEM(V157, "v157")
    VGC_ENUM_ITEM(V158, "v158")
    VGC_ENUM_ITEM(V159, "v159")
    VGC_ENUM_ITEM(V160, "v160")
    VGC_ENUM_ITEM(V161, "v161")
    VGC_ENUM_ITEM(V162, "v162")
    VGC_ENUM_ITEM(V163, "v163")
    VGC_ENUM_ITEM(V164, "v164")
    VGC_ENUM_ITEM(V165, "v165")
    VGC_ENUM_ITEM(V166, "v166")
    VGC_ENUM_ITEM(V167, "v167")
    VGC_ENUM_ITEM(V168, "v168")
    VGC_ENUM_ITEM(V169, "v169")
    VGC_ENUM_ITEM(V170, "v170")
    VGC_ENUM_ITEM(V171, "v171")
    VGC_ENUM_ITEM(V172, "v172")
    VGC_ENUM_ITEM(V173, "v173")
    VGC_ENUM_ITEM(V174, "v174")
    VGC_ENUM_ITEM(V175, "v175")
    VGC_ENUM_ITEM(V176, "v176")
    VGC_ENUM_ITEM(V177, "v177")
    VGC_ENUM_ITEM(V178, "v178")
    VGC_ENUM_ITEM(V179, "v179")
    VGC_ENUM_ITEM(V180, "v180")
    VGC_ENUM_ITEM(V181, "v181")
    VGC_ENUM_ITEM(V182, "v182")
    VGC_ENUM_ITEM(V183, "v183")
    VGC_ENUM_ITEM(V184, "v184")
    VGC_ENUM_ITEM(V185, "v185")
    VGC_ENUM_ITEM(V186, "v186")
    VGC_ENUM_ITEM(V187, "v187")
    VGC_ENUM_ITEM(V188, "v188")
    VGC_ENUM_ITEM(V189, "v189")
    VGC_ENUM_ITEM(V190, "v190")
    VGC_ENUM_ITEM(V191, "v191")
    VGC_ENUM_ITEM(V192, "v192")
    VGC_ENUM_ITEM(V193, "v193")
    VGC_ENUM_ITEM(V194, "v194")
    VGC_ENUM_ITEM(V195, "v195")
    VGC_ENUM_ITEM(V196, "v196")
    VGC_ENUM_ITEM(V197, "v197")
    VGC_ENUM_ITEM(V198, "v198")
    VGC_ENUM_ITEM(V199, "v199")
    VGC_ENUM_ITEM(V200, "v200")
VGC_DEFINE_ENUM_END()

// clang-format on

} // namespace vgc::foo

TEST(TestFormat, Enum) {
    using vgc::core::Enum;
    using vgc::core::format;
    using vgc::foo::LongEnum;
    using vgc::foo::MyEnum;
    using vgc::foo::VeryLongEnum;

    // clang-format off
    std::string_view prettyFunction1 =
        "const class vgc::core::detail::EnumData &__cdecl vgc::ui::enumData_(enum vgc::ui::Key)";
    std::string_view prettyFunction2 =
        "const ::vgc::core::detail::EnumData &vgc::ui::enumData_(Key)";
    // clang-format on

    EXPECT_EQ(vgc::core::detail::fullEnumClassName(prettyFunction1), "vgc::ui::Key");
    EXPECT_EQ(vgc::core::detail::fullEnumClassName(prettyFunction2), "vgc::ui::Key");

    EXPECT_EQ(Enum::shortName(MyEnum::MyValue), "MyValue");
    EXPECT_EQ(Enum::fullName(MyEnum::MyValue), "vgc::foo::MyEnum::MyValue");
    EXPECT_EQ(Enum::prettyName(MyEnum::MyValue), "My Value");

    std::string s1 = format("{:-^29}", MyEnum::MyValue);
    std::string_view s2 = Enum::prettyName(MyEnum::MyOtherValue);
    std::string_view s3 = Enum::prettyName(LongEnum::V1);
    std::string_view s4 = Enum::prettyName(LongEnum::V122);
    std::string_view s5 = Enum::prettyName(VeryLongEnum::V1);
    std::string_view s6 = Enum::prettyName(VeryLongEnum::V200);
    EXPECT_EQ(s1, "--vgc::foo::MyEnum::MyValue--");
    EXPECT_EQ(s2, "My Other Value");
    EXPECT_EQ(s3, "v1");
    EXPECT_EQ(s4, "v122");
    for (int i = 1; i <= 122; ++i) {
        std::string v = "v";
        std::string_view prettyName = Enum::prettyName(static_cast<LongEnum>(i));
        EXPECT_EQ(prettyName, format("v{}", i));
    }
    EXPECT_EQ(s5, "v1");
    EXPECT_EQ(s6, "v200");
    for (int i = 1; i <= 200; ++i) {
        std::string v = "v";
        std::string_view prettyName = Enum::prettyName(static_cast<VeryLongEnum>(i));
        EXPECT_EQ(prettyName, format("v{}", i));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
