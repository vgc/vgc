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
from vgc.geometry import Range1d, Range1f

Range1Types = [Range1d, Range1f]

class TestRange(unittest.TestCase):

    def testDefaultConstructor(self):
        for Range1 in Range1Types:
            r = Range1()
            self.assertEqual(r.pMin, 0)
            self.assertEqual(r.pMax, 0)

    def testMinMaxConstructor(self):
        for Range1 in Range1Types:
            r = Range1(1, 3)
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 3)

            r = Range1(3, 1)
            self.assertEqual(r.pMin, 3)
            self.assertEqual(r.pMax, 1)

    def testPositionSizeConstructor(self):
        for Range1 in Range1Types:
            r = Range1(position=1, size=3)
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 4)

            r = Range1(position=1, size=-3)
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, -2)

    def testCopyByReference(self):
        for Range1 in Range1Types:
            r = Range1(1, 3)
            s = r
            self.assertIs(r, s)
            self.assertEqual(r, s)
            r.pMin = 5
            self.assertEqual(r.pMin, 5)
            self.assertEqual(s.pMin, 5)
            self.assertEqual(r, s)

    def testCopyByValue(self):
        for Range1 in Range1Types:
            r = Range1(1, 3)
            s = Range1(r)
            self.assertIsNot(r, s)
            self.assertEqual(r, s)
            r.pMin = 5
            self.assertEqual(r.pMin, 5)
            self.assertEqual(s.pMin, 1)
            self.assertNotEqual(r, s)

    def testEmpty(self):
        for Range1 in Range1Types:
            r = Range1.empty
            self.assertTrue(r.isEmpty())
            self.assertTrue( Range1.empty.isEmpty())
            self.assertFalse(Range1().isEmpty())
            self.assertFalse(Range1(1, 3).isEmpty())
            self.assertTrue( Range1(3, 1).isEmpty())
            self.assertFalse(Range1(3, 1).normalized().isEmpty())
            self.assertFalse(Range1(position=1, size= 1).isEmpty())
            self.assertTrue( Range1(position=1, size=-1).isEmpty())
            self.assertFalse(Range1(position=1, size= 1).normalized().isEmpty())
            self.assertFalse(Range1(position=1, size=-1).normalized().isEmpty())

    def testNormalize(self):
        for Range1 in Range1Types:
            r = Range1(1, 3)
            r.normalize()
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 3)

            r = Range1(3, 1)
            r.normalize()
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 3)

    def testNormalized(self):
        for Range1 in Range1Types:
            r = Range1(1, 3).normalized()
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 3)

            r = Range1(3, 1).normalized()
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 3)

    def testPositionAndSize(self):
        for Range1 in Range1Types:
            r = Range1(1, 4)
            self.assertEqual(r.position, 1)
            self.assertEqual(r.size, 3)
            self.assertEqual(r.pMax, 4)

            r.position = 5
            self.assertEqual(r.position, 5)
            self.assertEqual(r.size, 3)
            self.assertEqual(r.pMax, 8)

            r.size = 5
            self.assertEqual(r.position, 5)
            self.assertEqual(r.size, 5)
            self.assertEqual(r.pMax, 10)

    def testPMin(self):
        for Range1 in Range1Types:
            r = Range1(1, 4)
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 4)
            self.assertEqual(r.size, 3)

            r.pMin = 5
            self.assertEqual(r.pMin, 5)
            self.assertEqual(r.pMax, 4)
            self.assertEqual(r.size, -1)

    def testPMax(self):
        for Range1 in Range1Types:
            r = Range1(1, 4)
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 4)
            self.assertEqual(r.size, 3)

            r.pMax = 5
            self.assertEqual(r.pMin, 1)
            self.assertEqual(r.pMax, 5)
            self.assertEqual(r.size, 4)

    def testIsClose(self):
        for Range1 in Range1Types:
            self.assertTrue(Range1(0, 1).isClose(Range1(0, 1)))
            self.assertTrue(Range1(0, 1).isClose(Range1(0, 1 + 1e-10)))
            self.assertFalse(Range1(0, 1).isClose(Range1(1e-10, 1)))
            self.assertFalse(Range1(0, 1).isClose(Range1(0.0009, 1)))
            self.assertFalse(Range1(0, 1).isClose(Range1(0.002, 1)))

    def testIsNear(self):
        absTol = 0.001
        for Range1 in Range1Types:
            self.assertTrue(Range1(0, 1).isNear(Range1(0, 1), absTol))
            self.assertTrue(Range1(0, 1).isNear(Range1(0, 1 + 1e-10), absTol))
            self.assertTrue(Range1(0, 1).isNear(Range1(1e-10, 1), absTol))
            self.assertTrue(Range1(0, 1).isNear(Range1(0.0009, 1), absTol))
            self.assertFalse(Range1(0, 1).isNear(Range1(0.002, 1), absTol))

    def testComparisonOperators(self):
        for Range1 in Range1Types:
            r1 = Range1(1, 3)
            r2 = Range1(position=1, size=2)
            r3 = Range1(1, 4)
            self.assertTrue(r1 == r2)
            self.assertTrue(r1 != r3)
            self.assertFalse(r1 != r2)
            self.assertFalse(r1 == r3)

    def testUnitedWithRange1(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 1)
            r2 = Range1(2, 3)
            r3 = r1.unitedWith(r2)
            r4 = r1.unitedWith(Range1.empty)
            self.assertEqual(r3, Range1(0, 3))
            self.assertEqual(r4, Range1(0, 1))

            r1 = Range1(0, 1)
            r2 = Range1(3, 2)
            r3 = r1.unitedWith(r2)
            self.assertTrue(r2.isEmpty())
            self.assertEqual(r3, Range1(0, 2))

            r1 = Range1(0, 1)
            r2 = Range1(1.9, 2)
            r3 = Range1(2.0, 2)
            r4 = Range1(2.1, 2)
            s2 = r1.unitedWith(r2)
            s3 = r1.unitedWith(r3)
            s4 = r1.unitedWith(r4)
            self.assertFalse(r2.isEmpty())
            self.assertFalse(r3.isEmpty())
            self.assertTrue(r4.isEmpty())
            self.assertEqual(s2, Range1(0, 2))
            self.assertEqual(s3, Range1(0, 2))
            self.assertEqual(s4, Range1(0, 2))

    def testUnitedWithFloat(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 1)
            r2 = r1.unitedWith(2)
            r3 = r1.unitedWith(-1)
            self.assertEqual(r2, Range1(0, 2))
            self.assertEqual(r3, Range1(-1, 1))

            r1 = Range1.empty
            r2 = r1.unitedWith(1)
            self.assertEqual(r1, Range1.empty)
            self.assertEqual(r2, Range1(1, 1))
            self.assertFalse(r2.isEmpty())

    def testUniteWithRange1(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 1)
            r2 = Range1(2, 3)
            r1.uniteWith(r2)
            self.assertEqual(r1, Range1(0, 3))

    def testUniteWithFloat(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 1)
            r1.uniteWith(2)
            self.assertEqual(r1, Range1(0, 2))
            r1.uniteWith(-1)
            self.assertEqual(r1, Range1(-1, 2))

            r1 = Range1.empty
            r1.uniteWith(1)
            self.assertEqual(r1, Range1(1, 1))
            self.assertFalse(r1.isEmpty())

    def testIntersectedWith(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 3)
            r2 = Range1(2, 4)
            r3 = Range1(5, 6)
            r4 = Range1(2, 1)
            s2 = r1.intersectedWith(r2)
            s3 = r1.intersectedWith(r3)
            s4 = r1.intersectedWith(r4)
            s5 = r1.intersectedWith(Range1.empty)
            self.assertTrue(r4.isEmpty())
            self.assertEqual(s2, Range1(2, 3))
            self.assertEqual(s3, Range1(5, 3))
            self.assertEqual(s4, Range1(2, 1))
            self.assertEqual(s5, Range1.empty)

    def testIntersectWith(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 3)
            r2 = Range1(2, 4)
            r1.intersectWith(r2)
            self.assertEqual(r1, Range1(2, 3))

    def testIntersects(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 3)
            r2 = Range1(2, 4)
            r3 = Range1(5, 6)
            r4 = Range1.empty
            self.assertTrue(r1.intersects(r1))
            self.assertTrue(r1.intersects(r2))
            self.assertFalse(r1.intersects(r3))
            self.assertFalse(r1.intersects(r4))

    def testContainsRange1(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 3)
            r2 = Range1(1, 2)
            r3 = Range1(2, 4)
            r4 = Range1(5, 6)
            r5 = Range1.empty
            self.assertTrue(r1.contains(r1))
            self.assertTrue(r1.contains(r2))
            self.assertFalse(r1.contains(r3))
            self.assertFalse(r1.contains(r4))
            self.assertTrue(r1.contains(r5))

    def testContainsVec2(self):
        for Range1 in Range1Types:
            r1 = Range1(0, 2)
            pointsInside = [0, 1, 2]
            pointsOutside = [-1, 3]
            for p in pointsInside:
                self.assertTrue(r1.contains(p))
            for p in pointsOutside:
                self.assertFalse(r1.contains(p))

if __name__ == '__main__':
    unittest.main()
