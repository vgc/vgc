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
    Signal, signal, slot, testBoundCallback, TestWrapObject, Object as VGCObject
)

class TestSignal(unittest.TestCase):

    def testPySignal(self):
        s = Signal()
        res = {}
        def slot():
          res['ok'] = True
        # s.connect(slot)
        s.emit()
        # self.assertTrue(res.get('ok', False))

    def testDecorators(self):

        class TestObj:
            @signal
            def test_signal(a):
                pass

            @slot
            def test_slot(a, b, j=4):
                pass

            @slot
            def plop(s, e):
                print(e)

        print("TestObj.test_signal.__name__:", TestObj.test_signal.__name__)
        print("dir(TestObj.test_signal)", dir(TestObj.test_signal))
        a = TestObj()
        self.assertTrue(a.test_signal(2) == 2)

        #print(inspect.getargspec(slot))
        #self.assertTrue(hasattr(TestObj.test_slot, "__slot_tag__"))
         # try to add custom attrs to pybound function
        #testBoundCallback.__slot_tag__ = 1

        testBoundCallback(a.plop)

        a = TestWrapObject()

        self.assertEqual(a.a, 0)
        self.assertEqual(a.b, 0)
 
        #a.slotID(42, 0.5)
        #self.assertEqual(a.a, 42)
        #self.assertEqual(a.b, 0.5)


        #self.assertTrue(dir(TestObj.test_signal) is None)


if __name__ == '__main__':
    unittest.main()
