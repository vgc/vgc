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

from vgc.geometry import Vec2d, fastIntersects

class TestVec2(unittest.TestCase):

    def testFastIntersects(self):
        a = Vec2d(-0.3, -0.2)
        b = Vec2d(1.7, 1.4)
        c = Vec2d(0.2, 1.6)
        d = Vec2d(1.2, -0.4)

        e = Vec2d(0.5, 0.2)
        f = Vec2d(0.5, 0.4)
        g = Vec2d(0.5, 0.6)
        h = Vec2d(1.1, 0.4)

        self.assertTrue(fastIntersects(a, b, c, d))
        self.assertTrue(fastIntersects(a, b, d, c))
        self.assertTrue(fastIntersects(b, a, c, d))
        self.assertTrue(fastIntersects(b, a, d, c))
        # colinearity is not considered as intersection.
        self.assertFalse(fastIntersects(a, c, a, c))
        self.assertFalse(fastIntersects(c, a, c, a))
        # intersecting at start point counts.
        self.assertTrue(fastIntersects(a, c, a, d))
        self.assertTrue(fastIntersects(e, g, f, h))
        self.assertTrue(fastIntersects(f, h, e, g))
        # intersecting at end point does not count.
        self.assertFalse(fastIntersects(c, a, d, a))
        self.assertFalse(fastIntersects(e, g, h, f))
        self.assertFalse(fastIntersects(h, f, e, g))

if __name__ == '__main__':
    unittest.main()
