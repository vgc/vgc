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


def someFont(library):
    return library.addFont(someFontPath())


def someGlyph(font):
    return font.getGlyphFromCodePoint(0x41)


def someSizedFont(font):
    ppem = 15
    hinting = FontHinting.Native
    params = SizedFontParams(ppem, hinting)
    return font.getSizedFont(params)


def someSizedGlyph(sizedFont):
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
        library = FontLibrary()
        font = someFont(library)
        self.assertEqual(font.index, 0)

    def testGetSizedFont(self):
        library = FontLibrary()
        font = someFont(library)
        ppem = 15
        hinting = FontHinting.Native
        params = SizedFontParams(ppem, hinting)
        sizedFont = font.getSizedFont(params)
        self.assertEqual(sizedFont.params, params)

    def testGetGlyphFromCodePoint(self):
        library = FontLibrary()
        font = someFont(library)
        glyph = font.getGlyphFromCodePoint(0x41)
        self.assertEqual(glyph.name, "A")

    def testGetGlyphFromCodePointZero(self):
        library = FontLibrary()
        font = someFont(library)
        glyph = font.getGlyphFromCodePoint(0x0)
        self.assertIsNone(glyph)

    def testGetGlyphFromIndex(self):
        library = FontLibrary()
        font = someFont(library)
        index = font.getGlyphIndexFromCodePoint(0x41)
        glyph = font.getGlyphFromIndex(index)
        self.assertEqual(glyph.index, index)
        self.assertEqual(glyph.name, "A")

    def testGetGlyphFromIndexZero(self):
        library = FontLibrary()
        font = someFont(library)
        glyph = font.getGlyphFromIndex(0)
        self.assertEqual(glyph.index, 0)
        self.assertEqual(glyph.name, ".notdef")

    def testGetGlyphFromIndexInvalid(self):
        library = FontLibrary()
        font = someFont(library)
        with self.assertRaises(FontError):
            glyph = font.getGlyphFromIndex(12345)

    def testGetGlyphIndexFromCodePoint(self):
        library = FontLibrary()
        font = someFont(library)
        index = font.getGlyphIndexFromCodePoint(0x41)
        self.assertNotEqual(index, 0)


class TestGlyph(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            glyph = Glyph()

    def testFont(self):
        library = FontLibrary()
        font = someFont(library)
        index = font.getGlyphIndexFromCodePoint(0x41)
        glyph = font.getGlyphFromIndex(index)
        self.assertEqual(glyph.font, font)

    def testIndex(self):
        library = FontLibrary()
        font = someFont(library)
        index = font.getGlyphIndexFromCodePoint(0x41)
        glyph = font.getGlyphFromIndex(index)
        self.assertEqual(glyph.index, index)

    def testName(self):
        library = FontLibrary()
        font = someFont(library)
        glyph = someGlyph(font)
        self.assertEqual(glyph.name, "A")


class TestSizedFont(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            sizedFont = SizedFont()

    def testFont(self):
        library = FontLibrary()
        font = someFont(library)
        ppem = 15
        hinting = FontHinting.Native
        params = SizedFontParams(ppem, hinting)
        sizedFont = font.getSizedFont(params)
        self.assertEqual(sizedFont.font, font)

    def testParams(self):
        library = FontLibrary()
        font = someFont(library)
        ppem = 15
        hinting = FontHinting.Native
        params = SizedFontParams(ppem, hinting)
        sizedFont = font.getSizedFont(params)
        self.assertEqual(sizedFont.params, params)

    def testAscent(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        self.assertGreater(sizedFont.ascent, 14)
        self.assertLess(sizedFont.ascent, 15)

    def testDescent(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        self.assertGreater(sizedFont.descent, -5)
        self.assertLess(sizedFont.descent, -4)

    def testHeight(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        self.assertGreater(sizedFont.height, 18)
        self.assertLess(sizedFont.height, 19)

    def testGetSizedGlyphFromCodePoint(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        sizedGlyph = sizedFont.getSizedGlyphFromCodePoint(0x41)
        self.assertEqual(sizedGlyph.name, "A")

    def testGetGlyphFromCodePointZero(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        sizedGlyph = sizedFont.getSizedGlyphFromCodePoint(0x0)
        self.assertIsNone(sizedGlyph)

    def testGetGlyphFromIndex(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        index = sizedFont.getGlyphIndexFromCodePoint(0x41)
        sizedGlyph = sizedFont.getSizedGlyphFromIndex(index)
        self.assertEqual(sizedGlyph.index, index)
        self.assertEqual(sizedGlyph.name, "A")

    def testGetGlyphFromIndexZero(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        sizedGlyph = sizedFont.getSizedGlyphFromIndex(0)
        self.assertEqual(sizedGlyph.index, 0)
        self.assertEqual(sizedGlyph.name, ".notdef")

    def testGetGlyphFromIndexInvalid(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        with self.assertRaises(FontError):
            sizedGlyph = sizedFont.getSizedGlyphFromIndex(12345)

    def testGetGlyphIndexFromCodePoint(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        index = sizedFont.getGlyphIndexFromCodePoint(0x41)
        self.assertNotEqual(index, 0)


class TestSizedGlyph(unittest.TestCase):

    def testConstructor(self):
        with self.assertRaises(TypeError):
            sizedGlyph = SizedGlyph()

    def testGlyph(self):
        library = FontLibrary()
        font = someFont(library)
        glyph = font.getGlyphFromCodePoint(0x41)
        ppem = 15
        hinting = FontHinting.Native
        params = SizedFontParams(ppem, hinting)
        sizedFont = font.getSizedFont(params)
        sizedGlyph = sizedFont.getSizedGlyphFromCodePoint(0x41)
        self.assertEqual(sizedGlyph.glyph, glyph)

    def testIndex(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        index = sizedFont.getGlyphIndexFromCodePoint(0x41)
        sizedGlyph = sizedFont.getSizedGlyphFromIndex(index)
        self.assertEqual(sizedGlyph.index, index)

    def testName(self):
        library = FontLibrary()
        font = someFont(library)
        sizedFont = someSizedFont(font)
        sizedGlyph = someSizedGlyph(sizedFont)
        self.assertEqual(sizedGlyph.name, "A")

if __name__ == '__main__':
    unittest.main()
