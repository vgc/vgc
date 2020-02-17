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

from vgc.core import (
    TimeUnit,
    secondsToString
)

class TestFormat(unittest.TestCase):

    def testTimeUnit(self):
        TimeUnit.Seconds
        TimeUnit.Milliseconds
        TimeUnit.Microseconds
        TimeUnit.Nanoseconds

    def testSecondsToString(self):
        self.assertEqual(secondsToString(1), "1s")
        self.assertEqual(secondsToString(12), "12s")

        self.assertEqual(secondsToString(12, TimeUnit.Seconds), "12s")
        self.assertEqual(secondsToString(12, TimeUnit.Milliseconds), "12000ms")
        self.assertEqual(secondsToString(12, TimeUnit.Microseconds), "12000000µs")
        self.assertEqual(secondsToString(12, TimeUnit.Nanoseconds), "12000000000ns")

        self.assertEqual(secondsToString(12, TimeUnit.Seconds, 2), "12.00s")
        self.assertEqual(secondsToString(12, TimeUnit.Milliseconds, 2), "12000.00ms")
        self.assertEqual(secondsToString(12, TimeUnit.Microseconds, 2), "12000000.00µs")
        self.assertEqual(secondsToString(12, TimeUnit.Nanoseconds, 2), "12000000000.00ns")

        self.assertEqual(secondsToString(0.0123456, TimeUnit.Seconds, 2), "0.01s")
        self.assertEqual(secondsToString(0.0123456, TimeUnit.Milliseconds, 2), "12.35ms")

if __name__ == '__main__':
    unittest.main()
