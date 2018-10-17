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

from vgc.core import (
    ParseError,
    RangeError,
    TimeUnit,
    secondsToString,
    toDoubleApprox
)

class TestRead(unittest.TestCase):

    def assertDoubleApproxIsAccurate(self, l):
        expected = [float(s) for s in l]
        result = [toDoubleApprox(s) for s in l]
        self.assertEqual(expected, result)

    def assertDoubleApproxIsApprox(self, l):
        expected = [float(s) for s in l]
        result = [toDoubleApprox(s) for s in l]
        for a,b in zip(expected, result):
            m = min(abs(a), abs(b))
            if m > 0.0:
                self.assertLess((a-b)/m, 1.0e-15)
            else:
                # Both a and b are exactly zero. This is guaranteed to pass but let's test anyway.
                self.assertTrue(a == b)

    def assertDoubleApproxParseError(self, l):
        for s in l:
            with self.assertRaises(ParseError):
                toDoubleApprox(s)

    def assertDoubleApproxRangeError(self, l):
        for s in l:
            with self.assertRaises(RangeError):
                toDoubleApprox(s)

    def testDoubleApprox(self):
        # Zero must be read accurately.
        self.assertDoubleApproxIsAccurate(["0", "0.0", ".0", "0.", "00", "0000", "00.", ".00"])
        self.assertDoubleApproxIsAccurate(["+0", "+0.0", "+.0", "+0.", "+00", "+0000", "+00.", "+.00"])
        self.assertDoubleApproxIsAccurate(["-0", "-0.0", "-.0", "-0.", "-00", "-0000", "-00.", "-.00"])
        self.assertDoubleApproxIsAccurate(["0e0", "0.0e0", ".0e0", "0.e0", "00e0", "0000e0", "00.e0", ".00e0"])
        self.assertDoubleApproxIsAccurate(["0e+0", "0.0e+0", ".0e+0", "0.e+0", "00e+0", "0000e+0", "00.e+0", ".00e+0"])
        self.assertDoubleApproxIsAccurate(["0e-0", "0.0e-0", ".0e-0", "0.e-0", "00e-0", "0000e-0", "00.e-0", ".00e-0"])

        # Integers up to 15 digits should be read accurately.
        self.assertDoubleApproxIsAccurate(["1", "2", "3", "4", "5", "6", "7", "8", "9"])
        self.assertDoubleApproxIsAccurate(["+1", "+2", "+3", "+4", "+5", "+6", "+7", "+8", "+9"])
        self.assertDoubleApproxIsAccurate(["-1", "-2", "-3", "-4", "-5", "-6", "-7", "-8", "-9"])
        self.assertDoubleApproxIsAccurate(["1.0", "2.0", "3.0", "4.0", "5.0", "6.0", "7.0", "8.0", "9.0"])
        self.assertDoubleApproxIsAccurate(["+1.0", "+2.0", "+3.0", "+4.0", "+5.0", "+6.0", "+7.0", "+8.0", "+9.0"])
        self.assertDoubleApproxIsAccurate(["-1.0", "-2.0", "-3.0", "-4.0", "-5.0", "-6.0", "-7.0", "-8.0", "-9.0"])
        self.assertDoubleApproxIsAccurate(["01", "02", "03", "04", "05", "06", "07", "08", "09"])
        self.assertDoubleApproxIsAccurate(["010", "020", "030", "040", "050", "060", "070", "080", "090"])
        self.assertDoubleApproxIsAccurate(["1e42", "1.e42", "1.0e0", "0.1e1", "0.3e1", ".1e1", "0.1234e+4", "123000e-3"])
        self.assertDoubleApproxIsAccurate(["123456789012345", "1234567890123e+2", "1.23456789012345e+15"])

        # XXX This is a bug. It should be read accurately.
        # The issue is that the 16 trailing zeros create rounding errors.
        # We'll fix later. We need to defer multiplying by 10 when reading trailing zeros until
        # we get a non-zero digit.
        self.assertDoubleApproxIsAccurate(["999999999999998"])     # 15 digits: ok
        self.assertDoubleApproxIsAccurate(["999999999999998.0"])   # 15 digits + 1 trailing zero: ok
        #self.assertDoubleApproxIsAccurate(["999999999999998.00"]) # 15 digits + 2 trailing zeros: should be ok but doesn't pass
        self.assertDoubleApproxIsApprox(["999999999999998.00"])    # Small comfort: at least this passes

        # Non-integers with finite base-2 fractional part should be read accurately.
        self.assertDoubleApproxIsAccurate(["0.5", "0.25", "0.125"])

        # Integers with more than 15 digits can only be read approximately.
        self.assertDoubleApproxIsApprox(["1234567890123456", "12345678901234567", "123456789012345678"])

        # Non-integers with infinite base-2 fractional part can only be read approximately
        self.assertDoubleApproxIsApprox(["0.01", "0.009e10", "0.3", "-0.2", "-42.55", "42.142857", "1.42E1", "42e-1"])

        # This is the smallest allowed value without underflow. It can only be read approximately.
        self.assertDoubleApproxIsApprox(["1e-307", "1000000e-313"])

        # This is the largest allowed value without overflow. It can only be read approximately.
        self.assertDoubleApproxIsApprox(["9.9999999999999999e+307", "0.0000099999999999999999e+313"])

        # Testing ParseError
        # we need at least one digit in the significand
        self.assertDoubleApproxParseError(["", ".", "+", "-", "+.", "-.", "e1", ".e1", "+e1", "-e1", "+.e1", "-.e1"])
        self.assertDoubleApproxParseError(["Hi", ".Hi", "+Hi", "-Hi", "+.Hi", "-.Hi", "e1Hi", ".e1Hi", "+e1Hi", "-e1Hi", "+.e1Hi", "-.e1Hi"])
        # we need at least one digit in the exponent
        self.assertDoubleApproxParseError(["1e", "1e+", "1e-"])
        self.assertDoubleApproxParseError(["1eHi", "1e+Hi", "1e-Hi"])
        # Can't have spaces between sign and digits
        self.assertDoubleApproxParseError(["+ 1", "- 1"])
        self.assertDoubleApproxParseError(["1e+ 1", "1e- 1"])

        # Testing RangeError
        self.assertDoubleApproxRangeError(["1e308", "10e307", "0.1e309"])

        # Test underflow: These are silently rounded to zero, no error is emitted.
        # Note: we round subnormals to zero, so that we never generate subnormals
        for s in ["1e-308"]:
            self.assertEqual(toDoubleApprox(s), 0.0)

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
