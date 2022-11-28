// Copyright 2022 The VGC Developers
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

#include <array>
#include <iterator>

#include <gtest/gtest.h>
#include <vgc/core/array.h>
#include <vgc/core/span.h>

using vgc::Int;
using vgc::core::Array;
using vgc::core::dynamicExtent;
using vgc::core::IndexError;
using vgc::core::LogicError;
using vgc::core::NegativeIntegerError;
using vgc::core::Span;

inline constexpr vgc::core::NoInit noInit = vgc::core::noInit;

#define EXPECT_LENGTH(a, n)                                                              \
    {                                                                                    \
        EXPECT_EQ((a).size(), (n));                                                      \
        EXPECT_EQ((a).length(), (n));                                                    \
    }

TEST(TestSpan, Construct) {
    const int ca[4] = {10, 42, 12, 8};
    const std::array<int, 4> a = {10, 42, 12, 8};
    const Array<int> v = {10, 42, 12, 8};

    // Span()
    {
        Span<const int> s;
        EXPECT_LENGTH(s, 0);
    }
    {
        Span<const int, 0> s;
        EXPECT_LENGTH(s, 0);
    }

    // Span(T* first, IntType length)
    {
        Span<const int> s(a.data() + 1, 2);
        EXPECT_LENGTH(s, 2);
        EXPECT_EQ(s[0], 42);
    }
    {
        Span<const int, 2> s(a.data() + 1, 2);
        EXPECT_LENGTH(s, 2);
        EXPECT_EQ(s[0], 42);
    }
    EXPECT_THROW((Span<const int, 2>(a.data() + 1, 1)), LogicError);
    EXPECT_THROW((Span<const int>(a.data(), -1)), NegativeIntegerError);
    EXPECT_THROW((Span<const int, 2>(a.data(), -1)), NegativeIntegerError);
    EXPECT_THROW((Span<const int>(a.data(), dynamicExtent)), NegativeIntegerError);
    EXPECT_THROW((Span<const int, 2>(a.data(), dynamicExtent)), NegativeIntegerError);

    // Span(T* first, T* last)
    {
        Span<const int> s(v.begin() + 1, v.begin() + 3);
        EXPECT_LENGTH(s, 2);
        EXPECT_EQ(s[0], 42);
    }
    {
        Span<const int, 3> s(v.begin() + 1, v.end());
        EXPECT_LENGTH(s, 3);
        EXPECT_EQ(s[0], 42);
    }
    EXPECT_THROW((Span<const int, 2>(v.begin() + 1, v.end())), LogicError);

    // Span(core::TypeIdentity<T> (&arr)[n])
    {
        Span<const int> s(ca);
        EXPECT_LENGTH(s, 4);
        EXPECT_EQ(s[0], 10);
    }
    {
        Span<const int, 4> s(ca);
        EXPECT_LENGTH(s, 4);
        EXPECT_EQ(s[0], 10);
    }

    // Span(std::array<U, n>& arr)
    {
        std::array<int, 4> a_ = {10, 42, 12, 8};
        {
            Span<const int> s(a_);
            EXPECT_LENGTH(s, 4);
            EXPECT_EQ(s[0], 10);
        }
        {
            Span<const int, 4> s(a_);
            EXPECT_LENGTH(s, 4);
            EXPECT_EQ(s[0], 10);
        }
    }

    // Span(const std::array<U, n>& arr)
    {
        Span<const int> s(a);
        EXPECT_LENGTH(s, 4);
        EXPECT_EQ(s[0], 10);
    }
    {
        Span<const int, 4> s(a);
        EXPECT_LENGTH(s, 4);
        EXPECT_EQ(s[0], 10);
    }

    // Span(const Span<U, e>& other)
    {
        std::array<int, 4> a_ = {10, 42, 12, 8};
        {
            Span<int> s0(a_);
            Span<const int> s1(s0);
            EXPECT_LENGTH(s1, 4);
            EXPECT_EQ(s1[0], 10);
        }
        {
            Span<int> s0(a_);
            Span<const int, 4> s1(s0);
            EXPECT_LENGTH(s1, 4);
            EXPECT_EQ(s1[0], 10);
        }
        {
            Span<int, 4> s0(a_);
            Span<const int> s1(s0);
            EXPECT_LENGTH(s1, 4);
            EXPECT_EQ(s1[0], 10);
        }
        {
            Span<int, 4> s0(a_);
            Span<const int, 4> s1(s0);
            EXPECT_LENGTH(s1, 4);
            EXPECT_EQ(s1[0], 10);
        }
    }

    // Span(Array<U>& arr)
    {
        Array<int> a_ = {10, 42, 12, 8};
        {
            Span<int> s(a_);
            EXPECT_LENGTH(s, 4);
            EXPECT_EQ(s[0], 10);
        }
        {
            Span<const int, 4> s(a_);
            EXPECT_LENGTH(s, 4);
            EXPECT_EQ(s[0], 10);
        }
        VGC_DEBUG_TMP("000");
        try {
            EXPECT_THROW((Span<const int, 3>(a_)), LogicError);
        }
        catch (std::exception& e) {
            VGC_DEBUG_TMP(e.what());
        }
        VGC_DEBUG_TMP("aaa");
    }
    VGC_DEBUG_TMP("bbb");
}

