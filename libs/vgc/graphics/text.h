// Copyright 2021 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VGC_GRAPHICS_TEXT_H
#define VGC_GRAPHICS_TEXT_H

#include <vgc/core/innercore.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/font.h>

namespace vgc {
namespace graphics {

namespace internal {

class ShapedTextImpl;
class TextBoundaryIteratorImpl;

} // namespace internal

/// \enum vgc::graphics::TextHorizontalAlign
/// \brief How to align text horizontally
///
// TODO: Justify
//       (?) Start / End
//       (?) enum TextDirection { LeftToRight, ... }
//       (?) enum TextHorizontalAlignLast { Auto, Left, ... }
//
// Note: there is no need for TextHorizontalAlign::JustifyAll if we
//       have a TextHorizontalAlignLast enum, which covers this case
//       and is more expressive.
//
// See: https://developer.mozilla.org/en-US/docs/Web/CSS/text-align
//      https://developer.mozilla.org/en-US/docs/Web/CSS/direction
//      https://developer.mozilla.org/en-US/docs/Web/CSS/text-align-last
//      https://stackoverflow.com/questions/10684533/text-align-justify-but-to-the-right
//      https://drafts.csswg.org/css-text-3/#justification
//
enum class TextHorizontalAlign : Int8 {
    Left,
    Right,
    Center
};

/// \enum vgc::graphics::TextVerticalAlign
/// \brief How to align text vertically
///
enum class TextVerticalAlign : Int8 {
    Top,
    Middle,
    Bottom
};

/// \enum vgc::graphics::TextProperties
/// \brief Specifies text properties such as alignment, justification, etc.
///
/// For now, TextProperties simply stores a desired TextHorizontalAlign and
/// TextVerticalAlign. In the future, the idea is to extend this class to
/// cover most CSS3 properties enumerated here:
///
/// https://drafts.csswg.org/css-text-3/#property-index
///
class VGC_GRAPHICS_API TextProperties {
public:
    /// Constructs a TextProperties with the given alignment.
    ///
    TextProperties(TextHorizontalAlign horizontalAlign = TextHorizontalAlign::Left,
                   TextVerticalAlign verticalAlign = TextVerticalAlign::Top) :
        horizontalAlign_(horizontalAlign),
        verticalAlign_(verticalAlign) {}

    /// Returns the horizontal alignment.
    ///
    TextHorizontalAlign horizontalAlign() const {
        return horizontalAlign_;
    }

    /// Sets the horizontal alignment.
    ///
    void setHorizontalAlign(TextHorizontalAlign horizontalAlign) {
        horizontalAlign_ = horizontalAlign;
    }

    /// Returns the vertical alignment.
    ///
    TextVerticalAlign verticalAlign() const {
        return verticalAlign_;
    }

    /// Sets the vertical alignment.
    ///
    void setVerticalAlign(TextVerticalAlign verticalAlign) {
        verticalAlign_ = verticalAlign;
    }

private:
    TextHorizontalAlign horizontalAlign_;
    TextVerticalAlign verticalAlign_;
};

/// \class vgc::graphics::ShapedGlyph
/// \brief Represents one glyph of a shaped text.
///
/// A ShapedGlyph stores a reference to a FontGlyph as well as information
/// where to draw the glyph. It is typically used as part of a ShapedText
/// object.
///
/// See ShapedText for more details.
///
/// Note that for performance and thread-safety reasons, a ShapedGlyph stores a
/// raw FontGlyph pointer, not a FontGlyphPtr. This means that it doesn't keep
/// alive the referenced FontGlyph: it is the responsibility of client code to
/// ensure that the referenced FontGlyph outlive the ShapedGlyph itself. For
/// example, each ShapedText object does keep alive its underlying FontFace
/// (ans thus all its glyphs too), so it is safe to use the glyphs in
/// ShapedText::glyphs() as long as the ShapedText is alive.
///
class VGC_GRAPHICS_API ShapedGlyph {
public:
    /// Creates a ShapedGlyph.
    ///
    ShapedGlyph(FontGlyph* fontGlyph,
                const core::Vec2d& offset,
                const core::Vec2d& advance,
                const core::Vec2d& position,
                Int bytePosition) :
        fontGlyph_(fontGlyph),
        offset_(offset),
        advance_(advance),
        position_(position),
        bytePosition_(bytePosition) {}

