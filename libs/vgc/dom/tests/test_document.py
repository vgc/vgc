#!/usr/bin/python3

# Copyright 2021 The VGC Developers
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

from vgc.dom import Document, Element, NodeType, NodeArray

class TestDocument(unittest.TestCase):

    def testConstructor(self):
        doc = Document()
        self.assertEqual(doc.nodeType, NodeType.Document)

    def testRootElement(self):
        doc = Document()
        self.assertIsNone(doc.rootElement)

        n1 = Element(doc, "n1")
        self.assertEqual(doc.rootElement, n1)

        n2 = Element(n1, "n2")
        n2.replace(doc.rootElement)
        self.assertEqual(doc.rootElement, n2)
        self.assertFalse(n1.isAlive())
        self.assertTrue(n2.isAlive())

    def testSaveAndOpen(self):
        filePath = "testSave.vgc"

        doc = Document()
        root = Element(doc, "vgc")
        path = Element(root, "path")
        doc.save(filePath)
        doc = None

        doc = Document.open(filePath)
        self.assertEqual(doc.rootElement.tagName, "vgc")
        self.assertEqual(doc.rootElement.firstChild.tagName, "path")

    def testCopyPaste(self):
        doc = Document()
        root = Element(doc, "root")
        n1 = Element(root, "n1")
        n2 = Element(root, "n2")
        n3 = Element(root, "n3")
        n4 = Element(root, "n4")
        n21 = Element(n2, "n21")
        n22 = Element(n2, "n22")
        n23 = Element(n2, "n23")
        n31 = Element(n3, "n31")
        n211 = Element(n21, "n211")
        n212 = Element(n21, "n212")

        cpy = doc.copy((n3, n31, n23, n21))
        self.assertEqual(tuple(c.tagName for c in cpy.rootElement.children), ("n21", "n23", "n3"))
        doc.paste(cpy, root)
        self.assertEqual(tuple(c.tagName for c in root.children), ("n1", "n2", "n3", "n4", "n21", "n23", "n3"))

if __name__ == '__main__':
    unittest.main()