TEST(TestSpan, Copy) {
    const std::array<int, 4> a = {10, 42, 12, 8};
    {
        Span<const int, 4> s0(a);
        Span<const int, 4> s(s0);
        EXPECT_EQ(s.length(), 4);
        EXPECT_EQ(s[0], 10);
    }
    {
        Span<const int> s0(a);
        Span<const int> s(s0);
        EXPECT_EQ(s.length(), 4);
        EXPECT_EQ(s[0], 10);
    }
    {
        Span<const int> s0(a);
        Span<const int> s = {};
        s = s0;
        EXPECT_EQ(s.length(), 4);
        EXPECT_EQ(s[0], 10);
    }
}

TEST(TestSpan, Iterators) {
    std::array<int, 3> a = {10, 42, 12};
    Span<int> sX(a);
    Span<int, 3> sN(a);
    std::vector<int> tmp;

    {
        tmp.assign(sX.begin(), sX.end());
        EXPECT_EQ(tmp[0], 10);
        EXPECT_EQ(tmp[1], 42);
        EXPECT_EQ(tmp[2], 12);
        tmp.assign(sN.begin(), sN.end());
        EXPECT_EQ(tmp[0], 10);
        EXPECT_EQ(tmp[1], 42);
        EXPECT_EQ(tmp[2], 12);
        tmp.clear();
    }

    {
        for (int& x : sX) {
            x += 100;
        }
        EXPECT_EQ(a[0], 110);
        EXPECT_EQ(a[1], 142);
        EXPECT_EQ(a[2], 112);

        for (int& x : sN) {
            x -= 100;
        }
        EXPECT_EQ(a[0], 10);
        EXPECT_EQ(a[1], 42);
        EXPECT_EQ(a[2], 12);
    }

    {
        tmp.assign(sX.rbegin(), sX.rend());
        EXPECT_EQ(tmp[0], 12);
        EXPECT_EQ(tmp[1], 42);
        EXPECT_EQ(tmp[2], 10);
        tmp.assign(sN.rbegin(), sN.rend());
        EXPECT_EQ(tmp[0], 12);
        EXPECT_EQ(tmp[1], 42);
        EXPECT_EQ(tmp[2], 10);
        tmp.clear();
    }

    {
        *sX.rbegin() = 8;
        EXPECT_EQ(a[2], 8);
        *sN.rbegin() = 6;
        EXPECT_EQ(a[2], 6);
    }
}

