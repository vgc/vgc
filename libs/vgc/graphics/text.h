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

#include <string_view>

#include <vgc/core/flags.h>
#include <vgc/core/innercore.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/font.h>

namespace vgc::graphics {

namespace detail {

class ShapedTextImpl;
class TextBoundaryIteratorImpl;

} // namespace detail

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
    TextProperties(
        TextHorizontalAlign horizontalAlign = TextHorizontalAlign::Left,
        TextVerticalAlign verticalAlign = TextVerticalAlign::Top)

        : horizontalAlign_(horizontalAlign)
        , verticalAlign_(verticalAlign) {
    }

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

// clang-format off

/// \enum vgc::graphics::TextBoundaryMarker
/// \brief Stores or queries boundary information about text positions.
///
/// This enumeration represents the different boundary properties
/// that can be stored or queried about text positions.
///
/// # Text segmentation
///
/// Text written in natural languages can typically be decomposed into text
/// elements such as graphemes ("user-perceived character"), words, and
/// sentences. Guidelines for determining the boundaries between these elements
/// is provided by the Unicode standard at:
///
/// https://www.unicode.org/reports/tr29
///
/// Computing these boundaries is important in user interfaces, as it allows a
/// user to interact with the text in meaningful ways, for example, moving the
/// text caret one grapheme or one word at a time, selecting a word by
/// double-clicking on it, etc.
///
/// We call "significant" any word which is not only made of whitespace or
/// punctuaction. This is sometimes useful to navigate from word to word, by
/// automatically skipping whitespaces and punctuation.
///
/// # Line break opportunities
///
/// For word-wrapping purposes, it is important to know at which positions it
/// is prohibited, allowed, or mandatory to split a long line into multiple
/// lines. Guidelines for determining such line break opportunities are
/// provided by the Unicode standard at:
///
/// https://www.unicode.org/reports/tr14
///
/// All line break opportunities, whether mandatory or optional, are marked
/// with the `LineBreakOpportunity` flag.
///
/// If the `MandatoryLineBreak` flag is set, then the line break is mandatory.
/// This typically occurs at the first position, the last position, or when a
/// "newline character" (CR or CRLF) is found.
///
/// If the `MandatoryLineBreak` flag is not set, then the line break is
/// optional. This typically occurs in two situations:
///
/// 1. At the beginning of a significant word, in which case the line can be
/// broken without any additional marking, and the `SoftHyphen` flag is set to
/// false.
///
/// 2. Just after an explicit soft hyphen character (U+00AD), or in the middle
/// of word at a position that is considered breakable based on an algorithm or
/// dictionary. In this case, the `SoftHyphen` flag is set to true, and a
/// hyphen mark should typically be added at the end of the line if one decides
/// to break the line here.
///
enum class TextBoundaryMarker : UInt16 {
    None                  = 0x00,

    Grapheme              = 0x01,
    Word                  = 0x02,
    Sentence              = 0x04,

    SignificantWordStart  = 0x08,
    SignificantWordEnd    = 0x10,

    LineBreakOpportunity  = 0x20,
    MandatoryLineBreak    = 0x40,
    SoftHyphen            = 0x80

    // TODO: add other useful boundaries: script, bidi, span, etc.
};
VGC_DEFINE_FLAGS(TextBoundaryMarkers, TextBoundaryMarker)

// clang-format on

using TextBoundaryMarkersArray = core::Array<TextBoundaryMarkers>;

/// Computes boundary markers for the given UTF-8 encoded text.
///
/// The length of the returned array is `text.length() + 1`.
///
VGC_GRAPHICS_API
TextBoundaryMarkersArray computeBoundaryMarkers(std::string_view text);

/// \class vgc::graphics::ShapedGlyph
/// \brief Represents a position within a shaped text.
///
class ShapedTextPositionInfo {
public:
    /// Creates a ShapedTextPosition.
    ///
    ShapedTextPositionInfo(
        Int glyphIndex,
        Int byteIndex,
        const geometry::Vec2f& advance,
        TextBoundaryMarkers boundaryMarkers)

        : glyphIndex_(glyphIndex)
        , byteIndex_(byteIndex)
        , advance_(advance)
        , boundaryMarkers_(boundaryMarkers) {
    }

    /// Returns the index of the ShapedGlyph just after this position.
    ///
    /// Returns -1 if there is no ShapedGlyph after this position.
    ///
    /// If this position is in the middle of a glyph (i.e., a glyph spans
    /// several graphemes, typically because of a ligature), then this glyph
    /// index is returned.
    ///
    Int glyphIndex() const {
        return glyphIndex_;
    }