    /// Returns the FontGlyph that this ShapedGlyph references.
    ///
    /// Note that a ShapedText never contain ShapedGlyph elements whose
    /// fontGlyph() is NULL.
    ///
    /// - In case of a missing glyph in the face, the ".notdef" glyph [1] is
    ///   returned. It is typically represented as a rectangle (sometimes with
    ///   an "X" or "?" inside). More info:
    ///
    ///   https://docs.microsoft.com/en-us/typography/opentype/spec/recom#glyph-0-the-notdef-glyph
    ///
    /// - In case of other shaping problems (such as invalid unicode input),
    ///   the `U+FFFD � REPLACEMENT CHARACTER` might be used, or the codepoints
    ///   causing problems are simply skipped.
    ///
    FontGlyph* fontGlyph() const {
        return fontGlyph_;
    }

    /// Returns how much the glyph should be moved before drawing
    /// it. This should not affect how much the line advances.
    ///
    core::Vec2d offset() const {
        return offset_;
    }

    // Returns how much the line advances after drawing this ShapedGlyph. The
    // X-coordinate corresponds to the advance when setting text in horizontal
    // direction, and the Y-coordinate corresponds to the advance when setting
    // text in vertical direction.
    //
    core::Vec2d advance() const {
        return advance_;
    }

    /// Returns where to draw the fontGlyph() relative to the origin of the
    /// ShapedText this ShapedGlyph belongs to.
    ///
    /// This is equal to the sum of this ShapedGlyph's offset and the advances
    /// of all the previous ShapedGlyph of the ShapedText.
    ///
    core::Vec2d position() const {
        return position_;
    }

    /// Returns the smallest UTF-8 byte index in the original text that
    /// corresponds to this ShapedGlyph. If one unicode character is shaped
    /// into multiple glyphs, then all these glyphs will have the same byte
    /// index. If several unicode characters are merged into one glyph, then
    /// this function returns the smallest index.
    ///
    Int bytePosition() const {
        return bytePosition_;
    }

    /// Fills this ShapedGlyph, taking into account its relative position() and
    /// the given origin:
    ///
    /// ```
    ///                      ██
    ///                      ██
    ///                      ██
    ///       position()     ██
    ///   ────────────────>  █████
    /// ─┼─────────────────┼────────────  baseline
    /// origin
    /// ```
    ///
    /// The output triangles are appended to the given output parameter `data`
    /// in the following format:
    ///
    /// ```
    /// [x1, y1, r1, g1, b1,     // First vertex of first triangle
    ///  x2, y2, r2, g2, b2,     // Second vertex of first triangle
    ///  x3, y3, r3, g3, b3,     // Third vertex of first triangle
    ///
    ///  x4, y4, r4, g4, b4,     // First vertex of second triangle
    ///  x5, y5, r5, g5, b5,     // Second vertex of second triangle
    ///  x6, y6, r6, g6, b6,     // Third vertex of second triangle
    ///
    ///  ...]
    /// ```
    ///
    /// Note that fontGlyph()->outline() is a Curves2d where the Y-axis points
    /// up, as per OpenType conventions. This function takes care of reverting
    /// the Y coordinate to follow the VGC convention of having the Y-axis
    /// pointing down, and takes care of translating the glyph to its correct
    /// position.
    ///
    void fill(core::FloatArray& data,
              const core::Vec2d& origin,
              float r, float g, float b) const;

private:
    FontGlyph* fontGlyph_;
    core::Vec2d offset_;
    core::Vec2d advance_;
    core::Vec2d position_;
    Int bytePosition_;
};

using ShapedGlyphArray = core::Array<ShapedGlyph>;

/// \class vgc::graphics::ShapedText
/// \brief Performs text shaping and stores the resulting shaped text.
///
/// The ShapedText class performs text shaping and stores the resulting shaped
/// text. In other words, it takes as input a given text string and a given
/// FontFace, and stores as output a sequence of FontGlyph elements together
/// with additional information to know where each glyph should be drawn.
///
/// ```cpp
/// vgc::graphics::FontFace* face = someFace();
/// vgc::graphics::ShapedText text(face, "Hello");
/// for (const vgc::graphics::ShapedGlyph& glyph : text.glyphs()) {
///     ...
/// }
/// ```
///
class VGC_GRAPHICS_API ShapedText {
public:
    /// Creates a new ShapedText.
    ///
    ShapedText(FontFace* face, const std::string& text);

