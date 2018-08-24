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

from vgc.core import readDouble

class TestRead(unittest.TestCase):

    def testDouble(self):
        strings =  ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"]
        expected = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0]
        result = [readDouble(s) for s in strings]
        self.assertEqual(result, expected)

        strings =  ["10", "05", "42", "84562", "00", "00042"]
        expected = [10.0, 5.0, 42.0, 84562.0, 0.0, 42.0]
        result = [readDouble(s) for s in strings]
        self.assertEqual(result, expected)

        strings =  ["0.5", "42.142857"]
        expected = [0.5, 42.142857]
        result = [readDouble(s) for s in strings]
        self.assertEqual(result, expected)

        strings =  ["-0", "-1", "-2", "-12", "-0.2", "-42.55"]
        expected = [0.0, -1.0, -2.0, -12.0, -0.2, -42.55]
        result = [readDouble(s) for s in strings]
        self.assertEqual(result, expected)

        strings =  ["1e3", "1.42E1", "1e+1", "42e-1", "-3e1"]
        expected = [1000.0, 14.2, 10, 4.2, -30.0]
        result = [readDouble(s) for s in strings]
        self.assertEqual(result, expected)

        # The following tests currently fail to pass due to rounding errors.
        # See comment in implementation of read().
        #strings =  ["0.01", "0.3"]
        #expected = [0.01, 0.3]
        #result = [readDouble(s) for s in strings]
        #self.assertEqual(result, expected)



if __name__ == '__main__':
    unittest.main()
