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

from vgc.dom import NodeType, Node, Element, Document

class TestNodeType(unittest.TestCase):

    def testValues(self):
        NodeType.Document
        NodeType.Element

class TestNode(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            node = Node()

    def testNodeType(self):
        node = Element("foo")
        self.assertEqual(node.nodeType, NodeType.Element)

    def testAppendChild(self):
        node1 = Element("foo")
        node2 = Element("bar")
        self.assertEqual(node1.appendChild(node2), node2)

    def testAppendChildDocument(self):
        element = Element("foo")
        document = Document()
        self.assertFalse(element.appendChild(document))

    def testAppendChildRootElement(self):
        document = Document()
        element1 = Element("foo")
        element2 = Element("bar")
        document.appendChild(element1)
        self.assertEqual(document.rootElement, element1)
        self.assertFalse(document.appendChild(element2))

if __name__ == '__main__':
    unittest.main()