    /// Creates a copy of the given ShapedText.
    ///
    ShapedText(const ShapedText& other);

    /// Move-constructs the given ShapedText.
    ///
    ShapedText(ShapedText&& other);

    /// Assigns a copy of the given ShapedText.
    ///
    ShapedText& operator=(const ShapedText& other);

    /// Move-assigns the other ShapedText.
    ///
    ShapedText& operator=(ShapedText&& other);

    /// Destroys this ShapedText.
    ///
    ~ShapedText();

    /// Returns the FontFace of this ShapedText.
    ///
    FontFace* fontFace() const;

    /// Returns the input text string of this ShapedText.
    ///
    const std::string& text() const;

    /// Modifies the input text string of this ShapedText. This automatically
    /// recomputes the glyphs.
    ///
    void setText(const std::string& text);

    /// Returns the ShapedGlyph elements composing this ShapedText.
    ///
    /// The ShapedGlyph elements are guaranteed to be valid as long as either
    /// this ShapedText are its FontFace is alive.
    ///
    const ShapedGlyphArray& glyphs() const;

    // Returns how much the line advances after drawing this ShapedText. The
    // X-coordinate corresponds to the advance when setting text in horizontal
    // direction, and the Y-coordinate corresponds to the advance when setting
    // text in vertical direction.
    //
    // This is equal to the sum of `glyph->advance()` for all the ShapedGlyph
    // elements in glyphs().
    //
    core::Vec2d advance() const;

    /// Fills this ShapedText at the given origin:
    ///
    /// ```
    ///    ██  ██  █████  ██     ██      ████
    ///    ██  ██  ██     ██     ██     ██  ██
    ///    ██████  █████  ██     ██     ██  ██
    ///    ██  ██  ██     ██     ██     ██  ██
    ///    ██  ██  █████  █████  █████   ████
    /// ─┼──────────────────────────────────────  baseline
    /// origin
    /// ```
    ///
    /// The output triangles are appended to the given output parameter `data`
    /// in the following format:
    ///
    /// ```
    /// [x1, y1, r1, g1, b1,     // First vertex of first triangle
    ///  x2, y2, r2, g2, b2,     // Second vertex of first triangle
    ///  x3, y3, r3, g3, b3,     // Third vertex of first triangle
    ///
    ///  x4, y4, r4, g4, b4,     // First vertex of second triangle
    ///  x5, y5, r5, g5, b5,     // Second vertex of second triangle
    ///  x6, y6, r6, g6, b6,     // Third vertex of second triangle
    ///
    ///  ...]
    /// ```
    ///
    /// Note that this function follows the VGC convention of having the Y-axis
    /// pointing down, so in the example ASCII art above, if origin = (0, 0),
    /// then all the Y-coordinates of the triangle vertices will be negative.
    ///
    void fill(core::FloatArray& data,
              const core::Vec2d& origin,
              float r, float g, float b) const;