    /// Returns the UTF-8 byte index in the original text that
    /// corresponds to this position.
    ///
    /// Note that a single unicode character never results in multiple
    /// graphemes, so this function always return a different byteIndex for
    /// different positions.
    ///
    Int byteIndex() const {
        return byteIndex_;
    }

    /// Returns how much the line advances from the beginning of the ShapedText
    /// to this position. The X-coordinate corresponds to the advance when
    /// setting text in horizontal direction, and the Y-coordinate corresponds
    /// to the advance when setting text in vertical direction.
    ///
    geometry::Vec2f advance() const {
        return advance_;
    }

    /// Returns whether this position is at the start/end of a grapheme, the
    /// start/end of a word, etc.
    ///
    TextBoundaryMarkers boundaryMarkers() const {
        return boundaryMarkers_;
    }

private:
    friend class detail::ShapedTextImpl;
    Int glyphIndex_;
    Int byteIndex_;
    geometry::Vec2f advance_;
    TextBoundaryMarkers boundaryMarkers_;

private:
    friend class ShapedText;
};

/// \class vgc::graphics::ShapedGlyph
/// \brief Represents one glyph of a shaped text.
///
/// A ShapedGlyph stores a reference to a SizedGlyph as well as information
/// where to draw the glyph. It is typically used as part of a ShapedText
/// object.
///
/// See ShapedText for more details.
///
/// Note that for performance and thread-safety reasons, a ShapedGlyph stores a
/// raw SizedGlyph pointer, not a GlyphPtr. This means that it doesn't keep
/// alive the referenced SizedGlyph: it is the responsibility of client code to
/// ensure that the referenced SizedGlyph outlive the ShapedGlyph itself. For
/// example, each ShapedText object does keep alive its underlying SizedFont
/// (ans thus all its glyphs too), so it is safe to use the glyphs in
/// ShapedText::glyphs() as long as the ShapedText is alive.
///
class VGC_GRAPHICS_API ShapedGlyph {
public:
    /// Creates a ShapedGlyph.
    ///
    ShapedGlyph(
        SizedGlyph* sizedGlyph,
        const geometry::Vec2f& offset,
        const geometry::Vec2f& advance,
        const geometry::Vec2f& position,
        Int bytePosition)

        : sizedGlyph_(sizedGlyph)
        , offset_(offset)
        , advance_(advance)
        , position_(position)
        , bytePosition_(bytePosition)
        , boundingBox_(core::NoInit{}) {

        // Convert bounding box from SizedGlyph coords to ShapedText coords
        geometry::Rect2f bbox = sizedGlyph->boundingBox();
        geometry::Vec2f positionf(position);
        boundingBox_.setPMin(positionf.x() + bbox.xMin(), positionf.y() - bbox.yMax());
        boundingBox_.setPMax(positionf.x() + bbox.xMax(), positionf.y() - bbox.yMin());
    }

    /// Returns the SizedGlyph that this ShapedGlyph references.
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
    SizedGlyph* sizedGlyph() const {
        return sizedGlyph_;
    }

    /// Returns how much the glyph should be moved before drawing it. This
    /// should not affect how much the line advances. This is expressed in
    /// ShapedText coordinates, that is, with the Y-axis pointing down.
    ///
    const geometry::Vec2f& offset() const {
        return offset_;
    }

    /// Returns how much the line advances after drawing this ShapedGlyph. The
    /// X-coordinate corresponds to the advance when setting text in horizontal
    /// direction, and the Y-coordinate corresponds to the advance when setting
    /// text in vertical direction. This is expressed in ShapedText
    /// coordinates, that is, with the Y-axis pointing down.
    ///
    const geometry::Vec2f& advance() const {
        return advance_;
    }

