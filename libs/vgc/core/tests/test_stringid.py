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

from vgc.core import StringId

class TestStringId(unittest.TestCase):

    def testDefaultConstructor(self):
        sid = StringId()
        self.assertEqual(str(sid), "")

    def testConstructors(self):
        sid = StringId("stringId")
        self.assertEqual(str(sid), "stringId")

    def testComparisonOperators(self):
        sid0a = StringId("stringId0")
        sid0b = StringId("stringId0")
        sid1 = StringId("stringId1")
        self.assertTrue(sid0a < sid1 or sid1 < sid0a)
        self.assertEqual(sid0a, sid0b)
        self.assertNotEqual(id(sid0a), id(sid0b))
        self.assertNotEqual(sid0a, sid1)
        self.assertEqual(sid0a, "stringId0")
        self.assertEqual("stringId0", sid0a)
        self.assertNotEqual(sid1, "stringId0")
        self.assertNotEqual("stringId0", sid1)

    def testRepr(self):
        sid = StringId("test")
        self.assertEqual(repr(sid), "vgc.core.StringId('test')")

if __name__ == '__main__':
    unittest.main()