TEST(TestSpan, GetFirstLast) {
    std::array<int, 3> ca = {10, 42, 12};
    Span<int> sX(ca);
    Span<int, 3> sN(ca);

    EXPECT_EQ(sX.first(), 10);
    EXPECT_EQ(sN.first(), 10);
    sX.first() = 50;
    EXPECT_EQ(ca[0], 50);
    sN.first() = 51;
    EXPECT_EQ(ca[0], 51);

    EXPECT_EQ(sX.last(), 12);
    EXPECT_EQ(sN.last(), 12);
    sX.last() = 150;
    EXPECT_EQ(ca[2], 150);
    sN.last() = 151;
    EXPECT_EQ(ca[2], 151);

    Span<int> sE = {};
    EXPECT_THROW(sE.first(), IndexError);
    EXPECT_THROW(sE.last(), IndexError);
}

TEST(TestSpan, GetChecked) {
    std::array<int, 3> ca = {10, 42, 12};
    Span<int> sX(ca);
    Span<int, 3> sN(ca);

    EXPECT_EQ(sX[size_t(0)], 10);
    EXPECT_EQ(sX[Int(1)], 42);
    EXPECT_EQ(sX[size_t(0)], 10);
    EXPECT_EQ(sX[Int(1)], 42);

    EXPECT_EQ(sN[size_t(0)], 10);
    EXPECT_EQ(sN[Int(1)], 42);
    EXPECT_EQ(sN[size_t(0)], 10);
    EXPECT_EQ(sN[Int(1)], 42);

    sX[size_t(2)] = 50;
    EXPECT_EQ(ca[2], 50);
    sX[Int(2)] = 150;
    EXPECT_EQ(ca[2], 150);

    sN[Int(2)] = 151;
    EXPECT_EQ(ca[2], 151);
    sN[size_t(2)] = 51;
    EXPECT_EQ(ca[2], 51);

    EXPECT_THROW(sX[-1], IndexError);
    EXPECT_THROW(sX[Int(-1)], IndexError);
    EXPECT_THROW(sX[10], IndexError);
    EXPECT_THROW(sX[size_t(10)], IndexError);
    EXPECT_THROW(sX[Int(10)], IndexError);

    EXPECT_THROW(sN[-1], IndexError);
    EXPECT_THROW(sN[Int(-1)], IndexError);
    EXPECT_THROW(sN[10], IndexError);
    EXPECT_THROW(sN[size_t(10)], IndexError);
    EXPECT_THROW(sN[Int(10)], IndexError);
}

TEST(TestSpan, GetUnchecked) {
    std::array<int, 5> padded = {0, 10, 42, 12, 0};
    int* ca = padded.data() + 1;
    Span<int> sX(ca, ca + 3);
    Span<int, 3> sN(ca, ca + 3);

    EXPECT_EQ(sX.getUnchecked(size_t(0)), 10);
    EXPECT_EQ(sX.getUnchecked(Int(1)), 42);
    EXPECT_EQ(sX.getUnchecked(size_t(0)), 10);
    EXPECT_EQ(sX.getUnchecked(Int(1)), 42);

    EXPECT_EQ(sN.getUnchecked(size_t(0)), 10);
    EXPECT_EQ(sN.getUnchecked(Int(1)), 42);
    EXPECT_EQ(sN.getUnchecked(size_t(0)), 10);
    EXPECT_EQ(sN.getUnchecked(Int(1)), 42);

    sX.getUnchecked(size_t(2)) = 50;
    EXPECT_EQ(ca[2], 50);
    sX.getUnchecked(Int(2)) = 150;
    EXPECT_EQ(ca[2], 150);

    sN.getUnchecked(Int(2)) = 151;
    EXPECT_EQ(ca[2], 151);
    sN.getUnchecked(size_t(2)) = 51;
    EXPECT_EQ(ca[2], 51);

    // indices -1 and 3 are safe to access.

    EXPECT_NO_THROW(sX.getUnchecked(-1));
    EXPECT_NO_THROW(sX.getUnchecked(Int(-1)));
    EXPECT_NO_THROW(sX.getUnchecked(3));
    EXPECT_NO_THROW(sX.getUnchecked(size_t(3)));
    EXPECT_NO_THROW(sX.getUnchecked(Int(3)));

    EXPECT_NO_THROW(sN.getUnchecked(-1));
    EXPECT_NO_THROW(sN.getUnchecked(Int(-1)));
    EXPECT_NO_THROW(sN.getUnchecked(3));
    EXPECT_NO_THROW(sN.getUnchecked(size_t(3)));
    EXPECT_NO_THROW(sN.getUnchecked(Int(3)));
}

