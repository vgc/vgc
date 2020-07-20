#!/usr/bin/python3

# Copyright 2020 The VGC Developers
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
from vgc.core import resourcePath
from vgc.graphics import (
    FontError,
    FontFace,
    FontGlyph,
    FontLibrary
)


def someFacePath():
    return resourcePath("graphics/fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf")


def someFace():
    library = FontLibrary()
    return library.addFace(someFacePath())


def someGlyph():
    face = someFace()
    return face.getGlyphFromCodePoint(0x41)


class TestFontLibrary(unittest.TestCase):

    def testConstructor(self):
        library = FontLibrary()

    def testAddFont(self):
        library = FontLibrary()
        face = library.addFace(someFacePath())
        self.assertTrue(face.isAlive())


class TestFontFace(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            face = FontFace()

    def testGetGlyphFromCodePoint(self):
        face = someFace()
        glyph = face.getGlyphFromCodePoint(0x41)
        self.assertEqual(glyph.name, "A")

    def testGetGlyphFromCodePointZero(self):
        face = someFace()
        glyph = face.getGlyphFromCodePoint(0x0)
        self.assertIsNone(glyph)

    def testGetGlyphFromIndex(self):
        face = someFace()
        index = face.getGlyphIndexFromCodePoint(0x41)
        glyph = face.getGlyphFromIndex(index)
        self.assertEqual(glyph.index, index)
        self.assertEqual(glyph.name, "A")

    def testGetGlyphFromIndexZero(self):
        face = someFace()
        glyph = face.getGlyphFromIndex(0)
        self.assertEqual(glyph.index, 0)
        self.assertEqual(glyph.name, ".notdef")

    def testGetGlyphFromIndexInvalid(self):
        face = someFace()
        with self.assertRaises(FontError):
            glyph = face.getGlyphFromIndex(12345)

    def testGetGlyphIndexFromCodePoint(self):
        face = someFace()
        index = face.getGlyphIndexFromCodePoint(0x41)
        self.assertNotEqual(index, 0)


class TestFontGlyph(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            face = FontGlyph()

    def testIndex(self):
        face = someFace()
        index = face.getGlyphIndexFromCodePoint(0x41)
        glyph = face.getGlyphFromIndex(index)
        self.assertEqual(glyph.index, index)

    def testName(self):
        glyph = someGlyph()
        self.assertEqual(glyph.name, "A")


if __name__ == '__main__':
    unittest.main()
