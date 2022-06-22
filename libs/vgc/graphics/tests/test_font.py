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
from vgc.core import resourcePath
from vgc.graphics import (
    FontError,
    FontHinting,
    SizedFontParams,
    FontLibrary,
    Font,
    Glyph,
    SizedFont,
    SizedGlyph
)


def someFontPath():
    return resourcePath("graphics/fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf")


def someFont():
    library = FontLibrary()
    return library.addFont(someFontPath())


def someGlyph():
    font = someFont()
    return font.getGlyphFromCodePoint(0x41)


def someSizedFont():
    font = someFont()
    ppem = 15
    hinting = FontHinting.Native
    params = SizedFontParams(ppem, hinting)
    return font.getSizedFont(params)


def someSizedGlyph():
    sizedFont = someSizedFont()
    return sizedFont.getSizedGlyphFromCodePoint(0x41)


class TestFontLibrary(unittest.TestCase):

    def testConstructor(self):
        library = FontLibrary()

    def testAddFont(self):
        library = FontLibrary()
        font = library.addFont(someFontPath())
        self.assertTrue(font.isAlive())

    def testDefaultFont(self):
        library = FontLibrary()
        self.assertIsNone(library.defaultFont)
        font = library.addFont(someFontPath())
        library.defaultFont = font
        self.assertEqual(library.defaultFont, font)


class TestFont(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            font = Font()

    def testLibrary(self):
        library = FontLibrary()
        font = library.addFont(someFontPath())
        self.assertEqual(font.library, library)

    def testIndex(self):
        font = someFont()
        self.assertEqual(font.index, 0)

    def testGetSizedFont(self):
        font = someFont()
        ppem = 15
        hinting = FontHinting.Native
        params = SizedFontParams(ppem, hinting)
        sizedFont = font.getSizedFont(params)
        self.assertEqual(sizedFont.params, params)

    def testGetGlyphFromCodePoint(self):
        font = someFont()
        glyph = font.getGlyphFromCodePoint(0x41)
        self.assertEqual(glyph.name, "A")

    def testGetGlyphFromCodePointZero(self):
        font = someFont()
        glyph = font.getGlyphFromCodePoint(0x0)
        self.assertIsNone(glyph)

    def testGetGlyphFromIndex(self):
        font = someFont()
        index = font.getGlyphIndexFromCodePoint(0x41)
        glyph = font.getGlyphFromIndex(index)
        self.assertEqual(glyph.index, index)
        self.assertEqual(glyph.name, "A")

    def testGetGlyphFromIndexZero(self):
        font = someFont()
        glyph = font.getGlyphFromIndex(0)
        self.assertEqual(glyph.index, 0)
        self.assertEqual(glyph.name, ".notdef")

    def testGetGlyphFromIndexInvalid(self):
        font = someFont()
        with self.assertRaises(FontError):
            glyph = font.getGlyphFromIndex(12345)

    def testGetGlyphIndexFromCodePoint(self):
        font = someFont()
        index = font.getGlyphIndexFromCodePoint(0x41)
        self.assertNotEqual(index, 0)


class TestGlyph(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            glyph = Glyph()

    def testIndex(self):
        font = someFont()
        index = font.getGlyphIndexFromCodePoint(0x41)
        glyph = font.getGlyphFromIndex(index)
        self.assertEqual(glyph.index, index)

    def testName(self):
        glyph = someGlyph()
        self.assertEqual(glyph.name, "A")


class TestSizedFont(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            sizedFont = SizedFont()

    def testFont(self):
        font = someFont()
        ppem = 15
        hinting = FontHinting.Native
        params = SizedFontParams(ppem, hinting)
        sizedFont = font.getSizedFont(params)
        self.assertEqual(sizedFont.font, font)

    def testParams(self):
        font = someFont()
        ppem = 15
        hinting = FontHinting.Native
        params = SizedFontParams(ppem, hinting)
        sizedFont = font.getSizedFont(params)
        self.assertEqual(sizedFont.params, params)

    def testAscent(self):
        sizedFont = someSizedFont()
        self.assertGreater(sizedFont.ascent, 14)
        self.assertLess(sizedFont.ascent, 15)

    def testDescent(self):
        sizedFont = someSizedFont()
        self.assertGreater(sizedFont.descent, -5)
        self.assertLess(sizedFont.descent, -4)

    def testHeight(self):
        sizedFont = someSizedFont()
        self.assertGreater(sizedFont.height, 18)
        self.assertLess(sizedFont.height, 19)

    def testGetSizedGlyphFromCodePoint(self):
        sizedFont = someSizedFont()
        sizedGlyph = sizedFont.getSizedGlyphFromCodePoint(0x41)
        self.assertEqual(sizedGlyph.name, "A")

    def testGetGlyphFromCodePointZero(self):
        sizedFont = someSizedFont()
        sizedGlyph = sizedFont.getSizedGlyphFromCodePoint(0x0)
        self.assertIsNone(sizedGlyph)

    def testGetGlyphFromIndex(self):
        sizedFont = someSizedFont()
        index = sizedFont.getGlyphIndexFromCodePoint(0x41)
        sizedGlyph = sizedFont.getSizedGlyphFromIndex(index)
        self.assertEqual(sizedGlyph.index, index)
        self.assertEqual(sizedGlyph.name, "A")

    def testGetGlyphFromIndexZero(self):
        sizedFont = someSizedFont()
        sizedGlyph = sizedFont.getSizedGlyphFromIndex(0)
        self.assertEqual(sizedGlyph.index, 0)
        self.assertEqual(sizedGlyph.name, ".notdef")

    def testGetGlyphFromIndexInvalid(self):
        sizedFont = someSizedFont()
        with self.assertRaises(FontError):
            sizedGlyph = sizedFont.getSizedGlyphFromIndex(12345)

    def testGetGlyphIndexFromCodePoint(self):
        sizedFont = someSizedFont()
        index = sizedFont.getGlyphIndexFromCodePoint(0x41)
        self.assertNotEqual(index, 0)


class TestSizedGlyph(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            sizedGlyph = SizedGlyph()

    def testGlyph(self):
        font = someFont()
        glyph = font.getGlyphFromCodePoint(0x41)
        ppem = 15
        hinting = FontHinting.Native
        params = SizedFontParams(ppem, hinting)
        sizedFont = font.getSizedFont(params)
        sizedGlyph = sizedFont.getSizedGlyphFromCodePoint(0x41)
        self.assertEqual(sizedGlyph.glyph, glyph)

    def testIndex(self):
        sizedFont = someSizedFont()
        index = sizedFont.getGlyphIndexFromCodePoint(0x41)
        sizedGlyph = sizedFont.getGlyphFromIndex(index)
        self.assertEqual(sizedGlyph.index, index)

    def testName(self):
        sizedGlyph = someSizedGlyph()
        self.assertEqual(sizedGlyph.name, "A")

if __name__ == '__main__':
    unittest.main()
