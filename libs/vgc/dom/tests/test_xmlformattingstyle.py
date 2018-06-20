#!/usr/bin/python3

# XXX There is no proper testing architecture implemented yet.
# In the meantime, just run the test manually via:
#
#     cd <vgc-build-dir>
#     PYTHONPATH=python python3 <vgc-source-dir>/libs/vgc/dom/tests/test_xmlformattingstyle.py

import unittest

from vgc.dom import XmlIndentStyle, XmlFormattingStyle

class Test_xmlformattingstyle(unittest.TestCase):

    def testXmlIndentStyleValues(self):
        XmlIndentStyle.Spaces
        XmlIndentStyle.Tabs

    def testXmlFormatingStyleConstructor(self):
        style = XmlFormattingStyle()

    def testXmlFormatingStyleDefaultValues(self):
        style = XmlFormattingStyle()
        self.assertEqual(style.indentStyle, XmlIndentStyle.Spaces)
        self.assertEqual(style.indentSize, 2)
        self.assertEqual(style.attributeIndentSize, 4)

if __name__ == '__main__':
    unittest.main()
