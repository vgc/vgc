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

#include <clocale>
#include <sstream>

#include <vgc/core/parse.h>

TEST(TestParse, ReadChar) {
    std::string s = "hello";
    vgc::core::StringReader in(s);
    char c = vgc::core::readCharacter(in);
    EXPECT_EQ(c, 'h');
}

TEST(TestParse, SkipExpectedString) {
    {
        std::string s = "";
        vgc::core::StringReader in(s);
        vgc::core::skipExpectedString(in, "");
        EXPECT_NO_THROW(vgc::core::skipExpectedEof(in));
    }
    {
        std::string s = "hello";
        vgc::core::StringReader in(s);
        vgc::core::skipExpectedString(in, "");
        EXPECT_EQ(vgc::core::readCharacter(in), 'h');
    }
    {
        std::string s = "hello";
        vgc::core::StringReader in(s);
        vgc::core::skipExpectedString(in, "hell");
        EXPECT_EQ(vgc::core::readCharacter(in), 'o');
    }
    {
        std::string s = "hello";
        std::string s2 = "hell";
        vgc::core::StringReader in(s);
        vgc::core::skipExpectedString(in, s2);
        EXPECT_EQ(vgc::core::readCharacter(in), 'o');
    }
    {
        std::string s = "hello";
        vgc::core::StringReader in(s);
        vgc::core::skipExpectedString(in, "hello");
        EXPECT_NO_THROW(vgc::core::skipExpectedEof(in));
    }
    {
        std::string s = "hell";
        vgc::core::StringReader in(s);
        EXPECT_THROW(vgc::core::skipExpectedString(in, "hello"), vgc::core::ParseError);
    }
    {
        std::string s = "help";
        vgc::core::StringReader in(s);
        EXPECT_THROW(vgc::core::skipExpectedString(in, "hello"), vgc::core::ParseError);
    }
    {
        std::string s = "hello";
        vgc::core::StringReader in(s);
        EXPECT_THROW(vgc::core::skipExpectedString(in, "help"), vgc::core::ParseError);
    }
    {
        std::string s = "";
        vgc::core::StringReader in(s);
        EXPECT_THROW(vgc::core::skipExpectedString(in, "help"), vgc::core::ParseError);
    }
}

// We need this to ensure that std::stod uses the C locale
void setCLocale() {
    std::setlocale(LC_ALL, std::locale::classic().name().c_str());
}

void readDoubleApproxExpectEq(const std::vector<std::string>& v) {
    setCLocale();
    for (const std::string& s : v) {
        vgc::core::StringReader in(s);
        double parsed = vgc::core::readDoubleApprox(in);
        double expected = std::stod(s);
        EXPECT_EQ(parsed, expected) << "Tested string: \"" << s << "\"";
    }
}

void readDoubleApproxExpectNear(const std::vector<std::string>& v) {
    setCLocale();
    for (const std::string& s : v) {
        vgc::core::StringReader in(s);
        double parsed = vgc::core::readDoubleApprox(in);
        double expected = std::stod(s);
        double m = (std::min)(std::abs(parsed), std::abs(expected));
        if (m > 0) {
            EXPECT_LT((parsed - expected) / m, 1e-15) << "Tested string: \"" << s << "\"";
        }
        else {
            EXPECT_EQ(parsed, expected) << "Tested string: \"" << s << "\"";
        }
        // Note: Google Test provides EXPECT_NEAR, but it just computes the
        // absolute difference. For example, 1e-30 and 1e-50 should not be
        // considered near at all, but would pass the following test:
        //   EXPECT_NEAR(1e-30, 1e-50, 1e-15);
        // Conversely, 123456789012345678 and 123456789012345600 should be
        // considered near (15 identical significant digits), but would fail
        // the following test:
        //   EXPECT_NEAR(123456789012345678, 123456789012345600, 1e-15);
    }
}

void readDoubleApproxExpectParseError(const std::vector<std::string>& v) {
    for (const std::string& s : v) {
        vgc::core::StringReader in(s);
        EXPECT_THROW(vgc::core::readDoubleApprox(in), vgc::core::ParseError)
            << "Tested string: \"" << s << "\"";
    }
}

void readDoubleApproxExpectRangeError(const std::vector<std::string>& v) {
    for (const std::string& s : v) {
        vgc::core::StringReader in(s);
        EXPECT_THROW(vgc::core::readDoubleApprox(in), vgc::core::RangeError)
            << "Tested string: \"" << s << "\"";
    }
}

void readDoubleApproxExpectZero(const std::vector<std::string>& v) {
    for (const std::string& s : v) {
        vgc::core::StringReader in(s);
        double parsed = vgc::core::readDoubleApprox(in);
        EXPECT_EQ(parsed, 0.0) << "Tested string: \"" << s << "\"";
    }
}