    /// Fills this ShapedText from glyph index `start` (included) to glyph
    /// index `end` (excluded).
    ///
    /// See the other overloads of fill() for documentation of the remaining
    /// arguments.
    ///
    void fill(core::FloatArray& data,
              const core::Vec2d& origin,
              float r, float g, float b,
              Int start, Int end) const;

private:
    internal::ShapedTextImpl* impl_;
};

/// \class vgc::graphics::TextCursor
/// \brief Represents the position and properties of the text cursor.
///
class VGC_GRAPHICS_API TextCursor {
public:
    /// Creates a TextCursor.
    ///
    TextCursor(bool isVisible = false,
               Int bytePosition = 0) :
        isVisible_(isVisible),
        bytePosition_(bytePosition) {}

    /// Returns whether the TextCursor is visible.
    ///
    bool isVisible() const {
        return isVisible_;
    }

    /// Sets whether the TextCursor is visible.
    ///
    void setVisible(bool isVisible) {
        isVisible_ = isVisible;
    }

    /// Returns the cursor position as a UTF-8 byte index.
    ///
    Int bytePosition() const {
        return bytePosition_;
    }

    /// Sets the cursor position given as a UTF-8 byte index.
    ///
    void setBytePosition(Int bytePosition) {
        bytePosition_ = bytePosition;
    }

private:
    bool isVisible_;
    Int bytePosition_;
};

/// \class vgc::graphics::TextBoundaryType
/// \brief The different types of Unicode text segmentation
///
/// See also:
/// - TextBoundaryIterator
/// - http://www.unicode.org/reports/tr29/
/// - https://doc.qt.io/qt-5/qtextboundaryfinder.html
///
enum class TextBoundaryType {
    Grapheme = 0,
    Word = 1,
    Sentence = 2,
    Line = 3
};

// TODO: TextBoundaryReason(s). See QTextBoundaryFinder::BoundaryReason(s)

/// \class vgc::graphics::TextBoundaryIterator
/// \brief Iterates through a string based on Unicode text boundaries.
///
/// This is a thin wrapper around QTextBoundaryFinder, based on std::string
/// rather than QChar/QString.
///
/// See:
/// - https://doc.qt.io/qt-5/qtextboundaryfinder.html
/// - https://unicode.org/reports/tr29/
///
class VGC_GRAPHICS_API TextBoundaryIterator {
public:
    /// Creates a TextBoundaryIterator that iterates over the given UTF-8 string
    /// based on the given boundary type.
    ///
    TextBoundaryIterator(TextBoundaryType type, const std::string& string);

    /// Destroys the TextBoundaryIterator.
    ///
    ~TextBoundaryIterator();

    // Disable copy, move, assign, and move-assign
    TextBoundaryIterator(const TextBoundaryIterator& other) = delete;
    TextBoundaryIterator(TextBoundaryIterator&& other) = delete;
    TextBoundaryIterator& operator=(const TextBoundaryIterator& other) = delete;
    TextBoundaryIterator& operator=(TextBoundaryIterator&& other) = delete;

    /// Returns whether the current position is at a boundary.
    ///
    bool isAtBoundary() const;

    /// Returns whether the iterator is in a valid state.
    ///
    bool isValid() const;

    /// Returns the current position of the iterator, that is, the
    /// corresponding index in the input UTF-8 string. The range is from 0 to
    /// the length of the input string (included).
    ///
    Int	position() const;

    /// Moves the iterator to the given position, snapping it to a valid position.
    ///
    void setPosition(Int position);

    /// Moves the iterator to the end of the string. This is equivalent to
    /// setPosition(string.size()).
    ///
    void toEnd();

    /// Moves the iterator to the next boundary and returns that position.
    /// Returns -1 if there is no next boundary.
    ///
    Int	toNextBoundary();

    /// Moves the iterator to the previous boundary and returns that position.
    /// Returns -1 if there is no previous boundary.
    ///
    Int	toPreviousBoundary();

    /// Moves the iterator to the beginning of the string. This is equivalent to
    /// setPosition(0).
    ///
    void toStart();

    /// Returns the ty pe of this TextBoundaryIterator.
    ///
    TextBoundaryType type() const;

private:
    internal::TextBoundaryIteratorImpl* impl_;
};

} // namespace graphics
} // namespace vgc

#endif // VGC_GRAPHICS_TEXT_H
