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

from vgc.core import IntArray, SharedConstIntArray
from vgc.dom import Value
from vgc.geometry import Vec2d

from vgc.core.detail import (
    sharedConstIntArrayDataAddress
)

class TestValue(unittest.TestCase):

    def testInt(self):
        i = 42
        v = Value(i)
        j = v.toPyObject()
        self.assertEqual(i, j)

    def testVec2d(self):
        v = Value(Vec2d(1, 2))
        self.assertEqual(v.toPyObject(), Vec2d(1, 2))
        self.assertNotEqual(v.toPyObject(), Vec2d(2, 2))

    def testPyObject(self):

        class Foo:
            def __init__(self, i):
                self.i = i

        f = Foo(42)
        v = Value(f)
        self.assertEqual(f, v.toPyObject())

    '''
    def testGetSharedConst(self):
        a = IntArray([3, 1, 2])
        va = Value(a)
        vaVal1 = va.getIntArray()
        vaVal2 = va.getIntArray()
        self.assertEqual(
            sharedConstIntArrayDataAddress(vaVal1),
            sharedConstIntArrayDataAddress(vaVal2))

    def testConstructFromSharedConst(self):
        a = IntArray([3, 1, 2])
        sa = SharedConstIntArray(a)
        va = Value(sa)
        vaVal = va.getIntArray()
        self.assertEqual(
            sharedConstIntArrayDataAddress(sa),
            sharedConstIntArrayDataAddress(vaVal))
        self.assertEqual(a, vaVal)

    def testSetSharedConst(self):
        a = IntArray([3, 1, 2])
        sa = SharedConstIntArray(a)
        va = Value()
        va.set(sa)
        vaVal = va.getIntArray()
        self.assertEqual(
            sharedConstIntArrayDataAddress(sa),
            sharedConstIntArrayDataAddress(vaVal))
        self.assertEqual(a, vaVal)

    def testComparisons(self):
        va = Value(IntArray([3, 1, 2]))
        b = IntArray([3, 1, 2])
        self.assertEqual(b, va)
        self.assertEqual(va, b)
        c = IntArray([3, 1, 1])
        sc = SharedConstIntArray(c)
        self.assertFalse(c == va)
        self.assertFalse(sc == va)
        self.assertFalse(va == c)
        self.assertFalse(va == sc)
        self.assertTrue(c != va)
        self.assertTrue(sc != va)
        self.assertTrue(va != c)
        self.assertTrue(va != sc)
    '''

if __name__ == '__main__':
    unittest.main()
