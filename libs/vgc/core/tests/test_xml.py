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
<!-- Created with VGC Illustration -->
<vgc>
  <b>Some &quot;bold&quot; text</b>
  <path id="p0&lt;"  d =  'M 0 0 L 10 10'/>
</vgc>
"""

svgInkscapeExample= """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   sodipodi:docname="test-icon.svg"
   inkscape:version="1.0rc1 (09960d6, 2020-04-09)"
   id="svg8"
   version="1.1"
   viewBox="0 0 210 297"
   height="297mm"
   width="210mm">
  <defs
     id="defs2" />
  <sodipodi:namedview
     inkscape:window-maximized="0"
     inkscape:window-y="52"
     inkscape:window-x="1378"
     inkscape:window-height="943"
     inkscape:window-width="1252"
     showgrid="false"
     inkscape:document-rotation="0"
     inkscape:current-layer="layer1"
     inkscape:document-units="mm"
     inkscape:cy="445.71429"
     inkscape:cx="400"
     inkscape:zoom="0.35"
     inkscape:pageshadow="2"
     inkscape:pageopacity="0.0"
     borderopacity="1.0"
     bordercolor="#666666"
     pagecolor="#ffffff"
     id="base" />
  <metadata
     id="metadata5">
    <rdf:RDF>
      <cc:Work
         rdf:about="">
        <dc:format>image/svg+xml</dc:format>
        <dc:type
           rdf:resource="http://purl.org/dc/dcmitype/StillImage" />
        <dc:title></dc:title>
      </cc:Work>
    </rdf:RDF>
  </metadata>
  <g
     id="layer1"
     inkscape:groupmode="layer"
     inkscape:label="Layer 1">
    <path
       inkscape:connector-curvature="0"
       id="path833"
       d="m 21.166666,35.52976 5.291667,-12.095236 6.047618,12.851191 H 44.60119 l -9.07143,9.827379 3.779764,11.339287 -13.607143,-6.803572 -11.339286,4.535714 4.535714,-9.827381 -9.0714282,-9.827382 z"
       style="fill:#000000;stroke:#000000;stroke-width:0.26458300000000001px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1" />
  </g>
