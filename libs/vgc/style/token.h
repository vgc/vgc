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

/// \enum vgc::style::TokenType
/// \brief The type of a Token
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
/// \sa Token
///
enum class TokenType : Int8 {
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

/// Converts the TokenType enum value into a string literal, for printing
/// purposes.
///
const char* toStringLiteral(TokenType type);

/// Writes the given TokenType to the output stream.
///
template<typename OStream>
void write(OStream& out, TokenType type) {
    core::write(out, toStringLiteral(type));
}

/// \enum vgc::style::TokenHashFlag
/// \brief The flag component of a token of type `TokenType::Hash`.
///
/// This flag informs whether a token of type `TokenType::Hash` stores a
/// string that can be interpreted as a valid identifier (for example,
/// `#main-content`), or if instead it is unrestricted, that is, it cannot be
/// interpreted as an identifier (for example, because it starts with a digit,
/// like in the hex color `#00ff00`).
///
enum class TokenHashFlag : Int8 {
    Identifier,
    Unrestricted
};

/// \enum vgc::style::TokenNumericFlag
/// \brief The flag component of a token of numeric type.
///
/// This flag informs whether a token of type `TokenType::Number`,
/// `TokenType::Percentage`, or `TokenType::Dimension` stores a
/// numeric value stored as an integer or a floating point.
///
enum class TokenNumericFlag : Int8 {
    Integer,
    FloatingPoint
};

/// \class vgc::style::TokenNumericValue
/// \brief The numeric value of a token of numeric type.
///
/// Stores the numeric value of a token of type `TokenType::Number`,
/// `TokenType::Percentage`, or `TokenType::Dimension`.
///
/// The numeric value can be either an `integer` or a `floatingPoint`, as
/// specified by the `Token::numericFlag()` of the token.
///
union TokenNumericValue {
    Int64 integer;
    double floatingPoint;
};

namespace detail {

class TokenStream;
class UnparsedValue;

} // namespace detail

/// \class vgc::style::Token
/// \brief One element of the output of tokenizing a style string.
///
/// See: https://www.w3.org/TR/css-syntax-3/#tokenization
///
class Token {
public:
    /// Returns the `TokenType` of this token.
    ///
    TokenType type() const {
        return type_;
    }

    /// Returns an iterator to the beginning of this token in the string it was
    /// parsed from.
    ///
    /// This iterator is guaranteed to be valid during the execution of a
    /// `Value` parsing function, but you shouldn't store a copy of this
    /// iterator as it can be invalidated afterwards.
    ///
    const char* begin() const {
        return begin_;
    }

    /// Returns an iterator to the end of this token in the string it was
    /// parsed from.
    ///
    /// This iterator is guaranteed to be valid during the execution of
    /// a `Value` parsing function, but you shouldn't store a copy of this
    /// iterator as it can be invalidated afterwards.
    ///
    const char* end() const {
        return end_;
    }

    /// If this token is of type `Hash`, this method returns whether the string
    /// after the hashtag can be interpreted as an identifier (e.g.,
    /// `#main-content`), or if it is a more generic string that cannot be
    /// interpred as an identifier (e.g., if it starts with a digit, such as in
    /// the hex color `#00ff00`).
    ///
    TokenHashFlag hashFlag() const {
        return static_cast<TokenHashFlag>(flag_);
    }

    /// If this token is of type `Number`, `Percentage`, or `Dimension`, this
    /// metohod returns whether the parsed value was an integer of a floating
    /// point.
    ///
    TokenNumericFlag numericFlag() const {
        return static_cast<TokenNumericFlag>(flag_);
    }

    /// Returns the string value of this token.
    ///
    /// Note that the string returned by this method is not the same as the
    /// range of character `[begin(), end())`, as it only include the most
    /// relevant information. For example:
    ///
    /// - for tokens of type `Dimension`: it only includes the unit
    /// - for tokens of type `Hash`, it does not include the hashtag characters
    /// - for tokens of type `Function`, it does not include the opening parenthesis
    ///
    std::string_view stringValue() const {
        return stringValue_;
    }

