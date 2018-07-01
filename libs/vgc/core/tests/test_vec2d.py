#!/usr/bin/python3

# Copyright 2018 The VGC Developers
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

from vgc.core import Vec2d

class TestVec2d(unittest.TestCase):

    def testDefaultConstructor(self):
        # Note: in Python, Vec2d() does zero-initialization, unlike in C++
        v = Vec2d()
        self.assertEqual(v.x, 0)
        self.assertEqual(v.y, 0)

    def testInitializingConstructor(self):
        v = Vec2d(12.5, 42)
        self.assertEqual(v.x, 12.5)
        self.assertEqual(v.y, 42)

    def testCopyByReference(self):
        v1 = Vec2d(12.5, 42)
        v2 = v1
        self.assertIs(v1, v2)
        self.assertEqual(v1, v2)
        v2.x = 15
        self.assertEqual(v1.x, 15)
        self.assertEqual(v1, v2)

    def testCopyByValue(self):
        v1 = Vec2d(12.5, 42)
        v2 = Vec2d(v1)
        self.assertIsNot(v1, v2)
        self.assertEqual(v1, v2)
        v2.x = 15
        self.assertEqual(v1.x, 12.5)
        self.assertNotEqual(v1, v2)

    def testBracketOperator(self):
        v = Vec2d(12.5, 42)
        self.assertEqual(v[0], 12.5)
        self.assertEqual(v[1], 42)
        v[0] = 13.5
        v[1] += 1
        self.assertEqual(v[0], 13.5)
        self.assertEqual(v[1], 43)

    def testXY(self):
        v = Vec2d(12.5, 42)
        self.assertEqual(v.x, 12.5)
        self.assertEqual(v.y, 42)
        v.x = 13.5
        v.y += 1
        self.assertEqual(v.x, 13.5)
        self.assertEqual(v.y, 43)

    def testArithmeticOperators(self):
        v1 = Vec2d(1, 2)
        v1 += Vec2d(3, 4)
        self.assertEqual(v1.x, 4)
        self.assertEqual(v1.y, 6)
        self.assertEqual(v1, Vec2d(4, 6))
        v2 = v1 + Vec2d(1, 1)
        self.assertEqual(v1, Vec2d(4, 6))
        self.assertEqual(v2, Vec2d(5, 7))
        v2 -= Vec2d(3, 2)
        self.assertEqual(v2, Vec2d(2, 5))
        v1 = v1 - v2;
        self.assertEqual(v1, Vec2d(2, 1))
        v1 *= 2
        self.assertEqual(v1, Vec2d(4, 2))
        v3 = 3 * v1 * 2
        self.assertEqual(v3, Vec2d(24, 12))
        v3 /= 2
        self.assertEqual(v3, Vec2d(12, 6))
        v4 = v3 / 12
        self.assertEqual(v4, Vec2d(1, 0.5))

    def testComparisonOperators(self):
        v1 = Vec2d(1, 2)
        v2 = Vec2d(3, 4)
        self.assertTrue(v1 == v1)
        self.assertTrue(v1 == Vec2d(1, 2))
        self.assertTrue(v1 != v2)
        self.assertTrue(v1 != Vec2d(3, 4))
        self.assertTrue(v1 < v2)
        self.assertTrue(v1 < Vec2d(2, 0))
        self.assertTrue(v1 < Vec2d(1, 3))
        self.assertTrue(v1 <= v1)
        self.assertTrue(v1 <= v2)
        self.assertTrue(v1 <= Vec2d(2, 0))
        self.assertTrue(v1 <= Vec2d(1, 3))
        self.assertTrue(v2 > v1)
        self.assertTrue(v2 > Vec2d(2, 100))
        self.assertTrue(v2 >= v2)
        self.assertTrue(v2 >= v1)
        self.assertTrue(v2 >= Vec2d(2, 100))

    def testLength(self):
        for v in [Vec2d(3, 4), Vec2d(-3, 4), Vec2d(3, -4), Vec2d(-3, -4)]:
            self.assertEqual(v.length(), 5)
            self.assertEqual(v.squaredLength(), 25)

    def testNormalize(self):
        v = Vec2d(3, 4)
        v.normalize()
        self.assertEqual(v.length(), 1)
        self.assertEqual(v, Vec2d(0.6, 0.8))

    def testNormalized(self):
        v1 = Vec2d(3, 4)
        v2 = v1.normalized()
        self.assertEqual(v2.length(), 1)
        self.assertEqual(v1, Vec2d(3, 4))
        self.assertEqual(v2, Vec2d(0.6, 0.8))

    def testOrthogonalize(self):
        v = Vec2d(3, 4)
        v.orthogonalize()
        self.assertEqual(v, Vec2d(-4, 3))

    def testOrthogonalized(self):
        v1 = Vec2d(3, 4)
        v2 = v1.orthogonalized()
        self.assertEqual(v1, Vec2d(3, 4))
        self.assertEqual(v2, Vec2d(-4, 3))

if __name__ == '__main__':
    unittest.main()
