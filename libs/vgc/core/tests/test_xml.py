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

from vgc.core import XmlTokenType, XmlStreamReader

xmlExample1 = """
<vgc>
  <path id="p0" />
</vgc>
"""

class TestXmlStreamReader(unittest.TestCase):

    def testConstructor(self):
        xml = XmlStreamReader(xmlExample1)
        self.assertIsNotNone(xml)

    # TODO: testFromFile(self)

    def testReadNext(self):
        xml = XmlStreamReader(xmlExample1)
        self.assertTrue(xml.readNext())
        while xml.readNext():
            pass
        self.assertFalse(xml.readNext())

    def testStartDocument(self):
        xml = XmlStreamReader(xmlExample1)
        self.assertEqual(xml.tokenType, XmlTokenType.StartDocument)

    def testEndDocument(self):
        xml = XmlStreamReader(xmlExample1)
        while xml.readNext():
            pass
        self.assertEqual(xml.tokenType, XmlTokenType.EndDocument)

    def testCharacterData(self):
        xml = XmlStreamReader(xmlExample1)
        #while xml.readNext():
        #    print(xml.characterData)
        #self.assertFalse(true)

    def testRawText(self):
        xml = XmlStreamReader(xmlExample1)
        while xml.readNext():
            print(f'{xml.tokenType}: "{xml.rawText}"')
        self.assertFalse(true)


if __name__ == '__main__':
    unittest.main()
