#!/usr/bin/python3

# Copyright 2020 The VGC Developers
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

from vgc.core import Vec2d, Vec2dArray

class TestVec2dArray(unittest.TestCase):

    def testDefaultConstructor(self):
        a = Vec2dArray()
        self.assertEqual(len(a), 0)

    def testInitializingConstructors(self):
        n = 3
        x0 = Vec2d(0.5, 0.7)

        a1 = Vec2dArray(n)
        self.assertEqual(len(a1), n)
        for x in a1:
            self.assertEqual(x, Vec2d(0, 0))

        a2 = Vec2dArray(n, x0)
        self.assertEqual(len(a2), n)
        for x in a2:
            self.assertEqual(x, x0)

        a3 = Vec2dArray([(1, 2), (3, 4)])
        self.assertEqual(len(a3), 2)
        self.assertEqual(a3[0], Vec2d(1, 2))
        self.assertEqual(a3[1], Vec2d(3, 4))

    def testAppend(self):
        a = Vec2dArray()
        a.append(Vec2d(1, 2))
        a.append(Vec2d(3, 4))
        self.assertEqual(a, Vec2dArray([(1, 2), (3, 4)]))

    def testParse(self):
        a = Vec2dArray("[(1, 2), (3, 4.5)]")
        self.assertTrue(a == Vec2dArray([(1, 2), (3, 4.5)]))
        a = Vec2dArray("[]")
        self.assertTrue(a == Vec2dArray())

if __name__ == '__main__':
    unittest.main()
