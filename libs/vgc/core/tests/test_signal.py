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
import inspect

from pathlib import Path

from vgc.core import (
    signal, slot, ConstructibleTestObject, CppSignalTestObject
)

# Has similar signals and slots as the C++ test object.
class PySignalTestObject(ConstructibleTestObject):

    def __init__(self):
        super(PySignalTestObject, self).__init__()
        self.sumInt = 0
        self.sumFloat = 0
        self.slotNoArgsCallCount = 0

    @signal
    def signalNoArgs(self):
        pass

    @signal
    def signalInt(self, a: int):
        pass

    @signal
    def signalIntFloat(self, a: int, b: float):
        pass

    @signal
    def signalIntFloatBool(self, a: int, b: float, c: bool):
        pass

    @slot
    def slotNoArgs(self):
        self.slotNoArgsCallCount += 1

    @slot
    def slotInt(self, a: int):
        self.sumInt += a

    @slot
    def slotIntFloat(self, a: int, b: float):
        self.sumInt += a
        self.sumFloat += b

    @slot
    def slotFloat(self, a: float):
        self.sumFloat += a


def testSignalToSlot(test, o1, o2):
    o2.sumInt = 0
    o1.signalInt.connect(o2.slotInt)
    o1.signalInt.connect(o2.slotInt)
    o1.signalInt.emit(21)
    test.assertEqual(o2.sumInt, 42)


def testSignalToSignalToSlot(test, o1, o2, o3):
    o3.sumInt = 0
    o1.signalInt.connect(o2.signalInt)
    o2.signalInt.connect(o3.slotInt)
    o1.signalInt.emit(42)
    test.assertEqual(o3.sumInt, 42)


class TestCppSignal(unittest.TestCase):

    def testSlot(self):
        o1 = CppSignalTestObject()
        o1.slotInt(42)
        self.assertEqual(o1.sumInt, 42)

    def testSignalToSlot(self):
        o1 = CppSignalTestObject()
        o2 = CppSignalTestObject()
        testSignalToSlot(self, o1, o2)

    def testSignalToSignalToSlot(self):
        o1 = CppSignalTestObject()
        o2 = CppSignalTestObject()
        o3 = CppSignalTestObject()
        testSignalToSignalToSlot(self, o1, o2, o3)


class TestPySignal(unittest.TestCase):

    def testRefs(self):
        o1 = PySignalTestObject()
        o2 = PySignalTestObject()
        self.assertNotEqual(id(o1.slotInt), id(o2.slotInt))
        self.assertNotEqual(id(o1.signalInt), id(o2.signalInt))
        self.assertNotEqual(id(o1.slotInt.object), id(o2.slotInt.object))
        self.assertNotEqual(id(o1.signalInt.object), id(o2.signalInt.object))

    def testSlot(self):
        o1 = PySignalTestObject()
        o1.slotInt(42)
        self.assertEqual(o1.sumInt, 42)

    def testSignalToSlot(self):
        o1 = PySignalTestObject()
        o2 = PySignalTestObject()
        testSignalToSlot(self, o1, o2)

    def testSignalToSignalToSlot(self):
        o1 = PySignalTestObject()
        o2 = PySignalTestObject()
        o3 = PySignalTestObject()
        testSignalToSignalToSlot(self, o1, o2, o3)

    def testSignalToFunc(self):
        o1 = PySignalTestObject()
        a = [0]
        def foo(b):
            a[0] += b
        o1.signalInt.connect(foo)
        o1.signalInt.emit(21)
        o1.signalInt.emit(21)
        self.assertEqual(a[0], 42)

    def testIncompatibilityErrors(self):
        o1 = PySignalTestObject()
        o2 = CppSignalTestObject()
        with self.assertRaises(ValueError) as context:
            o1.signalNoArgs.connect(o1.slotInt)
        with self.assertRaises(ValueError) as context:
            o1.signalNoArgs.connect(o2.slotInt)
        with self.assertRaises(ValueError) as context:
            o2.signalNoArgs.connect(o1.slotInt)
        with self.assertRaises(ValueError) as context:
            o2.signalNoArgs.connect(o2.slotInt)

class TestCrossLanguageSignal(unittest.TestCase):

    def testSignalToSlot(self):
        o1 = CppSignalTestObject()
        o2 = PySignalTestObject()
        testSignalToSlot(self, o1, o2)
        testSignalToSlot(self, o2, o1)

    def testCppSignalToCppSignalToPySlot(self):
        o1 = CppSignalTestObject()
        o2 = CppSignalTestObject()
        o3 = PySignalTestObject()
        testSignalToSignalToSlot(self, o1, o2, o3)

    def testCppSignalToPySignalToCppSlot(self):
        o1 = CppSignalTestObject()
        o2 = PySignalTestObject()
        o3 = CppSignalTestObject()
        testSignalToSignalToSlot(self, o1, o2, o3)

    def testCppSignalToPySignalToPySlot(self):
        o1 = CppSignalTestObject()
        o2 = PySignalTestObject()
        o3 = PySignalTestObject()
        testSignalToSignalToSlot(self, o1, o2, o3)

    def testPySignalToPySignalToCppSlot(self):
        o1 = PySignalTestObject()
        o2 = PySignalTestObject()
        o3 = CppSignalTestObject()
        testSignalToSignalToSlot(self, o1, o2, o3)

    def testPySignalToCppSignalToPySlot(self):
        o1 = PySignalTestObject()
        o2 = CppSignalTestObject()
        o3 = PySignalTestObject()
        testSignalToSignalToSlot(self, o1, o2, o3)

    def testPySignalToCppSignalToCppSlot(self):
        o1 = PySignalTestObject()
        o2 = CppSignalTestObject()
        o3 = CppSignalTestObject()
        testSignalToSignalToSlot(self, o1, o2, o3)


if __name__ == '__main__':
    unittest.main()
