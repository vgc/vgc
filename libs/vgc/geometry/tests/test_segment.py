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

from vgc.geometry import Segment2d, Segment2f, Vec2d, Vec2f, SegmentIntersectionType

Segment2Types = [Segment2d, Segment2f]
Segment2Vec2Types = [(Segment2d, Vec2d), (Segment2f, Vec2f)]

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

    def testIntersect(self):
        for Segment2, Vec2 in Segment2Vec2Types:

            a = Vec2(-0.3, -0.2)
            b = Vec2(1.7, 1.4)
            c = Vec2(0.2, 1.6)
            d = Vec2(1.2, -0.4)

            e = Vec2(0.5, 0.2)
            f = Vec2(0.5, 0.4)
            g = Vec2(0.5, 0.6)
            h = Vec2(1.1, 0.4)

            # Point-intersection in the middle
            # TODO: improve tests by actually testing intersection values

            i = Segment2(a, b).intersect(Segment2(c, d))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

            i = Segment2(a, b).intersect(Segment2(d, c))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

            i = Segment2(b, a).intersect(Segment2(c, d))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

            i = Segment2(b, a).intersect(Segment2(d, c))
            self.assertEqual(i.type, SegmentIntersectionType.Point)

            # Non-intersecting collinear segments
            # TODO: support this case. Currently, is is incorrectly returned as Segment.

            # Segment intersection
            # TODO: properly support and test this case. Currently, it is indeed returned
            # as Segment, but with dummy points and params.
            # TODO: test strict sub-segment, starting at one of the endpoint or not.

            i = Segment2(a, c).intersect(Segment2(a, c))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            i = Segment2(c, a).intersect(Segment2(c, a))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            i = Segment2(a, c).intersect(Segment2(c, a))
            self.assertEqual(i.type, SegmentIntersectionType.Segment)

            i = Segment2(c, a).intersect(Segment2(a, c))
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
            # TODO: improve tests by actually testing intersection values

            i = Segment2(e, g).intersect(Segment2(f, h))
            self.assertEqual(i.type, SegmentIntersectionType.Point)


if __name__ == '__main__':
    unittest.main()
