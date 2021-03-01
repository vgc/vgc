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

#ifndef VGC_UI_STYLETOKEN_H
#define VGC_UI_STYLETOKEN_H

#include <vgc/core/array.h>
#include <vgc/core/format.h>
#include <vgc/ui/api.h>

namespace vgc {
namespace ui {

/// \enum vgc::ui::StyleTokenType
/// \brief The type of a StyleToken
///
/// See: https://www.w3.org/TR/css-syntax-3/#tokenization
///
/// Note: This API should be considered beta and is subject to change. In
/// particular, we may remove Cdo and Cdc in the future, deviating from the
/// official CSS token types.
///
/// \sa StyleToken
///
enum class StyleTokenType : Int8 {
    Eof = 0,
    Ident,
    Function,
    AtKeyword,
    Hash,
    String,
    BadString,
    Url,
    BadUrl,
    Delim,
    Number,
    Percentage,
    Dimension,
    Whitespace,
    Cdo,
    Cdc,
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
/// Note: This API should be considered beta and is subject to change.
///
const char* toStringLiteral(StyleTokenType type);

/// Writes the given StyleTokenType to the output stream.
///
/// Note: This API should be considered beta and is subject to change.
///
template<typename OStream>
void write(OStream& out, StyleTokenType type)
{
    core::write(out, toStringLiteral(type));
}

/// \enum vgc::ui::StyleTokenFlag
/// \brief The type flag component of a StyleToken.
///
/// See: https://www.w3.org/TR/css-syntax-3/#tokenization
///
/// Note: This API should be considered beta and is subject to change.
///
/// See: https://www.w3.org/TR/css-syntax-3/#tokenization
///
/// \sa StyleToken
///
enum class StyleTokenFlag : Int8 {
    Id,
    Unrestricted,
    Integer,
    Number
};

/// \class vgc::ui::StyleTokenNumericValue
/// \brief The numeric value of a StyleToken.
///
/// See: https://www.w3.org/TR/css-syntax-3/#tokenization
///
/// Note: This API should be considered beta and is subject to change.
/// In particular, we may change to using an std::variant rather than a union.
///
/// \sa StyleToken
///
union StyleTokenNumericValue {
    Int64 integer;
    double number;
};

/// \class vgc::ui::StyleToken
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
    StyleTokenFlag flag;

    // Initializes a dummy token starting and ending at s.
    // All other fields are uninitialized.
    StyleToken(const char* s) : begin(s), end(s), type(StyleTokenType::Delim) {}
};

/// Writes the given StyleToken to the output stream.
///
/// Note: This API should be considered beta and is subject to change.
///
template<typename OStream>
void write(OStream& out, const StyleToken& token)
{
    using core::write;
    write(out, token.type);
    switch (token.type) {
    case StyleTokenType::Ident:
    case StyleTokenType::Function:
    case StyleTokenType::AtKeyword:
    case StyleTokenType::String:
    case StyleTokenType::Url:
    case StyleTokenType::Delim:
        write(out, "(\"");
        write(out, token.codePointsValue);
        write(out, "\")");
        break;
    case StyleTokenType::Hash:
        write(out, "(");
        if (token.flag == StyleTokenFlag::Id) {
            write(out, "(Id, \"");
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
        if (token.flag == StyleTokenFlag::Integer) {
            write(out, "Integer, ");
            write(out, token.numericValue.integer);
        }
        else {
            write(out, "Number, ");
            write(out, token.numericValue.number);
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

/// \typedef vgc::ui::StyleTokenArray
/// \brief The output of tokenizing a style stream.
///
/// Note: This API should be considered beta and is subject to change. In
/// particular, we may decide that StyleTokenArray should be a separate class
/// rather than a typedef, managing the lifetime of the decoded style string in
/// order to ensure that the tokens always point within a valid string.
///
/// \sa StyleToken
///
using StyleTokenArray = core::Array<StyleToken>;


/// \typedef vgc::ui::StyleTokenIterator
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
std::string decodeStyleString(const std::string& s);

/// Tokenizes the given null-terminated string into an array of StyleTokens.
/// The given null-terminated string must outlive the tokens, since they point
/// to characters in the string. The given null-terminated string is assumed to
/// be already "decoded" using decodeStyleString().
///
/// Note: This API should be considered beta and is subject to change.
///
/// \sa StyleTokenArray, StyleToken
///
StyleTokenArray tokenizeStyleString(const char* s);

} // namespace ui
} // namespace vgc

#endif // VGC_UI_STYLETOKEN_H
