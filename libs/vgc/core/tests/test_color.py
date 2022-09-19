#!/usr/bin/python3

# Copyright 2021 The VGC Developers
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

from vgc.core import Color


class TestColor(unittest.TestCase):

    def testDefaultConstructor(self):
        # Note: in Python, Color() does zero-initialization, unlike in C++
        c = Color()
        self.assertEqual(c[0], 0)
        self.assertEqual(c[1], 0)
        self.assertEqual(c[2], 0)
        self.assertEqual(c[3], 1)

    def testInitializingConstructor(self):
        c1 = Color(0.25, 0.375, 0.875)
        self.assertEqual(c1[0], 0.25)
        self.assertEqual(c1[1], 0.375)
        self.assertEqual(c1[2], 0.875)
        self.assertEqual(c1[3], 1.0)
        c2 = Color(0.25, 0.375, 0.875, 0.5)
        self.assertEqual(c2[0], 0.25)
        self.assertEqual(c2[1], 0.375)
        self.assertEqual(c2[2], 0.875)
        self.assertEqual(c2[3], 0.5)

    def testCopyByReference(self):
        c1 = Color(12.5, 42, 0.5)
        c2 = c1
        self.assertIs(c1, c2)
        self.assertEqual(c1, c2)
        c2[0] = 15
        c2.g = 10
        self.assertEqual(c1[0], 15)
        self.assertEqual(c1[1], 10)
        self.assertEqual(c1, c2)

    def testCopyByValue(self):
        c1 = Color(12.5, 42, 0.5)
        c2 = Color(c1)
        self.assertIsNot(c1, c2)
        self.assertEqual(c1, c2)
        c2[0] = 15
        c2.g = 10
        self.assertEqual(c1[0], 12.5)
        self.assertEqual(c1[1], 42)
        self.assertNotEqual(c1, c2)

    def testBracketOperator(self):
        c = Color(12.5, 42, 0.75, 0.5)
        self.assertEqual(c[0], 12.5)
        self.assertEqual(c[1], 42)
        self.assertEqual(c[2], 0.75)
        self.assertEqual(c[3], 0.5)
        c[0] = 13.5
        c[1] += 1
        c[2] += 2
        c[3] += 3
        self.assertEqual(c[0], 13.5)
        self.assertEqual(c[1], 43)
        self.assertEqual(c[2], 2.75)
        self.assertEqual(c[3], 3.5)

    def testNamedComponents(self):
        c = Color(12.5, 42, 0.75, 0.5)
        self.assertEqual(c.r, 12.5)
        self.assertEqual(c.g, 42)
        self.assertEqual(c.b, 0.75)
        self.assertEqual(c.a, 0.5)
        c.r = 13.5
        c.g += 1
        c.b += 2
        c.a += 3
        self.assertEqual(c.r, 13.5)
        self.assertEqual(c.g, 43)
        self.assertEqual(c.b, 2.75)
        self.assertEqual(c.a, 3.5)

    def testArithmeticOperators(self):
        c1 = Color(1, 10, 100, 1000)
        c1 += Color(2, 20, 200, 2000)
        self.assertEqual(c1, Color(3, 30, 300, 3000))
        c2 = c1 + Color(1, 10, 100, 1000)
        self.assertEqual(c1, Color(3, 30, 300, 3000))
        self.assertEqual(c2, Color(4, 40, 400, 4000))
        c2 -= Color(3, 30, 300, 3000)
        self.assertEqual(c2, Color(1, 10, 100, 1000))
        c1 = c1 - c2
        self.assertEqual(c1, Color(2, 20, 200, 2000))
        c1 *= 2
        self.assertEqual(c1, Color(4, 40, 400, 4000))
        c3 = 0.5 * c1 * 4
        self.assertEqual(c3, Color(8, 80, 800, 8000))
        c3 /= 2
        self.assertEqual(c3, Color(4, 40, 400, 4000))
        c4 = c3 / 8
        self.assertEqual(c4, Color(0.5, 5, 50, 500))

    def testComparisonOperators(self):
        c1 = Color(1, 2, 0)
        c2 = Color(3, 4, 0)
        self.assertTrue(c1 == c1)
        self.assertTrue(c1 == Color(1, 2, 0))
        self.assertTrue(c1 != c2)
        self.assertTrue(c1 != Color(3, 4, 0))
        self.assertTrue(c1 < c2)
        self.assertTrue(c1 < Color(2, 0, 0))
        self.assertTrue(c1 < Color(1, 3, 0))
        self.assertTrue(c1 <= c1)
        self.assertTrue(c1 <= c2)
        self.assertTrue(c1 <= Color(2, 0, 0))
        self.assertTrue(c1 <= Color(1, 3, 0))
        self.assertTrue(c2 > c1)
        self.assertTrue(c2 > Color(2, 100, 0))
        self.assertTrue(c2 >= c2)
        self.assertTrue(c2 >= c1)
        self.assertTrue(c2 >= Color(2, 100, 0))

    def testParse(self):
        c1 = Color("rgb(1, 2, 3)")
        c2 = Color("rgba(1, 2, 3, 0.5)")
        self.assertTrue(c1 == Color(1.0 / 255.0, 2.0 / 255.0, 3.0 / 255.0))
        self.assertTrue(c2 == Color(1.0 / 255.0, 2.0 / 255.0, 3.0 / 255.0, 0.5))


if __name__ == '__main__':
    unittest.main()
