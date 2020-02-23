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

import math
import unittest

from vgc.core import isClose, isNear

class TestArithmetic(unittest.TestCase):

    def testIsClose(self):
        relTol = 0.05
        absTol = 0.05
        self.assertTrue(isClose(0.0, 0.0, relTol))
        self.assertTrue(isClose(42.0, 42.0, relTol))
        self.assertTrue(isClose(0.0, 0.0, 0.0))
        self.assertTrue(isClose(42.0, 42.0, 0.0))
        self.assertTrue(isClose(101.0,  103.0,  relTol))
        self.assertTrue(isClose(1.01,   1.03,   relTol))
        self.assertTrue(isClose(0.0101, 0.0103, relTol))
        self.assertFalse(isClose(101.0,  108.0,  relTol))
        self.assertFalse(isClose(1.01,   1.08,   relTol))
        self.assertFalse(isClose(0.0101, 0.0108, relTol))
        self.assertFalse(isClose(1e-100, 0.0))
        self.assertFalse(isClose(1e-100, 0.0, relTol))
        self.assertTrue(isClose(1e-100, 0.0, relTol, absTol))
        self.assertTrue(isClose(math.inf, math.inf))
        self.assertTrue(isClose(-math.inf, -math.inf))
        self.assertFalse(isClose(math.inf, -math.inf))
        self.assertFalse(isClose(-math.inf, math.inf))
        self.assertFalse(isClose(math.inf, 1.0))
        self.assertFalse(isClose(-math.inf, 1.0))
        self.assertFalse(isClose(math.inf, -1.0))
        self.assertFalse(isClose(-math.inf, -1.0))
        self.assertFalse(isClose(math.nan, 1.0))
        self.assertFalse(isClose(math.nan, math.inf))
        self.assertFalse(isClose(math.nan, math.nan))
        self.assertTrue(isClose(math.inf, math.inf, relTol))
        self.assertTrue(isClose(-math.inf, -math.inf, relTol))
        self.assertFalse(isClose(math.inf, -math.inf, relTol))
        self.assertFalse(isClose(-math.inf, math.inf, relTol))
        self.assertFalse(isClose(math.inf, 1.0, relTol))
        self.assertFalse(isClose(-math.inf, 1.0, relTol))
        self.assertFalse(isClose(math.inf, -1.0, relTol))
        self.assertFalse(isClose(-math.inf, -1.0, relTol))
        self.assertFalse(isClose(math.nan, 1.0, relTol))
        self.assertFalse(isClose(math.nan, math.inf, relTol))
        self.assertFalse(isClose(math.nan, math.nan, relTol))

    def testIsNear(self):
        absTol = 0.05
        self.assertTrue(isNear(0.0, 0.0, absTol))
        self.assertTrue(isNear(42.0, 42.0, absTol))
        self.assertTrue(isNear(0.0, 0.0, 0.0))
        self.assertTrue(isNear(42.0, 42.0, 0.0))
        self.assertFalse(isNear(101.0,  103.0,  absTol))
        self.assertTrue(isNear(1.01,   1.03,   absTol))
        self.assertTrue(isNear(0.0101, 0.0103, absTol))
        self.assertFalse(isNear(101.0,  108.0,  absTol))
        self.assertFalse(isNear(1.01,   1.08,   absTol))
        self.assertTrue(isNear(0.0101, 0.0108, absTol))
        self.assertTrue(isNear(1e-100, 0.0, absTol))
        self.assertTrue(isNear(math.inf, math.inf, absTol))
        self.assertTrue(isNear(-math.inf, -math.inf, absTol))
        self.assertTrue(isNear(math.inf, math.inf, 0.0))
        self.assertTrue(isNear(-math.inf, -math.inf, 0.0))
        self.assertFalse(isNear(math.inf, -math.inf, absTol))
        self.assertFalse(isNear(-math.inf, math.inf, absTol))
        self.assertFalse(isNear(math.inf, 1.0, absTol))
        self.assertFalse(isNear(-math.inf, 1.0, absTol))
        self.assertFalse(isNear(math.inf, -1.0, absTol))
        self.assertFalse(isNear(-math.inf, -1.0, absTol))
        self.assertFalse(isNear(math.nan, 1.0, absTol))
        self.assertFalse(isNear(math.nan, math.inf, absTol))
        self.assertFalse(isNear(math.nan, math.nan, absTol))

if __name__ == '__main__':
    unittest.main()
