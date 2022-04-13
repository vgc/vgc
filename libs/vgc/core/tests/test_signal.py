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
    signal, slot, Object as VGCObject, ConstructibleTestObject # , CppTestSignalObject
)

# Has similar signals and slots as the c++ test object.
class TestSignalObject(ConstructibleTestObject):

    def __init__(self):
        super(TestSignalObject, self).__init__()
        self.sumA = 0
        self.sumB = 0
        self.slotNoArgsCallCount = 0

    @signal
    def signalNoArgs(self):
        pass

    @signal
    def signalInt(self, a : int):
        pass

    @signal
    def signalIntFloat(self, a : int, b : float):
        pass

    @signal
    def signalIntFloatBool(self, a : int, b : float, c : bool):
        pass

    @slot
    def slotNoArgs(self):
        self.slotNoArgsCallCount += 1

    @slot
    def slotInt(self, a : int):
        self.sumA += a

    @slot
    def slotIntFloat(self, a : int, b : float):
        self.sumA += a
        self.sumB += b

    @slot
    def slotFloat(self,a : float):
        self.sumA += a


class TestSignal(unittest.TestCase):

    def testGetters(self):
        o = TestSignalObject()
        self.assertTrue(o.signalInt.object == o)
        self.assertTrue(o.slotIntFloat.object == o)

    def testSlots(self):
        o = TestSignalObject()
        p = (42, 21.)
        o.slotIntFloat(*p)
        self.assertTrue((o.sumA, o.sumB) == p)

    def testConnectPyToPy(self):
        o1 = TestSignalObject()
        o2 = TestSignalObject()
        o1.signalIntFloat.connect(o2.slotNoArgs)
        o1.signalIntFloat.connect(o2.slotInt)
        o1.signalIntFloat.connect(o2.slotIntFloat)
        o1.signalIntFloat.connect(o2.slotFloat)
        o1.signalIntFloat.emit(1, 1.)
        self.assertTrue(o2.slotNoArgsCallCount == 1)
        self.assertTrue(o2.sumA == 3)
        self.assertTrue(o2.sumB == 1.)
        with self.assertRaises(ValueError) as context:
            o1.signalNoArgs.connect(o2.slotInt)


if __name__ == '__main__':
    unittest.main()
