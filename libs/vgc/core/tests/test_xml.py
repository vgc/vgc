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

xmlExample = """
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<vgc>
  <b>Some &quot;bold&quot; text</b>
  <path id="p0"/>
</vgc>
"""

class TestXmlStreamReader(unittest.TestCase):

    def testConstructor(self):
        xml = XmlStreamReader(xmlExample)
        self.assertIsNotNone(xml)

    # TODO: testFromFile(self)

    def testReadNext(self):
        xml = XmlStreamReader(xmlExample)
        self.assertTrue(xml.readNext())
        while xml.readNext():
            pass
        self.assertFalse(xml.readNext())

    def testStartDocument(self):
        xml = XmlStreamReader(xmlExample)
        self.assertEqual(xml.tokenType, XmlTokenType.StartDocument)

    def testEndDocument(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            pass
        self.assertEqual(xml.tokenType, XmlTokenType.EndDocument)

    def testName(self):
        xml = XmlStreamReader(xmlExample)
        expectedStartNames = ['vgc', 'b', 'path']
        expectedEndNames = ['b', 'path', 'vgc']
        startNames = []
        endNames = []
        while xml.readNext():
            if xml.tokenType == XmlTokenType.StartElement:
                startNames.append(xml.name)
            elif xml.tokenType == XmlTokenType.EndElement:
                endNames.append(xml.name)
        self.assertEqual(startNames, expectedStartNames)
        self.assertEqual(endNames, expectedEndNames)

    def testCharacterData(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.tokenType == XmlTokenType.StartElement and xml.name == 'b':
                xml.readNext();
                self.assertEqual(xml.tokenType, XmlTokenType.CharacterData)
                self.assertEqual(xml.rawText, 'Some &quot;bold&quot; text')
                self.assertEqual(xml.characterData, 'Some "bold" text')

    def testRawText(self):
        xml = XmlStreamReader(xmlExample)
        allRawText = ""
        while xml.readNext():
            allRawText += xml.rawText;
        self.assertEqual(allRawText, xmlExample)

    # Useful for testing
    def printRawText(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            print(f'{xml.tokenType}: "{xml.rawText}"')


if __name__ == '__main__':
    unittest.main()