    /// Returns where to draw the sizedGlyph() relative to the origin of the
    /// ShapedText this ShapedGlyph belongs to.
    ///
    /// This is equal to the sum of this ShapedGlyph's offset and the advances
    /// of all the previous ShapedGlyph of the ShapedText.
    ///
    /// This is expressed in ShapedText coordinates, that is, with the Y-axis
    /// pointing down.
    ///
    const geometry::Vec2f& position() const {
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

    /// Returns the bounding box of the shaped glyph in ShapedText coordinates
    /// (that is, with the Y-axis pointing down). In most cases, the X
    /// coordinates of this bounding box are positive and the Y coordinates are
    /// negatives.
    ///
    const geometry::Rect2f& boundingBox() const {
        return boundingBox_;
    }

    // clang-format off

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
    void fill(
        core::FloatArray& data,
        const geometry::Vec2f& origin,
        float r, float g, float b) const;

    // clang-format on

    /// Overload of fill that doesn't output color information, that is, the
    /// output triangles are appended to the given output parameter `data` in
    /// the following format:
    ///
    /// ```
    /// [x1, y1,     // First vertex of first triangle
    ///  x2, y2,     // Second vertex of first triangle
    ///  x3, y3,     // Third vertex of first triangle
    ///
    ///  x4, y4,     // First vertex of second triangle
    ///  x5, y5,     // Second vertex of second triangle
    ///  x6, y6,     // Third vertex of second triangle
    ///
    ///  ...]
    ///
    void fill(core::FloatArray& data, const geometry::Vec2f& origin) const;

private:
    SizedGlyph* sizedGlyph_;
    geometry::Vec2f offset_;
    geometry::Vec2f advance_;
    geometry::Vec2f position_;
    Int bytePosition_;
    geometry::Rect2f boundingBox_;
};

/// \class vgc::graphics::ShapedGrapheme
/// \brief Represents one grapheme of a shaped text.
///
/// A grapheme represents the concept of "user-perceived character": for
/// example the base character `A` with a grave accent gives the grapheme `À`.
/// This concept is typically used for text cursor navigation: you want the
/// cursor to advance grapheme by grapheme, rather than glyph by glyph or
/// codepoint by codepoint.
///
/// Indeed, several Unicode codepoints may be combined together to form a
/// single grapheme. Also, several glyphs may be necessary to render a single
/// grapheme.
///
/// Conversely, it is also possible for a single glyph to span several
/// graphemes, such as in the case of ligatures. For example, the text "ff" is
/// often rendered as one glyph, while represented as two codepoints, and
/// representing two graphemes.
///
/// Note that unlike glyphs, converting a Unicode text to a sequence of
/// graphemes does not depend on a choice of font, and in particular does not
/// require shaping. If you do not need shaping, you can directly use the class
/// TextBoundaryIterator to iterate over the graphemes of a given string, which
/// is much cheaper than creating a ShapedText.
///
/// However, if you do need shaping, then iterating over the ShapedGrapheme
/// elements of a ShapedText makes it possible to know the 2D location of the
/// graphemes, as well as the correspondence between the graphemes and glyphs.
/// In particular, it gives the advance and position of each grapheme,
/// correctly handling the case of multiple glyphs per grapheme, and the case
/// of ligatures.
///
class VGC_GRAPHICS_API ShapedGrapheme {
public:
    /// Creates a ShapedGrapheme.
    ///
    ShapedGrapheme(
        Int glyphIndex,
        const geometry::Vec2f& advance,
        const geometry::Vec2f& position,
        Int bytePosition)

        : glyphIndex_(glyphIndex)
        , advance_(advance)
        , position_(position)
        , bytePosition_(bytePosition) {
    }

    /// Returns the index of the ShapedGlyph  corresponding to this grapheme.
    ///
    /// If this grapheme is made of several glyphs, it returns the first.
    ///
    /// If this grapheme is part of a glyph that spans several graphemes,
    /// then all these graphemes will return the same glyph.
    ///
    Int glyphIndex() const {
        return glyphIndex_;
    }

    /// Returns how much the line advances after this grapheme. The
    /// X-coordinate corresponds to the advance when setting text in horizontal
    /// direction, and the Y-coordinate corresponds to the advance when setting
    /// text in vertical direction.
    ///
    /// If this grapheme is made of several glyphs, it returns the sum of the
    /// glyph advances.
    ///
    /// If several graphemes are part of the same glyph (e.g., ligatures), it
    /// returns the glyph advance divided by the number of graphemes which are
    /// part of the glyph.
    ///
    geometry::Vec2f advance() const {
        return advance_;
    }

    /// Returns where this grapheme is located relative to the origin of the
    /// ShapedText.
    ///
    /// This is equal to the sum of the advances of all the previous graphemes
    /// of the ShapedText.
    ///
    geometry::Vec2f position() const {
        return position_;
    }