TEST(TestSpan, GetWrapped) {
    std::array<int, 3> ca = {10, 42, 12};
    Span<int> sX(ca);
    Span<int, 3> sN(ca);

    EXPECT_EQ(sX.getWrapped(-6), 10);
    EXPECT_EQ(sX.getWrapped(-5), 42);
    EXPECT_EQ(sX.getWrapped(-4), 12);
    EXPECT_EQ(sX.getWrapped(-3), 10);
    EXPECT_EQ(sX.getWrapped(-2), 42);
    EXPECT_EQ(sX.getWrapped(-1), 12);
    EXPECT_EQ(sX.getWrapped(0), 10);
    EXPECT_EQ(sX.getWrapped(1), 42);
    EXPECT_EQ(sX.getWrapped(2), 12);
    EXPECT_EQ(sX.getWrapped(3), 10);
    EXPECT_EQ(sX.getWrapped(4), 42);
    EXPECT_EQ(sX.getWrapped(5), 12);
    EXPECT_EQ(sX.getWrapped(6), 10);
    EXPECT_EQ(sX.getWrapped(7), 42);
    EXPECT_EQ(sX.getWrapped(8), 12);

    EXPECT_EQ(sN.getWrapped(-6), 10);
    EXPECT_EQ(sN.getWrapped(-5), 42);
    EXPECT_EQ(sN.getWrapped(-4), 12);
    EXPECT_EQ(sN.getWrapped(-3), 10);
    EXPECT_EQ(sN.getWrapped(-2), 42);
    EXPECT_EQ(sN.getWrapped(-1), 12);
    EXPECT_EQ(sN.getWrapped(0), 10);
    EXPECT_EQ(sN.getWrapped(1), 42);
    EXPECT_EQ(sN.getWrapped(2), 12);
    EXPECT_EQ(sN.getWrapped(3), 10);
    EXPECT_EQ(sN.getWrapped(4), 42);
    EXPECT_EQ(sN.getWrapped(5), 12);
    EXPECT_EQ(sN.getWrapped(6), 10);
    EXPECT_EQ(sN.getWrapped(7), 42);
    EXPECT_EQ(sN.getWrapped(8), 12);

    sX.getWrapped(-1) = 40;
    EXPECT_EQ(ca[2], 40);
    sX.getWrapped(1) = 50;
    EXPECT_EQ(ca[1], 50);
    sX.getWrapped(3) = 60;
    EXPECT_EQ(ca[0], 60);

    sN.getWrapped(-1) = 140;
    EXPECT_EQ(ca[2], 140);
    sN.getWrapped(1) = 150;
    EXPECT_EQ(ca[1], 150);
    sN.getWrapped(3) = 160;
    EXPECT_EQ(ca[0], 160);
}

TEST(TestSpan, Data) {
    std::array<int, 3> ca = {10, 42, 12};
    Span<int> sX(ca);
    Span<int, 3> sN(ca);
    int* sXd = sX.data();
    int* sNd = sN.data();

    EXPECT_EQ(sXd[0], 10);
    EXPECT_EQ(sXd[1], 42);
    EXPECT_EQ(sXd[2], 12);
    EXPECT_EQ(sNd[0], 10);
    EXPECT_EQ(sNd[1], 42);
    EXPECT_EQ(sNd[2], 12);

    *sXd = 40;
    EXPECT_EQ(ca[0], 40);

    *sNd = 50;
    EXPECT_EQ(ca[0], 50);
}

