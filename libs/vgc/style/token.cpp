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

#include <vgc/style/token.h>

#include <string.h> // strcmp
#include <vgc/core/parse.h> // readTo double and integer

namespace vgc::style {

const char* toStringLiteral(StyleTokenType type) {
    using StringLiteral = const char*;
    static constexpr StringLiteral s[] = {
        "EndOfFile",
        "Identifier",
        "Function",
        "AtKeyword",
        "Hash",
        "String",
        "BadString",
        "Url",
        "BadUrl",
        "Delimiter",
        "Number",
        "Percentage",
        "Dimension",
        "Whitespace",
        "Colon",
        "Semicolon",
        "Comma",
        "LeftSquareBracket",
        "RightSquareBracket",
        "LeftParenthesis",
        "RightParenthesis",
        "LeftCurlyBracket",
        "RightCurlyBracket"
    };
    return s[static_cast<Int8>(type)];
}

namespace {

// Note: in the CSS specification, the tokenizer algorithm is defined in terms
// of Unicode code points. In our implementation, we directly use UTF-8 bytes
// instead: it works as is in most cases (since UTF-8 is ASCII compatible and
// the tokenizer typically search for specific ASCII characters), but in some
// cases special care is needed.
//
// Most of the helper functions below work with the type `char`. This represents
// the first byte of a UTF-8 encoded character.

const char eof = '\0';
const char* replacementCharacter = u8"\uFFFD";

// https://www.w3.org/TR/css-syntax-3/#digit
bool isDigit_(char c)
{
    return (c >= '0' && c <= '9');
}

// https://www.w3.org/TR/css-syntax-3/#hex-digit
bool isHexDigit_(char c)
{
    return (c >= '0' && c <= '9') ||
           (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

// https://www.w3.org/TR/css-syntax-3/#hex-digit
UInt32 hexDigitToUInt32_(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    }
    else {
        return 10 + c - 'a';
    }
}

// https://infra.spec.whatwg.org/#surrogate
bool isSurrogateCodePoint_(UInt32 c)
{
    return c >= 0xD800 && c <= 0xDFFF;
}

// https://www.w3.org/TR/css-syntax-3/#maximum-allowed-code-point
bool isGreaterThanMaximumAllowedCodePoint_(UInt32 c)
{
    return c > 0x10FFFF;
}

// https://www.w3.org/TR/css-syntax-3/#uppercase-letter
bool isUppercaseLetter_(char c)
{
    return (c >= 'A' && c <= 'Z');
}

// https://www.w3.org/TR/css-syntax-3/#lowercase-letter
bool isLowercaseLetter_(char c)
{
    return (c >= 'a' && c <= 'z');
}

// https://www.w3.org/TR/css-syntax-3/#letter
bool isLetter_(char c)
{
    return isUppercaseLetter_(c) || isLowercaseLetter_(c);
}

// https://www.w3.org/TR/css-syntax-3/#non-ascii-code-point
bool isNonAsciiCodePoint_(char c)
{
    unsigned char c_ = static_cast<char>(c);
    return c_ >= 0x80;
}

// https://www.w3.org/TR/css-syntax-3/#name-start-code-point
bool isNameStartCodePoint_(char c)
{
    return isLetter_(c) || isNonAsciiCodePoint_(c) || c == '_';
}

// https://www.w3.org/TR/css-syntax-3/#name-code-point
bool isNameCodePoint_(char c)
{
    return isNameStartCodePoint_(c) || isDigit_(c) || c == '-';
}

// https://www.w3.org/TR/css-syntax-3/#non-printable-code-point
bool isNonPrintableCodePoint_(char c)
{
    unsigned char c_ = static_cast<char>(c);
    return c_ <= 0x08 ||
           c_ == 0x0B ||
           (c_ >= 0x0E && c_ <= 0x1F) ||
           c_ == 0x7F;
}

// https://www.w3.org/TR/css-syntax-3/#whitespace
bool isWhitespace_(char c)
{
    return c == '\n' || c == '\t' || c == ' ';
}

// Determines whether this byte is a non-continuation byte of a valid UTF-8
// encoded stream. These have the form:
//
// 0xxxxxxx, or
// 110xxxxx, or
// 1110xxxx, or
// 11110xxx
//
// Returns the number of bytes of this character.
//
size_t isUtf8NonContinuationByte_(char c) {
    unsigned char c_ = static_cast<char>(c);
    if (c_ < 0x80) {
        return 1;
    }
    else if ((c_ >> 5) == 6) {
        return 2;
    }
    else if ((c_ >> 4) == 14) {
        return 3;
    }
    else if ((c_ >> 5) == 30) {
        return 4;
    }
    else {
        return 0;
    }
}

// Determines whether this byte is a continuation byte of a valid UTF-8 encoded
// stream. These have the form 10xxxxxx.
//
bool isUtf8ContinuationByte_(char c) {
    unsigned char c_ = static_cast<char>(c);
    return (c_ >> 6) == 2;
}

class TokenStream {
public:
    // Constructs a TokenStream from the given character range. The given
    // character range must outlive the TokenStream, since the tokens point to
    // characters in the range. The given character range is assumed to be
    // already "decoded" and contain a final '\0' character.
    TokenStream(const char* s) :
        c1p_(nullptr),
        c1_(eof),
        c2_(*s),
        token_(s),
        hasNext_(false) {}

    // Consumes and returns the next token. The behavior is undefined if the
    // previous token was 'Eof'.
    StyleToken get() {
        if (hasNext_) {
            hasNext_ = false;
        }
        else {
            consumeToken_();
        }
        return token_;
    }

    // Unconsumes the current token, such that the current token will be
    // consumed again in the next call of get(). The behavior is undefined if
    // get() has never been called, or called twice in a row without an
    // inbetween call to get().
    void unget() {
        hasNext_ = true;
    }

private:
    // https://www.w3.org/TR/css-syntax-3/#current-input-code-point
    // https://www.w3.org/TR/css-syntax-3/#next-input-code-point
    const char* c1p_; // pointer to first byte of current input code point
    char c1_;         // == *c1p_
    char c2_;         // == *token_.end
    char c3_;         // == *(token_.end+1)    (or eof if c2_ is eof)
    char c4_;         // == *(token_.end+2)    (or eof if c3_ is eof)
    StyleToken token_; // Last consumed token or currently being consumed token
                  // token_.end: pointer to first byte of next input code point
    bool hasNext_; // Whether the next token is already computed

    // Consumes the next input code point. This advances token_.end by one
    // UTF-8 encoded code point, and sets c1_ and c1_ accordingly.
    //
    // Init:    |--|---|--|EOF          -    : one byte
    //           ^                     |---| : one code-point
    //           c2 (= *token_.end)     EOF  : null-terminating byte
    //
    // Consume: |--|---|--|EOF
    //           ^  ^
    //          c1  c2
    //
    // Consume: |--|---|--|EOF
    //              ^   ^
    //             c1   c2
    //
    // Consume: |--|---|--|EOF
    //                  ^  ^
    //                 c1  c2
    //
    // Consume: |--|---|--|EOF
    //                     ^
    //                    c1,c2
    //
    // Consume: |--|---|--|EOF
    //                     ^
    //                    c1,c2
    //
    // Note that if c2_ is equal to EOF before calling this function, then
    // token_.end is not advanced, c1p_ becomes equal to token_.end, and c1_
    // becomes EOF. The input stream then stays forever in this state, even if
    // reconsumeInput_() is called.
    //
    void consumeInput_() {
        c1p_ = token_.end;
        c1_ = c2_;
        if (c1_ != eof) {
            do {
                ++token_.end;
                c2_ = *token_.end;
            } while (isUtf8ContinuationByte_(c2_));
            c3_ = (c2_ == eof) ? eof : *(token_.end + 1);
            c4_ = (c3_ == eof) ? eof : *(token_.end + 2);
        }
    }

    // Reconsumes the current input code point.
    // Sets the value of c2_ accordingly.
    // Leaves the value of c1p_ and c1_ undefined (typically, there should be
    // a call to consumeInput_() just after calling reconsumeInput_()).
    // Undefined behavior if token_.end = first char of input.
    // https://www.w3.org/TR/css-syntax-3/#reconsume-the-current-input-code-point
    void reconsumeInput_() {
        if (c1_ != eof) {
            do {
                --token_.end;
                c2_ = *token_.end;
            } while (isUtf8ContinuationByte_(c2_));
            c3_ = (c2_ == eof) ? eof : *(token_.end + 1);
            c4_ = (c3_ == eof) ? eof : *(token_.end + 2);
        }
    }

    // Add the current code point to the token value
    //
    void appendCurrentCodePointToTokenValue_() {
        for(const char* c = c1p_; c != token_.end; ++c) {
            token_.codePointsValue += *c;
        }
    }

    // Returns whether the next input code points are equal to the given code
    // points. Returns false if the current input code points is EOF.
    bool areNextCodePointsEqualTo_(const char* s) {
        if (c1_ == eof) {
            return false;
        }
        return strcmp(s, token_.end) == 0;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-token
    void consumeToken_() {
        token_.begin = token_.end;
        token_.codePointsValue.clear();
        consumeComments_();
        consumeInput_();
        switch (c1_) {
        case eof:
            token_.type = StyleTokenType::EndOfFile;
            break;
        case ' ':
        case '\t':
        case '\n':
            consumeWhitespace_();
            break;
        case '"':
        case '\'':
            consumeStringToken_();
            break;
        case '#':
            if (isNameCodePoint_(c2_) || startsValidEscape_(c2_, c3_)) {
                token_.type = StyleTokenType::Hash;
                token_.setHashFlag(StyleTokenHashFlag::Unrestricted);
                if (startsIdentifier_(c2_, c3_, c4_)) {
                    token_.setHashFlag(StyleTokenHashFlag::Identifier);
                }
                consumeName_();
            }
            else {
                token_.type = StyleTokenType::Delimiter;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case '(':
            token_.type = StyleTokenType::LeftParenthesis;
            break;
        case ')':
            token_.type = StyleTokenType::RightParenthesis;
            break;
        case '+':
            if (startsNumber_(c1_, c2_, c3_)) {
                reconsumeInput_();
                consumeNumericToken_();
            }
            else {
                token_.type = StyleTokenType::Delimiter;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case ',':
            token_.type = StyleTokenType::Comma;
            break;
        case '-':
            if (startsNumber_(c1_, c2_, c3_)) {
                reconsumeInput_();
                consumeNumericToken_();
            }
            // Uncomment if you wish to support CDO/CDC tokens
            // else if (areNextCodePointsEqualTo_("->")) {
            //     consumeInput_();
            //     consumeInput_();
            //     token_.type = StyleTokenType::CommentDelimiterClose;
            // }
            else if (startsIdentifier_(c1_, c2_, c3_)) {
                reconsumeInput_();
                consumeIdentifierLikeToken_();
            }
            else {
                token_.type = StyleTokenType::Delimiter;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case '.':
            if (startsNumber_(c1_, c2_, c3_)) {
                reconsumeInput_();
                consumeNumericToken_();
            }
            else {
                token_.type = StyleTokenType::Delimiter;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case ':':
            token_.type = StyleTokenType::Colon;
            break;
        case ';':
            token_.type = StyleTokenType::Semicolon;
            break;
        // Uncomment if you wish to support CDO/CDC tokens
        // case '<':
        //     if (areNextCodePointsEqualTo_("!--")) {
        //         consumeInput_();
        //         consumeInput_();
        //         consumeInput_();
        //         token_.type = StyleTokenType::CommentDelimiterOpen;
        //     }
        //     else {
        //         token_.type = StyleTokenType::Delimiter;
        //         appendCurrentCodePointToTokenValue_();
        //     }
        //     break;
        case '@':
            if (startsIdentifier_(c2_, c3_, c4_)) {
                token_.type = StyleTokenType::AtKeyword;
                consumeName_();
            }
            else {
                token_.type = StyleTokenType::Delimiter;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case '[':
            token_.type = StyleTokenType::LeftSquareBracket;
            break;
        case ']':
            token_.type = StyleTokenType::RightSquareBracket;
            break;
        case '{':
            token_.type = StyleTokenType::LeftCurlyBracket;
            break;
        case '}':
            token_.type = StyleTokenType::RightCurlyBracket;
            break;
        case '\\':
            if (startsValidEscape_(c1_, c2_)) {
                reconsumeInput_();
                consumeIdentifierLikeToken_();
            }
            else {
                // Parse error!
                token_.type = StyleTokenType::Delimiter;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        default:
            if (isDigit_(c1_)) {
                reconsumeInput_();
                consumeNumericToken_();
            }
            else if (isNameStartCodePoint_(c1_)) {
                reconsumeInput_();
                consumeIdentifierLikeToken_();
            }
            else {
                token_.type = StyleTokenType::Delimiter;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-comment
    // https://www.w3.org/TR/css-syntax-3/#serialization
    //
    // "The tokenizer described in this specification does not produce tokens
    // for comments, or otherwise preserve them in any way. Implementations may
    // preserve the contents of comments and their location in the token
    // stream. If they do, this preserved information must have no effect on
    // the parsing step."
    //
    // For now, we choose not to produce tokens for comments. This is why we
    // use a 'while' loop (to consume successive comments), and do not set a
    // token. Note that this means that there can be consecutive Whitespace
    // tokens.
    //
    void consumeComments_() {
        while (c2_ == '/') {
            char c3 = *(token_.end + 1);
            if (c3 != '*') {
                break;
            }
            consumeInput_();
            consumeInput_();
            while (c2_ != eof) {
                consumeInput_();
                if (c1_ == '*' && c2_ == '/') {
                    consumeInput_();
                    break; // We've consumed a valid comment
                }
            }
            // Here, either we've consumed a valid comment, or we reached EOF,
            // or both. If we reached EOF without consuming a valid comment,
            // then it's a parse error, but we ignore it and keep going: the
            // next token will be an EOF token.
        }
    }

    void consumeWhitespace_() {
        token_.type = StyleTokenType::Whitespace;
        while (isWhitespace_(c2_)) {
            consumeInput_();
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-string-token
    void consumeStringToken_() {
        token_.type = StyleTokenType::String;
        const char endingCodePoint = c1_;
        while (true) {
            consumeInput_();
            if (c1_ == endingCodePoint) {
                return;
            }
            else if (c1_ == eof) {
                // Parse error. CSS3 spec says "return the <string-token>".
                // But for VGCSS, we reconsume the EOF and return a BadString.
                reconsumeInput_();
                token_.type = StyleTokenType::BadString;
                return;
            }
            else if (c1_ == '\n') {
                // Parse error. CSS3 spec says "Reconsume the current input
                // code point, create a <bad-string-token>, and return it".
                reconsumeInput_();
                token_.type = StyleTokenType::BadString;
                return;
            }
            else if (c1_ == '\\') {
                if (c2_ == eof) {
                    // Parse error. CSS3 spec says "do nothing". In CSS3, this
                    // means that the next iteration will consume the EOF and
                    // return the string token. In VGCSS, this means that the
                    // next iteration will consume the EOF, reconsume it, and
                    // return a BadString.
                }
                else if (c2_ == '\n') {
                    consumeInput_();
                }
                else {
                    consumeEscapedCodePoint_();
                }
            }
            else {
                appendCurrentCodePointToTokenValue_();
            }
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#starts-with-a-valid-escape
    bool startsValidEscape_(char c1, char c2) {
        return (c1 == '\\') && (c2 != '\n');
    }

    // https://www.w3.org/TR/css-syntax-3/#would-start-an-identifier
    bool startsIdentifier_(char c1, char c2, char c3) {
        if (c1 == '-') {
            if (isNameStartCodePoint_(c2) ||
                c2 == '-' ||
                startsValidEscape_(c2, c3))
            {
                return true;
            }
            else {
                return false;
            }
        }
        else if (isNameStartCodePoint_(c1)) {
            return true;
        }
        else if (c1 == '\\') {
            if (startsValidEscape_(c1, c2)) {
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#starts-with-a-number
    bool startsNumber_(char c1, char c2, char c3) {
        if (c1 == '+' || c1 == '-') {
            c1 = c2;
            c2 = c3;
        }
        return isDigit_(c1) || (c1 == '.' && isDigit_(c2));
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-escaped-code-point
    // The returned codePoint is appended directly to token_.codePointsValue
    void consumeEscapedCodePoint_() {
        consumeInput_();
        if (c1_ == eof) {
            // Parse error. CSS3 spec says "return U+FFFD REPLACEMENT CHARACTER".
            token_.codePointsValue += replacementCharacter;
            reconsumeInput_();
        }
        else if (isHexDigit_(c1_)) {
            // Consume as many hex digits as possible (max 6)
            int numDigits = 1;
            UInt32 codePoint = hexDigitToUInt32_(c1_);
            while (numDigits < 6 && isHexDigit_(c2_ )) {
                codePoint = 16 * codePoint + hexDigitToUInt32_(c2_);
                ++numDigits;
                consumeInput_();
            }
            // Consume trailing whitespace
            if (isWhitespace_(c2_)) {
                consumeInput_();
                // Note: this means that a newline may appear in a string token.
                // See https://github.com/w3c/csswg-drafts/issues/5835
            }
            // Convert code point to UTF-8 bytes
            if (codePoint == 0 ||
                isSurrogateCodePoint_(codePoint) ||
                isGreaterThanMaximumAllowedCodePoint_(codePoint))
            {
                token_.codePointsValue += replacementCharacter;
            }
            else if (codePoint < 0x80) {
                // 1-byte UTF-8: 0xxxxxxx
                token_.codePointsValue += static_cast<char>(codePoint);
            }
            else if (codePoint < 0x800) {
                // 2-byte UTF-8: 110xxxxx 10xxxxxx
                UInt32 x1 = codePoint >> 6;        // 6 = num of x's in 2nd byte
                UInt32 x2 = codePoint - (x1 << 6); // 6 = num of x's in 2nd byte
                UInt32 b1 = (6 << 5) + x1; // 6 = 0b110 ; 5 = num of x's in 1st byte
                UInt32 b2 = (2 << 6) + x2; // 2 = 0b10  ; 6 = num of x's in 2nd byte
                token_.codePointsValue += static_cast<char>(b1);
                token_.codePointsValue += static_cast<char>(b2);
            }
            else if (codePoint < 0x10000) {
                // 3-byte UTF-8: 1110xxxx 10xxxxxx 10xxxxxx
                UInt32 y1 = codePoint >> 6;
                UInt32 x3 = codePoint - (y1 << 6);
                UInt32 x1 = y1 >> 6;
                UInt32 x2 = y1 - (x1 << 6);
                UInt32 b1 = (14 << 4) + x1;
                UInt32 b2 = (2 << 6) + x2;
                UInt32 b3 = (2 << 6) + x3;
                token_.codePointsValue += static_cast<char>(b1);
                token_.codePointsValue += static_cast<char>(b2);
                token_.codePointsValue += static_cast<char>(b3);
            }
            else {
                // 4-byte UTF-8: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                UInt32 y2 = codePoint >> 6;
                UInt32 x4 = codePoint - (y2 << 6);
                UInt32 y1 = y2 >> 6;
                UInt32 x3 = y2 - (y1 << 6);
                UInt32 x1 = y1 >> 6;
                UInt32 x2 = y1 - (x1 << 6);
                UInt32 b1 = (30 << 3) + x1;
                UInt32 b2 = (2 << 6) + x2;
                UInt32 b3 = (2 << 6) + x3;
                UInt32 b4 = (2 << 6) + x4;
                token_.codePointsValue += static_cast<char>(b1);
                token_.codePointsValue += static_cast<char>(b2);
                token_.codePointsValue += static_cast<char>(b3);
                token_.codePointsValue += static_cast<char>(b4);
            }
        }
        else {
            appendCurrentCodePointToTokenValue_();
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-numeric-token
    void consumeNumericToken_() {
        consumeNumber_(); // write the numeric value and flag to token_
        char c3 = (c2_ == eof) ? eof : *(token_.end + 1);
        char c4 = (c3 == eof) ? eof : *(token_.end + 2);
        if (startsIdentifier_(c2_, c3, c4)) {
            token_.type = StyleTokenType::Dimension;
            consumeName_(); // write the unit to token_.codePointsValue
        }
        else if (c2_ == '%') {
            consumeInput_();
            token_.type = StyleTokenType::Percentage;
        }
        else {
            token_.type = StyleTokenType::Number;
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-number
    // The returned value and type is directly set in the token's numeric value and flag
    // Note that we use token_.codePointsValue as a buffer to store the
    // repr of the number. It is assume to be initially empty.
    void consumeNumber_() {
        token_.setNumericFlag(StyleTokenNumericFlag::Integer);
        if (c2_ == '+' || c2_ == '-') {
            token_.codePointsValue += c2_;
            consumeInput_();
        }
        while (isDigit_(c2_)) {
            token_.codePointsValue += c2_;
            consumeInput_();
        }
        char c3 = (c2_ == eof) ? eof : *(token_.end + 1);
        if (c2_ == '.' && isDigit_(c3)) {
            token_.setNumericFlag(StyleTokenNumericFlag::FloatingPoint);
            token_.codePointsValue += c2_;
            token_.codePointsValue += c3;
            consumeInput_();
            consumeInput_();
            while (isDigit_(c2_)) {
                token_.codePointsValue += c2_;
                consumeInput_();
            }
        }
        if (c2_ == 'e' || c2_ == 'E') {
            c3 = (c2_ == eof) ? eof : *(token_.end + 1);
            if (isDigit_(c3)) {
                token_.setNumericFlag(StyleTokenNumericFlag::FloatingPoint);
                token_.codePointsValue += c2_;
                token_.codePointsValue += c3;
                consumeInput_();
                consumeInput_();
            }
            else if ((c3 == '+' || c3 == '-')) {
                char c4 = (c3 == eof) ? eof : *(token_.end + 2);
                if (isDigit_(c4)) {
                    token_.setNumericFlag(StyleTokenNumericFlag::FloatingPoint);
                    token_.codePointsValue += c2_;
                    token_.codePointsValue += c3;
                    token_.codePointsValue += c4;
                    consumeInput_();
                    consumeInput_();
                    consumeInput_();
                }
            }
            while (isDigit_(c2_)) {
                token_.codePointsValue += c2_;
                consumeInput_();
            }
        }
        core::StringReader sr(token_.codePointsValue);
        if (token_.numericFlag() == StyleTokenNumericFlag::FloatingPoint) {
            core::readTo(token_.numericValue.floatingPoint, sr);
        }
        else {
            core::readTo(token_.numericValue.integer, sr);
        }
        token_.codePointsValue.clear();
    }

    bool isUrl_() {
        if (token_.codePointsValue.size() != 3) {
            return false;
        }
        char c1 = token_.codePointsValue[0];
        char c2 = token_.codePointsValue[1];
        char c3 = token_.codePointsValue[2];
        return (c1 == 'u' || c1 == 'U') &&
               (c2 == 'r' || c2 == 'R') &&
               (c3 == 'l' || c3 == 'L') &&
               c2_ == '(';
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-ident-like-token
    void consumeIdentifierLikeToken_() {
        consumeName_();
        if (isUrl_()) {
            consumeInput_();
            // Consume all whitespace characters except the last.
            // Note: keeping one whitespace ensures that we generate a
            // whitespace token if this identifier-like token is a function
            // token rather than a URL token
            while (isWhitespace_(c2_) && isWhitespace_(c3_)) {
                consumeInput_();
            }
            if (c2_ == '\"' || c2_ == '\'' ||
                    (isWhitespace_(c2_) && (c3_ == '\"' || c3_ == '\'')))
            {
                token_.type = StyleTokenType::Function;
            }
            else {
                token_.codePointsValue.clear();
                consumeUrlToken_();
            }
        }
        else if (c2_ == '(') {
            consumeInput_();
            token_.type = StyleTokenType::Function;
        }
        else {
            token_.type = StyleTokenType::Identifier;
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-name
    // The returned string is appended directly to token_.codePointsValue
    void consumeName_() {
        while (true) {
            consumeInput_();
            if (isNameCodePoint_(c1_)) {
                appendCurrentCodePointToTokenValue_();
            }
            else if (startsValidEscape_(c1_, c2_)) {
                consumeEscapedCodePoint_();
            }
            else {
                reconsumeInput_();
                break;
            }
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-url-token
    // The returned url is appended directly to token_.codePointsValue
    void consumeUrlToken_() {
        token_.type = StyleTokenType::Url;
        while (isWhitespace_(c2_)) {
            consumeInput_();
        }
        while (true) {
            consumeInput_();
            if (c1_ == ')') {
                return;
            }
            else if (c1_ == eof) {
                // Parse error. Standard says "return the url token".
                return;
            }
            else if (isWhitespace_(c1_)) {
                while (isWhitespace_(c2_)) {
                    consumeInput_();
                }
                if (c2_ == ')') {
                    consumeInput_();
                }
                else if (c2_ == eof) {
                    // Parse error. Standard says "return the url token".
                    consumeInput_();
                }
                else {
                    consumeBadUrlRemnants_();
                }
                return;
            }
            else if (c1_ == '\"' || c1_ == '\'' ||
                     isNonPrintableCodePoint_(c1_))
            {
                // Parse error.
                consumeBadUrlRemnants_();
                return;
            }
            else if (c1_ == '\\') {
                if (startsValidEscape_(c1_, c2_)) {
                    consumeEscapedCodePoint_();
                }
                else {
                    // Parse error
                    consumeBadUrlRemnants_();
                    return;
                }
            }
            else {
                appendCurrentCodePointToTokenValue_();
            }
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-the-remnants-of-a-bad-url
    void consumeBadUrlRemnants_() {
        token_.type = StyleTokenType::BadUrl;
        while (true) {
            consumeInput_();
            if (c1_ == ')' || c1_ == eof) {
                break;
            }
            else if (startsValidEscape_(c1_, c2_)) {
                consumeEscapedCodePoint_();
            }
            else {
                // do nothing
            }
        }
        // Remove code points added by consumeEscapedCodePoint_()
        token_.codePointsValue.clear();
    }
};

} // namespace

std::string decodeStyleString(std::string_view s)
{
    std::string res;
    if (s.size() == 0) {
        // Nothing
    }
    else if (s.size() == 1) {
        char c = s[0];
        if (c == '\r' || c == '\f') {
            res.reserve(2);
            res += '\n';
        }
        else if (c == '\0' || !(isUtf8NonContinuationByte_(c) != 1)) {
            res.reserve(5);
            res += replacementCharacter;
        }
        else {
            res.reserve(2);
            res += c;
        }
    }
    else {
        // Pre-allocate memory. In case of CRLF -> LF, it might be slightly
        // more than necessary. In case of '\0' -> "\uFFFD", it might not be
        // enough and cause a reallocation. Either way, it's no big deal and
        // there's no need to be smarter than this. The '+1' is for the final
        // EOF character.
        res.reserve(s.size() + 1);

        // Decode all characters.
        for (size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '\r') {
                if (i + 1 < s.size() && s[i + 1] == '\n') {
                    ++i;
                }
                res += '\n';
            }
            else if (c == '\f') {
                res += '\n';
            }
            else if (c == '\0') {
                res += replacementCharacter;
            }
            else {
                size_t numBytes = isUtf8NonContinuationByte_(c);
                if (numBytes == 0) {
                    // Invalid byte or un-expected continuation byte
                    res += replacementCharacter;
                }
                else if (i + numBytes > s.size()) {
                    // String ends before the end of the character
                    res += replacementCharacter;
                    break;
                }
                else {
                    bool isValid = true;
                    for (size_t j = i + 1; j < i + numBytes; ++j) {
                        if (!isUtf8ContinuationByte_(s[j])) {
                            // Non-continuation byte before the end of the character
                            res += replacementCharacter;
                            i = j - 1; // i will be j in next iteration
                            isValid = false;
                            break;
                        }
                    }
                    if (isValid) {
                        for (size_t j = i; j < i + numBytes; ++j) {
                            res += s[j];
                        }
                        i += numBytes - 1; // i will be "i + numBytes" in next iteration
                    }
                }
            }
        }
    }
    res += eof;
    return res;
}

StyleTokenArray tokenizeStyleString(const char* s)
{
    StyleTokenArray res;
    TokenStream stream(s);
    while (true) {
        StyleToken t = stream.get();
        if (t.type == StyleTokenType::EndOfFile) {
            break;
        }
        else {
            res.append(std::move(t));
        }
    }
    return res;
}

} // namespace vgc::style
