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

from vgc.geometry import Triangle2d, Triangle2f, Vec2d, Vec2f

Triangle2Types = [Triangle2d, Triangle2f]
Triangle2Vec2Types = [(Triangle2d, Vec2d), (Triangle2f, Vec2f)]

class TestVec2(unittest.TestCase):

    def testDefaultConstructor(self):
        for Triangle2 in Triangle2Types:
            t = Triangle2()
            self.assertEqual(t.a.x, 0)
            self.assertEqual(t.a.y, 0)
            self.assertEqual(t.b.x, 0)
            self.assertEqual(t.b.y, 0)
            self.assertEqual(t.c.x, 0)
            self.assertEqual(t.c.y, 0)

    def testContains(self):
        for (Triangle2, Vec2) in Triangle2Vec2Types:
            a = Vec2(0, 0)
            b = Vec2(10, 0)
            c = Vec2(0, 10)
            t1 = Triangle2(a, b, c)
            t2 = Triangle2(a, c, b)

            # Inside
            self.assertTrue(t1.contains(Vec2(1, 1)))
            self.assertTrue(t2.contains(Vec2(1, 1)))

            # Border
            self.assertTrue(t1.contains(Vec2(0, 0)))
            self.assertTrue(t2.contains(Vec2(0, 0)))
            self.assertTrue(t1.contains(Vec2(5, 0)))
            self.assertTrue(t2.contains(Vec2(5, 0)))
            self.assertTrue(t1.contains(Vec2(10, 0)))
            self.assertTrue(t2.contains(Vec2(10, 0)))
            self.assertTrue(t1.contains(Vec2(5, 5)))
            self.assertTrue(t2.contains(Vec2(5, 5)))
            self.assertTrue(t1.contains(Vec2(0, 10)))
            self.assertTrue(t2.contains(Vec2(0, 10)))
            self.assertTrue(t1.contains(Vec2(0, 5)))
            self.assertTrue(t2.contains(Vec2(0, 5)))

            # Outside
            self.assertFalse(t1.contains(Vec2(-5, -5)))
            self.assertFalse(t2.contains(Vec2(-5, -5)))
            self.assertFalse(t1.contains(Vec2(0, -5)))
            self.assertFalse(t2.contains(Vec2(0, -5)))
            self.assertFalse(t1.contains(Vec2(5, -5)))
            self.assertFalse(t2.contains(Vec2(5, -5)))
            self.assertFalse(t1.contains(Vec2(10, -5)))
            self.assertFalse(t2.contains(Vec2(10, -5)))
            self.assertFalse(t1.contains(Vec2(15, -5)))
            self.assertFalse(t2.contains(Vec2(15, -5)))
            self.assertFalse(t1.contains(Vec2(15, 0)))
            self.assertFalse(t2.contains(Vec2(15, 0)))
            self.assertFalse(t1.contains(Vec2(15, 5)))
            self.assertFalse(t2.contains(Vec2(15, 5)))
            self.assertFalse(t1.contains(Vec2(15, 10)))
            self.assertFalse(t2.contains(Vec2(15, 10)))
            self.assertFalse(t1.contains(Vec2(15, 15)))
            self.assertFalse(t2.contains(Vec2(15, 15)))
            self.assertFalse(t1.contains(Vec2(10, 15)))
            self.assertFalse(t2.contains(Vec2(10, 15)))
            self.assertFalse(t1.contains(Vec2(5, 15)))
            self.assertFalse(t2.contains(Vec2(5, 15)))
            self.assertFalse(t1.contains(Vec2(0, 15)))
            self.assertFalse(t2.contains(Vec2(0, 15)))
            self.assertFalse(t1.contains(Vec2(-5, 15)))
            self.assertFalse(t2.contains(Vec2(-5, 15)))
            self.assertFalse(t1.contains(Vec2(-5, 10)))
            self.assertFalse(t2.contains(Vec2(-5, 10)))
            self.assertFalse(t1.contains(Vec2(-5, 5)))
            self.assertFalse(t2.contains(Vec2(-5, 5)))
            self.assertFalse(t1.contains(Vec2(-5, 0)))
            self.assertFalse(t2.contains(Vec2(-5, 0)))

            # Degenerate case: the three points are equal
            a = Vec2(1, 2)
            b = Vec2(1, 2)
            c = Vec2(1, 2)
            t1 = Triangle2(a, b, c)
            self.assertTrue(t1.contains(Vec2(1, 2)))
            self.assertFalse(t1.contains(Vec2(0, 0)))
            self.assertFalse(t1.contains(Vec2(2, 1)))

            # Degenerate case: two points are equal
            a = Vec2(1, 2)
            b = Vec2(1, 2)
            c = Vec2(3, 4)
            t1 = Triangle2(a, b, c)
            t2 = Triangle2(a, c, b)
            self.assertTrue(t1.contains(Vec2(1, 2)))
            self.assertTrue(t2.contains(Vec2(1, 2)))
            self.assertTrue(t1.contains(Vec2(2, 3)))
            self.assertTrue(t2.contains(Vec2(2, 3)))
            self.assertTrue(t1.contains(Vec2(3, 4)))
            self.assertTrue(t2.contains(Vec2(3, 4)))
            self.assertFalse(t1.contains(Vec2(0, 0)))
            self.assertFalse(t2.contains(Vec2(0, 0)))
            self.assertFalse(t1.contains(Vec2(0, 1)))
            self.assertFalse(t2.contains(Vec2(0, 1)))
            self.assertFalse(t1.contains(Vec2(4, 5)))
            self.assertFalse(t2.contains(Vec2(4, 5)))
            self.assertFalse(t1.contains(Vec2(3, 2)))
            self.assertFalse(t2.contains(Vec2(3, 2)))
            self.assertFalse(t1.contains(Vec2(1, 4)))
            self.assertFalse(t2.contains(Vec2(1, 4)))

            # Degenerate case: the three points are different but aligned
            a = Vec2(1, 2)
            b = Vec2(3, 4)
            c = Vec2(4, 5)
            t1 = Triangle2(a, b, c)
            t2 = Triangle2(a, c, b)
            self.assertTrue(t1.contains(Vec2(1, 2)))
            self.assertTrue(t2.contains(Vec2(1, 2)))
            self.assertTrue(t1.contains(Vec2(2, 3)))
            self.assertTrue(t2.contains(Vec2(2, 3)))
            self.assertTrue(t1.contains(Vec2(3, 4)))
            self.assertTrue(t2.contains(Vec2(3, 4)))
            self.assertTrue(t1.contains(Vec2(4, 5)))
            self.assertTrue(t2.contains(Vec2(4, 5)))
            self.assertFalse(t1.contains(Vec2(0, 1)))
            self.assertFalse(t2.contains(Vec2(0, 1)))
            self.assertFalse(t1.contains(Vec2(5, 6)))
            self.assertFalse(t2.contains(Vec2(5, 6)))
            self.assertFalse(t1.contains(Vec2(3, 2)))
            self.assertFalse(t2.contains(Vec2(3, 2)))
            self.assertFalse(t1.contains(Vec2(1, 4)))
            self.assertFalse(t2.contains(Vec2(1, 4)))


if __name__ == '__main__':
    unittest.main()
