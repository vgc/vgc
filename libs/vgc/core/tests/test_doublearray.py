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

from vgc.core import DoubleArray, toDoubleArray

class TestDoubleArray(unittest.TestCase):

    def testDefaultConstructor(self):
        a = DoubleArray()
        self.assertEqual(len(a), 0)

    def testInitializingConstructors(self):
        n = 3
        x0 = 0.5

        a1 = DoubleArray(n)
        self.assertEqual(len(a1), n)
        for x in a1:
            self.assertEqual(x, 0)

        a2 = DoubleArray(n, x0)
        self.assertEqual(len(a2), n)
        for x in a2:
            self.assertEqual(x, x0)

        a3 = DoubleArray([3.14, 42])
        self.assertEqual(len(a3), 2)
        self.assertEqual(a3[0], 3.14)
        self.assertEqual(a3[1], 42)

    def testAppend(self):
        a = DoubleArray()
        a.append(3.14)
        a.append(42)
        self.assertEqual(a, DoubleArray([3.14, 42]))

    def testToDoubleArray(self):
        a = toDoubleArray("[3.5, 42]")
        self.assertTrue(a == DoubleArray([3.5, 42]))

if __name__ == '__main__':
    unittest.main()
