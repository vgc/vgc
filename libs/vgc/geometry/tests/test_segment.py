#!/usr/bin/python3

# Copyright 2023 The VGC Developers
# See the COPYRIGHT file at the top-level directory of this distribution
# and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest

import os, sys
sys.path.append(os.path.dirname(__file__))
import segment2data

from vgc.geometry import (
    Segment2d, Segment2f, Vec2d, Vec2f,
    Segment2fIntersection, Segment2dIntersection, SegmentIntersectionType)

Segment2Types = [Segment2d, Segment2f]
Segment2Vec2Types = [(Segment2d, Vec2d), (Segment2f, Vec2f)]
Segment2IntersectionTypes = [
    (Segment2d, Segment2dIntersection),
    (Segment2f, Segment2fIntersection)]

def swapVecXY(vec):
    if type(vec) is tuple:
        vec = vec[1], vec[0]
    else:
        vec.x, vec.y = vec.y, vec.x

# Tuples are immutable, so this cannot be done in-place
def swapTupleXY(t):
    return t[1], t[0]

def swapSegmentXY(segment):
    swapVecXY(segment.a)
    swapVecXY(segment.b)

class TestSegment2(unittest.TestCase):

    def testDefaultConstructor(self):
        for Segment2 in Segment2Types:
            s = Segment2()
            self.assertEqual(s.a, (0, 0))
            self.assertEqual(s.b, (0, 0))

    def testVec2Constructor(self):
        for Segment2, Vec2 in Segment2Vec2Types:
            s = Segment2(Vec2(1, 2), Vec2(3, 4))
            self.assertEqual(s.a, (1, 2))
            self.assertEqual(s.b, (3, 4))

    def testPairConstructor(self):
        for Segment2 in Segment2Types:
            s = Segment2((1, 2), (3, 4))
            self.assertEqual(s.a, (1, 2))
            self.assertEqual(s.b, (3, 4))

    def testScalarConstructor(self):
        for Segment2 in Segment2Types:
            s = Segment2(1, 2, 3, 4)
            self.assertEqual(s.a, (1, 2))
            self.assertEqual(s.b, (3, 4))

    def testGetItem(self):
        for Segment2 in Segment2Types:
            s = Segment2((1, 2), (3, 4))
            self.assertEqual(s[0], (1, 2))
            self.assertEqual(s[1], (3, 4))

    def testSetItem(self):
        for Segment2, Vec2 in Segment2Vec2Types:
            s = Segment2()
            s[0] = Vec2(1, 2)
            s[1] = Vec2(3, 4)
            self.assertEqual(s, Segment2((1, 2), (3, 4)))
            s[0] = (5, 6)
            s[1] = (7, 8)
            self.assertEqual(s, Segment2((5, 6), (7, 8)))

    def testGetPoint(self):
        for Segment2 in Segment2Types:
            s = Segment2((1, 2), (3, 4))
            self.assertEqual(s.a, (1, 2))
            self.assertEqual(s.b, (3, 4))

    def testSetPoint(self):
        for Segment2, Vec2 in Segment2Vec2Types:
            s = Segment2()
            s.a = Vec2(1, 2)
            s.b = Vec2(3, 4)
            self.assertEqual(s, Segment2((1, 2), (3, 4)))
            s.a = (5, 6)
            s.b = (7, 8)
            self.assertEqual(s, Segment2((5, 6), (7, 8)))

    def testGetScalar(self):
        for Segment2 in Segment2Types:
            s = Segment2((1, 2), (3, 4))
            self.assertEqual(s.a.x, 1)
            self.assertEqual(s.a.y, 2)
            self.assertEqual(s.b.x, 3)
            self.assertEqual(s.b.y, 4)

    def testSetScalar(self):
        for Segment2 in Segment2Types:
            s = Segment2()
            s.a.x = 1
            s.a.y = 2
            s.b.x = 3
            s.b.y = 4
            self.assertEqual(s, Segment2((1, 2), (3, 4)))

    def testIsDegenerate(self):
        for Segment2 in Segment2Types:
            s = Segment2()
            self.assertTrue(s.isDegenerate())
            s = Segment2((1, 2), (1, 2))
            self.assertTrue(s.isDegenerate())
            s = Segment2((1, 2), (3, 4))
            self.assertFalse(s.isDegenerate())

    def assertEqual_(self, s1, s2):

        self.assertEqual(s1, s2)
        self.assertLessEqual(s1, s2)
        self.assertGreaterEqual(s1, s2)

        self.assertEqual(s2, s1)
        self.assertLessEqual(s2, s1)
        self.assertGreaterEqual(s2, s1)

        self.assertTrue(s1 == s2)
        self.assertFalse(s1 != s2)
        self.assertFalse(s1 < s2)
        self.assertTrue(s1 <= s2)
        self.assertFalse(s1 > s2)
        self.assertTrue(s1 >= s2)

        self.assertTrue(s2 == s1)
        self.assertFalse(s2 != s1)
        self.assertFalse(s2 < s1)
        self.assertTrue(s2 <= s1)
        self.assertFalse(s2 > s1)
        self.assertTrue(s2 >= s1)

    def assertLess_(self, s1, s2):

        self.assertNotEqual(s1, s2)
        self.assertLess(s1, s2)
        self.assertLessEqual(s1, s2)
        self.assertGreater(s2, s1)
        self.assertGreaterEqual(s2, s1)

        self.assertFalse(s1 == s2)
        self.assertTrue(s1 != s2)
        self.assertTrue(s1 < s2)
        self.assertTrue(s1 <= s2)
        self.assertFalse(s1 > s2)
        self.assertFalse(s1 >= s2)

        self.assertFalse(s2 == s1)
        self.assertTrue(s2 != s1)
        self.assertFalse(s2 < s1)
        self.assertFalse(s2 <= s1)
        self.assertTrue(s2 > s1)
        self.assertTrue(s2 >= s1)

    def testComparisonOperators(self):
        for Segment2 in Segment2Types:

            s2 = Segment2((1, 3), (5, 7))
            self.assertEqual_(s2, s2)

            # ax differ
            s1 = Segment2((0, 3), (5, 7))
            s3 = Segment2((2, 3), (5, 7))
            self.assertLess_(s1, s2)
            self.assertLess_(s2, s3)
            for ay in range(10):
                for bx in range(10):
                    for by in range(10):
                        s1.a.y = ay
                        s1.b.x = bx
                        s1.b.y = by
                        s3.a.y = ay
                        s3.b.x = bx
                        s3.b.y = by
                        self.assertLess_(s1, s2)
                        self.assertLess_(s2, s3)

            # ax equal, ay differ
            s1 = Segment2((1, 2), (5, 7))
            s3 = Segment2((1, 4), (5, 7))
            self.assertLess_(s1, s2)
            self.assertLess_(s2, s3)
            for bx in range(10):
                for by in range(10):
                    s1.b.x = bx
                    s1.b.y = by
                    s3.b.x = bx
                    s3.b.y = by
                    self.assertLess_(s1, s2)
                    self.assertLess_(s2, s3)

            # ax, ay equal, bx differ
            s1 = Segment2((1, 3), (4, 7))
            s3 = Segment2((1, 3), (6, 7))
            self.assertLess_(s1, s2)
            self.assertLess_(s2, s3)
            for by in range(10):
                s1.b.y = by
                s3.b.y = by
                self.assertLess_(s1, s2)
                self.assertLess_(s2, s3)

            # ax, ay, bx equal, by differ
            s1 = Segment2((1, 3), (5, 6))
            s3 = Segment2((1, 3), (5, 8))
            self.assertLess_(s1, s2)
            self.assertLess_(s2, s3)

    def assertIntersectEqual(self, s1, s2, expected):
        res = s1.intersect(s2)
        if res != expected:
            raise AssertionError(
                f"Intersecting s1={s1} with s2={s2}.\n"
                f"        Output: {res}.\n"
                f"      Expected: {expected}.")

    def testIntersect(self):

        for Segment2, Vec2 in Segment2Vec2Types:

            a = Vec2(-0.3, -0.2)
            b = Vec2(1.7, 1.4)
            c = Vec2(0.2, 1.6)
            d = Vec2(1.2, -0.4)

            # Point-intersection in the middle

            i = Segment2(a, b).intersect(Segment2(c, d))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

            i = Segment2(a, b).intersect(Segment2(d, c))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

            i = Segment2(b, a).intersect(Segment2(c, d))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

            i = Segment2(b, a).intersect(Segment2(d, c))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

            # Non-intersecting collinear segments

            i = Segment2((1, 1), (2, 1)).intersect(Segment2((1, 2), (2, 2)))
            self.assertEqual(i.type, SegmentIntersectionType.Empty)

            # Segment intersection

            i = Segment2(a, c).intersect(Segment2(a, c))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            i = Segment2(c, a).intersect(Segment2(c, a))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            i = Segment2(a, c).intersect(Segment2(c, a))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            i = Segment2(c, a).intersect(Segment2(a, c))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            e = Vec2(0.5, 0.2)
            f = Vec2(0.5, 0.4)
            g = Vec2(0.5, 0.6)
            k = Vec2(0.5, 0.8)

            i = Segment2(e, k).intersect(Segment2(f, g))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            i = Segment2(e, k).intersect(Segment2(e, f))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            i = Segment2(e, g).intersect(Segment2(f, k))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            # Intersection between two segment endpoints

            i = Segment2(a, c).intersect(Segment2(a, d))
            self.assertEqual(i.type, SegmentIntersectionType.Point)
            self.assertEqual(i.p, a)
            self.assertEqual(i.t1, 0)
            self.assertEqual(i.t2, 0)

            i = Segment2(a, c).intersect(Segment2(d, a))
            self.assertEqual(i.type, SegmentIntersectionType.Point)
            self.assertEqual(i.p, a)
            self.assertEqual(i.t1, 0)
            self.assertEqual(i.t2, 1)

            i = Segment2(c, a).intersect(Segment2(a, d))
            self.assertEqual(i.type, SegmentIntersectionType.Point)
            self.assertEqual(i.p, a)
            self.assertEqual(i.t1, 1)
            self.assertEqual(i.t2, 0)

            i = Segment2(c, a).intersect(Segment2(d, a))
            self.assertEqual(i.type, SegmentIntersectionType.Point)
            self.assertEqual(i.p, a)
            self.assertEqual(i.t1, 1)
            self.assertEqual(i.t2, 1)

            # Intersection between segment endpoint and segment interior

            h = Vec2(1.1, 0.4)

            i = Segment2(e, g).intersect(Segment2(f, h))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

        intersectData = segment2data.intersectData

        for Segment2, Segment2Intersection in Segment2IntersectionTypes:
            for d in intersectData:
                for swapS in [False, True]:
                    for swap1 in [False, True]:
                        for swap2 in [False, True]:
                            for swapXY in [False, True]:
                                s1 = Segment2(d[0], d[1])
                                s2 = Segment2(d[2], d[3])
                                args = d[4:]
                                if swap1:
                                    s1 = Segment2(s1.b, s1.a)
                                    if s1.a != s1.b:
                                        if len(args) == 3:
                                            args[1] = 1 - args[1] # t1
                                        if len(args) == 6:
                                            args[2] = 1 - args[2] # s1
                                            args[3] = 1 - args[3] # t1
                                if swap2:
                                    s2 = Segment2(s2.b, s2.a)
                                    if s2.a != s2.b:
                                        if len(args) == 3:
                                            args[2] = 1 - args[2] # t2
                                        if len(args) == 6:
                                            args[4] = 1 - args[4] # s2
                                            args[5] = 1 - args[5] # t2
                                if swapS:
                                    s1, s2 = s2, s1
                                    if len(args) == 3:
                                        args[1], args[2] = args[2], args[1]
                                    if len(args) == 6:
                                        args[2:4], args[4:6] = args[4:6], args[2:4]
                                if swapXY:
                                    swapSegmentXY(s1)
                                    swapSegmentXY(s2)
                                    if len(args) == 3:
                                        args[0] = swapTupleXY(args[0])
                                    if len(args) == 6:
                                        args[0] = swapTupleXY(args[0])
                                        args[1] = swapTupleXY(args[1])
                                expected = Segment2Intersection(*args)
                                self.assertIntersectEqual(s1, s2, expected)
                                self.assertEqual(expected.segment, Segment2(expected.p, expected.q))


if __name__ == '__main__':
    unittest.main()
