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

from vgc.dom import Document, Element, NodeType

class TestDocument(unittest.TestCase):

    def testConstructor(self):
        doc = Document()
        self.assertEqual(doc.nodeType, NodeType.Document)

    def testRootElement(self):
        doc = Document()
        root = Element(doc, "vgc")
        self.assertEqual(doc.rootElement.name, "vgc")

    def testSave(self):
        doc = Document()
        root = Element(doc, "vgc")
        path = Element(root, "path")
        doc.save("testSave.vgc")

if __name__ == '__main__':
    unittest.main()
