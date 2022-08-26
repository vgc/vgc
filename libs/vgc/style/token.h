// Copyright 2022 The VGC Developers
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

#ifndef VGC_STYLE_TOKEN_H
#define VGC_STYLE_TOKEN_H

#include <vgc/core/array.h>
#include <vgc/core/format.h>

#include <vgc/style/api.h>

namespace vgc::style {

/// \enum vgc::style::StyleTokenType
/// \brief The type of a StyleToken
///
/// See: https://www.w3.org/TR/css-syntax-3/#tokenization
///
/// # Differences with CSS
///
/// In CSS, it is valid to write `<!--` and `-->` (HTML-style comment
/// delimiters), which are tokenized into tokens called CDO and CDC. These
/// tokens are then simply ignored by the CSS parser, but everything between a
/// CDO/CDC pair is actually parsed normally and not treated as a comment. The
/// rationale is to allow embedding CSS code in HTML while being backward
/// compatible with older browsers that do not support CSS, by using the
/// following trick:
///
/// ```html
/// <style type="text/css">
/// <!--
///    h1 { color: red }
///    p  { color: blue}
/// -->
/// </style>
/// ```
///
/// (See section 14.5 of https://www.w3.org/TR/REC-html40/present/styles.html)
///
/// In VGC stylesheets, we made the choice to disallow this, so `<!--` and
/// `-->` are tokenized following the other rules, therefore `<`, `!`, and `>`
/// are tokenized as separate delimiters, and `--` is tokenized as an
/// identifier.
///
/// \sa StyleToken
///
enum class StyleTokenType : Int8 {
    EndOfFile = 0,
    Identifier,
    Function,
    AtKeyword,
    Hash,
    String,
    BadString,
    Url,
    BadUrl,
    Delimiter,
    Number,
    Percentage,
    Dimension,
    Whitespace,
    Colon,
    Semicolon,
    Comma,
    LeftSquareBracket,
    RightSquareBracket,
    LeftParenthesis,
    RightParenthesis,
    LeftCurlyBracket,
    RightCurlyBracket
};

/// Converts the StyleTokenType enum value into a string literal, for printing
/// purposes.
///
const char* toStringLiteral(StyleTokenType type);

/// Writes the given StyleTokenType to the output stream.
///
template<typename OStream>
void write(OStream& out, StyleTokenType type) {
    core::write(out, toStringLiteral(type));
}

/// \enum vgc::style::StyleTokenHashFlag
/// \brief The flag component of a token of type `StyleTokenType::Hash`.
///
/// This flag informs whether a token of type `StyleTokenType::Hash` stores a
/// string that can be interpreted as a valid identifier (for example,
/// `#main-content`), or if instead it is unrestricted, that is, it cannot be
/// interpreted as an identifier (for example, because it starts with a digit,
/// like in the hex color `#ff0000`).
///
enum class StyleTokenHashFlag : Int8 {
    Identifier,
    Unrestricted
};

/// \enum vgc::style::StyleTokenNumericFlag
/// \brief The flag component of a token of numeric type.
///
/// This flag informs whether a token of type `StyleTokenType::Number`,
/// `StyleTokenType::Percentage`, or `StyleTokenType::Dimension` stores a
/// numeric value stored as an integer or a floating point.
///
enum class StyleTokenNumericFlag : Int8 {
    Integer,
    FloatingPoint
};

/// \class vgc::style::StyleTokenNumericValue
/// \brief The numeric value of a token of numeric type.
///
/// Stores the numeric value of a token of type `StyleTokenType::Number`,
/// `StyleTokenType::Percentage`, or `StyleTokenType::Dimension`.
///
/// The numeric value can be either an `integer` or a `floatingPoint`, as
/// specified by the `StyleToken::numericFlag()` of the token.
///
union StyleTokenNumericValue {
    Int64 integer;
    double floatingPoint;
};

/// \class vgc::style::StyleToken
/// \brief One element of the output of tokenizing a style string.
///
/// See: https://www.w3.org/TR/css-syntax-3/#tokenization
///
/// Note: This API should be considered beta and is subject to change. In
/// particular, all its member variables are currently public: these may become
/// private in the future. Also, `begin` and `end` are currently pointers to
/// characters within an external string. This is a bit unsafe and we might
/// choose a safer design in the future to better manage the lifetime of this
/// external string, and possibly use std::string_view or a similar structure.
///
struct StyleToken {
    const char* begin;
    const char* end;
    std::string codePointsValue;
    StyleTokenNumericValue numericValue;
    StyleTokenType type;

    void setHashFlag(StyleTokenHashFlag value) {
        flag = static_cast<Int8>(value);
    }

