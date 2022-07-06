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

#include <iterator>

#include <gtest/gtest.h>
#include <vgc/core/array.h>

using vgc::Int;
using vgc::core::Array;
using vgc::core::IndexError;
using vgc::core::LengthError;
using vgc::core::NegativeIntegerError;
using vgc::core::NoInit;

#define EXPECT_LENGTH(a, n) { EXPECT_EQ((a).size(), (n)); EXPECT_EQ((a).length(), (n)); }

// To properly test containers we have to check that:
// - its elements are not being over-destroyed or over-constructed.
// - its size always equals the count of alive elements (from the outside).
// TestTag is here to prevent sharing the static Statistics instance between
// test groups which can run concurrently.
//
template <typename TestTag>
class TestObject
{
    static constexpr int magic = 0x67BF402E;

public:
    struct Statistics {
        size_t overConstructCount = 0;
        size_t overDestroyCount = 0;
        int64_t aliveCount = 0;
    };

    TestObject() : i_(0) {
        init_();
    }

    TestObject(int i) : i_(i) {
        init_();
    }

    TestObject(const TestObject& other) : i_(other.i_) {
        init_();
    }

    TestObject(TestObject&& other) : i_(other.i_) {
        init_();
    }

    TestObject& operator=(const TestObject& other) {
        i_ = other.i_;
        return *this;
    }

    TestObject& operator=(TestObject&& other) {
        i_ = other.i_;
        return *this;
    }

    ~TestObject() {
        if (cookie_ != magic) {
            ++stats_().overDestroyCount;
        }
        --stats_().aliveCount;
        cookie_ = 0;
        i_ = -1;
    }

    static int64_t aliveCount() {
        return stats_().aliveCount;
    }

    static void doPostTestChecks(int64_t expectedAliveCount = 0) {
        if (stats_().overConstructCount > 0) {
            throw std::runtime_error("Some array elements have been over-constructed");
        }
        if (stats_().overDestroyCount > 0) {
            throw std::runtime_error("Some array elements have been over-destroyed");
        }
        if (stats_().aliveCount != expectedAliveCount) {
            throw std::runtime_error("Unexpected count of alive elements.");
        }
    }

    operator int() const { return i_; }

    friend std::istream& operator>>(std::istream& stream, TestObject& o) {
        return stream >> o.i_;
    }

private:
    void init_() {
        if (cookie_ == magic) {
            ++stats_().overConstructCount;
        }
        ++stats_().aliveCount;
        cookie_ = magic;
    }

    static Statistics& stats_() {
        static Statistics s = {};
        return s;
    }

    int cookie_ = 0;
    int i_;
};

