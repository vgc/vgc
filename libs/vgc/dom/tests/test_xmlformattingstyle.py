#!/usr/bin/python3

# XXX There is no proper testing architecture implemented yet.
# In the meantime, just run the test manually via:
#
#     cd <vgc-build-dir>
#     PYTHONPATH=python python3 <vgc-source-dir>/libs/vgc/dom/tests/test_xmlformattingstyle.py

import unittest

from vgc.dom import XmlIndentStyle, XmlFormattingStyle

class Test_xmlformattingstyle(unittest.TestCase):

    def test_equal_indent_styles(self):
        style1 = XmlIndentStyle.Spaces
        style2 = XmlIndentStyle.Spaces
        self.assertEqual(style1, style2)

    def test_different_indent_styles(self):
        style1 = XmlIndentStyle.Spaces
        style2 = XmlIndentStyle.Tabs
        self.assertTrue(style1 != style2)

    def testCreateXmlFormatingStyle(self):
        style = XmlFormattingStyle()

    def testXmlFormatingStyleDefaultValues(self):
        style = XmlFormattingStyle()
        self.assertEqual(style.indentStyle, XmlIndentStyle.Spaces)
        self.assertEqual(style.indentSize, 2)
        self.assertEqual(style.attributeIndentSize, 4)

if __name__ == '__main__':
    unittest.main()
