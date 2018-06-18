#!/usr/bin/python3

# XXX There is no proper testing architecture implemented yet.
# In the meantime, just run the test manually via:
#
#     cd <vgc-build-dir>
#     PYTHONPATH=python python3 <vgc-source-dir>/libs/vgc/dom/tests/test_xmlformattingstyle.py

from vgc.dom import XmlIndentStyle

style1 = XmlIndentStyle.Spaces
style2 = XmlIndentStyle.Tabs
style3 = XmlIndentStyle.Spaces

if not (style1 != style2):
    print("Assertion Failed: style1 != style2")

if not (style1 == style3):
    print("Assertion Failed: style1 == style3")