    void setNumericFlag(StyleTokenNumericFlag value) {
        flag = static_cast<Int8>(value);
    }

    StyleTokenHashFlag hashFlag() const {
        return static_cast<StyleTokenHashFlag>(flag);
    }

    StyleTokenNumericFlag numericFlag() const {
        return static_cast<StyleTokenNumericFlag>(flag);
    }

    // Initializes a dummy token starting and ending at s.
    // All other fields are uninitialized.
    StyleToken(const char* s)
        : begin(s)
        , end(s)
        , type(StyleTokenType::Delimiter) {
    }

    /// Returns the numericValue of this token as a float. Assumes the type of
    /// this token is either Number, Percentage, or Dimension.
    ///
    float toFloat() const {
        return numericFlag() == StyleTokenNumericFlag::Integer
                   ? static_cast<float>(numericValue.integer)
                   : static_cast<float>(numericValue.floatingPoint);
    }

    /// Returns the numericValue of this token as a double. Assumes the type of
    /// this token is either Number, Percentage, or Dimension.
    ///
    double toDouble() const {
        return numericFlag() == StyleTokenNumericFlag::Integer
                   ? static_cast<double>(numericValue.integer)
                   : static_cast<double>(numericValue.floatingPoint);
    }

private:
    Int8 flag;
};

/// Writes the given StyleToken to the output stream.
///
template<typename OStream>
void write(OStream& out, const StyleToken& token) {
    using core::write;
    write(out, token.type);
    switch (token.type) {
    case StyleTokenType::Identifier:
    case StyleTokenType::Function:
    case StyleTokenType::AtKeyword:
    case StyleTokenType::String:
    case StyleTokenType::Url:
    case StyleTokenType::Delimiter:
        write(out, "(\"");
        write(out, token.codePointsValue);
        write(out, "\")");
        break;
    case StyleTokenType::Hash:
        write(out, "(");
        if (token.hashFlag() == StyleTokenHashFlag::Identifier) {
            write(out, "(Identifier, \"");
        }
        else {
            write(out, "(Unrestricted, \"");
        }
        write(out, token.codePointsValue);
        write(out, "\")");
        break;
    case StyleTokenType::Number:
    case StyleTokenType::Percentage:
    case StyleTokenType::Dimension:
        write(out, "(");
        if (token.numericFlag() == StyleTokenNumericFlag::FloatingPoint) {
            write(out, "Integer, ");
            write(out, token.numericValue.integer);
        }
        else {
            write(out, "FloatingPoint, ");
            write(out, token.numericValue.floatingPoint);
        }
        if (token.type == StyleTokenType::Dimension) {
            write(out, ", \"");
            write(out, token.codePointsValue);
            write(out, "\"");
        }
        write(out, ")");
        break;
    default:
        break;
    }
}

/// \typedef vgc::style::StyleTokenArray
/// \brief The output of tokenizing a style stream.
///
/// \sa StyleToken
///
using StyleTokenArray = core::Array<StyleToken>;

/// \typedef vgc::style::StyleTokenIterator
/// \brief Iterating over a StyleTokenArray
///
/// \sa StyleTokenArray, StyleToken
///
using StyleTokenIterator = core::Array<StyleToken>::iterator;

// Decodes the input style string. This is a pre-processing step that must be
// run before calling tokenizeStyleString(). It cleans up any invalid
// characters.
//
// References:
// https://www.w3.org/TR/css-syntax-3/#input-byte-stream
// https://www.w3.org/TR/css-syntax-3/#input-preprocessing
//
// Notes:
// - We only support UTF-8 encoding as input.
// - We don't actually "decode" the UTF-8 bytes into Unicode code points, since
//   all std::strings in VGC are assumed to be UTF-8 bytes. However, this
//   function does check for invalid UTF-8 bytes, and convert them to the
//   replacement character, see:
//   https://en.wikipedia.org/wiki/UTF-8#Invalid_sequences_and_error_handling
// - As mandated by CSS, we replace CR, FF, and CRLF with LF.
// - As mandated by CSS, we replace U+0000 NULL with U+FFFD REPLACEMENT CHARACTER.
// - We append a final U+0000 NULL which we use as EOF, making tokenizing easier.
//
/// \sa StyleTokenArray, StyleToken,
///
std::string decodeStyleString(std::string_view s);

/// Tokenizes the given null-terminated string into an array of StyleTokens.
/// The given null-terminated string must outlive the tokens, since they point
/// to characters in the string. The given null-terminated string is assumed to
/// be already "decoded" using decodeStyleString().
///
/// \sa StyleTokenArray, StyleToken
///
StyleTokenArray tokenizeStyleString(const char* s);

} // namespace vgc::style

#endif // VGC_STYLE_TOKEN_H
