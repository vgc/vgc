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

from vgc.core import NotAliveError, StringId

from vgc.dom import (
    ChildCycleError,
    Document,
    Element,
    Path,
    Value,
    Node,
    NodeArray,
    NodeType,
    ReplaceDocumentError,
    SecondRootElementError,
    WrongChildTypeError,
    WrongDocumentError,
    lowestCommonAncestor
)

from vgc.geometry import (
    Vec2d,
    Vec2dArray,
)

def getChildNames(node):
    return [child.tagName for child in node.children]

class TestNodeType(unittest.TestCase):

    def testValues(self):
        NodeType.Document
        NodeType.Element

class TestNode(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            node = Node()            

    def testDocument(self):
        doc = Document()
        self.assertEqual(doc.document, doc)

        n1 = Element(doc, "foo")
        n2 = Element(n1, "bar")
        self.assertEqual(n1.document, doc)
        self.assertEqual(n2.document, doc)

    def testNodeType(self):
        doc = Document()
        element = Element(doc, "foo")
        self.assertEqual(doc.nodeType, NodeType.Document)
        self.assertEqual(element.nodeType, NodeType.Element)

    def testIsAlive(self):
        doc = Document()
        n1 = Element(doc, "n1")
        self.assertTrue(doc.isAlive())
        self.assertTrue(n1.isAlive())

    def testDestroy(self):
        doc = Document()
        n1 = Element(doc, "n1")
        n3 = Element(n1, "n2") # not a typo
        n3 = Element(n3, "n3")
        self.assertTrue(doc.isAlive())
        self.assertTrue(n1.isAlive())
        self.assertTrue(n3.parent.isAlive())
        self.assertTrue(n3.isAlive())
        self.assertTrue(doc.rootElement, n1)

        n1.remove()
        self.assertTrue(doc.isAlive())
        self.assertFalse(n1.isAlive())
        self.assertFalse(n3.isAlive())
        self.assertTrue(doc.rootElement == None)
        with self.assertRaises(NotAliveError):
            n2 = n3.parent

        doc.remove()
        self.assertFalse(doc.isAlive())
        with self.assertRaises(NotAliveError):
            n1 = doc.rootElement

        n1 = None
        with self.assertRaises(AttributeError):
            doc = n1.parent

        del n3
        with self.assertRaises(UnboundLocalError):
            n2 = n3.parent

        doc = Document()
        n1 = Element(doc, "n1")
        n2 = Element(n1, "n2")
        n3 = Element(n1, "n3")
        self.assertTrue(doc.isAlive())
        self.assertTrue(n1.isAlive())
        self.assertTrue(n2.isAlive())
        self.assertTrue(n3.isAlive())
        self.assertTrue(doc.rootElement == n1)
        self.assertTrue(n1.firstChild == n2)
        self.assertTrue(n1.lastChild == n3)

        del n2
        self.assertTrue(n1.firstChild.isAlive())

        n1 = None
        self.assertTrue(doc.rootElement.isAlive())
        self.assertTrue(doc.rootElement.firstChild.isAlive())
        self.assertTrue(n3.isAlive())

        doc = None
        self.assertFalse(n3.isAlive())

        doc = Document()
        n1 = Element(doc, "n1")
        del doc
        self.assertFalse(n1.isAlive())

        doc = Document()
        root = Element(doc, "root")
        n1 = Element(root, "n1")
        n2 = Element(root, "n2")
        n3 = Element(root, "n3")
        n4 = Element(root, "n4")
        self.assertEqual(getChildNames(root), ["n1", "n2", "n3", "n4"])

        n3.remove()
        self.assertEqual(getChildNames(root), ["n1", "n2", "n4"])

        n4.remove()
        self.assertEqual(getChildNames(root), ["n1", "n2"])

        n1.remove()
        self.assertEqual(getChildNames(root), ["n2"])

        n2.remove()
        self.assertEqual(getChildNames(root), [])

    def testParentChildRelationships(self):
        doc = Document()
        n1 = Element(doc, "foo")

        self.assertEqual(n1.parent,          doc)
        self.assertEqual(n1.firstChild,      None)
        self.assertEqual(n1.lastChild,       None)
        self.assertEqual(n1.previousSibling, None)
        self.assertEqual(n1.nextSibling,     None)

        n2 = Element(n1, "bar")
        n3 = Element(n1, "bar")
        n4 = Element(n1, "bar")

        self.assertEqual(n1.parent,          doc)
        self.assertEqual(n1.firstChild,      n2)
        self.assertEqual(n1.lastChild,       n4)
        self.assertEqual(n1.previousSibling, None)
        self.assertEqual(n1.nextSibling,     None)

        self.assertEqual(n2.parent,          n1)
        self.assertEqual(n2.firstChild,      None)
        self.assertEqual(n2.lastChild,       None)
        self.assertEqual(n2.previousSibling, None)
        self.assertEqual(n2.nextSibling,     n3)

        self.assertEqual(n3.parent,          n1)
        self.assertEqual(n3.firstChild,      None)
        self.assertEqual(n3.lastChild,       None)
        self.assertEqual(n3.previousSibling, n2)
        self.assertEqual(n3.nextSibling,     n4)

        self.assertEqual(n4.parent,          n1)
        self.assertEqual(n4.firstChild,      None)
        self.assertEqual(n4.lastChild,       None)
        self.assertEqual(n4.previousSibling, n3)
        self.assertEqual(n4.nextSibling,     None)

    def testChildren(self):
        doc = Document()
        n1 = Element(doc, "foo")
        n2 = Element(n1, "bar1")
        n3 = Element(n1, "bar2")
        n4 = Element(n1, "bar3")
        self.assertEqual(doc.numChildObjects(), 1)
        self.assertEqual(n1.numChildObjects(), 3)
        self.assertEqual(getChildNames(n1), ["bar1", "bar2", "bar3"])
        childObjects = []
        for child in n1.childObjects():
            childObjects.append(child)
        self.assertEqual(len(childObjects), 3)
        self.assertEqual(len(n1.childObjects()), 3)

    def testReparent(self):
        doc = Document()
        n1 = Element(doc, "n1")
        n2 = Element(n1, "n2")
        n3 = Element(n2, "n3")
        self.assertEqual(getChildNames(doc), ["n1"])
        self.assertEqual(getChildNames(n1),  ["n2"])
        self.assertEqual(getChildNames(n2),  ["n3"])
        self.assertEqual(getChildNames(n3),  [])

        self.assertTrue(n3.canReparent(n1))
        n3.reparent(n1)
        self.assertEqual(getChildNames(doc), ["n1"])
        self.assertEqual(getChildNames(n1),  ["n2", "n3"])
        self.assertEqual(getChildNames(n2),  [])
        self.assertEqual(getChildNames(n3),  [])

        # Special case 1: move last
        self.assertTrue(n2.canReparent(n1))
        n2.reparent(n1)
        self.assertEqual(getChildNames(n1),  ["n3", "n2"])

        # Special case 2: append root element again (= do nothing, or move last in case of comments)
        self.assertTrue(n1.canReparent(doc))
        n1.reparent(doc)
        self.assertEqual(getChildNames(doc), ["n1"])

    def testWrongDocumentError(self):
        doc1 = Document()
        n1 = Element(doc1, "n1")
        doc2 = Document()
        n2 = Element(doc2, "n2")
        self.assertFalse(n2.canReparent(n1))
        with self.assertRaises(WrongDocumentError):
            n2.reparent(n1)

    def testReparentDocument(self):
        doc1 = Document()
        n1 = Element(doc1, "foo")
        doc2 = Document()

        self.assertFalse(doc2.canReparent(doc1))
        with self.assertRaises(WrongDocumentError): # takes precedence over WrongChildTypeError
            doc2.reparent(doc1)

        self.assertFalse(doc1.canReparent(n1))
        with self.assertRaises(WrongChildTypeError): # takes precedence over ChildCycleError
            doc1.reparent(n1)

        self.assertFalse(doc2.canReparent(n1))
        with self.assertRaises(WrongDocumentError): # takes precedence over WrongChildTypeError
            doc2.reparent(n1)

    def testReparentRootElement(self):
        doc = Document()
        n1 = Element(doc, "foo")
        n2 = Element(n1, "bar")
        self.assertEqual(doc.rootElement, n1)

        self.assertFalse(n2.canReparent(doc))
        with self.assertRaises(SecondRootElementError):
            n2.reparent(doc)

    def testReparentCycle(self):
        doc = Document()
        n1 = Element(doc, "foo")
        n2 = Element(n1, "bar")

        self.assertFalse(n2.canReparent(n2))
        with self.assertRaises(ChildCycleError):
            n2.reparent(n2)

        self.assertFalse(n1.canReparent(n2))
        with self.assertRaises(ChildCycleError):
            n1.reparent(n2)

    def testReplace(self):
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

        self.assertTrue(n21.canReplace(n3))
        n21.replace(n3)
        self.assertEqual(getChildNames(root), ["n1", "n2", "n21", "n4"])
        self.assertEqual(getChildNames(n2), ["n22", "n23"])
        self.assertTrue(n21.isAlive())
        self.assertFalse(n3.isAlive())
        self.assertFalse(n31.isAlive())

        self.assertTrue(n2.canReplace(n2))
        n2.replace(n2)
        self.assertEqual(getChildNames(root), ["n1", "n2", "n21", "n4"])
        self.assertEqual(getChildNames(n2), ["n22", "n23"])

        self.assertTrue(n22.canReplace(n2))
        n22.replace(n2)
        self.assertEqual(getChildNames(root), ["n1", "n22", "n21", "n4"])
        self.assertTrue(n22.isAlive())
        self.assertFalse(n2.isAlive())
        self.assertFalse(n23.isAlive())

        self.assertTrue(n21.canReplace(n1))
        n21.replace(n1)
        self.assertEqual(getChildNames(root), ["n21", "n22", "n4"])
        self.assertTrue(n21.isAlive())
        self.assertFalse(n1.isAlive())

        self.assertTrue(n22.canReplace(n4))
        n22.replace(n4)
        self.assertEqual(getChildNames(root), ["n21", "n22"])
        self.assertTrue(n22.isAlive())
        self.assertFalse(n4.isAlive())

        self.assertTrue(root.canReplace(root))
        root.replace(root)
        self.assertEqual(getChildNames(doc), ["root"])
        self.assertEqual(getChildNames(root), ["n21", "n22"])
        self.assertTrue(root.isAlive())

        self.assertTrue(n21.canReplace(root))
        n21.replace(root)
        self.assertEqual(getChildNames(doc), ["n21"])
        self.assertEqual(getChildNames(n21), ["n211", "n212"])
        self.assertTrue(n21.isAlive())
        self.assertFalse(root.isAlive())
        self.assertFalse(n22.isAlive())

        self.assertTrue(doc.canReplace(doc))
        doc.replace(doc)
        self.assertTrue(doc.isAlive())
        self.assertEqual(getChildNames(doc), ["n21"])
        self.assertTrue(n21.isAlive())

    def testReplaceExceptions(self):
        doc = Document()
        root = Element(doc, "root")
        n1 = Element(root, "n1")

        doc2 = Document()
        root2 = Element(doc2, "root2")

        self.assertFalse(n1.canReplace(doc))
        with self.assertRaises(ReplaceDocumentError):
            n1.replace(doc)

        self.assertFalse(doc.canReplace(doc2))
        with self.assertRaises(ReplaceDocumentError):
            doc.replace(doc2)

        self.assertFalse(root.canReplace(root2))
        with self.assertRaises(WrongDocumentError):
            root.replace(root2)

        self.assertFalse(doc.canReplace(n1))
        with self.assertRaises(WrongChildTypeError):
            doc.replace(n1)

        # XXX Once NodeType::Comment is implemented, uncomment the following test.
        #     until then, replace() can never raise SecondRootElementError
        # c1 = Comment(doc, "comment")
        # self.assertFalse(n1.canReplace(c1))
        # with self.assertRaises(SecondRootElementError):
        #     n1.replace(c1)

        self.assertFalse(root.canReplace(n1))
        with self.assertRaises(ChildCycleError):
            root.replace(n1)

    def testIsDescendant(self):
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
        # Test all pairs: 12*12=144 tests
        for n in [doc, root, n1, n2, n3, n4, n21, n22, n23, n31, n211, n212]:
            self.assertTrue(n.isDescendantOf(n))
        for n in [root, n1, n2, n3, n4, n21, n22, n23, n31, n211, n212]:
            self.assertTrue(n.isDescendantOf(doc))
            self.assertFalse(doc.isDescendantOf(n))
        for n in [n1, n2, n3, n4, n21, n22, n23, n31, n211, n212]:
            self.assertTrue(n.isDescendantOf(root))
            self.assertFalse(root.isDescendantOf(n))
        for n in [n21, n22, n23, n211, n212]:
            self.assertTrue(n.isDescendantOf(n2))
            self.assertFalse(n2.isDescendantOf(n))
        for n in [n31]:
            self.assertTrue(n.isDescendantOf(n3))
            self.assertFalse(n3.isDescendantOf(n))
        for n in [n211, n212]:
            self.assertTrue(n.isDescendantOf(n21))
            self.assertFalse(n21.isDescendantOf(n))
        for n in [n2, n3, n4, n21, n22, n23, n31, n211, n212]:
            self.assertFalse(n.isDescendantOf(n1))
            self.assertFalse(n1.isDescendantOf(n))
        for n in [n3, n4, n31]:
            self.assertFalse(n.isDescendantOf(n2))
            self.assertFalse(n2.isDescendantOf(n))
        for n in [n4, n21, n22, n23, n211, n212]:
            self.assertFalse(n.isDescendantOf(n3))
            self.assertFalse(n3.isDescendantOf(n))
        for n in [n21, n22, n23, n31, n211, n212]:
            self.assertFalse(n.isDescendantOf(n4))
            self.assertFalse(n4.isDescendantOf(n))
        for n in [n22, n23, n31]:
            self.assertFalse(n.isDescendantOf(n21))
            self.assertFalse(n21.isDescendantOf(n))
        for n in [n23, n31, n211, n212]:
            self.assertFalse(n.isDescendantOf(n22))
            self.assertFalse(n22.isDescendantOf(n))
        for n in [n31, n211, n212]:
            self.assertFalse(n.isDescendantOf(n23))
            self.assertFalse(n23.isDescendantOf(n))
        for n in [n211, n212]:
            self.assertFalse(n.isDescendantOf(n31))
            self.assertFalse(n31.isDescendantOf(n))
        for n in [n212]:
            self.assertFalse(n.isDescendantOf(n211))
            self.assertFalse(n211.isDescendantOf(n))

    def testAncestors(self):
        doc = Document()
        root = Element(doc, "root")
        n1 = Element(root, "n1")
        n2 = Element(root, "n2")
        n21 = Element(n2, "n21")
        n22 = Element(n2, "n22")
        n211 = Element(n21, "n211")
        res = n211.ancestors()
        self.assertEqual(res[0], doc)
        self.assertEqual(len(res), 4)
        self.assertEqual(res, (doc, root, n2, n21))

    def testLowestCommonAncestorWith(self):
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
        docB = Document()
        rootB = Element(docB, "rootB")
        def test(a, b, lca):
            self.assertEqual(a.lowestCommonAncestorWith(b), lca)
            self.assertEqual(b.lowestCommonAncestorWith(a), lca)
        test(n211, n212, n21)
        test(n211, n22, n2)
        test(n212, n31, root)
        test(n212, rootB, None)

    def testLowestCommonAncestor(self):
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
        docB = Document()
        rootB = Element(docB, "rootB")
        self.assertEqual(lowestCommonAncestor((n211, n212)), n21)
        self.assertEqual(lowestCommonAncestor([n211, n212, n22]), n2)
        self.assertEqual(lowestCommonAncestor((n211, n22, n212)), n2)
        self.assertEqual(lowestCommonAncestor((n211, n22, n31)), root)
        self.assertEqual(lowestCommonAncestor((n211, n22, n31, rootB)), None)

    def testPathAccess(self):
        doc = Document()
        root = Element(doc, "root")
        n1 = Element(root, "n1")
        n1id = n1.getOrCreateId()
        n2 = Element(n1, "path")
        n2.name = "n2Name"
        self.assertEqual(doc.getElementFromPath(Path(f"#{n1id}/n2Name")), n2)
        self.assertEqual(doc.getElementFromPath(Path(f"#{n1id}/n2Name.positions[-1]")), n2)
        a = Vec2dArray(((1.5, 2.5), (3.2, 4.2)))
        n2.setAttribute(StringId("positions"), a)
        self.assertEqual(doc.getValueFromPath(Path(f"#{n1id}/n2Name.positions[-1]")), Vec2d(3.2, 4.2))

if __name__ == '__main__':
    unittest.main()