    /// Returns the smallest UTF-8 byte index in the original text that
    /// corresponds to this grapheme. If several unicode characters are
    /// composed into one grapheme, then this function returns the smallest
    /// index. Note that a single unicode character never results in multiple
    /// grapheme, so this function always return a different bytePosition for
    /// different graphemes.
    ///
    Int bytePosition() const {
        return bytePosition_;
    }

private:
    friend class detail::ShapedTextImpl;
    Int glyphIndex_;
    geometry::Vec2f advance_;
    geometry::Vec2f position_;
    Int bytePosition_;
};

using ShapedGlyphArray = core::Array<ShapedGlyph>;
using ShapedGraphemeArray = core::Array<ShapedGrapheme>;
using ShapedTextPositionInfoArray = core::Array<ShapedTextPositionInfo>;

/// \class vgc::graphics::ShapedText
/// \brief Performs text shaping and stores the resulting shaped text.
///
/// The ShapedText class performs text shaping and stores the resulting shaped
/// text. In other words, it takes as input a given text string and a given
/// SizedFont, and stores as output a sequence of SizedGlyph elements together
/// with additional information to know where each glyph should be drawn.
///
/// ```cpp
/// vgc::graphics::SizedFont* face = someFace();
/// vgc::graphics::ShapedText text(face, "Hello");
/// for (const vgc::graphics::ShapedGlyph& glyph : text.glyphs()) {
///     ...
/// }
/// ```
///
/// Note that the fill functions of this class are not thread-safe. In
/// particular, two threads cannot concurrently call fill functions on the same
/// ShapedText instance. The reason is that ShapedText uses internal buffers
/// for intermediate computations, which are stored as data members to avoid
/// dynamic allocations.
///
class VGC_GRAPHICS_API ShapedText {
public:
    /// Creates a new ShapedText.
    ///
    ShapedText(SizedFont* sizedFont, std::string_view text);

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

    /// Returns the SizedFont of this ShapedText.
    ///
    SizedFont* sizedFont() const;

    /// Modifies the sizedFont of this ShapedText. This automatically
    /// recomputes the glyphs, and there is no guarantee that the number of
    /// glyph is unchanged, even if using the same font on a different size.
    ///
    void setSizedFont(SizedFont* sizedFont);

    /// Returns the input text string of this ShapedText.
    ///
    const std::string& text() const;

    /// Modifies the input text string of this ShapedText. This automatically
    /// recomputes the glyphs.
    ///
    void setText(std::string_view text);

    /// Returns the ShapedGlyph elements composing this ShapedText.
    ///
    /// The ShapedGlyph elements are guaranteed to be valid as long as either
    /// this ShapedText and its SizedFont is alive.
    ///
    const ShapedGlyphArray& glyphs() const;

    /// Returns the ShapedGrapheme elements composing this ShapedText.
    ///
    /// The ShapedGrapheme elements are guaranteed to be valid as long as this
    /// ShapedText is alive.
    ///
    const ShapedGraphemeArray& graphemes() const;

    /// Returns the number of valid positions in this text.
    ///
    /// The valid positions are from `0` to `n - 1` where `n` is the number
    /// returned by this function.
    ///
    Int numPositions() const;

    /// Returns the smallest valid position in this text. This is always equal to `0`.
    ///
    Int minPosition() const {
        return 0;
    }

    /// Returns the largest valid position in this text. This is equal to `numPositions() - 1`.
    ///
    Int maxPosition() const {
        return numPositions() - 1;
    }

    /// Returns information about a given text position.
    ///
    /// If the given position isn't a valid position, then `positionInfo(i).byteIndex()` is equal to `-1`.
    ///
    ShapedTextPositionInfo positionInfo(Int position) const;

    // Returns how much the line advances after drawing this ShapedText. The
    // X-coordinate corresponds to the advance when setting text in horizontal
    // direction, and the Y-coordinate corresponds to the advance when setting
    // text in vertical direction.
    //
    geometry::Vec2f advance() const;

    // Returns how much the line advances after drawing all the graphemes until
    // the given text position.
    //
    geometry::Vec2f advance(Int position) const;

    // clang-format off

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
    void fill(
        core::FloatArray& data,
        const geometry::Vec2f& origin,
        float r, float g, float b) const;

    /// Fills this ShapedText from glyph index `start` (included) to glyph
    /// index `end` (excluded).
    ///
    /// See the other overloads of fill() for documentation of the remaining
    /// arguments.
    ///
    void fill(
        core::FloatArray& data,
        const geometry::Vec2f& origin,
        float r, float g, float b,
        Int start, Int end) const;

    /// Fills this ShapedText, clipping it to the rectangle given by `clipLeft`,
    /// `clipRight`, `clipTop`, and `clipBottom`.
    ///
    /// See the other overloads of fill() for documentation of the remaining
    /// arguments.
    ///
    void fill(
        core::FloatArray& data,
        const geometry::Vec2f& origin,
        float r, float g, float b,
        float clipLeft, float clipRight, float clipTop, float clipBottom) const;