</svg>
"""

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

    def testReadNextStartElement(self):
        expectedNames = ['vgc', 'b', 'path']
        names = []
        xml = XmlStreamReader(xmlExample)
        while xml.readNextStartElement():
            names.append(xml.name)
        self.assertEqual(names, expectedNames)

    def testSkipElement(self):
        expectedNames = ['svg', 'defs', 'namedview', 'g', 'path']
        names = []
        xml = XmlStreamReader(svgInkscapeExample)
        while xml.readNextStartElement():
            if xml.name == 'metadata':
                xml.skipElement()
                self.assertEqual(xml.eventType, XmlEventType.EndElement)
            else:
                names.append(xml.name)
        self.assertEqual(names, expectedNames)

    def testEventType(self):
        xml = XmlStreamReader('<?xml version="1.0"?><!--bar--><?php?><b>foo</b>')
        eventTypes = []
        expectedEventTypes = [
            XmlEventType.NoEvent,
            XmlEventType.StartDocument,
            XmlEventType.Comment,
            XmlEventType.ProcessingInstruction,
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

    def testQualifiedNameAndPrefix(self):
        xml = XmlStreamReader('<vgc a="foo"><dc:type rdf:resource="bar"/></vgc>')

        expectedStartQualifiedNames = ['vgc', 'dc:type']
        expectedStartPrefixes = ['', 'dc']
        expectedStartNames = ['vgc', 'type']

        expectedEndQualifiedNames = ['dc:type', 'vgc']
        expectedEndPrefixes = ['dc', '']
        expectedEndNames = ['type', 'vgc']

        expectedAttributesQualifiedNames = ['a', 'rdf:resource']
        expectedAttributesPrefixes = ['', 'rdf']
        expectedAttributesNames = ['a', 'resource']

        startQualifiedNames = []
        startPrefixes = []
        startNames = []

        endQualifiedNames = []
        endPrefixes = []
        endNames = []

        attributesQualifiedNames = []
        attributesPrefixes = []
        attributesNames = []

        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement:
                startQualifiedNames.append(xml.qualifiedName)
                startPrefixes.append(xml.prefix)
                startNames.append(xml.name)
                for i in range(xml.numAttributes):
                    attributesQualifiedNames.append(xml.attribute(i).qualifiedName)
                    attributesPrefixes.append(xml.attribute(i).prefix)
                    attributesNames.append(xml.attribute(i).name)
            elif xml.eventType == XmlEventType.EndElement:
                endQualifiedNames.append(xml.qualifiedName)
                endPrefixes.append(xml.prefix)
                endNames.append(xml.name)

        self.assertEqual(startQualifiedNames, expectedStartQualifiedNames)
        self.assertEqual(startPrefixes, expectedStartPrefixes)
        self.assertEqual(startNames, expectedStartNames)

        self.assertEqual(endQualifiedNames, expectedEndQualifiedNames)
        self.assertEqual(endPrefixes, expectedEndPrefixes)
        self.assertEqual(endNames, expectedEndNames)

        self.assertEqual(attributesQualifiedNames, expectedAttributesQualifiedNames)
        self.assertEqual(attributesPrefixes, expectedAttributesPrefixes)
        self.assertEqual(attributesNames, expectedAttributesNames)

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

    def testAttributeByIndex(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement and xml.name == 'path':
                self.assertEqual(xml.numAttributes, 2)

                attr = xml.attribute(0)
                self.assertEqual(attr.name, 'id')
                self.assertEqual(attr.value, 'p0<')
                self.assertEqual(attr.rawText, ' id="p0&lt;"')
                self.assertEqual(attr.leadingWhitespace, ' ')
                self.assertEqual(attr.separator, '=')
                self.assertEqual(attr.rawValue, 'p0&lt;')
                self.assertEqual(attr.quotationMark, '"')

                attr = xml.attribute(1)
                self.assertEqual(attr.name, 'd')
                self.assertEqual(attr.value, 'M 0 0 L 10 10')
                self.assertEqual(attr.rawText, "  d =  'M 0 0 L 10 10'")
                self.assertEqual(attr.leadingWhitespace, '  ')
                self.assertEqual(attr.separator, ' =  ')
                self.assertEqual(attr.rawValue, 'M 0 0 L 10 10')
                self.assertEqual(attr.quotationMark, "'")

    def testAttributeByName(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement and xml.name == 'path':
                attr = xml.attribute('id')
                self.assertEqual(attr.value, 'p0<')
                attr = xml.attribute('d')
                self.assertEqual(attr.value, 'M 0 0 L 10 10')
                attr = xml.attribute('foo')
                self.assertIsNone(attr)

    def testAttributeName(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement and xml.name == 'path':
                self.assertEqual(xml.attributeName(0), 'id')
                self.assertEqual(xml.attributeName(1), 'd')

    def testAttributeValueByIndex(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement and xml.name == 'path':
                self.assertEqual(xml.attributeValue(0), 'p0<')
                self.assertEqual(xml.attributeValue(1), 'M 0 0 L 10 10')

    def testAttributeValueByName(self):
        xml = XmlStreamReader(xmlExample)
        while xml.readNext():
            if xml.eventType == XmlEventType.StartElement and xml.name == 'path':
                self.assertEqual(xml.attributeValue('id'), 'p0<')
                self.assertEqual(xml.attributeValue('d'), 'M 0 0 L 10 10')
                self.assertIsNone(xml.attributeValue('foo'))

    def testComment(self):
        xml = XmlStreamReader('<html><!-- Hello World! --></html>')
        while xml.readNext():
            if xml.eventType == XmlEventType.Comment:
                self.assertEqual(xml.rawText, '<!-- Hello World! -->')
                self.assertEqual(xml.comment, ' Hello World! ')

        xml = XmlStreamReader('<html><!--- Hello World! --></html>')
        while xml.readNext():
            if xml.eventType == XmlEventType.Comment:
                self.assertEqual(xml.rawText, '<!--- Hello World! -->')
                self.assertEqual(xml.comment, '- Hello World! ')

        xml = XmlStreamReader('<html><!--Hello-World!--></html>')
        while xml.readNext():
            if xml.eventType == XmlEventType.Comment:
                self.assertEqual(xml.rawText, '<!--Hello-World!-->')
                self.assertEqual(xml.comment, 'Hello-World!')

        xml = XmlStreamReader('<!----><html/>')
        while xml.readNext():
            if xml.eventType == XmlEventType.Comment:
                self.assertEqual(xml.rawText, '<!---->')
                self.assertEqual(xml.comment, '')

        xml = XmlStreamReader('<!-- Hello World ---><html/>')
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)
        self.assertRaises(ParseError, xml.readNext)

        xml = XmlStreamReader('<!-- Hello--World --><html/>')
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)
        self.assertRaises(ParseError, xml.readNext)

    def testCDataSection(self):
        xml = XmlStreamReader('<script>echo <![CDATA[x + y < 2]]>;</script>')
        while xml.readNext():
            if xml.eventType == XmlEventType.Characters:
                self.assertEqual(xml.rawText, 'echo <![CDATA[x + y < 2]]>;')
                self.assertEqual(xml.characters, 'echo x + y < 2;')

    def testProcessingInstruction(self):
        xml = XmlStreamReader('<html><?php echo "Hello World!"; ?></html>')
        while xml.readNext():
            if xml.eventType == XmlEventType.ProcessingInstruction:
                self.assertEqual(xml.processingInstructionTarget, 'php')
                self.assertEqual(xml.processingInstructionData, ' echo "Hello World!"; ')

    def testNoRootElement(self):
        xml = XmlStreamReader('')
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)
        self.assertRaises(ParseError, xml.readNext)

    def testSecondRootError(self):
        xml = XmlStreamReader('<a></a><b></b>')
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartElement)
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.EndElement)
        self.assertRaises(ParseError, xml.readNext)

    def testElementStartEndMismatchError(self):
        xml = XmlStreamReader('<a><b></a>')
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartElement)
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartElement)
        self.assertRaises(ParseError, xml.readNext)

    def testElementNotStartedError1(self):
        xml = XmlStreamReader('</a>')
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)
        self.assertRaises(ParseError, xml.readNext)

    def testElementNotStartedError2(self):
        xml = XmlStreamReader('<a></a></b>')
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartElement)
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.EndElement)
        self.assertRaises(ParseError, xml.readNext)

    def testElementNotEndedError(self):
        xml = XmlStreamReader('<a>')
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartDocument)
        xml.readNext()
        self.assertEqual(xml.eventType, XmlEventType.StartElement)
        self.assertRaises(ParseError, xml.readNext)


if __name__ == '__main__':
    unittest.main()