TEST(TestSpan, Length) {
    std::array<int, 3> ca = {10, 42, 12};
    Span<int> sX = {};
    Span<int, 0> s0 = {};
    Span<int, 3> sN = ca;

    EXPECT_EQ(sX.length(), 0);
    EXPECT_EQ(sX.size(), 0);
    EXPECT_EQ(s0.length(), 0);
    EXPECT_EQ(s0.size(), 0);
    sX = Span<int>(ca);
    EXPECT_EQ(sX.length(), 3);
    EXPECT_EQ(sX.size(), 3);
    EXPECT_EQ(sN.length(), 3);
    EXPECT_EQ(sN.size(), 3);
}

TEST(TestSpan, Empty) {
    std::array<int, 3> ca = {10, 42, 12};
    Span<int> sX = {};
    Span<int, 0> s0 = {};

    EXPECT_TRUE(sX.empty());
    EXPECT_TRUE(sX.isEmpty());
    EXPECT_TRUE(s0.empty());
    EXPECT_TRUE(s0.isEmpty());
    sX = Span<int>(ca);
    EXPECT_FALSE(sX.empty());
    EXPECT_FALSE(sX.isEmpty());
}

TEST(TestSpan, Subspan) {
    std::array<int, 6> ca = {3, 4, 5, 42, 10, 42};
    Span<int> sX(ca);
    Span<int, 6> sN(ca);

    EXPECT_EQ(toString(sX.first<3>()), "[3, 4, 5]");
    EXPECT_EQ(toString(sX.last<3>()), "[42, 10, 42]");
    EXPECT_EQ(toString(sX.subspan<2, 2>()), "[5, 42]");
    EXPECT_EQ(toString(sX.subspan<2, dynamicExtent>()), "[5, 42, 10, 42]");
    EXPECT_EQ(toString(sX.subspan<2>()), "[5, 42, 10, 42]");
    EXPECT_EQ(toString(sX.first(3)), "[3, 4, 5]");
    EXPECT_EQ(toString(sX.last(3)), "[42, 10, 42]");
    EXPECT_EQ(toString(sX.subspan(2, 2)), "[5, 42]");
    EXPECT_EQ(toString(sX.subspan(2, dynamicExtent)), "[5, 42, 10, 42]");
    EXPECT_EQ(toString(sX.subspan(2)), "[5, 42, 10, 42]");

    EXPECT_EQ(toString(sN.first<3>()), "[3, 4, 5]");
    EXPECT_EQ(toString(sN.last<3>()), "[42, 10, 42]");
    EXPECT_EQ(toString(sN.subspan<2, 2>()), "[5, 42]");
    EXPECT_EQ(toString(sN.subspan<2, dynamicExtent>()), "[5, 42, 10, 42]");
    EXPECT_EQ(toString(sN.subspan<2>()), "[5, 42, 10, 42]");
    EXPECT_EQ(toString(sN.first(3)), "[3, 4, 5]");
    EXPECT_EQ(toString(sN.last(3)), "[42, 10, 42]");
    EXPECT_EQ(toString(sN.subspan(2, 2)), "[5, 42]");
    EXPECT_EQ(toString(sN.subspan(2, dynamicExtent)), "[5, 42, 10, 42]");
    EXPECT_EQ(toString(sN.subspan(2)), "[5, 42, 10, 42]");

    EXPECT_THROW(sX.first<10>(), IndexError);
    EXPECT_THROW(sX.last<10>(), IndexError);
    EXPECT_THROW((sX.subspan<2, 10>()), IndexError);
    EXPECT_THROW((sX.subspan<10, 2>()), IndexError);
    EXPECT_THROW((sX.subspan<10, dynamicExtent>()), IndexError);
    EXPECT_THROW(sX.subspan<10>(), IndexError);
    EXPECT_THROW(sX.first(10), IndexError);
    EXPECT_THROW(sX.last(10), IndexError);
    EXPECT_THROW(sX.subspan(2, 10), IndexError);
    EXPECT_THROW(sX.subspan(10, 2), IndexError);
    EXPECT_THROW(sX.subspan(10, dynamicExtent), IndexError);
    EXPECT_THROW(sX.subspan(10), IndexError);

    EXPECT_THROW(sN.first(10), IndexError);
    EXPECT_THROW(sN.last(10), IndexError);
    EXPECT_THROW(sN.subspan(2, 10), IndexError);
    EXPECT_THROW(sN.subspan(10, 2), IndexError);
    EXPECT_THROW(sN.subspan(10, dynamicExtent), IndexError);
    EXPECT_THROW(sN.subspan(10), IndexError);
}

