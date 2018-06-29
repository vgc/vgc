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

from vgc.dom import XmlFormattingStyle, XmlIndentStyle

class TestXmlIndentStyle(unittest.TestCase):

    def testValues(self):
        XmlIndentStyle.Spaces
        XmlIndentStyle.Tabs

class TestXmlFormattingStyle(unittest.TestCase):

    def testConstructor(self):
        style = XmlFormattingStyle()

    def testDefaultValues(self):
        style = XmlFormattingStyle()
        self.assertEqual(style.indentStyle, XmlIndentStyle.Spaces)
        self.assertEqual(style.indentSize, 2)
        self.assertEqual(style.attributeIndentSize, 4)

if __name__ == '__main__':
    unittest.main()
