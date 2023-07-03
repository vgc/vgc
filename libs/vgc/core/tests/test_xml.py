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

from vgc.core import XmlEventType, XmlStreamReader, LogicError, ParseError

xmlExample = """<?xml version="1.0" encoding="UTF-8"?>
<vgc>
  <b>Some &quot;bold&quot; text</b>
  <path id="p0" d="M 0 0 L 10 10"/>
</vgc>
"""

xmlEventTypeExample = '<?xml version="1.0"?><b>foo</b>'

# Useful for testing
def printRawText(xmlData):
    xml = XmlStreamReader(xmlData)
    while xml.readNext():
        print(f'{xml.eventType}: "{xml.rawText}"')

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

    def testEventType(self):
        xml = XmlStreamReader(xmlEventTypeExample)
        eventTypes = []
        expectedEventTypes = [
            XmlEventType.NoEvent,
            XmlEventType.StartDocument,
            XmlEventType.StartElement,
            XmlEventType.Characters,
            XmlEventType.EndElement,
            XmlEventType.EndDocument]
        eventTypes.append(xml.eventType)
        while xml.readNext():
            eventTypes.append(xml.eventType)
        eventTypes.append(xml.eventType)
        self.assertEqual(eventTypes, expectedEventTypes)

    def testXmlDeclaration(self):
        xml = XmlStreamReader('<vgc/>')
        xml.readNext()
        self.assertFalse(xml.hasXmlDeclaration)
        self.assertFalse(xml.isEncodingSet)
        self.assertFalse(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.0')
        self.assertEqual(xml.encoding, 'UTF-8')
        self.assertEqual(xml.isStandalone, False)

        xml = XmlStreamReader('<?xml version="1.0"?><vgc/>')
        xml.readNext()
        self.assertTrue(xml.hasXmlDeclaration)
        self.assertFalse(xml.isEncodingSet)
        self.assertFalse(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.0')
        self.assertEqual(xml.encoding, 'UTF-8')
        self.assertEqual(xml.isStandalone, False)

        xml = XmlStreamReader('<?xml version="1.2"?><vgc/>')
        xml.readNext()
        self.assertTrue(xml.hasXmlDeclaration)
        self.assertFalse(xml.isEncodingSet)
        self.assertFalse(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.2')
        self.assertEqual(xml.encoding, 'UTF-8')
        self.assertEqual(xml.isStandalone, False)

        xml = XmlStreamReader('<?xml version="1.0" encoding="UTF-8"?><vgc/>')
        xml.readNext()
        self.assertTrue(xml.hasXmlDeclaration)
        self.assertTrue(xml.isEncodingSet)
        self.assertFalse(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.0')
        self.assertEqual(xml.encoding, 'UTF-8')
        self.assertEqual(xml.isStandalone, False)

        xml = XmlStreamReader('<?xml version="1.0" encoding="UTF-16"?><vgc/>')
        xml.readNext()
        self.assertTrue(xml.hasXmlDeclaration)
        self.assertTrue(xml.isEncodingSet)
        self.assertFalse(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.0')
        self.assertEqual(xml.encoding, 'UTF-16')
        self.assertEqual(xml.isStandalone, False)

        xml = XmlStreamReader('<?xml version="1.0" standalone="no"?><vgc/>')
        xml.readNext()
        self.assertTrue(xml.hasXmlDeclaration)
        self.assertFalse(xml.isEncodingSet)
        self.assertTrue(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.0')
        self.assertEqual(xml.encoding, 'UTF-8')
        self.assertEqual(xml.isStandalone, False)

        xml = XmlStreamReader('<?xml version="1.0" standalone="yes"?><vgc/>')
        xml.readNext()
        self.assertTrue(xml.hasXmlDeclaration)
        self.assertFalse(xml.isEncodingSet)
        self.assertTrue(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.0')
        self.assertEqual(xml.encoding, 'UTF-8')
        self.assertEqual(xml.isStandalone, True)

        xml = XmlStreamReader('<?xml version="1.0" encoding="UTF-8" standalone="no"?><vgc/>')
        xml.readNext()
        self.assertTrue(xml.hasXmlDeclaration)
        self.assertTrue(xml.isEncodingSet)
        self.assertTrue(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.0')
        self.assertEqual(xml.encoding, 'UTF-8')
        self.assertEqual(xml.isStandalone, False)

        xml = XmlStreamReader('<?xml version="1.0" encoding="UTF-8" standalone="yes"?><vgc/>')
        xml.readNext()
        self.assertTrue(xml.hasXmlDeclaration)
        self.assertTrue(xml.isEncodingSet)
        self.assertTrue(xml.isStandaloneSet)
        self.assertEqual(xml.version, '1.0')
        self.assertEqual(xml.encoding, 'UTF-8')
        self.assertEqual(xml.isStandalone, True)

    def testStartDocument(self):
        xml = XmlStreamReader(xmlExample)
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)

    def testEndDocument(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            pass
        self.assertEqual(xml.eventType, XmlEventType.EndDocument)

    def testName(self):
        xml = XmlStreamReader(xmlExample)
        expectedStartNames = ['vgc', 'b', 'path']
        expectedEndNames = ['b', 'path', 'vgc']
        startNames = []
        endNames = []
        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement:
                startNames.append(xml.name)
            elif xml.eventType == XmlEventType.EndElement:
                endNames.append(xml.name)
        self.assertEqual(startNames, expectedStartNames)
        self.assertEqual(endNames, expectedEndNames)

    def testCharacters(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement and xml.name == 'b':
                xml.readNext();
                self.assertEqual(xml.eventType, XmlEventType.Characters)
                self.assertEqual(xml.rawText, 'Some &quot;bold&quot; text')
                self.assertEqual(xml.characters, 'Some "bold" text')

    def testCharactersExceptions(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.eventType != XmlEventType.Characters:
                self.assertRaises(LogicError, lambda: xml.characters)

    def testRawText(self):
        xml = XmlStreamReader(xmlExample)
        allRawText = ""
        while xml.readNext():
            allRawText += xml.rawText;
        self.assertEqual(allRawText, xmlExample)

    def testNumAttributes(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement and xml.name == 'path':
                self.assertEqual(xml.numAttributes, 2)

    def testProcessingInstruction(self):
        xml = XmlStreamReader('<html><?php echo "Hello World!"; ?></html>')
        while xml.readNext():
            if xml.eventType == XmlEventType.ProcessingInstruction:
                self.assertEqual(xml.processingInstructionTarget, 'php')
                self.assertEqual(xml.processingInstructionData, ' echo "Hello World!"; ')



if __name__ == '__main__':
    unittest.main()