TEST(TestParse, ReadDoubleApprox) {
    // Zero must be read accurately.
    readDoubleApproxExpectEq({"0", "0.0", ".0", "0.", "00", "0000", "00.", ".00"});
    readDoubleApproxExpectEq(
        {"+0", "+0.0", "+.0", "+0.", "+00", "+0000", "+00.", "+.00"});
    readDoubleApproxExpectEq(
        {"-0", "-0.0", "-.0", "-0.", "-00", "-0000", "-00.", "-.00"});
    readDoubleApproxExpectEq(
        {"0e0", "0.0e0", ".0e0", "0.e0", "00e0", "0000e0", "00.e0", ".00e0"});
    readDoubleApproxExpectEq(
        {"0e+0", "0.0e+0", ".0e+0", "0.e+0", "00e+0", "0000e+0", "00.e+0", ".00e+0"});
    readDoubleApproxExpectEq(
        {"0e-0", "0.0e-0", ".0e-0", "0.e-0", "00e-0", "0000e-0", "00.e-0", ".00e-0"});

    // Integers up to 15 digits should be read accurately.
    readDoubleApproxExpectEq({"1", "2", "3", "4", "5", "6", "7", "8", "9"});
    readDoubleApproxExpectEq({"+1", "+2", "+3", "+4", "+5", "+6", "+7", "+8", "+9"});
    readDoubleApproxExpectEq({"-1", "-2", "-3", "-4", "-5", "-6", "-7", "-8", "-9"});
    readDoubleApproxExpectEq(
        {"1.0", "2.0", "3.0", "4.0", "5.0", "6.0", "7.0", "8.0", "9.0"});
    readDoubleApproxExpectEq(
        {"+1.0", "+2.0", "+3.0", "+4.0", "+5.0", "+6.0", "+7.0", "+8.0", "+9.0"});
    readDoubleApproxExpectEq(
        {"-1.0", "-2.0", "-3.0", "-4.0", "-5.0", "-6.0", "-7.0", "-8.0", "-9.0"});
    readDoubleApproxExpectEq({"01", "02", "03", "04", "05", "06", "07", "08", "09"});
    readDoubleApproxExpectEq(
        {"010", "020", "030", "040", "050", "060", "070", "080", "090"});
    readDoubleApproxExpectEq(
        {"1e42", "1.e42", "1.0e0", "0.1e1", "0.3e1", ".1e1", "0.1234e+4", "123000e-3"});
    readDoubleApproxExpectEq(
        {"123456789012345", "1234567890123e+2", "1.23456789012345e+15"});

    // XXX This is a bug. It should be read accurately.
    // The issue is that the 16 trailing zeros create rounding errors.
    // We'll fix later. We need to defer multiplying by 10 when reading trailing zeros until
    // we get a non-zero digit.
    readDoubleApproxExpectEq({"999999999999998"});   // 15 digits: ok
    readDoubleApproxExpectEq({"999999999999998.0"}); // 15 digits + 1 trailing zero: ok
    //readDoubleApproxExpectEq({"999999999999998.00"}); // 15 digits + 2 trailing zeros: should be ok but doesn't pass
    readDoubleApproxExpectNear(
        {"999999999999998.00"}); // Small comfort: at least this passes

    // Non-integers with finite base-2 fractional part should be read accurately.
    readDoubleApproxExpectEq({"0.5", "0.25", "0.125"});

    // Integers with more than 15 digits can only be read approximately.
    readDoubleApproxExpectNear(
        {"1234567890123456", "12345678901234567", "123456789012345678"});

    // Non-integers with infinite base-2 fractional part can only be read approximately
    readDoubleApproxExpectNear(
        {"0.01", "0.009e10", "0.3", "-0.2", "-42.55", "42.142857", "1.42E1", "42e-1"});

    // This is the smallest allowed value without underflow. It can only be read approximately.
    readDoubleApproxExpectNear({"1e-307", "1000000e-313"});

    // This is the largest allowed value without overflow. It can only be read approximately.
    readDoubleApproxExpectNear(
        {"9.9999999999999999e+307", "0.0000099999999999999999e+313"});

    // Testing ParseError
    // we need at least one digit in the significand
    readDoubleApproxExpectParseError(
        {"", ".", "+", "-", "+.", "-.", "e1", ".e1", "+e1", "-e1", "+.e1", "-.e1"});
    readDoubleApproxExpectParseError(
        {"Hi",
         ".Hi",
         "+Hi",
         "-Hi",
         "+.Hi",
         "-.Hi",
         "e1Hi",
         ".e1Hi",
         "+e1Hi",
         "-e1Hi",
         "+.e1Hi",
         "-.e1Hi"});
    // we need at least one digit in the exponent
    readDoubleApproxExpectParseError({"1e", "1e+", "1e-"});
    readDoubleApproxExpectParseError({"1eHi", "1e+Hi", "1e-Hi"});
    // Can't have spaces between sign and digits
    readDoubleApproxExpectParseError({"+ 1", "- 1"});
    readDoubleApproxExpectParseError({"1e+ 1", "1e- 1"});

    // Testing RangeError
    readDoubleApproxExpectRangeError({"1e308", "10e307", "0.1e309"});

    // Test underflow: These are silently rounded to zero, no error is emitted.
    // Note: we round subnormals to zero, so that we never generate subnormals
    readDoubleApproxExpectZero({"1e-308"});
}

TEST(TestParse, ReadMixed) {
    std::string s = "42 10.0hi";
    vgc::core::StringReader in(s);
    int x;
    double y;
    char c;
    char d;
    in >> x >> y >> c >> d;
    EXPECT_EQ(x, 42);
    EXPECT_EQ(y, 10.0);
    EXPECT_EQ(c, 'h');
    EXPECT_EQ(d, 'i');
    EXPECT_FALSE(in.get(c));
}

TEST(TestParse, Parse) {
    EXPECT_EQ(vgc::core::parse<int>("42"), 42);
    EXPECT_EQ(vgc::core::parse<int>(" 42"), 42);
    EXPECT_EQ(vgc::core::parse<int>(" 42 \n"), 42);
    EXPECT_THROW(vgc::core::parse<int>("42 hello"), vgc::core::ParseError);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
