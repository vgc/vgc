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

#include <memory>

#include <vgc/core/arithmetic.h>

// clang-format off

namespace foo {

// The 6 test classes below are from:
// https://stackoverflow.com/questions/29765961/default-value-and-zero-initialization-mess
struct A { int m;                    };
struct B { int m;            ~B(){}; };
struct C { int m; C():m(){}; ~C(){}; };
struct D { int m; D(){};             };
struct E { int m; E() = default;     };
struct F { int m; F();               }; F::F() = default;

void setZero(A& x) { x.m = 0; }
void setZero(B& x) { x.m = 0; }
void setZero(C& x) { x.m = 0; }
void setZero(D& x) { x.m = 0; }
void setZero(E& x) { x.m = 0; }
void setZero(F& x) { x.m = 0; }

} // namespace foo

// We use this macro to fill stack memory with something else than 0.
// Subsequent calls to EXPECT_NE(a.m, 0) are undefined behavior in theory, but
// pass in practice, and help illustrate that `a.m` is indeed not initialized
// to zero. Note that we initially tried the more aggressive test
// EXPECT_EQ(a.m, 42), but it didn't pass on all compilers (a.m wasn't equal to
// 42, but was still equal to some garbage value, not zero).
//
// Update 2020-12-14: Even the less aggressive EXPECT_NE(a.m, 0) fails when
// testing on the GitHub Actions Ubuntu machines. Therefore, we comment them out.
//
#define FILL                                                                             \
    {                                                                                    \
        int m = 42;                                                                      \
        EXPECT_EQ(m, 42);                                                                \
    }

// We use this macro to fill heap memory with something else than 0, before
// doing a placement new at that same exact location. Subsequent calls to
// EXPECT_EQ(a->m, 42) are undefined behavior in theory, but pass in practice,
// and help illustrate that `a->m` is indeed not initialized to zero.
//
#define FILLH(b)                                                                         \
    std::unique_ptr<int> bp(new int(42));                                                \
    int* b = bp.get();                                                                   \
    EXPECT_EQ(*b, 42)

TEST(TestZero, StackDefaultInitialization) {
    // { FILL; foo::A a; EXPECT_NE(a.m, 0); } // UB!
    // { FILL; foo::B a; EXPECT_NE(a.m, 0); } // UB!
    { FILL; foo::C a; EXPECT_EQ(a.m, 0); }
    // { FILL; foo::D a; EXPECT_NE(a.m, 0); } // UB!
    // { FILL; foo::E a; EXPECT_NE(a.m, 0); } // UB!
    // { FILL; foo::F a; EXPECT_NE(a.m, 0); } // UB!
}

TEST(TestZero, StackValueInitialization) {
    { FILL; foo::A a = foo::A(); EXPECT_EQ(a.m, 0); }
    { FILL; foo::B a = foo::B(); EXPECT_EQ(a.m, 0); }
    { FILL; foo::C a = foo::C(); EXPECT_EQ(a.m, 0); }
    // { FILL; foo::D a = foo::D(); EXPECT_NE(a.m, 0); } // UB!
    { FILL; foo::E a = foo::E(); EXPECT_EQ(a.m, 0); }
    // { FILL; foo::F a = foo::F(); EXPECT_NE(a.m, 0); } // UB!
}

TEST(TestZero, StackListInitialization) {
    { FILL; foo::A a{}; EXPECT_EQ(a.m, 0); }
    { FILL; foo::B a{}; EXPECT_EQ(a.m, 0); }
    { FILL; foo::C a{}; EXPECT_EQ(a.m, 0); }
    // { FILL; foo::D a{}; EXPECT_NE(a.m, 0); } // UB!
    { FILL; foo::E a{}; EXPECT_EQ(a.m, 0); }
    // { FILL; foo::F a{}; EXPECT_NE(a.m, 0); } // UB!
}

TEST(TestZero, StackExplicitZero) {
    { FILL; foo::A a = vgc::core::zero<foo::A>(); EXPECT_EQ(a.m, 0); }
    { FILL; foo::B a = vgc::core::zero<foo::B>(); EXPECT_EQ(a.m, 0); }
    { FILL; foo::C a = vgc::core::zero<foo::C>(); EXPECT_EQ(a.m, 0); }
    { FILL; foo::D a = vgc::core::zero<foo::D>(); EXPECT_EQ(a.m, 0); }
    { FILL; foo::E a = vgc::core::zero<foo::E>(); EXPECT_EQ(a.m, 0); }
    { FILL; foo::F a = vgc::core::zero<foo::F>(); EXPECT_EQ(a.m, 0); }
}

TEST(TestZero, HeapDefaultInitialization) {
    { FILLH(b); foo::A* a = new (b) foo::A; EXPECT_EQ(a->m, 42); } // ~UB
    { FILLH(b); foo::B* a = new (b) foo::B; EXPECT_EQ(a->m, 42); } // ~UB
    { FILLH(b); foo::C* a = new (b) foo::C; EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::D* a = new (b) foo::D; EXPECT_EQ(a->m, 42); } // ~UB
    { FILLH(b); foo::E* a = new (b) foo::E; EXPECT_EQ(a->m, 42); } // ~UB
    { FILLH(b); foo::F* a = new (b) foo::F; EXPECT_EQ(a->m, 42); } // ~UB
}

TEST(TestZero, HeapValueInitialization) {
    { FILLH(b); foo::A* a = new (b) foo::A(); EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::B* a = new (b) foo::B(); EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::C* a = new (b) foo::C(); EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::D* a = new (b) foo::D(); EXPECT_EQ(a->m, 42); } // ~UB
    { FILLH(b); foo::E* a = new (b) foo::E(); EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::F* a = new (b) foo::F(); EXPECT_EQ(a->m, 42); } // ~UB
}

TEST(TestZero, HeapListInitialization) {
    { FILLH(b); foo::A* a = new (b) foo::A{}; EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::B* a = new (b) foo::B{}; EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::C* a = new (b) foo::C{}; EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::D* a = new (b) foo::D{}; EXPECT_EQ(a->m, 42); } // ~UB
    { FILLH(b); foo::E* a = new (b) foo::E{}; EXPECT_EQ(a->m, 0);  }
    { FILLH(b); foo::F* a = new (b) foo::F{}; EXPECT_EQ(a->m, 42); } // ~UB
}

TEST(TestZero, HeapExplicitZero) {
    { FILLH(b); foo::A* a = new (b) foo::A(vgc::core::zero<foo::A>()); EXPECT_EQ(a->m, 0); }
    { FILLH(b); foo::B* a = new (b) foo::B(vgc::core::zero<foo::B>()); EXPECT_EQ(a->m, 0); }
    { FILLH(b); foo::C* a = new (b) foo::C(vgc::core::zero<foo::C>()); EXPECT_EQ(a->m, 0); }
    { FILLH(b); foo::D* a = new (b) foo::D(vgc::core::zero<foo::D>()); EXPECT_EQ(a->m, 0); }
    { FILLH(b); foo::E* a = new (b) foo::E(vgc::core::zero<foo::E>()); EXPECT_EQ(a->m, 0); }
    { FILLH(b); foo::F* a = new (b) foo::F(vgc::core::zero<foo::F>()); EXPECT_EQ(a->m, 0); }
}

// clang-format on

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