    /// Returns the numeric value of this token as a float. Assumes the type of
    /// this token is either `Number`, `Percentage`, or `Dimension`.
    ///
    /// If the `numericFlag()` of this token is `Integer`, the numeric
    /// value is converted to the nearest representable `float`.
    ///
    float floatValue() const {
        return numericFlag() == TokenNumericFlag::Integer
                   ? static_cast<float>(numericValue_.integer)
                   : static_cast<float>(numericValue_.floatingPoint);
    }

    /// Returns the numeric value of this token as an integer. Assumes the type of
    /// this token is either `Number`, `Percentage`, or `Dimension`.
    ///
    /// If the `numericFlag()` of this token is `FloatingPoint`, the numeric
    /// value is rounded to the nearest representable integer.
    ///
    Int64 intValue() const {
        return numericFlag() == TokenNumericFlag::Integer
                   ? numericValue_.integer
                   : static_cast<Int64>(std::round(numericValue_.floatingPoint));
    }

private:
    friend detail::TokenStream;
    friend detail::UnparsedValue;

    // Pointer to the original stylesheet string, or to a copy of a subset of
    // this string (see detail::UnparsedValue).
    //
    // XXX How to be sure that the underlying string is kept alive?
    // Should we use a better design, e.g., having all token store
    // a shared_ptr to the original string?
    //
    const char* begin_;
    const char* end_;

    std::string stringValue_; // XXX Use string_view?
    TokenNumericValue numericValue_;
    TokenType type_;
    Int8 flag_;

    // Initializes a dummy token starting and ending at s.
    // All other fields are uninitialized.
    Token(const char* s)
        : begin_(s)
        , end_(s)
        , type_(TokenType::Delimiter) {
    }

    void setHashFlag_(TokenHashFlag value) {
        flag_ = static_cast<Int8>(value);
    }

    void setNumericFlag_(TokenNumericFlag value) {
        flag_ = static_cast<Int8>(value);
    }
};

/// Writes the given Token to the output stream.
///
template<typename OStream>
void write(OStream& out, const Token& token) {
    using core::write;
    write(out, token.type());
    switch (token.type()) {
    case TokenType::Identifier:
    case TokenType::Function:
    case TokenType::AtKeyword:
    case TokenType::String:
    case TokenType::Url:
    case TokenType::Delimiter:
        write(out, "(\"");
        write(out, token.stringValue());
        write(out, "\")");
        break;
    case TokenType::Hash:
        write(out, "(");
        if (token.hashFlag() == TokenHashFlag::Identifier) {
            write(out, "(Identifier, \"");
        }
        else {
            write(out, "(Unrestricted, \"");
        }
        write(out, token.stringValue());
        write(out, "\")");
        break;
    case TokenType::Number:
    case TokenType::Percentage:
    case TokenType::Dimension:
        write(out, "(");
        if (token.numericFlag() == TokenNumericFlag::FloatingPoint) {
            write(out, "Integer, ");
            write(out, token.intValue());
        }
        else {
            write(out, "FloatingPoint, ");
            write(out, token.floatValue());
        }
        if (token.type() == TokenType::Dimension) {
            write(out, ", \"");
            write(out, token.stringValue());
            write(out, "\"");
        }
        write(out, ")");
        break;
    default:
        break;
    }
}

/// \typedef vgc::style::TokenArray
/// \brief The output of tokenizing a style stream.
///
/// \sa Token
///
using TokenArray = core::Array<Token>;

/// \typedef vgc::style::TokenIterator
/// \brief Iterating over a TokenArray
///
/// \sa TokenArray, Token
///
using TokenIterator = core::Array<Token>::const_iterator;

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
/// \sa TokenArray, Token,
///
std::string decodeStyleString(std::string_view s);

/// Tokenizes the given null-terminated string into an array of Tokens.
/// The given null-terminated string must outlive the tokens, since they point
/// to characters in the string. The given null-terminated string is assumed to
/// be already "decoded" using decodeStyleString().
///
/// \sa TokenArray, Token
///
TokenArray tokenizeStyleString(const char* s);

} // namespace vgc::style

#endif // VGC_STYLE_TOKEN_H
