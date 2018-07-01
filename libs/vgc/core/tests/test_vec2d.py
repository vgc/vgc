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
        v = Vec2d()
        # Note: no zero-initialization happening.
        # XXX Should Python bindings do zero-initialization, unlike
        # the C++ version?

    def testInitializingConstructor(self):
        v = Vec2d(12.5, 42)
        self.assertEqual(v.x, 12.5)
        self.assertEqual(v.y, 42)

    def testCopyByReference(self):
        v1 = Vec2d(12.5, 42)
        v2 = v1
        self.assertEqual(v2.x, 12.5)
        self.assertEqual(v2.y, 42)
        self.assertEqual(v2, v1)
        self.assertIs(v2, v1)
        v2.x = 15
        self.assertEqual(v1.x, 15)
        self.assertEqual(v2, v1)
        self.assertIs(v2, v1)

    def testCopyByValue(self):
        v1 = Vec2d(12.5, 42)
        v2 = Vec2d(v1)
        self.assertEqual(v2.x, 12.5)
        self.assertEqual(v2.y, 42)
        #self.assertEqual(v2, v1) # XXX Fail for now TODO Wrap equality operator
        self.assertIsNot(v2, v1)
        v2.x = 15
        self.assertEqual(v1.x, 12.5)
        self.assertNotEqual(v2, v1)
        self.assertIsNot(v2, v1)

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

    def testArithmetic(self):
        v1 = Vec2d(1, 2)
        v1 += Vec2d(3, 4)
        self.assertEqual(v1.x, 4)
        self.assertEqual(v1.y, 6)
        #self.assertEqual(v1, Vec2d(4, 6)) XXX fails due to lack of equality operator wrapper
        # XXX TODO test other operators once equality wrapper is written

    def testLength(self):
        v = Vec2d(3, 4)
        self.assertEqual(v.length(), 5)
        self.assertEqual(v.squaredLength(), 25)
        v.normalize()
        self.assertEqual(v.length(), 1)
        self.assertEqual(v.squaredLength(), 1)
        self.assertEqual(v.x, 0.6)
        self.assertEqual(v.y, 0.8)

    def testNormalize(self):
        v = Vec2d(3, 4)
        v.normalize()
        self.assertEqual(v.length(), 1)
        self.assertEqual(v.squaredLength(), 1)
        self.assertEqual(v.x, 0.6)
        self.assertEqual(v.y, 0.8)

    def testNormalized(self):
        v1 = Vec2d(3, 4)
        v2 = v1.normalized()
        self.assertEqual(v1.length(), 5)
        self.assertEqual(v2.length(), 1)
        self.assertEqual(v2.x, 0.6)
        self.assertEqual(v2.y, 0.8)

    def testOrthogonalize(self):
        v = Vec2d(3, 4)
        v.orthogonalize()
        self.assertEqual(v.x, -4)
        self.assertEqual(v.y, 3)

    def testOrthogonalized(self):
        v1 = Vec2d(3, 4)
        v2 = v1.orthogonalized()
        self.assertEqual(v1.x, 3)
        self.assertEqual(v1.y, 4)
        self.assertEqual(v2.x, -4)
        self.assertEqual(v2.y, 3)

if __name__ == '__main__':
    unittest.main()