    /// Fills this ShapedText from glyph index `start` (included) to glyph
    /// index `end` (excluded), clipping it to the rectangle given by
    /// `clipLeft`, `clipRight`, `clipTop`, and `clipBottom`.
    ///
    /// See the other overloads of fill() for documentation of the remaining
    /// arguments.
    ///
    void fill(
        core::FloatArray& data,
        const geometry::Vec2f& origin,
        float r, float g, float b,
        Int start, Int end,
        float clipLeft, float clipRight, float clipTop, float clipBottom) const;

    // clang-format on

    /// Returns the smallest text position whose UTF-8 byte index is greater or
    /// equal than the given `byteIndex`.
    ///
    /// Returns maxPosition() if no such position exists.
    ///
    Int positionFromByte(Int byteIndex) const;

    /// Returns the text position closest to the given 2D `point` that has
    /// all the given `boundaryMarkers`.
    ///
    Int positionFromPoint(
        const geometry::Vec2f& point,
        TextBoundaryMarkers boundaryMarkers = TextBoundaryMarker::Grapheme) const;

    /// Returns the pair of positions enclosing the given 2D `point` that has
    /// all the given `boundaryMarkers`.
    ///
    /// If the given `point` is before the first position or after the last
    /// position, then no such pair of enclosing positions exist, and instead
    /// the two returned positions are both equal to the position closest to
    /// the given `point`.
    ///
    std::pair<Int, Int> positionPairFromPoint(
        const geometry::Vec2f& point,
        TextBoundaryMarkers boundaryMarkers = TextBoundaryMarker::Grapheme) const;

    /// Returns the smallest position with all the given `boundaryMarkers` that
    /// is located strictly after the given `position`.
    ///
    /// If no such position exists, then:
    /// - if `clamp` is true (the default), returns `numPositions() - 1`
    /// - if `clamp` is false, returns `-1`
    ///
    Int nextBoundary(Int position, TextBoundaryMarkers boundaryMarkers, bool clamp = true)
        const;

    /// Returns the smallest position with all the given `boundaryMarkers` that
    /// is located at or after the given `position`.
    ///
    /// This function returns the given `position` if it already has all the
    /// given `boundaryMarkers`.
    ///
    /// If no such position exists, then:
    /// - if `clamp` is true (the default), returns `numPositions() - 1`
    /// - if `clamp` is false, returns `-1`
    ///
    Int nextOrEqualBoundary(
        Int position,
        TextBoundaryMarkers boundaryMarkers,
        bool clamp = true) const;

    /// Returns the largest position with all the given `boundaryMarkers` that
    /// is located strictly before the given `position`.
    ///
    /// If no such position exists, then:
    /// - if `clamp` is true (the default), returns `0`
    /// - if `clamp` is false, returns `-1`
    ///
    Int previousBoundary(
        Int position,
        TextBoundaryMarkers boundaryMarkers,
        bool clamp = true) const;

    /// Returns the largest position with all the given `boundaryMarkers` that
    /// is located at or before the given `position`.
    ///
    /// This function returns the given `position` if it already has all the
    /// given `boundaryMarkers`.
    ///
    /// If no such position exists, then:
    /// - if `clamp` is true (the default), returns `0`
    /// - if `clamp` is false, returns `-1`
    ///
    Int previousOrEqualBoundary(
        Int position,
        TextBoundaryMarkers boundaryMarkers,
        bool clamp = true) const;

private:
    detail::ShapedTextImpl* impl_;
};

/// \class vgc::graphics::TextScroll
/// \brief Represents whether the text is scrolled left/right or up/down.
///
class VGC_GRAPHICS_API TextScroll {
public:
    /// Creates a TextScroll with the given horizontal scrolling `x` and
    /// vertical scrolling `y`.
    ///
    TextScroll(Int x = 0, Int y = 0)
        : x_(x)
        , y_(y) {
    }

    /// Returns the horizontal scrolling.
    ///
    Int x() const {
        return x_;
    }

    /// Sets the horizontal scrolling.
    ///
    void setX(Int x) {
        x_ = x;
    }

    /// Returns the vertical scrolling.
    ///
    Int y() const {
        return y_;
    }

    /// Sets the vertical scrolling.
    ///
    void setY(Int y) {
        y_ = y;
    }

private:
    Int x_;
    Int y_;
};

/// \class vgc::graphics::TextCursor
/// \brief Represents the position and properties of the text cursor.
///
class VGC_GRAPHICS_API TextCursor {
public:
    /// Creates a TextCursor.
    ///
    TextCursor(bool isVisible = false, Int bytePosition = 0)
        : isVisible_(isVisible)
        , bytePosition_(bytePosition) {
    }

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

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_TEXT_H