TEST(TestSpan, Contains) {
    std::array<int, 6> ca = {3, 4, 5, 42, 10, 42};
    Span<int> sX(ca);
    Span<int, 6> sN(ca);

    EXPECT_TRUE(sX.contains(42));
    EXPECT_FALSE(sX.contains(43));

    EXPECT_TRUE(sN.contains(42));
    EXPECT_FALSE(sN.contains(43));
}

TEST(TestSpan, Find) {
    std::array<int, 6> ca = {3, 4, 5, 42, 10, 42};
    Span<int> sX(ca);
    Span<int, 6> sN(ca);

    EXPECT_EQ(sX.find(42), sX.begin() + 3);
    EXPECT_EQ(sX.find(43), sX.end());
    EXPECT_EQ(sX.find([](const int& v) { return v > 40; }), sX.begin() + 3);
    EXPECT_EQ(sX.find([](const int& v) { return v > 100; }), sX.end());

    EXPECT_EQ(sN.find(42), sN.begin() + 3);
    EXPECT_EQ(sN.find(43), sN.end());
    EXPECT_EQ(sN.find([](const int& v) { return v > 40; }), sN.begin() + 3);
    EXPECT_EQ(sN.find([](const int& v) { return v > 100; }), sN.end());
}

TEST(TestSpan, Search) {
    std::array<int, 6> ca = {3, 4, 5, 42, 10, 42};
    Span<int> sX(ca);
    Span<int, 6> sN(ca);

    EXPECT_EQ(sX.search(42), &sX[3]);
    EXPECT_EQ(sX.search(43), nullptr);
    EXPECT_EQ(sX.search([](const int& v) { return v > 40; }), &sX[3]);
    EXPECT_EQ(sX.search([](const int& v) { return v > 100; }), nullptr);

    EXPECT_EQ(sN.search(42), &sN[3]);
    EXPECT_EQ(sN.search(43), nullptr);
    EXPECT_EQ(sN.search([](const int& v) { return v > 40; }), &sN[3]);
    EXPECT_EQ(sN.search([](const int& v) { return v > 100; }), nullptr);
}

TEST(TestSpan, Index) {
    std::array<int, 6> ca = {3, 4, 5, 42, 10, 42};
    Span<int> sX(ca);
    Span<int, 6> sN(ca);

    EXPECT_EQ(sX.index(42), 3);
    EXPECT_EQ(sX.index(43), -1);
    EXPECT_EQ(sX.index([](const int& v) { return v > 40; }), 3);
    EXPECT_EQ(sX.index([](const int& v) { return v > 100; }), -1);

    EXPECT_EQ(sN.index(42), 3);
    EXPECT_EQ(sN.index(43), -1);
    EXPECT_EQ(sN.index([](const int& v) { return v > 40; }), 3);
    EXPECT_EQ(sN.index([](const int& v) { return v > 100; }), -1);
}

TEST(TestSpan, ToString) {
    std::array<int, 6> ca = {3, 4, 5, 42, 10, 42};
    Span<int> sX(ca);
    Span<int, 6> sN(ca);
    EXPECT_EQ(toString(sX), "[3, 4, 5, 42, 10, 42]");
    EXPECT_EQ(toString(sN), "[3, 4, 5, 42, 10, 42]");
    sX = Span<int>();
    Span<int, 0> s0 = {};
    EXPECT_EQ(toString(sX), "[]");
    EXPECT_EQ(toString(s0), "[]");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
