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
from vgc.dom import Document, Element, Path


ATTR_a = StringId("a")

class TestElement(unittest.TestCase):

    def testConstructor(self):
        p = Path()
        self.assertTrue(p.isRelative())
        pn1 = Path.fromId(StringId("n1"))
        self.assertTrue(pn1.isAbsolute())
        pn1a2 = Path("#n1.a[2]")
        self.assertTrue(pn1a2.isAbsolute())

    def testToString(self):
        self.assertEqual(Path().toString(), ".")
        self.assertEqual(Path("").toString(), ".")
        self.assertEqual(Path(".").toString(), ".")
        self.assertEqual(Path(".cc").toString(), ".cc")
        self.assertEqual(Path(".cc[0]").toString(), ".cc[0]")
        self.assertEqual(Path("/").toString(), "/")
        self.assertEqual(Path("/.cc").toString(), "/.cc")
        self.assertEqual(Path("/aa/bb").toString(), "/aa/bb")
        self.assertEqual(Path("aa/bb").toString(), "aa/bb")
        self.assertEqual(Path("#aa/bb").toString(), "#aa/bb")
        self.assertEqual(Path("#aa/bb.cc").toString(), "#aa/bb.cc")
        self.assertEqual(Path("#n1.a[2]").toString(), "#n1.a[2]")

    def testProps(self):
        # isAbsolute
        self.assertFalse(Path("").isAbsolute())
        self.assertFalse(Path(".").isAbsolute())
        self.assertFalse(Path("aa/bb").isAbsolute())
        self.assertTrue(Path("#aa/bb").isAbsolute())
        self.assertTrue(Path("/aa/bb").isAbsolute())
        self.assertTrue(Path("/.cc").isAbsolute())
        # isRelative
        self.assertTrue(Path("").isRelative())
        self.assertTrue(Path(".").isRelative())
        self.assertTrue(Path("aa/bb").isRelative())
        self.assertFalse(Path("#aa/bb").isRelative())
        self.assertFalse(Path("/aa/bb").isRelative())
        self.assertFalse(Path("/.cc").isRelative())
        # isRelativeToId
        self.assertFalse(Path("").isIdBased())
        self.assertFalse(Path(".").isIdBased())
        self.assertFalse(Path("aa/bb").isIdBased())
        self.assertTrue(Path("#aa/bb").isIdBased())
        self.assertFalse(Path("/aa/bb").isIdBased())
        self.assertFalse(Path("/.cc").isIdBased())
        # isElementPath
        self.assertTrue(Path("").isElementPath())
        self.assertTrue(Path(".").isElementPath())
        self.assertTrue(Path("aa/bb").isElementPath())
        self.assertTrue(Path("#aa/bb").isElementPath())
        self.assertTrue(Path("/aa/bb").isElementPath())
        self.assertFalse(Path("/.cc").isElementPath())
        self.assertFalse(Path("/aa/bb.cc").isElementPath())
        # isAttributePath
        self.assertFalse(Path("").isAttributePath())
        self.assertFalse(Path(".").isAttributePath())
        self.assertFalse(Path("aa/bb").isAttributePath())
        self.assertFalse(Path("#aa/bb").isAttributePath())
        self.assertFalse(Path("/aa/bb").isAttributePath())
        self.assertTrue(Path("/.cc").isAttributePath())
        self.assertTrue(Path("/aa/bb.cc").isAttributePath())

    def testComparisons(self):
        self.assertEqual(Path("/aa/bb.cc"), Path("/aa/bb.cc"))
        self.assertNotEqual(Path("/aa/bb.cc"), Path("/aa/bb.cd"))
        self.assertLess(Path("/aa/bb.cc"), Path("/aa/bb.cd"))

    def testDerivedPaths(self):
        self.assertEqual(Path("/aa/bb.cc").getElementPath(), Path("/aa/bb"))
        self.assertEqual(Path("/aa/bb.cc").getElementRelativeAttributePath().toString(), ".cc")
        self.assertEqual(Path("/aa/bb.cc").getElementRelativeAttributePath(), Path(".cc"))

if __name__ == '__main__':
    unittest.main()
