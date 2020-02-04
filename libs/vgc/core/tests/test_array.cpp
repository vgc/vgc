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
#include <vgc/core/array.h>

using vgc::Int;
using vgc::core::Array;
using vgc::core::IndexError;
using vgc::core::NegativeIntegerError;

TEST(TestArray, Construct) {
    // Note: it's important to test the zero-init after the non-zero init, to
    // decrease the chance that the memory is zero "by chance".
    { Array<int> a;                 EXPECT_EQ(a.size(), 0); }
    { Array<int> a{};               EXPECT_EQ(a.size(), 0); }
    { Array<int> a(10, 42);         EXPECT_EQ(a.size(), 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(10);             EXPECT_EQ(a.size(), 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  } // zero-init
    { Array<int> a(Int(10), 42);    EXPECT_EQ(a.size(), 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(Int(10));        EXPECT_EQ(a.size(), 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  } // zero-init
    { Array<int> a(size_t(10), 42); EXPECT_EQ(a.size(), 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(size_t(10));     EXPECT_EQ(a.size(), 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  } // zero-init
    { Array<int> a{10, 42};         EXPECT_EQ(a.size(), 2);  EXPECT_EQ(a[0], 10); EXPECT_EQ(a[1], 42); }
    { Array<int> a;                 EXPECT_EQ(a.length(), 0); }
    { Array<int> a{};               EXPECT_EQ(a.length(), 0); }
    { Array<int> a(10, 42);         EXPECT_EQ(a.length(), 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(10);             EXPECT_EQ(a.length(), 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  } // zero-init
    { Array<int> a(Int(10), 42);    EXPECT_EQ(a.length(), 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(Int(10));        EXPECT_EQ(a.length(), 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  } // zero-init
    { Array<int> a(size_t(10), 42); EXPECT_EQ(a.length(), 10); EXPECT_EQ(a[0], 42); EXPECT_EQ(a[9], 42); }
    { Array<int> a(size_t(10));     EXPECT_EQ(a.length(), 10); EXPECT_EQ(a[0], 0);  EXPECT_EQ(a[9], 0);  } // zero-init
    { Array<int> a{10, 42};         EXPECT_EQ(a.length(), 2);  EXPECT_EQ(a[0], 10); EXPECT_EQ(a[1], 42); }
    EXPECT_THROW(Array<int>(-1),          NegativeIntegerError);
    EXPECT_THROW(Array<int>(Int(-1)),     NegativeIntegerError);
    EXPECT_THROW(Array<int>(-1, 42),      NegativeIntegerError);
    EXPECT_THROW(Array<int>(Int(-1), 42), NegativeIntegerError);
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
    Array<int> a;
    Array<int> b = {10, 42, 3, 4};
    a.assign(10, 1);                    EXPECT_EQ(a.length(), 10); EXPECT_EQ(a[0], 1); EXPECT_EQ(a[9], 1);
    a.assign(Int(11), 2);               EXPECT_EQ(a.length(), 11); EXPECT_EQ(a[0], 2); EXPECT_EQ(a[10], 2);
    a.assign(size_t(12), 3);            EXPECT_EQ(a.length(), 12); EXPECT_EQ(a[0], 3); EXPECT_EQ(a[11], 3);
    a.assign(b.begin(), b.begin() + 2); EXPECT_EQ(a.length(), 2);  EXPECT_EQ(a[0], 10); EXPECT_EQ(a[1], 42);
    a.assign({11, 43});                 EXPECT_EQ(a.length(), 2);  EXPECT_EQ(a[0], 11); EXPECT_EQ(a[1], 43);
    EXPECT_THROW(a.assign(-1, 42),      NegativeIntegerError);
    EXPECT_THROW(a.assign(Int(-1), 42), NegativeIntegerError);
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
    EXPECT_THROW(a[size_t(-1)],      IndexError);
    EXPECT_THROW(a[Int(-1)],         IndexError);
    EXPECT_THROW(a[-1]         = 10, IndexError);
    EXPECT_THROW(a[size_t(-1)] = 10, IndexError);
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
    a.insert(a.begin(), b); EXPECT_EQ(a.length(), 1); EXPECT_EQ(a[0][0], 42);
    a.insert(a.begin(), std::move(c)); EXPECT_EQ(a.length(), 2); EXPECT_EQ(a[0][0], 43); EXPECT_EQ(c.length(), 0);
    a.insert(a.begin(), size_t(2), b); EXPECT_EQ(a.length(), 4);
    a.insert(a.begin(), Int(2), b); EXPECT_EQ(a.length(), 6);
    EXPECT_THROW(a.insert(a.begin(), -1, b), NegativeIntegerError);
    Array<Array<int>> d = a;
    a.insert(a.begin(), d.begin(), d.end()); EXPECT_EQ(a.length(), 12);
    a.insert(a.begin(), {{1, 2}, {3, 4}, {5, 6}}); EXPECT_EQ(a.length(), 15);
}

TEST(TestArray, InsertAtIndex) {
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
    Array<int> a = {10, 42, 12};
    Array<int> b = {10, 12};
    Array<int> c = {10};
    auto it = a.erase(a.begin()+1); EXPECT_EQ(a, b); EXPECT_EQ(*it, 12);
    auto it2 = a.erase(a.begin()+1); EXPECT_EQ(a, c); EXPECT_EQ(it2, a.end());
}

TEST(TestArray, EraseRangeIterator) {
    Array<int> a = {10, 42, 12};
    Array<int> b = {12};
    Array<int> c = {10};
    auto it = a.erase(a.begin(), a.begin()+2); EXPECT_EQ(a, b); EXPECT_EQ(*it, 12);
    auto it2 = a.erase(a.begin(), a.begin()); EXPECT_EQ(a, b); EXPECT_EQ(it2, a.begin());
    auto it3 = a.erase(a.end(), a.end()); EXPECT_EQ(a, b); EXPECT_EQ(it3, a.end());
    auto it4 = a.erase(a.begin(), a.end()); EXPECT_TRUE(a.isEmpty()); EXPECT_EQ(it4, a.end());
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

TEST(TestArray, AppendAndPrepend) {
    Array<int> a;
    Array<int> b = {10, 42, 12};
    a.append(10);
    a.append(42);
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
    Array<Array<int>> f = {{7, 8}, {1, 2}, {3, 4}, {5, 6}};
    c.append(std::move(d));
    c.prepend(std::move(e));
    EXPECT_EQ(c, f);
    EXPECT_EQ(d.length(), 0);
    EXPECT_EQ(e.length(), 0);
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

TEST(TestArray, Resize) {
    Array<int> a = {15, 10, 42, 12};
    Array<int> b = {15, 10, 42};
    Array<int> c = {15, 10, 42, 0, 0};
    Array<int> d = {15, 10, 42, 0, 0, 15, 15, 15};
    a.resize(3); EXPECT_EQ(a, b);
    a.resize(5); EXPECT_EQ(a, c);
    a.resize(8, 15); EXPECT_EQ(a, d);
    EXPECT_THROW(a.resize(-1), NegativeIntegerError);
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

TEST(TestArray, ToString) {
    Array<int> a = {1, 2}; EXPECT_EQ(toString(a), "[1, 2]");
    Array<int> b = {};     EXPECT_EQ(toString(b), "[]");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
