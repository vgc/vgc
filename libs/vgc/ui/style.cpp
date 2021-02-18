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

#include <vgc/ui/style.h>

#include <string.h> // strcmp
#include <vgc/core/io.h>
#include <vgc/core/parse.h>
#include <vgc/core/paths.h>
#include <vgc/ui/lengthtype.h>

namespace vgc {
namespace ui {

namespace {

StyleSheetPtr createGlobalStyleSheet() {
    std::string path = core::resourcePath("ui/stylesheets/default.vgcss");
    std::string s = core::readFile(path);
    return StyleSheet::create(s);
}

} // namespace

/// Get global stylesheet.
///
StyleSheet* styleSheet()
{
    static StyleSheetPtr s = createGlobalStyleSheet();
    return s.get();
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
const char* replacementCharacter = "\uFFFD";

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

// Decode the input string.
//
// References:
// https://www.w3.org/TR/css-syntax-3/#input-byte-stream
// https://www.w3.org/TR/css-syntax-3/#input-preprocessing
//
// Notes:
// - We only support UTF-8 encoding.
// - We don't actually "decode" the UTF-8 bytes to code points: we keep
//   it UTF-8 since we assume all std::string to be UTF-8. However,
//   we do check for invalid UTF-8, see:
//   https://en.wikipedia.org/wiki/UTF-8#Invalid_sequences_and_error_handling
// - As mandated by CSS, we replace CR, FF, and CRLF with LF.
// - As mandated by CSS, we replace U+0000 NULL with U+FFFD REPLACEMENT CHARACTER.
// - We append a final U+0000 NULL which we use as EOF, making tokenizing easier.
//
// TODO: detect invalid UTF-8 encoding, and replace the invalid bytes with
// U+FFFD REPLACEMENT CHARACTER. This is useful for security reasons, see:
// https://en.wikipedia.org/wiki/UTF-8#Invalid_sequences_and_error_handling
//
std::string decode_(const std::string& s)
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

// https://www.w3.org/TR/css-syntax-3/#tokenization
enum class TokenType : Int8 {
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

const char* toStringLiteral(TokenType type) {
    using StringLiteral = const char*;
    static constexpr StringLiteral s[] = {
        "Eof",
        "Ident",
        "Function",
        "AtKeyword",
        "Hash",
        "String",
        "BadString",
        "Url",
        "BadUrl",
        "Delim",
        "Number",
        "Percentage",
        "Dimension",
        "Whitespace",
        "Cdo",
        "Cdc",
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

template<typename OStream>
void write(OStream& out, TokenType type)
{
    core::write(out, toStringLiteral(type));
}

enum class TokenFlag : Int8 {
    Id,
    Unrestricted,
    Integer,
    Number
};

union TokenNumericValue {
    Int64 integer;
    double number;
};

struct Token {
    const char* begin;
    const char* end;
    std::string codePointsValue;
    TokenNumericValue numericValue;
    TokenType type;
    TokenFlag flag;

    // Initializes a dummy token starting and ending at s.
    // All other fields are uninitialized.
    Token(const char* s) : begin(s), end(s), type(TokenType::Delim) {}
};

template<typename OStream>
void write(OStream& out, const Token& token)
{
    using core::write;
    write(out, token.type);
    switch (token.type) {
    case TokenType::Ident:
    case TokenType::Function:
    case TokenType::AtKeyword:
    case TokenType::String:
    case TokenType::Url:
    case TokenType::Delim:
        write(out, "(\"");
        write(out, token.codePointsValue);
        write(out, "\")");
        break;
    case TokenType::Hash:
        write(out, "(");
        if (token.flag == TokenFlag::Id) {
            write(out, "(Id, \"");
        }
        else {
            write(out, "(Unrestricted, \"");
        }
        write(out, token.codePointsValue);
        write(out, "\")");
        break;
    case TokenType::Number:
    case TokenType::Percentage:
    case TokenType::Dimension:
        write(out, "(");
        if (token.flag == TokenFlag::Integer) {
            write(out, "Integer, ");
            write(out, token.numericValue.integer);
        }
        else {
            write(out, "Number, ");
            write(out, token.numericValue.number);
        }
        if (token.type == TokenType::Dimension) {
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
    Token get() {
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
    Token token_; // Last consumed token or currently being consumed token
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
            }
            while (isUtf8ContinuationByte_(c2_));
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
            }
            while (isUtf8ContinuationByte_(c2_));
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
            token_.type = TokenType::Eof;
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
                token_.type = TokenType::Hash;
                token_.flag = TokenFlag::Unrestricted;
                if (startsIdentifier_(c2_, c3_, c4_)) {
                    token_.flag = TokenFlag::Id;
                }
                consumeName_();
            }
            else {
                token_.type = TokenType::Delim;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case '(':
            token_.type = TokenType::LeftParenthesis;
            break;
        case ')':
            token_.type = TokenType::RightParenthesis;
            break;
        case '+':
            if (startsNumber_(c1_, c2_, c3_)) {
                reconsumeInput_();
                consumeNumericToken_();
            }
            else {
                token_.type = TokenType::Delim;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case ',':
            token_.type = TokenType::Comma;
            break;
        case '-':
            if (startsNumber_(c1_, c2_, c3_)) {
                reconsumeInput_();
                consumeNumericToken_();
            }
            else if (areNextCodePointsEqualTo_("->")) {
                consumeInput_();
                consumeInput_();
                token_.type = TokenType::Cdc;
            }
            else if (startsIdentifier_(c1_, c2_, c3_)) {
                reconsumeInput_();
                consumeIdentLikeToken_();
            }
            else {
                token_.type = TokenType::Delim;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case '.':
            if (startsNumber_(c1_, c2_, c3_)) {
                reconsumeInput_();
                consumeNumericToken_();
            }
            else {
                token_.type = TokenType::Delim;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case ':':
            token_.type = TokenType::Colon;
            break;
        case ';':
            token_.type = TokenType::Semicolon;
            break;
        case '<':
            if (areNextCodePointsEqualTo_("!--")) {
                consumeInput_();
                consumeInput_();
                consumeInput_();
                token_.type = TokenType::Cdo;
            }
            else {
                token_.type = TokenType::Delim;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case '@':
            if (startsIdentifier_(c2_, c3_, c4_)) {
                token_.type = TokenType::AtKeyword;
                consumeName_();
            }
            else {
                token_.type = TokenType::Delim;
                appendCurrentCodePointToTokenValue_();
            }
            break;
        case '[':
            token_.type = TokenType::LeftSquareBracket;
            break;
        case ']':
            token_.type = TokenType::RightSquareBracket;
            break;
        case '{':
            token_.type = TokenType::LeftCurlyBracket;
            break;
        case '}':
            token_.type = TokenType::RightCurlyBracket;
            break;
        case '\\':
            if (startsValidEscape_(c1_, c2_)) {
                reconsumeInput_();
                consumeIdentLikeToken_();
            }
            else {
                // Parse error!
                token_.type = TokenType::Delim;
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
                consumeIdentLikeToken_();
            }
            else {
                token_.type = TokenType::Delim;
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
        token_.type = TokenType::Whitespace;
        while (isWhitespace_(c2_)) {
            consumeInput_();
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-string-token
    void consumeStringToken_() {
        token_.type = TokenType::String;
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
                token_.type = TokenType::BadString;
                return;
            }
            else if (c1_ == '\n') {
                // Parse error. CSS3 spec says "Reconsume the current input
                // code point, create a <bad-string-token>, and return it".
                reconsumeInput_();
                token_.type = TokenType::BadString;
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
            token_.type = TokenType::Dimension;
            consumeName_(); // write the unit to token_.codePointsValue
        }
        else if (c2_ == '%') {
            consumeInput_();
            token_.type = TokenType::Percentage;
        }
        else {
            token_.type = TokenType::Number;
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-number
    // The returned value and type is directly set in token_.numericValue and token_.flag
    // Note that we use token_.codePointsValue as a buffer to store the
    // repr of the number. It is assume to be initially empty.
    void consumeNumber_() {
        token_.flag = TokenFlag::Integer;
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
            token_.flag = TokenFlag::Number;
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
            bool hasExponentialPart = false;
            c3 = (c2_ == eof) ? eof : *(token_.end + 1);
            if (isDigit_(c3)) {
                hasExponentialPart = true;
                token_.flag = TokenFlag::Number;
                token_.codePointsValue += c2_;
                token_.codePointsValue += c3;
                consumeInput_();
                consumeInput_();
            }
            else if ((c3 == '+' || c3 == '-')) {
                char c4 = (c3 == eof) ? eof : *(token_.end + 2);
                if (isDigit_(c4)) {
                    hasExponentialPart = true;
                    token_.flag = TokenFlag::Number;
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
        if (token_.flag == TokenFlag::Number) {
            core::readTo(token_.numericValue.number, sr);
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
    void consumeIdentLikeToken_() {
        consumeName_();
        if (isUrl_()) {
            consumeInput_();
            // Consume all whitespace characters except the last.
            // Note: keeping one whitespace ensures that we generate a
            // whitespace token if this ident-like token is a function
            // token rather than a URL token
            while (isWhitespace_(c2_) && isWhitespace_(c3_)) {
                consumeInput_();
            }
            if (c2_ == '\"' || c2_ == '\'' ||
                    (isWhitespace_(c2_) && (c3_ == '\"' || c3_ == '\'')))
            {
                token_.type = TokenType::Function;
            }
            else {
                token_.codePointsValue.clear();
                consumeUrlToken_();
            }
        }
        else if (c2_ == '(') {
            consumeInput_();
            token_.type = TokenType::Function;
        }
        else {
            token_.type = TokenType::Ident;
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
        token_.type = TokenType::Url;
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
        token_.type = TokenType::BadUrl;
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

using TokenArray = core::Array<Token>;
using TokenIterator = core::Array<Token>::iterator;

// Returns a TokenArray from the given character range. The given character
// range must outlive the tokens, since they point to characters in the range.
// The given character range is assumed to be already "decoded" and contain a
// final '\0' character.
//
TokenArray tokenize_(const char* s)
{
    TokenArray res;
    TokenStream stream(s);
    while (true) {
        Token t = stream.get();
        if (t.type == TokenType::Eof) {
            break;
        }
        else {
            res.append(std::move(t));
        }
    }
    return res;
}


} // namespace


namespace internal {

// https://www.w3.org/TR/css-syntax-3/#parsing
//
// Note: we use a class with static functions (rather than free functions) to
// make it easier for the StyleSheet class (and other classes) to simply
// befriend this class, instead of befriending all the free functions.
//
class StyleParser {
public:
    // https://www.w3.org/TR/css-syntax-3/#parse-stylesheet
    static StyleSheetPtr parseStyleSheet(const std::string& styleString) {
        StyleSheetPtr styleSheet = StyleSheet::create();
        std::string decoded = decode_(styleString);
        TokenArray tokens = tokenize_(decoded.data());
        bool topLevel = true;
        TokenIterator it = tokens.begin();
        core::Array<StyleRuleSetPtr> rules = consumeRuleList_(it, tokens.end(), topLevel);
        for (StyleRuleSetPtr& rule : rules) {
            styleSheet->appendChildObject_(rule.get());
            styleSheet->ruleSets_.append(rule.get());
        }
        return styleSheet;
    }

    // TODO: implement the other entry points, see:
    // https://www.w3.org/TR/css-syntax-3/#parser-entry-points

private:
    // https://www.w3.org/TR/css-syntax-3/#consume-list-of-rules
    // Note: we use 'styleSheet != nullptr' as top-level flag
    static core::Array<StyleRuleSetPtr> consumeRuleList_(TokenIterator& it, TokenIterator end, bool topLevel) {
        core::Array<StyleRuleSetPtr> res;
        while (true) {
            if (it == end) {
                break;
            }
            else if (it->type == TokenType::Whitespace) {
                ++it;
            }
            else if (it->type == TokenType::Cdo || it->type == TokenType::Cdc) {
                // We handle '<!--' and '-->' tokens by ignoring the tokens,
                // i.e., the block within the tokens are NOT commented out.
                // This is the intented behavior: these tokens are a historical
                // hack to allow embedding CSS within a HTML <style> element,
                // while not confusing ancient browsers that don't support the
                // <style> elements at all. See:
                //
                // https://stackoverflow.com/questions/9812489/html-comments-in-css
                //
                // Note: we might want to completely remove handling these for
                // our VGCSS syntax, which is slightly divergent from CSS anyway.
                // However let's keep it for now, in case we want to use this code
                // as a base for an actual CSS parser.
                //
                if (topLevel) {
                    ++it;
                    continue;
                }
                else {
                    StyleRuleSetPtr rule = consumeQualifiedRule_(it, end);
                    if (rule) {
                        res.append(rule);
                    }
                }
            }
            else if (it->type == TokenType::AtKeyword) {
                // TODO: append a StyleAtRule to the result
                consumeAtRule_(it, end);
            }
            else {
                StyleRuleSetPtr rule = consumeQualifiedRule_(it, end);
                if (rule) {
                    res.append(rule);
                }
            }
        }
        return res;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-at-rule
    static void consumeAtRule_(TokenIterator& it, TokenIterator end) {
        // For now, we just consume the rule without returning anything.
        // In the future, we'll return a StyleAtRule
        ++it; // skip At token
        while (true) {
            if (it == end) {
                // Parse Error: return the partially consumed AtRule
                break;
            }
            else if (it->type == TokenType::Semicolon) {
                ++it;
                break;
            }
            else if (it->type == TokenType::LeftCurlyBracket) {
                consumeSimpleBlock_(it, end);
                // TODO: assign the simple block to the AtRule's block
                break;
            }
            else {
                consumeComponentValue_(it, end);
                // TODO: append the component value to the AtRule's prelude
            }
        }
        // TODO: return the AtRule
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-qualified-rule
    //
    // Assumes `it != end`.
    //
    // Note: this function returns a null StyleRuleSetPtr when the spec says to
    // "return nothing".
    //
    // Note: https://www.w3.org/TR/css-syntax-3/#style-rules
    //
    //   « Qualified rules at the top-level of a CSS stylesheet are style
    //     rules. Qualified rules in other contexts may or may not be style
    //     rules, as defined by the context. »
    //
    // Since in this implementation, all calls to consumeQualifiedRule_() are
    // made at the top-level of the stylesheet, we treat all qualified rules as
    // style rules, and directly create and populate a StyleRuleSet. If we ever
    // come across a use case were a qualifed rule should not be a style rule,
    // then we'll have to make this implementation more generic.
    //
    static StyleRuleSetPtr consumeQualifiedRule_(TokenIterator& it, TokenIterator end) {
        StyleRuleSetPtr rule = StyleRuleSet::create();
        const char* preludeBegin = it->begin;
        while (true) {
            if (it == end) {
                // Parse Error: return nothing
                return StyleRuleSetPtr();
            }
            else if (it->type == TokenType::LeftCurlyBracket) {
                const char* preludeEnd = it->begin;
                ++it;

                // Set the whole prelude as one selector
                // TODO: parse the prelude as a list of selector:
                // https://www.w3.org/TR/selectors-4/#selector-list
                StyleSelectorPtr selector = StyleSelector::create();
                selector->text_ = std::string(preludeBegin, preludeEnd);
                rule->appendChildObject_(selector.get());
                rule->selectors_.append(selector.get());

                // Consume list of declarations
                bool expectRightCurlyBracket = true;
                core::Array<StyleDeclarationPtr> declarations = consumeDeclarationList_(it, end, expectRightCurlyBracket);
                for (StyleDeclarationPtr& declaration : declarations) {
                    rule->appendChildObject_(declaration.get());
                    rule->declarations_.append(declaration.get());
                }
                break;

                // Note: for a qualifed rule which is not a style rule, we
                // should more generically consume a simple block rather than a
                // declaration list.
            }
            else {
                consumeComponentValue_(it, end);
            }
        }
        return rule;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-list-of-declarations
    //
    // Note: in the link above, the case RightCurlyBracket is not handled,
    // because the spec assumes that the block is first parsed using
    // consumeSimpleBlock_(), and only then its content is parsed as a list of
    // declarations as a second pass. Instead, we do both in one pass, so we
    // need to handle the possibility of a closing RightCurlyBracket.
    //
    static core::Array<StyleDeclarationPtr> consumeDeclarationList_(TokenIterator& it, TokenIterator end, bool expectRightCurlyBracket) {
        core::Array<StyleDeclarationPtr> res;
        while (true) {
            if (it == end) {
                if (expectRightCurlyBracket) {
                    // Parse error: return the partially consumed list
                    break;
                }
                else {
                    // Finished consuming all declaration (not an error)
                    break;
                }
            }
            else if (it->type == TokenType::Whitespace) {
                ++it;
            }
            else if  (it->type == TokenType::Semicolon) {
                ++it;
            }
            else if (it->type == TokenType::AtKeyword) {
                consumeAtRule_(it, end);
                // Note: for now, the at rule is simply skipped and
                // not appended to the list of declarations.
            }
            else if (it->type == TokenType::Ident) {
                TokenIterator declarationBegin = it;
                while (true) {
                    if (it == end ||
                        it->type == TokenType::Semicolon ||
                        (expectRightCurlyBracket && it->type == TokenType::RightCurlyBracket)) {

                        break;
                    }
                    else {
                        consumeComponentValue_(it, end);
                    }
                }
                TokenIterator declarationEnd = it;
                StyleDeclarationPtr declaration = consumeDeclaration_(declarationBegin, declarationEnd);
                if (declaration) {
                    res.append(declaration);
                }
            }
            else if (expectRightCurlyBracket && it->type == TokenType::RightCurlyBracket) {
                ++it;
                break;
            }
            else {
                // Parse error: throw away component values until semicolon or eof
                while (true) {
                    if (it == end ||
                        it->type == TokenType::Semicolon ||
                        (expectRightCurlyBracket && it->type == TokenType::RightCurlyBracket)) {

                        break;
                    }
                    else {
                        consumeComponentValue_(it, end);
                    }
                }
            }
        }
        return res;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-declaration
    // Assumes that the current token is the identifier.
    // May return a null pointer in case of parse errors.
    static StyleDeclarationPtr consumeDeclaration_(TokenIterator& it, TokenIterator end) {
        StyleDeclarationPtr declaration = StyleDeclaration::create();
        declaration->property_ = it->codePointsValue;
        ++it;
        // Consume whitespaces
        while (it != end && it->type == TokenType::Whitespace) {
            ++it;
        }
        // Ensure first non-whitespace token is a Colon
        if (it == end || it->type != TokenType::Colon) {
            // Parse error: return nothing
            return StyleDeclarationPtr();
        }
        else {
            ++it;
        }
        // Consume whitespaces
        while (it != end && it->type == TokenType::Whitespace) {
            ++it;
        }
        // Consume value components
        if (it != end) {
            TokenIterator valueBegin = it;
            while (it != end) {
                consumeComponentValue_(it, end);
            }
            TokenIterator valueLast = it - 1;
            while (valueLast->type == TokenType::Whitespace) {
                --valueLast;
            }
            // TODO: handle "!important"
            declaration->value_ = std::string(valueBegin->begin, valueLast->end);
        }
        return declaration;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-component-value
    // Assumes that `it` is not end
    static void consumeComponentValue_(TokenIterator& it, TokenIterator end) {
        if (it->type == TokenType::LeftParenthesis ||
            it->type == TokenType::LeftCurlyBracket ||
            it->type == TokenType::LeftSquareBracket) {

            consumeSimpleBlock_(it, end);
            // TODO: return it
        }
        else if (it->type == TokenType::Function) {
            consumeFunction_(it, end);
            // TODO: return it
        }
        else {
            ++it;
            // TODO: return consumed token
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-simple-block
    // Assumes that the `it` token is a left parenthesis or left curly/square bracket.
    static void consumeSimpleBlock_(TokenIterator& it, TokenIterator end) {
        TokenType startToken = it->type;
        TokenType endToken;
        if (startToken == TokenType::LeftParenthesis) {
            endToken = TokenType::RightParenthesis;
        }
        else if (startToken == TokenType::LeftCurlyBracket) {
            endToken = TokenType::RightCurlyBracket;
        }
        else { // startToken == TokenType::LeftSquareBracket
            endToken = TokenType::RightSquareBracket;
        }
        ++it;
        while (true) {
            if (it == end) {
                // Parse error: return the block
                break;
            }
            else if (it->type == endToken) {
                ++it;
                break;
            }
            else {
                consumeComponentValue_(it, end);
                // TODO: append component value to block's value
            }
        }
        // TODO: return block
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-function
    // assumes `it` is a function token
    static void consumeFunction_(TokenIterator& it, TokenIterator end) {
        // TODO: create function object, and set its name to it->codePoints
        ++it;
        while (true) {
            if (it == end) {
                // Parse error: return the function
                break;
            }
            else if (it->type == TokenType::RightParenthesis) {
                ++it;
                break;
            }
            else {
                consumeComponentValue_(it, end);
                // TODO: append component value to function's value
            }
        }
        // TODO: return function
    }
};

} // namespace

StyleSheet::StyleSheet() :
    Object()
{

}

StyleSheetPtr StyleSheet::create()
{
    return StyleSheetPtr(new StyleSheet());
}

StyleSheetPtr StyleSheet::create(const std::string& s)
{
    return internal::StyleParser::parseStyleSheet(s);
}

StyleRuleSet::StyleRuleSet() :
    Object()
{

}

StyleRuleSetPtr StyleRuleSet::create()
{
    return StyleRuleSetPtr(new StyleRuleSet());
}

StyleSelector::StyleSelector() :
    Object()
{

}

StyleSelectorPtr StyleSelector::create()
{
    return StyleSelectorPtr(new StyleSelector());
}

StyleDeclaration::StyleDeclaration() :
    Object()
{

}

StyleDeclarationPtr StyleDeclaration::create()
{
    return StyleDeclarationPtr(new StyleDeclaration());
}

} // namespace ui
} // namespace vgc
