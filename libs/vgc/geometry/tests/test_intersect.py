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

from vgc.geometry import Vec2d, fastSegmentIntersects, fastSemiOpenSegmentIntersects

class TestVec2(unittest.TestCase):

    def testFastSegmentIntersects(self):
        a = Vec2d(-0.3, -0.2)
        b = Vec2d(1.7, 1.4)
        c = Vec2d(0.2, 1.6)
        d = Vec2d(1.2, -0.4)

        e = Vec2d(0.5, 0.2)
        f = Vec2d(0.5, 0.4)
        g = Vec2d(0.5, 0.6)
        h = Vec2d(1.1, 0.4)

        self.assertTrue(fastSegmentIntersects(a, b, c, d))
        self.assertTrue(fastSegmentIntersects(a, b, d, c))
        self.assertTrue(fastSegmentIntersects(b, a, c, d))
        self.assertTrue(fastSegmentIntersects(b, a, d, c))
        # collinearity is not considered as intersection.
        self.assertFalse(fastSegmentIntersects(a, c, a, c))
        self.assertFalse(fastSegmentIntersects(c, a, c, a))
        # intersecting at end points does count.
        self.assertTrue(fastSegmentIntersects(a, c, a, d))
        self.assertTrue(fastSegmentIntersects(e, g, f, h))
        self.assertTrue(fastSegmentIntersects(f, h, e, g))
        self.assertTrue(fastSegmentIntersects(c, a, d, a))
        self.assertTrue(fastSegmentIntersects(e, g, h, f))
        self.assertTrue(fastSegmentIntersects(h, f, e, g))

    def fastSemiOpenSegmentIntersects(self):
        a = Vec2d(-0.3, -0.2)
        b = Vec2d(1.7, 1.4)
        c = Vec2d(0.2, 1.6)
        d = Vec2d(1.2, -0.4)

        e = Vec2d(0.5, 0.2)
        f = Vec2d(0.5, 0.4)
        g = Vec2d(0.5, 0.6)
        h = Vec2d(1.1, 0.4)

        self.assertTrue(fastSemiOpenSegmentIntersects(a, b, c, d))
        self.assertTrue(fastSemiOpenSegmentIntersects(a, b, d, c))
        self.assertTrue(fastSemiOpenSegmentIntersects(b, a, c, d))
        self.assertTrue(fastSemiOpenSegmentIntersects(b, a, d, c))
        # collinearity is not considered as intersection.
        self.assertFalse(fastSemiOpenSegmentIntersects(a, c, a, c))
        self.assertFalse(fastSemiOpenSegmentIntersects(c, a, c, a))
        # intersecting at start point does count.
        self.assertTrue(fastSemiOpenSegmentIntersects(a, c, a, d))
        self.assertTrue(fastSemiOpenSegmentIntersects(e, g, f, h))
        self.assertTrue(fastSemiOpenSegmentIntersects(f, h, e, g))
        # intersecting at end point does not count.
        self.assertFalse(fastSemiOpenSegmentIntersects(c, a, d, a))
        self.assertFalse(fastSemiOpenSegmentIntersects(e, g, h, f))
        self.assertFalse(fastSemiOpenSegmentIntersects(h, f, e, g))

if __name__ == '__main__':
    unittest.main()
