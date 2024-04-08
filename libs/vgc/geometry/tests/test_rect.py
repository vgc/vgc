#!/usr/bin/python3

# Copyright 2022 The VGC Developers
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
from vgc.geometry import Vec2d, Vec2f, Rect2d, Rect2f

Rect2Types = [Rect2d, Rect2f]
Rect2Vec2Types = [(Rect2d, Vec2d), (Rect2f, Vec2f)]


class TestRect(unittest.TestCase):

    def testDefaultConstructor(self):
        for Rect2 in Rect2Types:
            r = Rect2()
            self.assertEqual(r.xMin, 0)
            self.assertEqual(r.yMin, 0)
            self.assertEqual(r.xMax, 0)
            self.assertEqual(r.yMax, 0)

    def testMinMaxConstructor(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r = Rect2(Vec2(1, 2), Vec2(3, 4))
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 3)
            self.assertEqual(r.yMax, 4)

            r = Rect2((1, 2), (3, 4))
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 3)
            self.assertEqual(r.yMax, 4)

            r = Rect2(1, 2, 3, 4)
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 3)
            self.assertEqual(r.yMax, 4)

            r = Rect2(Vec2(3, 4), Vec2(1, 2))
            self.assertEqual(r.xMin, 3)
            self.assertEqual(r.yMin, 4)
            self.assertEqual(r.xMax, 1)
            self.assertEqual(r.yMax, 2)

            r = Rect2((3, 4), (1, 2))
            self.assertEqual(r.xMin, 3)
            self.assertEqual(r.yMin, 4)
            self.assertEqual(r.xMax, 1)
            self.assertEqual(r.yMax, 2)

            r = Rect2(3, 4, 1, 2)
            self.assertEqual(r.xMin, 3)
            self.assertEqual(r.yMin, 4)
            self.assertEqual(r.xMax, 1)
            self.assertEqual(r.yMax, 2)

    def testPositionSizeConstructor(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            p = Vec2(1, 2)
            s = Vec2(3, 4)

            r = Rect2(position=p, size=s)
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 4)
            self.assertEqual(r.yMax, 6)

            r = Rect2(position=p, size=(3, 4))
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 4)
            self.assertEqual(r.yMax, 6)

            r = Rect2(position=(1, 2), size=s)
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 4)
            self.assertEqual(r.yMax, 6)

            r = Rect2(position=(1, 2), size=(3, 4))
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 4)
            self.assertEqual(r.yMax, 6)

            r = Rect2(position=(1, 2), size=(-3, -5))
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, -2)
            self.assertEqual(r.yMax, -3)

    def testCopyByReference(self):
        for Rect2 in Rect2Types:
            r = Rect2(1, 2, 3, 4)
            s = r
            self.assertIs(r, s)
            self.assertEqual(r, s)
            r.x = 5
            self.assertEqual(r.x, 5)
            self.assertEqual(s.x, 5)
            self.assertEqual(r, s)

    def testCopyByValue(self):
        for Rect2 in Rect2Types:
            r = Rect2(1, 2, 3, 4)
            s = Rect2(r)
            self.assertIsNot(r, s)
            self.assertEqual(r, s)
            r.x = 5
            self.assertEqual(r.x, 5)
            self.assertEqual(s.x, 1)
            self.assertNotEqual(r, s)

    def testEmpty(self):
        for Rect2 in Rect2Types:
            r = Rect2.empty
            self.assertTrue(r.isEmpty())
            self.assertTrue(Rect2.empty.isEmpty())
            self.assertFalse(Rect2().isEmpty())
            self.assertFalse(Rect2(1, 2, 3, 4).isEmpty())
            self.assertTrue(Rect2(3, 4, 1, 2).isEmpty())
            self.assertFalse(Rect2(3, 4, 1, 2).normalized().isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(1, 2)).isEmpty())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, 2)).isEmpty())
            self.assertTrue(Rect2(position=(1, 2), size=(1, -2)).isEmpty())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, -2)).isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(1, 2)).normalized().isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(-1, 2)).normalized().isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(1, -2)).normalized().isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(-1, -2)).normalized().isEmpty())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, -1)).isEmpty())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, 0)).isEmpty())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, 1)).isEmpty())
            self.assertTrue(Rect2(position=(1, 2), size=(0, -1)).isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(0, 0)).isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(0, 1)).isEmpty())
            self.assertTrue(Rect2(position=(1, 2), size=(1, -1)).isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(1, 0)).isEmpty())
            self.assertFalse(Rect2(position=(1, 2), size=(1, 1)).isEmpty())

    def testDegenerate(self):
        for Rect2 in Rect2Types:
            r = Rect2.empty
            self.assertTrue(r.isDegenerate())
            self.assertTrue(Rect2.empty.isDegenerate())
            self.assertTrue(Rect2().isDegenerate())
            self.assertFalse(Rect2(1, 2, 3, 4).isDegenerate())
            self.assertTrue(Rect2(3, 4, 1, 2).isDegenerate())
            self.assertFalse(Rect2(3, 4, 1, 2).normalized().isDegenerate())
            self.assertFalse(Rect2(position=(1, 2), size=(1, 2)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, 2)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(1, -2)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, -2)).isDegenerate())
            self.assertFalse(Rect2(position=(1, 2), size=(1, 2)).normalized().isDegenerate())
            self.assertFalse(Rect2(position=(1, 2), size=(-1, 2)).normalized().isDegenerate())
            self.assertFalse(Rect2(position=(1, 2), size=(1, -2)).normalized().isDegenerate())
            self.assertFalse(Rect2(position=(1, 2), size=(-1, -2)).normalized().isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, -1)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, 0)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(-1, 1)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(0, -1)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(0, 0)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(0, 1)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(1, -1)).isDegenerate())
            self.assertTrue(Rect2(position=(1, 2), size=(1, 0)).isDegenerate())
            self.assertFalse(Rect2(position=(1, 2), size=(1, 1)).isDegenerate())

    def testNormalize(self):
        for Rect2 in Rect2Types:
            r = Rect2(1, 2, 3, 4)
            r.normalize()
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 3)
            self.assertEqual(r.yMax, 4)

            r = Rect2(3, 4, 1, 2)
            r.normalize()
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 3)
            self.assertEqual(r.yMax, 4)

    def testNormalized(self):
        for Rect2 in Rect2Types:
            r = Rect2(1, 2, 3, 4).normalized()
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 3)
            self.assertEqual(r.yMax, 4)

            r = Rect2(3, 4, 1, 2).normalized()
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)
            self.assertEqual(r.xMax, 3)
            self.assertEqual(r.yMax, 4)

    def testPosition(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r = Rect2(1, 2, 4, 6)
            self.assertEqual(r.position, Vec2(1, 2))
            self.assertEqual(r.size, Vec2(3, 4))
            self.assertEqual(r.pMax, Vec2(4, 6))
            self.assertEqual(r.x, 1)
            self.assertEqual(r.y, 2)

            r.position = Vec2(5, 6)
            self.assertEqual(r.position, Vec2(5, 6))
            self.assertEqual(r.size, Vec2(3, 4))
            self.assertEqual(r.pMax, Vec2(8, 10))

            r.x = 7
            self.assertEqual(r.position, Vec2(7, 6))
            self.assertEqual(r.size, Vec2(3, 4))
            self.assertEqual(r.pMax, Vec2(10, 10))

            r.y = 8
            self.assertEqual(r.position, Vec2(7, 8))
            self.assertEqual(r.size, Vec2(3, 4))
            self.assertEqual(r.pMax, Vec2(10, 12))

    def testSize(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r = Rect2(1, 2, 4, 6)
            self.assertEqual(r.position, Vec2(1, 2))
            self.assertEqual(r.size, Vec2(3, 4))
            self.assertEqual(r.pMax, Vec2(4, 6))
            self.assertEqual(r.width, 3)
            self.assertEqual(r.height, 4)

            r.size = Vec2(5, 6)
            self.assertEqual(r.position, Vec2(1, 2))
            self.assertEqual(r.size, Vec2(5, 6))
            self.assertEqual(r.pMax, Vec2(6, 8))

            r.width = 7
            self.assertEqual(r.position, Vec2(1, 2))
            self.assertEqual(r.size, Vec2(7, 6))
            self.assertEqual(r.pMax, Vec2(8, 8))

            r.height = 8
            self.assertEqual(r.position, Vec2(1, 2))
            self.assertEqual(r.size, Vec2(7, 8))
            self.assertEqual(r.pMax, Vec2(8, 10))

    def testPMin(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r = Rect2(1, 2, 4, 6)
            self.assertEqual(r.pMin, Vec2(1, 2))
            self.assertEqual(r.pMax, Vec2(4, 6))
            self.assertEqual(r.size, Vec2(3, 4))
            self.assertEqual(r.xMin, 1)
            self.assertEqual(r.yMin, 2)

            r.pMin = Vec2(5, 6)
            self.assertEqual(r.pMin, Vec2(5, 6))
            self.assertEqual(r.pMax, Vec2(4, 6))
            self.assertEqual(r.size, Vec2(-1, 0))

            r.xMin = 7
            self.assertEqual(r.pMin, Vec2(7, 6))
            self.assertEqual(r.pMax, Vec2(4, 6))
            self.assertEqual(r.size, Vec2(-3, 0))

            r.yMin = 8
            self.assertEqual(r.pMin, Vec2(7, 8))
            self.assertEqual(r.pMax, Vec2(4, 6))
            self.assertEqual(r.size, Vec2(-3, -2))

    def testPMax(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r = Rect2(1, 2, 4, 6)
            self.assertEqual(r.pMin, Vec2(1, 2))
            self.assertEqual(r.pMax, Vec2(4, 6))
            self.assertEqual(r.size, Vec2(3, 4))
            self.assertEqual(r.xMax, 4)
            self.assertEqual(r.yMax, 6)

            r.pMax = Vec2(5, 6)
            self.assertEqual(r.pMin, Vec2(1, 2))
            self.assertEqual(r.pMax, Vec2(5, 6))
            self.assertEqual(r.size, Vec2(4, 4))

            r.xMax = 7
            self.assertEqual(r.pMin, Vec2(1, 2))
            self.assertEqual(r.pMax, Vec2(7, 6))
            self.assertEqual(r.size, Vec2(6, 4))

            r.yMax = 8
            self.assertEqual(r.pMin, Vec2(1, 2))
            self.assertEqual(r.pMax, Vec2(7, 8))
            self.assertEqual(r.size, Vec2(6, 6))

    def testCorner(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r = Rect2(1, 2, 4, 6)
            self.assertEqual(r.corner(0, 0), Vec2(1, 2))
            self.assertEqual(r.corner(1, 0), Vec2(4, 2))
            self.assertEqual(r.corner(1, 1), Vec2(4, 6))
            self.assertEqual(r.corner(0, 1), Vec2(1, 6))
            self.assertEqual(r.corner(0), Vec2(1, 2))
            self.assertEqual(r.corner(1), Vec2(4, 2))
            self.assertEqual(r.corner(2), Vec2(4, 6))
            self.assertEqual(r.corner(3), Vec2(1, 6))

    def testIsClose(self):
        for Rect2 in Rect2Types:
            self.assertTrue(Rect2(0, 0, 1, 1).isClose(Rect2(0, 0, 1, 1)))
            self.assertTrue(Rect2(0, 0, 1, 1).isClose(Rect2(0, 0, 1 + 1e-10, 1)))
            self.assertFalse(Rect2(0, 0, 1, 1).isClose(Rect2(1e-10, 0, 1, 1)))
            self.assertFalse(Rect2(0, 0, 1, 1).isClose(Rect2(0.0009, 0, 1, 1)))
            self.assertFalse(Rect2(0, 0, 1, 1).isClose(Rect2(0.0009, 0.0009, 1, 1)))
            self.assertFalse(Rect2(0, 0, 1, 1).isClose(Rect2(0.002, 0, 1, 1)))

    def testIsNear(self):
        absTol = 0.001
        for Rect2 in Rect2Types:
            self.assertTrue(Rect2(0, 0, 1, 1).isNear(Rect2(0, 0, 1, 1), absTol))
            self.assertTrue(Rect2(0, 0, 1, 1).isNear(Rect2(0, 0, 1 + 1e-10, 1), absTol))
            self.assertTrue(Rect2(0, 0, 1, 1).isNear(Rect2(1e-10, 0, 1, 1), absTol))
            self.assertTrue(Rect2(0, 0, 1, 1).isNear(Rect2(0.0009, 0, 1, 1), absTol))
            self.assertFalse(Rect2(0, 0, 1, 1).isNear(Rect2(0.0009, 0.0009, 1, 1), absTol))
            self.assertFalse(Rect2(0, 0, 1, 1).isNear(Rect2(0.002, 0, 1, 1), absTol))

    def testAllNear(self):
        absTol = 0.001
        for Rect2 in Rect2Types:
            self.assertTrue(Rect2(0, 0, 1, 1).allNear(Rect2(0, 0, 1, 1), absTol))
            self.assertTrue(Rect2(0, 0, 1, 1).allNear(Rect2(0, 0, 1 + 1e-10, 1), absTol))
            self.assertTrue(Rect2(0, 0, 1, 1).allNear(Rect2(1e-10, 0, 1, 1), absTol))
            self.assertTrue(Rect2(0, 0, 1, 1).allNear(Rect2(0.0009, 0, 1, 1), absTol))
            self.assertTrue(Rect2(0, 0, 1, 1).allNear(Rect2(0.0009, 0.0009, 1, 1), absTol))
            self.assertFalse(Rect2(0, 0, 1, 1).allNear(Rect2(0.002, 0, 1, 1), absTol))

    def testComparisonOperators(self):
        for Rect2 in Rect2Types:
            r1 = Rect2(1, 2, 3, 4)
            r2 = Rect2(position=(1, 2), size=(2, 2))
            r3 = Rect2(1, 2, 3, 5)
            self.assertTrue(r1 == r2)
            self.assertTrue(r1 != r3)
            self.assertFalse(r1 != r2)
            self.assertFalse(r1 == r3)

    def testClampPoint(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r = Rect2(2, 3, 4, 5)
            self.assertEqual(r.clamp(Vec2(1, 2)), Vec2(2, 3))
            self.assertEqual(r.clamp(Vec2(2, 3)), Vec2(2, 3))
            self.assertEqual(r.clamp(Vec2(3, 4)), Vec2(3, 4))
            self.assertEqual(r.clamp(Vec2(4, 5)), Vec2(4, 5))
            self.assertEqual(r.clamp(Vec2(5, 6)), Vec2(4, 5))
            r = Rect2(4, 5, 2, 3)
            self.assertEqual(r.clamp(Vec2(1, 2)), Vec2(2, 3))
            self.assertEqual(r.clamp(Vec2(2, 3)), Vec2(2, 3))
            self.assertEqual(r.clamp(Vec2(3, 4)), Vec2(3, 4))
            self.assertEqual(r.clamp(Vec2(4, 5)), Vec2(4, 5))
            self.assertEqual(r.clamp(Vec2(5, 6)), Vec2(4, 5))

    def testClampRect(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r = Rect2(3, 4, 5, 6)
            self.assertEqual(r.clamp(Rect2(0, 1, 2, 3)), Rect2(3, 4, 3, 4))
            self.assertEqual(r.clamp(Rect2(1, 2, 3, 4)), Rect2(3, 4, 3, 4))
            self.assertEqual(r.clamp(Rect2(2, 3, 4, 5)), Rect2(3, 4, 4, 5))
            self.assertEqual(r.clamp(Rect2(3, 4, 5, 6)), Rect2(3, 4, 5, 6))
            self.assertEqual(r.clamp(Rect2(4, 5, 6, 7)), Rect2(4, 5, 5, 6))
            self.assertEqual(r.clamp(Rect2(5, 6, 7, 8)), Rect2(5, 6, 5, 6))
            self.assertEqual(r.clamp(Rect2(6, 7, 8, 9)), Rect2(5, 6, 5, 6))
            self.assertEqual(r.clamp(Rect2(4, 5, 4, 5)), Rect2(4, 5, 4, 5))
            self.assertEqual(r.clamp(Rect2(1, 2, 6, 7)), Rect2(3, 4, 5, 6))
            r = Rect2(5, 6, 3, 4)
            self.assertEqual(r.clamp(Rect2(0, 1, 2, 3)), Rect2(3, 4, 3, 4))
            self.assertEqual(r.clamp(Rect2(1, 2, 3, 4)), Rect2(3, 4, 3, 4))
            self.assertEqual(r.clamp(Rect2(2, 3, 4, 5)), Rect2(3, 4, 4, 5))
            self.assertEqual(r.clamp(Rect2(3, 4, 5, 6)), Rect2(3, 4, 5, 6))
            self.assertEqual(r.clamp(Rect2(4, 5, 6, 7)), Rect2(4, 5, 5, 6))
            self.assertEqual(r.clamp(Rect2(5, 6, 7, 8)), Rect2(5, 6, 5, 6))
            self.assertEqual(r.clamp(Rect2(6, 7, 8, 9)), Rect2(5, 6, 5, 6))
            self.assertEqual(r.clamp(Rect2(4, 5, 4, 5)), Rect2(4, 5, 4, 5))
            self.assertEqual(r.clamp(Rect2(1, 2, 6, 7)), Rect2(3, 4, 5, 6))

    def testUnitedWithRect2(self):
        for Rect2 in Rect2Types:
            r1 = Rect2(0, 0, 1, 1)
            r2 = Rect2(2, 2, 3, 3)
            r3 = r1.unitedWith(r2)
            r4 = r1.unitedWith(Rect2.empty)
            self.assertEqual(r3, Rect2(0, 0, 3, 3))
            self.assertEqual(r4, Rect2(0, 0, 1, 1))

            r1 = Rect2(0, 0, 1, 1)
            r2 = Rect2(3, 3, 2, 2)
            r3 = r1.unitedWith(r2)
            self.assertTrue(r2.isEmpty())
            self.assertEqual(r3, Rect2(0, 0, 2, 2))

            r1 = Rect2(0, 0, 1, 1)
            r2 = Rect2(1.9, 1.9, 2, 2)
            r3 = Rect2(2.0, 2.0, 2, 2)
            r4 = Rect2(2.1, 2.1, 2, 2)
            s2 = r1.unitedWith(r2)
            s3 = r1.unitedWith(r3)
            s4 = r1.unitedWith(r4)
            self.assertFalse(r2.isEmpty())
            self.assertFalse(r3.isEmpty())
            self.assertTrue(r4.isEmpty())
            self.assertEqual(s2, Rect2(0, 0, 2, 2))
            self.assertEqual(s3, Rect2(0, 0, 2, 2))
            self.assertEqual(s4, Rect2(0, 0, 2, 2))

    def testUnitedWithVec2(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r1 = Rect2(0, 0, 1, 1)
            r2 = r1.unitedWith(Vec2(2, 2))
            r3 = r1.unitedWith(Vec2(-1, 2))
            self.assertEqual(r2, Rect2(0, 0, 2, 2))
            self.assertEqual(r3, Rect2(-1, 0, 1, 2))

            r1 = Rect2.empty
            r2 = r1.unitedWith(Vec2(1, 2))
            self.assertEqual(r1, Rect2.empty)
            self.assertEqual(r2, Rect2(1, 2, 1, 2))
            self.assertFalse(r2.isEmpty())

    def testUniteWithRect2(self):
        for Rect2 in Rect2Types:
            r1 = Rect2(0, 0, 1, 1)
            r2 = Rect2(2, 2, 3, 3)
            r1.uniteWith(r2)
            self.assertEqual(r1, Rect2(0, 0, 3, 3))

    def testUniteWithVec2(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r1 = Rect2(0, 0, 1, 1)
            r1.uniteWith(Vec2(2, 2))
            self.assertEqual(r1, Rect2(0, 0, 2, 2))
            r1.uniteWith(Vec2(-1, -1))
            self.assertEqual(r1, Rect2(-1, -1, 2, 2))

            r1 = Rect2.empty
            r1.uniteWith(Vec2(1, 2))
            self.assertEqual(r1, Rect2(1, 2, 1, 2))
            self.assertFalse(r1.isEmpty())

    def testIntersectedWith(self):
        for Rect2 in Rect2Types:
            r1 = Rect2(0, 0, 3, 3)
            r2 = Rect2(2, 2, 4, 4)
            r3 = Rect2(5, 5, 6, 6)
            r4 = Rect2(2, 2, 1, 1)
            s2 = r1.intersectedWith(r2)
            s3 = r1.intersectedWith(r3)
            s4 = r1.intersectedWith(r4)
            s5 = r1.intersectedWith(Rect2.empty)
            self.assertTrue(r4.isEmpty())
            self.assertEqual(s2, Rect2(2, 2, 3, 3))
            self.assertEqual(s3, Rect2(5, 5, 3, 3))
            self.assertEqual(s4, Rect2(2, 2, 1, 1))
            self.assertEqual(s5, Rect2.empty)

    def testIntersectWith(self):
        for Rect2 in Rect2Types:
            r1 = Rect2(0, 0, 3, 3)
            r2 = Rect2(2, 2, 4, 4)
            r1.intersectWith(r2)
            self.assertEqual(r1, Rect2(2, 2, 3, 3))

    def testIntersects(self):
        for Rect2 in Rect2Types:
            r1 = Rect2(0, 0, 3, 3)
            r2 = Rect2(2, 2, 4, 4)
            r3 = Rect2(5, 5, 6, 6)
            r4 = Rect2.empty
            self.assertTrue(r1.intersects(r1))
            self.assertTrue(r1.intersects(r2))
            self.assertFalse(r1.intersects(r3))
            self.assertFalse(r1.intersects(r4))

    def testContainsRect2(self):
        for Rect2 in Rect2Types:
            r1 = Rect2(0, 0, 3, 3)
            r2 = Rect2(1, 1, 2, 2)
            r3 = Rect2(2, 2, 4, 4)
            r4 = Rect2(5, 5, 6, 6)
            r5 = Rect2.empty
            self.assertTrue(r1.contains(r1))
            self.assertTrue(r1.contains(r2))
            self.assertFalse(r1.contains(r3))
            self.assertFalse(r1.contains(r4))
            self.assertTrue(r1.contains(r5))

    def testContainsVec2(self):
        for Rect2, Vec2 in Rect2Vec2Types:
            r1 = Rect2(0, 0, 2, 2)
            pointsInside = [
                (0, 0), (1, 0), (2, 0),
                (0, 1), (1, 1), (2, 1),
                (0, 2), (1, 2), (2, 2)]
            pointsOutside = [
                (-1, -1), (0, -1), (1, -1), (2, -1), (3, -1),
                (-1, 0),                             (3, 0),
                (-1, 1),                             (3, 1),
                (-1, 2),                             (3, 2),
                (-1, 3),  (0, 3),  (1, 3),  (2, 3),  (3, 3)]
            for x, y in pointsInside:
                self.assertTrue(r1.contains(Vec2(x, y)))
                self.assertTrue(r1.contains(x, y))
            for x, y in pointsOutside:
                self.assertFalse(r1.contains(Vec2(x, y)))
                self.assertFalse(r1.contains(x, y))


if __name__ == '__main__':
    unittest.main()
