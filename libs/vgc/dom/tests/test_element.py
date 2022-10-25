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

from vgc.dom import Document, Element, NodeType, SecondRootElementError

class TestElement(unittest.TestCase):

    def testConstructor(self):
        doc = Document()
        n1 = Element(doc, "n1")
        self.assertEqual(n1.nodeType, NodeType.Element)
        self.assertEqual(n1.parent, doc)
        self.assertEqual(n1.tagName, "n1")

        n2 = Element(n1, "n2")
        self.assertEqual(n2.nodeType, NodeType.Element)
        self.assertEqual(n2.parent, n1)
        self.assertEqual(n2.tagName, "n2")

        n3 = Element(n1, "n3")
        self.assertEqual(n3.nodeType, NodeType.Element)
        self.assertEqual(n3.parent, n1)
        self.assertEqual(n3.tagName, "n3")

        with self.assertRaises(SecondRootElementError):
            n4 = Element(doc, "n4")

if __name__ == '__main__':
    unittest.main()