TEST(TestArray, Construct) {
    // Note: it's important to test the zero-init after the non-zero init, to
    // decrease the chance that the memory is zero "by chance".
    std::vector<int> v{1, 2, 3};

    { Array<int> a;                       EXPECT_LENGTH(a, 0); }
    { Array<int> a{};                     EXPECT_LENGTH(a, 0); }

    { Array<int> a(10, NoInit{});         EXPECT_LENGTH(a, 10);                                           }
    { Array<int> a(10, 42);               EXPECT_LENGTH(a, 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(10);                   EXPECT_LENGTH(a, 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  }

    { Array<int> a(Int(10), NoInit{});    EXPECT_LENGTH(a, 10);                                           }
    { Array<int> a(Int(10), 42);          EXPECT_LENGTH(a, 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(Int(10));              EXPECT_LENGTH(a, 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  }

    { Array<int> a(size_t(10), NoInit{}); EXPECT_LENGTH(a, 10); }
    { Array<int> a(size_t(10), 42);       EXPECT_LENGTH(a, 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(size_t(10));           EXPECT_LENGTH(a, 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  }

    { Array<int> a{10, NoInit{}};         EXPECT_LENGTH(a, 10); }
    { Array<int> a{10, 42};               EXPECT_LENGTH(a, 2);  EXPECT_EQ(a[0], 10); EXPECT_EQ(a[1], 42); }
    { Array<int> a{10};                   EXPECT_LENGTH(a, 1);  EXPECT_EQ(a[0], 10); }

    { Array<int> a(v);                    EXPECT_LENGTH(a, 3);  EXPECT_EQ(a[0], 1);  EXPECT_EQ(a[2], 3); }

    EXPECT_THROW(Array<int>(-1),           NegativeIntegerError);
    EXPECT_THROW(Array<int>(-1, NoInit{}), NegativeIntegerError);
    EXPECT_THROW(Array<int>(-1, 42),       NegativeIntegerError);
    EXPECT_THROW(Array<int>(vgc::core::tmax<size_t>, 42), LengthError);
    struct Tag {}; using TestObj = TestObject<Tag>;
    {
        // Tests init_
        Array<TestObj> a(10);       EXPECT_EQ(a.length(), TestObj::aliveCount());
    }
    EXPECT_NO_THROW(TestObj::doPostTestChecks());
}

TEST(TestArray, CopyAndMove) {
    Array<int> b = {10, 42, 3, 4};
    { Array<int> a(b);                        EXPECT_EQ(a.length(), 4); EXPECT_EQ(a[0], 10); EXPECT_EQ(a[3], 4);  }
    { Array<int> a(b.begin(), b.begin() + 2); EXPECT_EQ(a.length(), 2); EXPECT_EQ(a[0], 10); EXPECT_EQ(a[1], 42); }
    { Array<int> a(std::move(b));             EXPECT_EQ(a.length(), 4); EXPECT_EQ(a[0], 10); EXPECT_EQ(a[3], 4);  }
    EXPECT_EQ(b.length(), 0);
}

TEST(TestArray, CopyAssignAndMoveAssign) {
    Array<int> b = {10, 42, 3, 4};
    { Array<int> a = {};           EXPECT_EQ(a.length(), 0); }
    { Array<int> a = Array<int>(); EXPECT_EQ(a.length(), 0); }
    { Array<int> a = {10, 42};     EXPECT_EQ(a.length(), 2); EXPECT_EQ(a[0], 10); EXPECT_EQ(a[1], 42); }
    { Array<int> a = b;            EXPECT_EQ(a.length(), 4); EXPECT_EQ(a[0], 10); EXPECT_EQ(a[3], 4);  }
    { Array<int> a = std::move(b); EXPECT_EQ(a.length(), 4); EXPECT_EQ(a[0], 10); EXPECT_EQ(a[3], 4);  }
    EXPECT_EQ(b.length(), 0);

    Array<int> c = {10, 42, 3, 4};
    Array<int> a;
    a = {};           EXPECT_EQ(a.length(), 0);
    a = Array<int>(); EXPECT_EQ(a.length(), 0);
    a = {11, 42};     EXPECT_EQ(a.length(), 2); EXPECT_EQ(a[0], 11); EXPECT_EQ(a[1], 42);
    a = c;            EXPECT_EQ(a.length(), 4); EXPECT_EQ(a[0], 10); EXPECT_EQ(a[3], 4);
    c = {20, 52, 4, 5};
    a = std::move(c); EXPECT_EQ(a.length(), 4); EXPECT_EQ(a[0], 20); EXPECT_EQ(a[3], 5);
    EXPECT_EQ(c.length(), 0);
}

TEST(TestArray, Assign) {
    {
        Array<int> a;
        Array<int> b = {10, 42, 3, 4};
        std::vector<int> v = {10, 42, 3, 4};
        a.assign(10, 1);                    EXPECT_EQ(a.length(), 10); EXPECT_EQ(a[0], 1); EXPECT_EQ(a[9], 1);
        a.assign(Int(11), 2);               EXPECT_EQ(a.length(), 11); EXPECT_EQ(a[0], 2); EXPECT_EQ(a[10], 2);
        a.assign(size_t(12), 3);            EXPECT_EQ(a.length(), 12); EXPECT_EQ(a[0], 3); EXPECT_EQ(a[11], 3);
        a.assign(b.begin(), b.begin() + 2); EXPECT_EQ(a.length(), 2);  EXPECT_EQ(a[0], 10); EXPECT_EQ(a[1], 42);
        a.assign(v);                        EXPECT_EQ(a.length(), 4);  EXPECT_EQ(a[0], 10); EXPECT_EQ(a[3], 4);
        a.assign({11, 43});                 EXPECT_EQ(a.length(), 2);  EXPECT_EQ(a[0], 11); EXPECT_EQ(a[1], 43);
        EXPECT_THROW(a.assign(-1, 42),      NegativeIntegerError);
        EXPECT_THROW(a.assign(Int(-1), 42), NegativeIntegerError);
    }
    struct Tag {}; using TestObj = TestObject<Tag>;
    {
        // Tests assignFill_
        Array<TestObj> a(10);
        a.assign(3, 1);                 EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 10);
        a.assign(8, 2);                 EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 10);
        a.resize(16, 3);                EXPECT_EQ(a.length(), TestObj::aliveCount());
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
    {
        // Tests assignRange_ with forward iterator
        Array<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        Array<TestObj> a(6);
        a.assign(v.begin(), v.begin() + 3); EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 6);
        a.assign(v.begin(), v.begin() + 5); EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 6);
        a.assign(v.begin(), v.begin() + 9); EXPECT_EQ(a.length(), TestObj::aliveCount());
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
    {
        // Tests assignRange_ with input-only iterator
        std::istringstream sst4("1 2 3 4");
        std::istringstream sst6("1 2 3 4 5 6");
        using ISIt = std::istream_iterator<TestObj>;
        Array<TestObj> a(10);
        a.assign(ISIt{sst4}, ISIt{});       EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a[2], 3);
        a.assign(ISIt{sst6}, ISIt{});       EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a[4], 5);
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
}

TEST(TestArray, GetChecked) {
    Array<int> a = {10, 20, 30};
    const Array<int>& b = a;
    EXPECT_EQ(a[size_t(0)], 10);
    EXPECT_EQ(a[Int(1)], 20);
    EXPECT_EQ(b[size_t(0)], 10);
    EXPECT_EQ(b[Int(1)], 20);
    a[size_t(2)] = 40; EXPECT_EQ(a[2], 40);
    a[Int(2)]    = 50; EXPECT_EQ(a[2], 50);
    EXPECT_THROW(a[-1],              IndexError);
    EXPECT_THROW(a[Int(-1)],         IndexError);
    EXPECT_THROW(a[-1]         = 10, IndexError);
    EXPECT_THROW(a[Int(-1)]    = 10, IndexError);
    EXPECT_THROW(a[4],               IndexError);
    EXPECT_THROW(a[size_t(4)],       IndexError);
    EXPECT_THROW(a[Int(4)],          IndexError);
    EXPECT_THROW(a[4]          = 10, IndexError);
    EXPECT_THROW(a[size_t(4)]  = 10, IndexError);
    EXPECT_THROW(a[Int(4)]     = 10, IndexError);
}

TEST(TestArray, GetUnchecked) {
    Array<int> a = {10, 20, 30};
    const Array<int>& b = a;
    EXPECT_EQ(a.getUnchecked(size_t(0)), 10);
    EXPECT_EQ(a.getUnchecked(Int(1)), 20);
    EXPECT_EQ(b.getUnchecked(size_t(0)), 10);
    EXPECT_EQ(b.getUnchecked(Int(1)), 20);
    a.getUnchecked(size_t(2)) = 40; EXPECT_EQ(a[2], 40);
    a.getUnchecked(Int(2))    = 50; EXPECT_EQ(a[2], 50);
}

TEST(TestArray, GetWrapped) {
    Array<int> a = {10, 20, 30};
    const Array<int>& b = a;
    EXPECT_EQ(a.getWrapped(-6), 10);
    EXPECT_EQ(a.getWrapped(-5), 20);
    EXPECT_EQ(a.getWrapped(-4), 30);
    EXPECT_EQ(a.getWrapped(-3), 10);
    EXPECT_EQ(a.getWrapped(-2), 20);
    EXPECT_EQ(a.getWrapped(-1), 30);
    EXPECT_EQ(a.getWrapped(0), 10);
    EXPECT_EQ(a.getWrapped(1), 20);
    EXPECT_EQ(a.getWrapped(2), 30);
    EXPECT_EQ(a.getWrapped(3), 10);
    EXPECT_EQ(a.getWrapped(4), 20);
    EXPECT_EQ(a.getWrapped(5), 30);
    EXPECT_EQ(a.getWrapped(6), 10);
    EXPECT_EQ(a.getWrapped(7), 20);
    EXPECT_EQ(a.getWrapped(8), 30);
    EXPECT_EQ(b.getWrapped(-6), 10);
    EXPECT_EQ(b.getWrapped(-1), 30);
    EXPECT_EQ(b.getWrapped(0), 10);
    EXPECT_EQ(b.getWrapped(7), 20);
    a.getWrapped(-1) = 40; EXPECT_EQ(a[2], 40);
    a.getWrapped(1)  = 50; EXPECT_EQ(a[1], 50);
    a.getWrapped(3)  = 60; EXPECT_EQ(a[0], 60);
}

TEST(TestArray, GetFirstLast) {
    Array<int> a = {10, 20, 30};
    const Array<int>& b = a;
    EXPECT_EQ(a.first(), 10);
    EXPECT_EQ(b.first(), 10);
    a.first() = 50; EXPECT_EQ(a[0], 50);
    EXPECT_EQ(a.last(), 30);
    EXPECT_EQ(b.last(), 30);
    a.last() = 70; EXPECT_EQ(a[2], 70);
    a = {};
    EXPECT_THROW(a.first(),      IndexError);
    EXPECT_THROW(b.first(),      IndexError);
    EXPECT_THROW(a.first() = 10, IndexError);
    EXPECT_THROW(a.last(),       IndexError);
    EXPECT_THROW(b.last(),       IndexError);
    EXPECT_THROW(a.last() = 10,  IndexError);
}

TEST(TestArray, Data) {
    Array<int> a = {10, 20, 30};
    const Array<int>& b = a;
    int* ad = a.data();
    const int* bd = b.data();
    EXPECT_EQ(*ad, 10);
    EXPECT_EQ(*(ad+1), 20);
    EXPECT_EQ(*(ad+2), 30);
    EXPECT_EQ(ad[0], 10);
    EXPECT_EQ(ad[1], 20);
    EXPECT_EQ(ad[2], 30);
    EXPECT_EQ(*bd, 10);
    EXPECT_EQ(*(bd+1), 20);
    EXPECT_EQ(*(bd+2), 30);
    EXPECT_EQ(bd[0], 10);
    EXPECT_EQ(bd[1], 20);
    EXPECT_EQ(bd[2], 30);
    *ad = 40;     EXPECT_EQ(a[0], 40);
    *(ad+1) = 50; EXPECT_EQ(b[1], 50);
}

TEST(TestArray, Iterators) {
    Array<int> a = {10, 20, 30};
    const Array<int> b = {10, 20, 30};
    Array<int> c;

    // begin(), end() as const
    for (int x : b) { c.append(x); }
    EXPECT_EQ(c[0], 10); EXPECT_EQ(c[1], 20); EXPECT_EQ(c[2], 30);
    c.clear();

    // begin(), end() as non-const
    for (int& x : a) { x += 100; }
    EXPECT_EQ(a[0], 110); EXPECT_EQ(a[1], 120); EXPECT_EQ(a[2], 130);

    // cbegin(), cend()
    for (auto it = b.cbegin(); it < b.cend(); ++it) { c.append(*it); }
    EXPECT_EQ(c[0], 10); EXPECT_EQ(c[1], 20); EXPECT_EQ(c[2], 30);
    c.clear();

    // rbegin(), rend() as const
    c.assign(b.rbegin(), b.rend());
    EXPECT_EQ(c[0], 30); EXPECT_EQ(c[1], 20); EXPECT_EQ(c[2], 10);
    c.clear();

    // rbegin(), rend() as non-const
    for (auto it = a.rbegin(); it < a.rend(); ++it) { *it += 100; c.append(*it); }
    EXPECT_EQ(a[0], 210); EXPECT_EQ(a[1], 220); EXPECT_EQ(a[2], 230);
    EXPECT_EQ(c[0], 230); EXPECT_EQ(c[1], 220); EXPECT_EQ(c[2], 210);
    c.clear();

    // crbegin(), crend()
    for (auto it = b.crbegin(); it < b.crend(); ++it) { c.append(*it); }
    EXPECT_EQ(c[0], 30); EXPECT_EQ(c[1], 20); EXPECT_EQ(c[2], 10);
    c.clear();
}

TEST(TestArray, Empty) {
    Array<int> a = {};
    EXPECT_TRUE(a.empty());
    EXPECT_TRUE(a.isEmpty());
    a.append(42);
    EXPECT_FALSE(a.empty());
    EXPECT_FALSE(a.isEmpty());
}

TEST(TestArray, Length) {
    Array<int> a = {};
    EXPECT_EQ(a.length(), 0);
    EXPECT_EQ(a.size(), 0);
    EXPECT_EQ(a.reservedLength(), 0);
    a.append(42);
    EXPECT_EQ(a.length(), 1);
    EXPECT_EQ(a.size(), 1);
    EXPECT_GE(a.reservedLength(), 1);
    EXPECT_GE(a.max_size(), 1);
    EXPECT_GE(a.maxLength(), 1);
}

TEST(TestArray, Reserve) {
    Array<int> a = {42};
    a.reserve(20);
    EXPECT_EQ(a.length(), 1);
    EXPECT_EQ(a.reservedLength(), 20);

    // Check no reallocation if reserved length is enough
    int* data = a.data();
    a.resize(20);
    EXPECT_EQ(a.length(), 20);
    EXPECT_EQ(a.reservedLength(), 20);
    EXPECT_EQ(data, a.data());

    // Check reallocation if reserved length isn't enough
    // + check that the reserved length increased more than just by one!
    a.append(0);
    EXPECT_EQ(a.length(), 21);
    EXPECT_GT(a.reservedLength(), 21);
    EXPECT_NE(data, a.data());

    EXPECT_THROW(a.reserve(-1), NegativeIntegerError);
}

TEST(TestArray, Clear) {
    Array<int> a = {10, 42, 12};
    EXPECT_FALSE(a.isEmpty());
    a.clear();
    EXPECT_TRUE(a.isEmpty());
}

TEST(TestArray, InsertAtIterator) {
    Array<Array<int>> a;
    Array<int> b = {42};
    Array<int> c = {43};
    Array<int> e = {44};
    Array<int> f = {45};
    std::vector<Array<int>> v = {{10, 20}, {30, 40}, {50, 60}};
    a.insert(a.begin(), b); EXPECT_EQ(a.length(), 1); EXPECT_EQ(a[0][0], 42);
    a.insert(a.begin(), std::move(c)); EXPECT_EQ(a.length(), 2); EXPECT_EQ(a[0][0], 43); EXPECT_EQ(c.length(), 0);
    a.insert(a.begin(), size_t(2), b); EXPECT_EQ(a.length(), 4);
    a.insert(a.begin(), Int(2), b); EXPECT_EQ(a.length(), 6);
    EXPECT_THROW(a.insert(a.begin(), -1, b), NegativeIntegerError);
    Array<Array<int>> d = a;
    a.insert(a.begin(), d.begin(), d.end()); EXPECT_EQ(a.length(), 12);
    a.insert(a.begin(), {{1, 2}, {3, 4}, {5, 6}}); EXPECT_EQ(a.length(), 15);
    a.insert(a.begin() + 2, v); EXPECT_EQ(a.length(), 18); EXPECT_EQ(a[3], (Array<int>{30, 40}));
}

TEST(TestArray, InsertAtIndex) {
    {
        Array<int> a = {10, 42, 12};
        Array<int> b = {10, 42, 15, 12};
        Array<int> c = {4, 10, 42, 15, 12};
        Array<int> d = {4, 10, 42, 15, 12, 13};
        a.insert(2, 15); EXPECT_EQ(a, b);
        a.insert(0, 4); EXPECT_EQ(a, c);
        a.insert(5, 13); EXPECT_EQ(a, d);
        EXPECT_THROW(a.insert(-1, 10), IndexError);
        EXPECT_THROW(a.insert(7, 10), IndexError);

        Array<Array<int>> e = {{1, 2}, {3, 4}};
        Array<int> f = {5, 6};
        Array<int> g = {5, 6};
        e.insert(1, f);            EXPECT_EQ(e.length(), 3); EXPECT_EQ(f.length(), 2);
        e.insert(1, std::move(f)); EXPECT_EQ(e.length(), 4); EXPECT_EQ(f.length(), 0);
        EXPECT_THROW(e.insert(-1, std::move(g)), IndexError);

        Array<int> h = {10, 42, 12};
        Array<int> i = {10, 42, 15, 15, 15, 12};
        h.insert(2, 3, 15); EXPECT_EQ(h, i);
        EXPECT_THROW(h.insert(-1, 3, 15), IndexError);
        EXPECT_THROW(h.insert(2, -1, 15), NegativeIntegerError);

        Array<int> j = {10, 42, 15, 10, 42, 15, 15, 12};
        h.insert(3, i.begin(), i.begin() + 2); EXPECT_EQ(h, j);
        EXPECT_THROW(h.insert(-1, i.begin(), i.begin() + 2), IndexError);

        Array<int> k = {10, 1, 2, 42, 15, 10, 42, 15, 15, 12};
        h.insert(1, {1, 2}); EXPECT_EQ(h, k);
        EXPECT_THROW(h.insert(-1, {1, 2}), IndexError);

        Array<int> l = {10, 1, 100, 200, 2, 42, 15, 10, 42, 15, 15, 12};
        h.insert(2, std::vector<int>{100, 200}); EXPECT_EQ(h, l);
        EXPECT_THROW(h.insert(-1, std::vector<int>{100, 200}), IndexError);
    }
    struct Tag {}; using TestObj = TestObject<Tag>;
    {
        Array<TestObj> a(10);
        a.resize(8);
        // Tests emplaceReserved_
        a.append(17);                   EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 10); 
        a.append(11);                   EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 10); 
        // Tests emplaceReallocate_ with i != length()
        a.insert(4, 41);                EXPECT_EQ(a.length(), TestObj::aliveCount());
        EXPECT_EQ(a[4], 41); EXPECT_EQ(a[9], 17); EXPECT_EQ(a[10], 11);
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
    {
        Array<int> s1 = {1, 1, 1};
        Array<int> s2 = {2, 2};
        Array<int> s3 = {0, 0, 0, 0};
        Array<TestObj> r0 =  {7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
        Array<TestObj> r1 =  {7, 7, 7, 7};
        Array<TestObj> r2 =  {7, 7, 1, 1, 1, 7, 7};
        Array<TestObj> r3 =  {7, 7, 1, 2, 2, 1, 1, 7, 7};
        Array<TestObj> r4 =  {7, 7, 1, 2, 2, 0, 0, 0, 0, 1, 1, 7, 7};
        Array<TestObj> r2b = {7, 7, 1, 2, 3, 4, 5, 7, 7};
        const auto preCnt = TestObj::aliveCount();
        {
            // Tests prepareInsertRange_ via insertFill_
            Array<TestObj> a(10, 7);
            a.resize(4);                        EXPECT_EQ(a, r1);
            a.insert(2, 3, 1);                  EXPECT_EQ(a, r2); EXPECT_EQ(a.length(), TestObj::aliveCount() - preCnt); // shift without overlap
            a.insert(3, 2, 2);                  EXPECT_EQ(a, r3); EXPECT_EQ(a.length(), TestObj::aliveCount() - preCnt); // shift with overlap
            a.insert(5, 4, 0);                  EXPECT_EQ(a, r4); EXPECT_EQ(a.length(), TestObj::aliveCount() - preCnt); // reallocation
        }   EXPECT_NO_THROW(TestObj::doPostTestChecks(preCnt));
        {
            // Tests insertRange_ with forward iterator
            Array<TestObj> a(10, 7);
            a.resize(4);                        EXPECT_EQ(a, r1);
            a.insert(2, s1.begin(), s1.end());  EXPECT_EQ(a, r2); EXPECT_EQ(a.length(), TestObj::aliveCount() - preCnt); // shift without overlap
            a.insert(3, s2.begin(), s2.end());  EXPECT_EQ(a, r3); EXPECT_EQ(a.length(), TestObj::aliveCount() - preCnt); // shift with overlap
            a.insert(5, s3.begin(), s3.end());  EXPECT_EQ(a, r4); EXPECT_EQ(a.length(), TestObj::aliveCount() - preCnt); // reallocation
        }   EXPECT_NO_THROW(TestObj::doPostTestChecks(preCnt));
        {
            // Tests insertRange_ with input-only iterator
            std::istringstream sst4("1 2 3 4 5");
            using ISIt = std::istream_iterator<TestObj>;
            Array<TestObj> a(6, 7);
            a.resize(4);                        EXPECT_EQ(a, r1);   EXPECT_EQ(a.length(), TestObj::aliveCount() - preCnt);
            a.insert(2, ISIt{sst4}, ISIt{});    EXPECT_EQ(a, r2b);  EXPECT_EQ(a.length(), TestObj::aliveCount() - preCnt);
        }   EXPECT_NO_THROW(TestObj::doPostTestChecks(preCnt));
    }

    // Test overload resolution between insert(Int, const Range&) and insert(Int, const T&)
    {
        // A range class that is assignable from an std::vector<int>
        class IntVector {
            using iterator = std::vector<int>::iterator;
            std::vector<int> v;

        public:
            IntVector() : v() {}
            IntVector(std::initializer_list<int> ilist) : v(ilist.begin(), ilist.end()) {}
            IntVector(const std::vector<int>& vec) : v(vec) {}
            iterator begin() { return v.begin(); }
            iterator end() { return v.end(); }

            bool operator==(const IntVector& other) const { return v == other.v; }
        };

        Array<IntVector> a = {{1, 2}, {3, 4}};

        Array<std::vector<int>> b = {{5, 6}, {7, 8}};
        a.insert(a.length(), b);
        EXPECT_EQ(a, (Array<IntVector>{{1, 2}, {3, 4}, {5, 6}, {7, 8}}));

        IntVector c = {9, 10};
        a.insert(a.length(), c);
        EXPECT_EQ(a, (Array<IntVector>{{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}}));

        std::vector<int> d = {11, 12};
        a.insert(a.length(), d);
        EXPECT_EQ(a, (Array<IntVector>{{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}, {11, 12}}));

        // Note: the following:
        //
        //  Array<Array<short>> e = {{13, 14}, {15, 16}};
        //  a.insert(a.length(), e);
        //  EXPECT_EQ(a, (Array<Array<int>>{{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}, {11, 12}, {13, 14}, {15, 16}}));
        //
        // wouldn't compile because:
        // - insert(Int, const T&) isn't selected because:
        //     Array<Array<short>> isn't implicitly convertible to T (= Array<int>)
        // - insert(Int, const Range&) isn't selected because:
        //     T (= Array<int>) isn't assignable from Range::value_type (= Array<short>)
    }
}

class Foo {
    int x_, y_;
public:
    Foo(int x, int y) : x_(x), y_(y) {}
    int x() { return x_; }
    int y() { return y_; }
};

TEST(TestArray, Emplace) {
    Array<Foo> a;
    a.emplace(a.begin(), 12, 42);
    EXPECT_EQ(a[0].x(), 12); EXPECT_EQ(a[0].y(), 42);
    a.emplace(0, 13, 43);
    EXPECT_EQ(a[0].x(), 13); EXPECT_EQ(a[0].y(), 43);
    EXPECT_THROW(a.emplace(-1, 13, 43), IndexError);
}

TEST(TestArray, EraseAtIterator) {
    {
        Array<int> a = {10, 42, 12};
        Array<int> b = {10, 12};
        Array<int> c = {10};
        auto it = a.erase(a.begin()+1); EXPECT_EQ(a, b); EXPECT_EQ(*it, 12);
        auto it2 = a.erase(a.begin()+1); EXPECT_EQ(a, c); EXPECT_EQ(it2, a.end());
    }
    struct Tag {}; using TestObj = TestObject<Tag>;
    {
        Array<TestObj> a(10);
        a.erase(a.begin() + 3); EXPECT_EQ(a.length(), TestObj::aliveCount());
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
}

TEST(TestArray, EraseRangeIterator) {
    {
        Array<int> a = {10, 42, 12};
        Array<int> b = {12};
        Array<int> c = {10};
        auto it = a.erase(a.begin(), a.begin()+2); EXPECT_EQ(a, b); EXPECT_EQ(*it, 12);
        auto it2 = a.erase(a.begin(), a.begin()); EXPECT_EQ(a, b); EXPECT_EQ(it2, a.begin());
        auto it3 = a.erase(a.end(), a.end()); EXPECT_EQ(a, b); EXPECT_EQ(it3, a.end());
        auto it4 = a.erase(a.begin(), a.end()); EXPECT_TRUE(a.isEmpty()); EXPECT_EQ(it4, a.end());
    }
    struct Tag {}; using TestObj = TestObject<Tag>;
    {
        Array<TestObj> a(10);
        a.erase(a.begin() + 3, a.begin() + 5); EXPECT_EQ(a.length(), TestObj::aliveCount());
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
}

TEST(TestArray, RemoveAt) {
    Array<int> a = {8, 10, 42, 12, 15};
    Array<int> b = {8, 42, 12, 15};
    Array<int> c = {42, 12, 15};
    Array<int> d = {42, 12};
    a.removeAt(1); EXPECT_EQ(a, b);
    a.removeAt(0); EXPECT_EQ(a, c);
    a.removeAt(a.length()-1); EXPECT_EQ(a, d);
    EXPECT_THROW(a.removeAt(-1), IndexError);
    EXPECT_THROW(a.removeAt(a.length()), IndexError);
}

TEST(TestArray, RemoveOne) {
    Array<int> a = {8, 10, 12, 42, 12, 15};
    Array<int> b = {8, 10, 42, 12, 15};
    Array<int> c = {8, 42, 12, 15};
    a.removeOne(12); EXPECT_EQ(a, b);
    a.removeOne(10); EXPECT_EQ(a, c);
}

TEST(TestArray, RemoveAll) {
    Array<int> a = {8, 10, 12, 42, 12, 15};
    Array<int> b = {8, 10, 42, 15};
    a.removeAll(12); EXPECT_EQ(a, b);
}

TEST(TestArray, RemoveIf) {
    Array<int> a = {8, 10, 42, 12, 7, 15};
    Array<int> b = {8, 42, 7};
    a.removeIf([](int a){ return a >= 10 && a < 20; }); EXPECT_EQ(a, b);
}

TEST(TestArray, RemoveRange) {
    Array<int> a = {8, 10, 42, 12, 15};
    Array<int> b = {8, 12, 15};
    Array<int> c = {8, 12};
    a.removeRange(1, 3); EXPECT_EQ(a, b);
    a.removeRange(2, 3); EXPECT_EQ(a, c);
    EXPECT_THROW(a.removeRange(1, 0), IndexError);
    EXPECT_THROW(a.removeRange(-1, 0), IndexError);
    EXPECT_THROW(a.removeRange(2, 3), IndexError);
    EXPECT_THROW(a.removeRange(-1, 0), IndexError);
    EXPECT_THROW(a.removeRange(2, 3), IndexError);
}

TEST(TestArray, RemoveFirstAndLast) {
    Array<int> a = {15, 10, 42, 12};
    Array<int> b = {10, 42, 12};
    Array<int> c = {10, 42};
    a.removeFirst(); EXPECT_EQ(a, b);
    a.removeLast(); EXPECT_EQ(a, c);
    a.clear();
    EXPECT_THROW(a.removeFirst(), IndexError);
    EXPECT_THROW(a.removeLast(), IndexError);
}

TEST(TestArray, RemoveFirst) {
    Array<int> a = {8, 10, 42, 12, 15};
    Array<int> b = {10, 42, 12, 15};
    Array<int> c = {12, 15};
    a.removeFirst(1); EXPECT_EQ(a, b);
    a.removeFirst(2); EXPECT_EQ(a, c);
    EXPECT_THROW(a.removeFirst(100), IndexError);
    EXPECT_THROW(a.removeFirst(-1), IndexError);
}

TEST(TestArray, RemoveLast) {
    Array<int> a = {8, 10, 42, 12, 15};
    Array<int> b = {8, 10, 42, 12};
    Array<int> c = {8, 10};
    a.removeLast(1); EXPECT_EQ(a, b);
    a.removeLast(2); EXPECT_EQ(a, c);
    EXPECT_THROW(a.removeLast(100), IndexError);
    EXPECT_THROW(a.removeLast(-1), IndexError);
}

TEST(TestArray, AppendAndPrepend) {
    Array<int> a;
    Array<int> b = {10, 42, 12};
    a.append(10);
    a.emplaceLast(42);
    a.append(12);
    EXPECT_EQ(a, b);
    a.clear();
    a.prepend(12);
    a.prepend(42);
    a.prepend(10);
    EXPECT_EQ(a, b);

    Array<Array<int>> c = {{1, 2}, {3, 4}};
    Array<int> d = {5, 6};
    Array<int> e = {7, 8};
    Array<Array<int>> f = {{7, 8}, {1, 2}, {3, 4}, {5, 6}, {7, 7}};
    c.append(std::move(d));
    c.prepend(std::move(e));
    c.emplaceLast(2, 7);
    EXPECT_EQ(c, f);
    EXPECT_EQ(d.length(), 0);
    EXPECT_EQ(e.length(), 0);
}

TEST(TestArray, ExtendAndPreextend) {
    Array<int> empty = {};
    std::vector<int> v = {5, 6, 7};
    Array<int> b = {5, 6, 7};

    { Array<int> a; a.extend(empty);                    EXPECT_EQ(a, (Array<int>{})); }
    { Array<int> a; a.extend(v.begin(), v.begin());     EXPECT_EQ(a, (Array<int>{})); }
    { Array<int> a; a.extend(b.end(), b.end());         EXPECT_EQ(a, (Array<int>{})); }
    { Array<int> a; a.extend(2, 1);                     EXPECT_EQ(a, (Array<int>{1, 1})); }
    { Array<int> a; a.extend(v.begin(), v.begin() + 2); EXPECT_EQ(a, (Array<int>{5, 6})); }
    { Array<int> a; a.extend(b.begin(), b.begin() + 2); EXPECT_EQ(a, (Array<int>{5, 6})); }
    { Array<int> a; a.extend(v);                        EXPECT_EQ(a, (Array<int>{5, 6, 7})); }
    { Array<int> a; a.extend(b);                        EXPECT_EQ(a, (Array<int>{5, 6, 7})); }
    { Array<int> a; a.extend({5, 6, 7});                EXPECT_EQ(a, (Array<int>{5, 6, 7})); }

    { Array<int> a; a.preextend(empty);                    EXPECT_EQ(a, (Array<int>{})); }
    { Array<int> a; a.preextend(v.begin(), v.begin());     EXPECT_EQ(a, (Array<int>{})); }
    { Array<int> a; a.preextend(b.end(), b.end());         EXPECT_EQ(a, (Array<int>{})); }
    { Array<int> a; a.preextend(2, 1);                     EXPECT_EQ(a, (Array<int>{1, 1})); }
    { Array<int> a; a.preextend(v.begin(), v.begin() + 2); EXPECT_EQ(a, (Array<int>{5, 6})); }
    { Array<int> a; a.preextend(b.begin(), b.begin() + 2); EXPECT_EQ(a, (Array<int>{5, 6})); }
    { Array<int> a; a.preextend(v);                        EXPECT_EQ(a, (Array<int>{5, 6, 7})); }
    { Array<int> a; a.preextend(b);                        EXPECT_EQ(a, (Array<int>{5, 6, 7})); }
    { Array<int> a; a.preextend({5, 6, 7});                EXPECT_EQ(a, (Array<int>{5, 6, 7})); }

    { Array<int> a{1, 2}; a.extend(empty);                    EXPECT_EQ(a, (Array<int>{1, 2})); }
    { Array<int> a{1, 2}; a.extend(v.begin(), v.begin());     EXPECT_EQ(a, (Array<int>{1, 2})); }
    { Array<int> a{1, 2}; a.extend(b.end(), b.end());         EXPECT_EQ(a, (Array<int>{1, 2})); }
    { Array<int> a{1, 2}; a.extend(2, 1);                     EXPECT_EQ(a, (Array<int>{1, 2, 1, 1})); }
    { Array<int> a{1, 2}; a.extend(v.begin(), v.begin() + 2); EXPECT_EQ(a, (Array<int>{1, 2, 5, 6})); }
    { Array<int> a{1, 2}; a.extend(b.begin(), b.begin() + 2); EXPECT_EQ(a, (Array<int>{1, 2, 5, 6})); }
    { Array<int> a{1, 2}; a.extend(v);                        EXPECT_EQ(a, (Array<int>{1, 2, 5, 6, 7})); }
    { Array<int> a{1, 2}; a.extend(b);                        EXPECT_EQ(a, (Array<int>{1, 2, 5, 6, 7})); }
    { Array<int> a{1, 2}; a.extend({5, 6, 7});                EXPECT_EQ(a, (Array<int>{1, 2, 5, 6, 7})); }

    { Array<int> a{1, 2}; a.preextend(empty);                    EXPECT_EQ(a, (Array<int>{1, 2})); }
    { Array<int> a{1, 2}; a.preextend(v.begin(), v.begin());     EXPECT_EQ(a, (Array<int>{1, 2})); }
    { Array<int> a{1, 2}; a.preextend(b.end(), b.end());         EXPECT_EQ(a, (Array<int>{1, 2})); }
    { Array<int> a{1, 2}; a.preextend(2, 1);                     EXPECT_EQ(a, (Array<int>{1, 1, 1, 2})); }
    { Array<int> a{1, 2}; a.preextend(v.begin(), v.begin() + 2); EXPECT_EQ(a, (Array<int>{5, 6, 1, 2})); }
    { Array<int> a{1, 2}; a.preextend(b.begin(), b.begin() + 2); EXPECT_EQ(a, (Array<int>{5, 6, 1, 2})); }
    { Array<int> a{1, 2}; a.preextend(v);                        EXPECT_EQ(a, (Array<int>{5, 6, 7, 1, 2})); }
    { Array<int> a{1, 2}; a.preextend(b);                        EXPECT_EQ(a, (Array<int>{5, 6, 7, 1, 2})); }
    { Array<int> a{1, 2}; a.preextend({5, 6, 7});                EXPECT_EQ(a, (Array<int>{5, 6, 7, 1, 2})); }
}

TEST(TestArray, Resize) {
    {
        Array<int> a = {15, 10, 42, 12};
        Array<int> b = {15, 10, 42};
        Array<int> c = {15, 10, 42, 0, 0};
        Array<int> d = {15, 10, 42, 0, 0, 15, 15, 15};
        a.resize(3); EXPECT_EQ(a, b);
        a.resize(5); EXPECT_EQ(a, c);
        a.resize(8, 15); EXPECT_EQ(a, d);
        EXPECT_THROW(a.resize(-1), NegativeIntegerError);
    }
    struct Tag {}; using TestObj = TestObject<Tag>;
    {
        // Tests resize_
        Array<TestObj> a(10);
        a.resize(3);        EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 10);
        a.resize(8);        EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 10);
        a.resize(16);       EXPECT_EQ(a.length(), TestObj::aliveCount());
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
}

TEST(TestArray, ResizeNoInit) {
    {
        Array<int> a = {15, 10, 42, 12};
        Array<int> b = {15, 10, 42};
        Array<int> c = {15, 10, 42, 12};
        a.resizeNoInit(3);  EXPECT_EQ(a, b);
        a.resizeNoInit(4);  EXPECT_EQ(a, c);
        a.resizeNoInit(10); EXPECT_LENGTH(a, 10);
        EXPECT_THROW(a.resizeNoInit(-1), NegativeIntegerError);
    }
    struct Tag {}; using TestObj = TestObject<Tag>;
    {
        // Tests resize_
        Array<TestObj> a(10);
        a.resizeNoInit(3);        EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 10);
        a.resizeNoInit(8);        EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a.reservedLength(), 10);
        a.resizeNoInit(16);       EXPECT_EQ(a.length(), TestObj::aliveCount());
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
}

TEST(TestArray, ShrinkToFit) {
    struct Tag {}; using TestObj = TestObject<Tag>;
    {
        // Tests shrinkToFit_
        Array<TestObj> a(10);
        a.resize(3);        EXPECT_EQ(a.reservedLength(), 10);
        a[2] = 42;
        a.shrinkToFit();    EXPECT_EQ(a.reservedLength(), 3);
        EXPECT_EQ(a[2], 42);
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
}

TEST(TestArray, Swap) {
    Array<int> a1 = {1, 2};
    Array<int> a2 = {1, 2};
    Array<int> a3 = {1, 2};
    Array<int> b1 = {3, 4, 5};
    Array<int> b2 = {3, 4, 5};
    Array<int> b3 = {3, 4, 5};
    a2.swap(b2); EXPECT_EQ(a2, b1); EXPECT_EQ(b2, a1);
    swap(a3, b3); EXPECT_EQ(a3, b1); EXPECT_EQ(b3, a1);
}

TEST(TestArray, Compare) {
    Array<int> a = {1, 2};
    Array<int> b = {1, 2};
    Array<int> c = {1, 2, 3};
    Array<int> d = {2};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_GT(c, a);
    EXPECT_GT(d, a);
    EXPECT_GE(c, a);
    EXPECT_GE(d, a);
    EXPECT_GE(a, b);
    EXPECT_LT(a, c);
    EXPECT_LT(a, d);
    EXPECT_LE(a, c);
    EXPECT_LE(a, d);
    EXPECT_LE(a, b);
}

TEST(TestArray, Contains) {
    Array<int> a = {3, 4, 5, 42, 10};
    EXPECT_TRUE(a.contains(42));
    EXPECT_FALSE(a.contains(43));
}

TEST(TestArray, Find) {
    Array<int> a = {3, 4, 5, 42, 10, 42};
    EXPECT_EQ(a.find(42), a.begin() + 3);
    EXPECT_EQ(a.find(43), a.end());
    EXPECT_EQ(a.find([](const int& v){ return v > 40; }), a.begin() + 3);
    EXPECT_EQ(a.find([](const int& v){ return v > 100; }), a.end());
}

TEST(TestArray, Search) {
    Array<int> a = {3, 4, 5, 42, 10, 42};
    EXPECT_EQ(a.search(42), &a[3]);
    EXPECT_EQ(a.search(43), nullptr);
    EXPECT_EQ(a.search([](const int& v){ return v > 40; }), &a[3]);
    EXPECT_EQ(a.search([](const int& v){ return v > 100; }), nullptr);
}

TEST(TestArray, Index) {
    Array<int> a = {3, 4, 5, 42, 10, 42};
    EXPECT_EQ(a.index(42), 3);
    EXPECT_EQ(a.index(43), -1);
    EXPECT_EQ(a.index([](const int& v){ return v > 40; }), 3);
    EXPECT_EQ(a.index([](const int& v){ return v > 100; }), -1);
}

TEST(TestArray, ToString) {
    Array<int> a = {1, 2}; EXPECT_EQ(toString(a), "[1, 2]");
    Array<int> b = {};     EXPECT_EQ(toString(b), "[]");
}

TEST(TestArray, PrivRangeConstruct) {
    struct Tag {}; using TestObj = TestObject<Tag>;
    using ISIt = std::istream_iterator<TestObj>;
    std::istringstream sst("1 2 3 4 5");
    std::vector<int> v = {1, 2, 3, 4, 5};
    {
        // rangeConstruct_ with input-only iterator
        Array<TestObj> a(ISIt{sst}, ISIt{});
        EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a[2], 3);
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
    {
        // rangeConstruct_ with forward iterator
        Array<TestObj> a(v.begin(), v.end());
        EXPECT_EQ(a.length(), TestObj::aliveCount()); EXPECT_EQ(a[2], 3);
    }   EXPECT_NO_THROW(TestObj::doPostTestChecks());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
