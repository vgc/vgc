#!/usr/bin/python3

# XXX There is no proper testing architecture implemented yet.
# In the meantime, just run the test manually via:
#
#     cd <vgc-build-dir>
#     PYTHONPATH=python python3 <vgc-source-dir>/libs/vgc/dom/tests/test_document.py

import unittest

from vgc.dom import Document

class Test_document(unittest.TestCase):

    def test_create_document(self):
        document = Document()
        self.assertTrue(document)

if __name__ == '__main__':
    unittest.main()
