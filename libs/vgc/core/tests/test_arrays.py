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

from vgc.core import (
    IndexError,
    DoubleArray,
    FloatArray,
    IntArray,
    SharedConstIntArray
)

def get1DArrayClasses():
    return [DoubleArray, FloatArray, IntArray]

# Note: we use exactly representable floating-point values (e.g: 1.5 = 1 + 1/2)
# to avoid comparison errors when converting from Python's float (64bit) to
# vgc.core.FloatArray (32bit), back to Python's float for the comparison.

def get1DValues():
    return [0.5, 0.5, 42]

def get1DValuess():
    return [[1.5, 0.5, 2.5], [1.5, 0.5, 2.5], [42, 43, 41]]

def testDefaultConstructor_(self, Array):
    a = Array()
    self.assertEqual(len(a), 0)

def testZeroInitializingConstructor_(self, Array):
    n = 3
    a = Array(n)
    self.assertEqual(len(a), n)
    for x in a:
        self.assertEqual(x, 0)

def testValueInitializingConstructor_(self, Array, value):
    n = 3
    a = Array(n, value)
    self.assertEqual(len(a), n)
    for x in a:
        self.assertEqual(x, value)

def assertArrayEqualToList_(self, a, l):
    self.assertEqual(len(a), len(l))
    for i in range(len(a)):
        self.assertEqual(a[i], l[i])

def testListInitializingConstructor_(self, Array, values):
    a = Array(values)
    assertArrayEqualToList_(self, a, values)

def testPrepend_(self, Array, values):
    a = Array()
    for v in values[::-1]:
        a.prepend(v)
    assertArrayEqualToList_(self, a, values)

def testAppend_(self, Array, values):
    a = Array()
    for v in values:
        a.append(v)
    assertArrayEqualToList_(self, a, values)

def testPop_(self, Array, values):
    a = Array(values)
    b = list(values)
    x = a.pop()
    y = b.pop()
    self.assertEqual(x, y)
    assertArrayEqualToList_(self, a, b)

    a = Array(values)
    b = list(values)
    x = a.pop(0)
    y = b.pop(0)
    self.assertEqual(x, y)
    assertArrayEqualToList_(self, a, b)

    a = Array(values)
    b = list(values)
    x = a.pop(1)
    y = b.pop(1)
    self.assertEqual(x, y)
    assertArrayEqualToList_(self, a, b)

    a = Array(values)
    b = list(values)
    x = a.pop(-1)
    y = b.pop(-1)
    self.assertEqual(x, y)
    assertArrayEqualToList_(self, a, b)

    a = Array(values)
    b = list(values)
    x = a.pop(len(a)-1)
    y = b.pop(len(b)-1)
    self.assertEqual(x, y)
    assertArrayEqualToList_(self, a, b)

    a = Array(values)
    b = list(values)
    x = a.pop(-len(a))
    y = b.pop(-len(b))
    self.assertEqual(x, y)
    assertArrayEqualToList_(self, a, b)

    a = Array(values)
    b = list(values)
    x = a.pop(-1)
    y = b.pop(-1)
    self.assertEqual(x, y)
    assertArrayEqualToList_(self, a, b)

    a = Array(values)
    with self.assertRaises(IndexError):
        a.pop(-len(a)-1)
    with self.assertRaises(IndexError):
        a.pop(len(a))
    a = Array()
    with self.assertRaises(IndexError):
        a.pop()

class Test1DArrays(unittest.TestCase):

    def testDefaultConstructor(self):
        Arrays = get1DArrayClasses()
        for Array in Arrays:
           testDefaultConstructor_(self, Array)

    def testZeroInitializingConstructors(self):
        Arrays = get1DArrayClasses()
        for Array in Arrays:
           testZeroInitializingConstructor_(self, Array)

    def testValueInitializingConstructors(self):
        Arrays = get1DArrayClasses()
        values = get1DValues()
        for Array, value in zip(Arrays, values):
           testValueInitializingConstructor_(self, Array, value)

    def testListInitializingConstructors(self):
        Arrays = get1DArrayClasses()
        valuess = get1DValuess()
        for Array, values in zip(Arrays, valuess):
           testListInitializingConstructor_(self, Array, values)

    def testIndex(self):
        a = DoubleArray("[3.5, 4, 42.5, 1]")
        self.assertEqual(a.index(42.5), 2)

    def testPrepend(self):
        Arrays = get1DArrayClasses()
        valuess = get1DValuess()
        for Array, values in zip(Arrays, valuess):
           testPrepend_(self, Array, values)

    def testAppend(self):
        Arrays = get1DArrayClasses()
        valuess = get1DValuess()
        for Array, values in zip(Arrays, valuess):
           testAppend_(self, Array, values)

    def testInsert(self):
        a = DoubleArray("[3.5, 4, 42.5, 1]")
        a.insert(2, 42)
        a.insert(-4, 1)
        b = DoubleArray("[3.5, 1, 4, 42, 42.5, 1]")
        self.assertEqual(a, b)

    def testPop(self):
        Arrays = get1DArrayClasses()
        valuess = get1DValuess()
        for Array, values in zip(Arrays, valuess):
           testPop_(self, Array, values)

    def testParse(self):
        a = DoubleArray("[3.5, 42]")
        self.assertEqual(a, DoubleArray([3.5, 42]))
        a = FloatArray("[3.5, 42]")
        self.assertEqual(a, FloatArray([3.5, 42]))
        a = IntArray("[43, 42]")
        self.assertEqual(a, IntArray([43, 42]))
        a = DoubleArray("  [  3.5 \n,  42 ]   ")
        self.assertEqual(a, DoubleArray([3.5, 42]))
        a = DoubleArray("[]")
        self.assertEqual(a, DoubleArray())
        a = DoubleArray("  [ \n]   ")
        self.assertEqual(a, DoubleArray())

    def testFormat(self):
        a = DoubleArray("[3.5, 42, 1]")
        self.assertEqual(str(a), "[3.5, 42, 1]")

    def testSharedConst(self):
        a = IntArray([3, 1, 2])
        b = SharedConstIntArray(a)
        c = SharedConstIntArray(b)
        self.assertEqual(a, b)
        self.assertEqual(b, c)
        self.assertEqual(a, c)

if __name__ == '__main__':
    unittest.main()
