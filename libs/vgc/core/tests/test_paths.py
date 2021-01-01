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

from pathlib import Path

from vgc.core import (
    basePath,
    pythonPath,
    resourcePath,
    resourcesPath,
    setBasePath
)

class TestPaths(unittest.TestCase):

    def testBasePath(self):
        p = basePath()
        self.assertTrue(Path(p).is_dir())

    def testPythonPath(self):
        p = pythonPath()
        self.assertTrue(Path(p).is_dir())
        self.assertTrue((Path(p) / "vgc").is_dir())

    def testResourcesPath(self):
        p = resourcesPath()
        self.assertTrue(Path(p).is_dir())
        self.assertTrue((Path(p) / "core").is_dir())

    def testResourcePath(self):
        p = resourcePath("core/version.txt")
        self.assertTrue(Path(p).is_file())

    def testSetBasePath(self):
        p = basePath()
        q = "/some/other/path"
        setBasePath(q)
        self.assertEqual(basePath(), q)
        setBasePath(p)
        self.assertEqual(basePath(), p)

if __name__ == '__main__':
    unittest.main()
