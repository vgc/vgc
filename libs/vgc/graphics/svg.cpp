// Copyright 2023 The VGC Developers
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

#include <vgc/graphics/svg.h>

#include <vgc/core/colors.h>
#include <vgc/core/stringutil.h> // startsWith, endsWith, trimmed, split
#include <vgc/core/xml.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/graphics/logcategories.h>

namespace vgc::graphics {

namespace {

/* Below are potential classes that we may or may not want to use/expose in the future.

/// \enum vgc::graphics::CssValueType
/// \brief Indicates which type of unit applies to a CSS value
///
/// https://www.w3.org/TR/2000/REC-DOM-Level-2-Style-20001113/css.html#CSS-CSSValue
///
enum class CssValueType {
    Inherit,
    PrimitiveValue,
    ValueList,
    Custom
};

/// \class vgc::graphics::CssValue
/// \brief Stores a CSS value.
///
/// https://www.w3.org/TR/2000/REC-DOM-Level-2-Style-20001113/css.html#CSS-CSSValue
///
class CssValue {
public:
    CssValue(CssValueType type)
        : type_(type) {
    }

    CssValueType valueType() const {
        return valueType_;
    }

private:
    CssValueType valueType_;
    // std::string text_;
};

/// \class vgc::graphics::SvgColor
/// \brief Stores the value of `fill` and `stroke` attributes.
///
/// https://www.w3.org/TR/SVG11/types.html#InterfaceSVGColor
/// https://www.w3.org/TR/SVG11/types.html#DataTypeColor
/// https://www.w3.org/TR/2000/REC-DOM-Level-2-Style-20001113/css.html#CSS-RGBColor
///
class SvgColor : public CssValue {
   ...

private:
    SvgColorType colorType_;
    core::Color color_;
    IccColor iccColor_;
    ...
};
*/

// Returns whether the given character is a whitespace character.
//
bool isWhitespace(char c) {
    // Note: CSS accepts form feeds ('\f' or 0xC in C++), but SVG doesn't.
    //
    return c == 0x20 || c == 0x9 || c == 0xD || c == 0xA;
}

// Returns whether the given character is a [a-zA-Z] character.
//
bool isAlpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

// Returns whether the given character is a [0-9] character.
//
bool isDigit(char c) {
    return '0' <= c && c <= '9';
}

// Returns whether the given character is '+' or '-'.
//
bool isSign(char c) {
    return c == '+' || c == '-';
}

// Returns whether the string [it, end) starts with a number (or an unsigned
// number if `isSignAllowed` is false), as defined by the SVG 1.1 grammar:
//
//   https://www.w3.org/TR/SVG11/paths.html#PathDataBNF
//
//   number:   sign? unsigned
//   unsigned: ((digit+ "."?) | (digit* "." digit+)) exp?
//   exp:      ("e" | "E") sign? digit+
//   sign:     "+" | "-"
//   digit:    "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
//
// If a number is found, then the iterator `it` is advanced to the position
// just after the number. Otherwise, it is left unchanged.
//
// If a number is found, and the optional output parameter `number` is given,
// then it is set to the value of the number. Otherwise, it is left unchanged.
//
// Note: This function does NOT ignore leading whitespaces, that is,
// readNumber(" 42") returns false.
//
// Note: This function consumes as much as possible of the input string, as per
// the SVG grammar specification:
//
//   https://www.w3.org/TR/SVG11/paths.html#PathDataBNF
//
//   The processing of the BNF must consume as much of a given BNF production
//   as possible, stopping at the point when a character is encountered which
//   no longer satisfies the production. Thus, in the string "M 100-200", the
//   first coordinate for the "moveto" consumes the characters "100" and stops
//   upon encountering the minus sign because the minus sign cannot follow a
//   digit in the production of a "coordinate". The result is that the first
//   coordinate will be "100" and the second coordinate will be "-200".
//
//   Similarly, for the string "M 0.6.5", the first coordinate of the "moveto"
//   consumes the characters "0.6" and stops upon encountering the second
//   decimal point because the production of a "coordinate" only allows one
//   decimal point. The result is that the first coordinate will be "0.6" and
//   the second coordinate will be ".5".
//
// Note: In SVG 2, trailing commas have been disallowed, that is, "42." is a
// valid number in SVG 1.1, but invalid in SVG 2. We continue to accept them
// regardless. See:
//
//   https://svgwg.org/svg2-draft/paths.html#PathDataBNF
//
//   The grammar of previous specifications allowed a trailing decimal point
//   without any decimal digits for numbers (e.g 23.). SVG 2 harmonizes number
//   parsing with CSS [css-syntax-3], disallowing the relaxed grammar for
//   numbers. However, user agents may continue to accept numbers with trailing
//   decimal points when parsing is unambiguous. Authors and authoring tools
//   must not use the disallowed number format.
//
template<typename CharIterator>
bool readNumber(
    bool isSignAllowed,
    CharIterator& it,
    CharIterator end,
    double* number = nullptr) {

    const CharIterator numStart = it;

    // Current index
    CharIterator i = numStart;

    // Read sign
    if (isSignAllowed && i != end && isSign(*i)) {
        ++i;
    }

    // Read integer part
    bool hasDigits = false;
    while (i != end && isDigit(*i)) {
        hasDigits = true;
        ++i;
    }

    // Read decimal point
    if (i != end && *i == '.') {
        ++i;
    }

    // Read fractional part
    while (i != end && isDigit(*i)) {
        hasDigits = true;
        ++i;
    }
    if (!hasDigits) {
        return false;
    }

    // Read exponent part
    const CharIterator expStart = i;
    if (i != end && (*i == 'e' || *i == 'E')) {
        ++i;
        // Read sign
        if (i != end && isSign(*i)) {
            ++i;
        }
        // Read digits
        bool hasExponentDigits = false;
        while (i != end && isDigit(*i)) {
            hasExponentDigits = true;
            ++i;
        }
        if (!hasExponentDigits) {
            // Does not match the grammar for 'exp'.
            // Rollback to before attempting to read the optional exponent part.
            i = expStart;
        }
    }

    // Advance iterator, convert to double, and return whether found.
    //
    // Note: aside from unlikely out-of-memory errors, the conversion can't
    // fail since the SVG number grammar is a subset of our parse<double> grammar.
    //
    const CharIterator numEnd = i;
    if (number) {
        // Note: In C++20, we can do string_view(It start, It end).
        const auto* charPtr = &(*numStart);
        size_t count = numEnd - numStart;
        std::string_view sv(charPtr, count);
        core::StringReader in(sv);
        in >> *number;
    }
    it = numEnd;
    return true;
}

// Calls readNumber() with isSignedAllowed = true.
//
template<typename CharIterator>
bool readNumber(CharIterator& it, CharIterator end, double* number = nullptr) {

    return readNumber(true, it, end, number);
}

// Returns whether the given QString starts with a number.
//
// If a number is found, and the optional output parameter `number` is given,
// then it is set to the value of the number. Otherwise, it is left unchanged.
//
bool readNumber(std::string_view s, double* number = nullptr) {
    auto it = s.cbegin();
    auto end = s.cend();
    return readNumber(it, end, number);
}

// Calls readNumber() with isSignedAllowed = false.
//
template<typename CharIterator>
bool readUnsigned(CharIterator& it, CharIterator end, double* number = nullptr) {
    return readNumber(false, it, end, number);
}

// Applies the given transform to the given width.
//
// Note that as per spec, the transform also affects stroke-width. In case of
// non-uniform scaling (or skewing), we can't really be fully compliant (see
// https://stackoverflow.com/q/10357292 for what compliance looks like in case
// of non-uniform scaling), so we just scale the stroke width by sqrt(|det(t)|),
// which is basically the geometric mean of the x-scale and y-scale. We could
// do a bit better by taking the stroke tangent into account, but this would
// complicate the architecture a bit for something which is probably a rarely
// used edge case, and would still not be 100% compliant anyway.
//
// Also note that SVG Tiny 1.2 and SVG 2 define a "non-scaling-size" vector
// effect, which makes stroke-width ignoring the current transform. We don't
// implement that, but the implementation notes on SVG 2 are where we used the
// inspiration for choosing sqrt(|det(t)|) as our scale factor:
//
// https://www.w3.org/TR/2018/CR-SVG2-20181004/coords.html#VectorEffects
//
double applyTransform(const geometry::Mat3d& t, double width) {
    // Note: Ideally, we may want to cache meanScale for performance
    double meanScale = std::sqrt(std::abs(t(0, 0) * t(1, 1) - t(1, 0) * t(0, 1)));
    return meanScale * width;
}

// Applies the given transform to the given Vector2d.
//
geometry::Vec2d applyTransform(const geometry::Mat3d& t, const geometry::Vec2d& v) {
    return t.transformPoint(v);
}

// All possible path command types.
//
enum class SvgPathCommandType : unsigned char {
    ClosePath = 0, // Z (none)
    MoveTo = 1,    // M (x y)+
    LineTo = 2,    // L (x y)+
    HLineTo = 3,   // H x+
    VLineTo = 4,   // V y+
    CCurveTo = 5,  // C (x1 y1 x2 y2 x y)+
    SCurveTo = 6,  // S (x2 y2 x y)+
    QCurveTo = 7,  // Q (x1 y1 x y)+
    TCurveTo = 8,  // T (x y)+
    ArcTo = 9      // A (rx ry x-axis-rotation large-arc-flag sweep-flag x y)+
};

// All possible argument types of path commands.
//
enum class SvgPathArgumentType : Int8 {
    Number,
    Unsigned,
    Flag
};

// Returns the signature of the given path command type, that is, the
// description of the number and types of its arguments.
//
const std::vector<SvgPathArgumentType>& signature(SvgPathCommandType commandType) {
    using A = SvgPathArgumentType;
    static const std::vector<SvgPathArgumentType> s[10] = {
        {},                     // ClosePath
        {A::Number, A::Number}, // MoveTo
        {A::Number, A::Number}, // LineTo
        {A::Number},            // HLineTo
        {A::Number},            // VLineTo

        {A::Number, A::Number, A::Number, A::Number, A::Number, A::Number}, // CCurveTo
        {A::Number, A::Number, A::Number, A::Number},                       // SCurveTo
        {A::Number, A::Number, A::Number, A::Number},                       // QCurveTo
        {A::Number, A::Number},                                             // TCurveTo

        // Arc
        {A::Unsigned, A::Unsigned, A::Number, A::Flag, A::Flag, A::Number, A::Number}};

    return s[static_cast<unsigned char>(commandType)];
}

// Represents one path command, that is, a command character followed by all
// its arguments, possibly implicitely repeated. For example, the string
//
//   L 10 10 10 20
//
// can be represented as one SvgPathCommand, but is represented as two
// SvgPathCommands when normalized:
//
//   L 10 10 L 10 20
//
struct SvgPathCommand {
    SvgPathCommandType type;
    bool relative;
    std::vector<double> args;

    SvgPathCommand(SvgPathCommandType type, bool relative, std::vector<double>&& args)
        : type(type)
        , relative(relative)
        , args(args) {
    }
};

// Returns whether the string [it, end) starts with a flag, that is, the
// character '0' or '1'.
//
// If a flag is found, then the iterator `it` is advanced to the position
// just after the flag. Otherwise, it is left unchanged.
//
// If a flag is found, and the optional output parameter `number` is given,
// then it is set to the value of the flag expressed as a double (0.0 or 1.0).
//
// Note: This function does NOT ignore leading whitespaces, that is,
// isFlag(" 0") returns false.
//
template<typename CharIterator>
bool readFlag(CharIterator& it, CharIterator end, double* number = nullptr) {
    if (it != end && (*it == '0' || *it == '1')) {
        if (number) {
            *number = (*it == '0') ? 0.0 : 1.0;
        }
        ++it;
        return true;
    }
    else {
        return false;
    }
}

// Advances the given iterator `it` forward until a non-whitespace character or
// the `end` is found.
//
// Returns whether at least one character was read.
//
template<typename It>
bool readWhitespaces(It& it, It end) {
    auto it0 = it;
    while (it != end && isWhitespace(*it)) {
        ++it;
    }
    return it0 != it;
}

// Advances the given iterator `it` forward until an open parenthesis is found non-whitespace-non-comma
// character or the `end` is found. Only one comma is allowed, that is, if a
// second comma is encountered, it stops reading just before the second comma.
//
// Returns whether at least one character was read.
//
template<typename CharIterator>
bool readCommaWhitespaces(CharIterator& it, CharIterator end) {

    auto it0 = it;
    readWhitespaces(it, end);
    if (it != end && *it == ',') {
        ++it;
        readWhitespaces(it, end);
    }
    return it0 != it;
}

// Returns whether the string [it, end) starts with a function name, that is, a
// [a-zA-Z_] character, followed by any number of [a-zA-Z0-9_-] characters.
//
// If a function name is found, then the iterator `it` is advanced to the
// position just after the function name. Otherwise, it is left unchanged.
//
// If a function name is found, and the optional output parameter
// `functionName` is given, then it is set to the name of the function.
// Otherwise, it is left unchanged.
//
// Note: This function does NOT ignore leading whitespaces, that is,
// readFunctionName(" scale") returns false.
//
// Note: Unlike generic CSS functions, but like all transform functions, we do
// not accept functions starting with `--` or `-`, or including non-ASCII
// characters or espace sequences.
//
template<typename CharIterator>
bool readFunctionName(
    CharIterator& it,
    CharIterator end,
    std::string* functionName = nullptr) {

    auto it0 = it;

    // Read first [a-zA-Z_] characters
    if (it != end && (isAlpha(*it) || *it == '_')) {
        ++it;
    }
    else {
        it = it0;
        return false;
    }

    // Read subsequent [a-zA-Z0-9_] characters
    while (it != end && (isAlpha(*it) || isDigit(*it) || *it == '_' || *it == '-')) {
        ++it;
    }
    if (functionName) {
        functionName->assign(it0, it);
    }
    return true;
}

// Returns whether the string [it, end) starts with a function
// call, that is:
//
// function-name: [a-zA-Z_] [a-zA-Z0-9_-]*
// function-args: number (comma-wsp? number)*
// function-call: function-name wsp* '(' wsp* function-args? wsp* ')'
//
// If a function call is found, then the iterator `it` is advanced to the
// position just after the close parenthesis. Otherwise, it is left unchanged.
//
// If a function call is found, and the optional output parameter
// `functionName` is given, then it is set to the name of the function.
// Otherwise, it is set to an empty string.
//
// If a function call is found, and the optional output parameter `args` is
// given, then it is set to the list of arguments of the function. Otherwise,
// it is set to an empty list.
//
// Note: This function does NOT ignore leading whitespaces, that is,
// readFunctionCall(" scale(2)") returns false.
//
// Note: CSS doesn't allow for whitespaces between a function name and the open
// parenthesis, but the transform attribute of SVG does:
//
// SVG 1.1: https://www.w3.org/TR/SVG11/coords.html#TransformAttribute
// SVG 2:   https://drafts.csswg.org/css-transforms/#svg-syntax
// CSS 3:   https://drafts.csswg.org/css-syntax-3/#function-token-diagram
//
template<typename CharIterator>
bool readFunctionCall(
    CharIterator& it,
    CharIterator end,
    std::string* functionName = nullptr,
    std::vector<double>* args = nullptr) {

    auto it0 = it;

    // Read function name
    if (!readFunctionName(it, end, functionName)) {
        it = it0;
        if (functionName)
            functionName->clear();
        if (args)
            args->clear();
        return false;
    }

    // Read whitespaces and open parenthesis
    readWhitespaces(it, end);
    if (it != end && *it == '(') {
        ++it;
    }
    else {
        it = it0;
        if (functionName)
            functionName->clear();
        if (args)
            args->clear();
        return false;
    }

    // Read arguments
    if (args)
        args->clear();
    bool readArgs = true;
    bool isFirstArg = true;
    while (readArgs) {
        auto itBeforeArg = it;
        if (isFirstArg) {
            readWhitespaces(it, end);
        }
        else {
            readCommaWhitespaces(it, end);
        }
        double number;
        if (readNumber(it, end, &number)) {
            if (args)
                args->push_back(number);
        }
        else {
            it = itBeforeArg; // move before comma if any
            readArgs = false;
        }
        isFirstArg = false;
    }

    // Read whitespaces and close parenthesis
    readWhitespaces(it, end);
    if (it != end && *it == ')') {
        ++it;
        return true;
    }
    else {
        // => Error: invalid arg or missing close parenthesis
        it = it0;
        if (functionName)
            functionName->clear();
        if (args)
            args->clear();
        return false;
    }
}

// Parses the given string into a 4x4 transform matrix.
//
// Note that it is unclear from the SVG specification which exact syntax is
// allowed, as it has slightly changed from SVG 1.1 to SVG 2 (= CSS Transforms
// Module Level 1):
//
// https://www.w3.org/TR/SVG11/coords.html#TransformAttribute
// https://drafts.csswg.org/css-transforms/#svg-syntax
//
// - SVG 1.1 forces at least one comma-wsp between transform functions (ex1 =
//   "scale(2)scale(3)" is forbidden), but allows for multiple commas (ex2 =
//   "scale(2),,scale(3)" is allowed). On the other hand, in SVG 2, ex1 is
//   allowed, but ex2 is forbidden.
//
//   SVG 1.1:  transforms: transform | transform comma-wsp+ transforms
//   SVG 2:    transforms: transform | transform wsp* comma-wsp? transforms
//
// - In SVG 1.1, a comma-wsp is mandatory between arguments of a transform
//   function, while it is optional in SVG 2 (i.e., it allows "100-200" like in
//   path data).
//
//   SVG 1.1:  scale: "scale" wsp* "(" wsp* number (comma-wsp  number)? wsp* ")"
//   SVG 2:    scale: "scale" wsp* "(" wsp* number (comma-wsp? number)? wsp* ")"
//
// Therefore, we take a liberal approach and accept them all, using the SVG 2
// syntax for function arguments, and the following syntax for transforms:
//
//   transforms:     transform | transform comma-wsp* transforms
//   transform-list: wsp* transforms? wsp*
//
geometry::Mat3d parseTransform(std::string_view s) {
    geometry::Mat3d res = geometry::Mat3d::identity;
    auto it = s.cbegin();
    auto end = s.cend();
    bool readFunctions = true;
    bool isFirstFunction = true;
    while (readFunctions) {
        auto itBeforeFunction = it;
        if (isFirstFunction) {
            readWhitespaces(it, end);
        }
        else {
            while (readCommaWhitespaces(it, end)) {
                // keep reading comma-whitespaces
            }
        }
        std::string functionName;
        std::vector<double> args;
        if (readFunctionCall(it, end, &functionName, &args)) {
            if (functionName == "matrix") { // a b c d e f
                if (args.size() != 6) {
                    // Error: incorrect number of arguments
                    return geometry::Mat3d::identity;
                }
                // TODO: check that we don't mess up row-major vs column-major
                // clang-format off
                geometry::Mat3d m(
                    args[0], args[2], args[4],
                    args[1], args[3], args[5],
                    0, 0, 1);
                // clang-format on
                res *= m;
            }
            else if (functionName == "translate") { // tx [ty=0]
                if (args.size() != 1 && args.size() != 2) {
                    // Error: incorrect number of arguments
                    return geometry::Mat3d::identity;
                }
                if (args.size() == 1) {
                    args.push_back(0.0);
                }
                res.translate(args[0], args[1]);
            }
            else if (functionName == "scale") { // sx [sy=sx]
                if (args.size() != 1 && args.size() != 2) {
                    // Error: incorrect number of arguments
                    return geometry::Mat3d::identity;
                }
                if (args.size() == 1) {
                    args.push_back(args[0]);
                }
                res.scale(args[0], args[1]);
            }
            else if (functionName == "rotate") { // angle [cx=0 cy=0]
                if (args.size() != 1 && args.size() != 3) {
                    // Error: incorrect number of arguments
                    return geometry::Mat3d::identity;
                }
                if (args.size() == 1) {
                    args.push_back(0.0);
                    args.push_back(0.0);
                }
                res.translate(args[1], args[2]);
                res.rotate(args[0] / 180.0 * core::pi);
                res.translate(-args[1], args[2]);
            }
            else if (functionName == "skewX") { // angle
                if (args.size() != 1) {
                    // Error: incorrect number of arguments
                    return geometry::Mat3d::identity;
                }
                double t = std::tan(args[0] / 180.0 * core::pi);
                // TODO: check that we don't mess up row-major vs column-major
                // clang-format off
                geometry::Mat3d m(
                    1, t, 0,
                    0, 1, 0,
                    0, 0, 1);
                // clang-format on
                res *= m;
            }
            else if (functionName == "skewY") { // angle
                if (args.size() != 1) {
                    // Error: incorrect number of arguments
                    return geometry::Mat3d::identity;
                }
                double t = std::tan(args[0] / 180.0 * core::pi);
                // TODO: check that we don't mess up row-major vs column-major
                // clang-format off
                geometry::Mat3d m(
                    1, 0, 0,
                    t, 1, 0,
                    0, 0, 1);
                // clang-format on
                res *= m;
            }
            else {
                // Error: Unknown function
                return geometry::Mat3d::identity;
            }
        }
        else {
            it = itBeforeFunction; // move before commas if any
            readFunctions = false;
        }
        isFirstFunction = false;
    }
    readWhitespaces(it, end);
    if (it != end) {
        // Error: unexpected character
        return geometry::Mat3d::identity;
    }
    else {
        return res;
    }
}

// Parses the given path data string `d` into a sequence of SvgPathCommands,
// according to the SVG 1.1 grammar:
//
//   https://www.w3.org/TR/SVG11/paths.html#PathDataBNF
//
// In case of invalid syntax, an error string is written to the optional output
// parameter `error`, and the returned SvgPathCommands is the path data up to
// (but not including) the first command segment with an invalid syntax, as per
// the SVG recommendation:
//
//   https://www.w3.org/TR/SVG11/implnote.html#PathElementImplementationNotes
//   https://svgwg.org/svg2-draft/paths.html#PathDataErrorHandling
//
//   The general rule for error handling in path data is that the SVG user
//   agent shall render a ‘path’ element up to (but not including) the path
//   command containing the first error in the path data specification. This
//   will provide a visual clue to the user or developer about where the error
//   might be in the path data specification. This rule will greatly discourage
//   generation of invalid SVG path data.
//
//   If a path data command contains an incorrect set of parameters, then the
//   given path data command is rendered up to and including the last correctly
//   defined path segment, even if that path segment is a sub-component of a
//   compound path data command, such as a "lineto" with several pairs of
//   coordinates. For example, for the path data string 'M 10,10 L 20,20,30',
//   there is an odd number of parameters for the "L" command, which requires
//   an even number of parameters. The user agent is required to draw the line
//   from (10,10) to (20,20) and then perform error reporting since 'L 20 20'
//   is the last correctly defined segment of the path data specification.
//
//   Wherever possible, all SVG user agents shall report all errors to the
//   user.
//
std::vector<SvgPathCommand>
parsePathData(std::string_view d, std::string* error = nullptr) {
    using t = SvgPathCommandType;
    using a = SvgPathArgumentType;
    auto it = d.cbegin();
    auto end = d.cend();
    std::vector<SvgPathCommand> cmds;
    readWhitespaces(it, end);
    while (it != end) {

        // Read command type and relativeness
        SvgPathCommandType type;
        bool relative;
        switch (*it) {
            // clang-format off
        case 'Z': type = t::ClosePath; relative = false; break;
        case 'M': type = t::MoveTo;    relative = false; break;
        case 'L': type = t::LineTo;    relative = false; break;
        case 'H': type = t::HLineTo;   relative = false; break;
        case 'V': type = t::VLineTo;   relative = false; break;
        case 'C': type = t::CCurveTo;  relative = false; break;
        case 'S': type = t::SCurveTo;  relative = false; break;
        case 'Q': type = t::QCurveTo;  relative = false; break;
        case 'T': type = t::TCurveTo;  relative = false; break;
        case 'A': type = t::ArcTo;     relative = false; break;

        case 'z': type = t::ClosePath; relative = true; break;
        case 'm': type = t::MoveTo;    relative = true; break;
        case 'l': type = t::LineTo;    relative = true; break;
        case 'h': type = t::HLineTo;   relative = true; break;
        case 'v': type = t::VLineTo;   relative = true; break;
        case 'c': type = t::CCurveTo;  relative = true; break;
        case 's': type = t::SCurveTo;  relative = true; break;
        case 'q': type = t::QCurveTo;  relative = true; break;
        case 't': type = t::TCurveTo;  relative = true; break;
        case 'a': type = t::ArcTo;     relative = true; break;
            // clang-format on

        default:
            // Unknown command character, or failed to parse first arg
            // of non-first argtuple of previous command.
            if (error) {
                *error = "Failed to read command type or argument: ";
                *error += *it;
            }
            return cmds;
        }

        // Ensure first command is a MoveTo
        if (cmds.empty() && type != t::MoveTo) {
            if (error) {
                *error = "First command must be 'M' or 'm'. Found '";
                *error += *it;
                *error += "' instead.";
            }
            return cmds;
        }

        // Advance iterator on success
        ++it;

        // Read command arguments, unless the command take zero arguments.
        const std::vector<SvgPathArgumentType>& sig = signature(type);
        bool readArgtuples = (sig.size() > 0);
        bool isFirstArgtuple = true;
        bool hasError = false;
        std::vector<double> args;
        args.reserve(sig.size());
        while (readArgtuples) {
            auto itBeforeArgtuple = it;
            if (isFirstArgtuple) {
                readWhitespaces(it, end);
            }
            else {
                readCommaWhitespaces(it, end);
            }
            for (size_t i = 0; i < sig.size(); ++i) {
                if (i != 0) {
                    readCommaWhitespaces(it, end);
                }
                // Check whether next symbol is a valid argument
                bool isArg = false;
                double number = 0;
                switch (sig[i]) {
                case a::Number:
                    isArg = readNumber(it, end, &number);
                    break;
                case a::Unsigned:
                    isArg = readUnsigned(it, end, &number);
                    break;
                case a::Flag:
                    isArg = readFlag(it, end, &number);
                    break;
                }
                if (isArg) {
                    // If there's an argument, keep reading
                    args.push_back(number);
                }
                else {
                    // If there's no valid argument, but an argument was
                    // mandatory, then drop previous args in argtuple, and
                    // report error.
                    if (i != 0 || isFirstArgtuple) {
                        hasError = true;
                        if (error) {
                            *error = "Failed to read argument.";
                        }
                        while (i > 0) {
                            args.pop_back();
                            --i;
                        }
                    }
                    // Whether it's an error or not, since there's no valid
                    // argument, we stop reading args for this command, and
                    // move on to the next command. Note that we need to
                    // move back the iterator to where it was before attempting
                    // to read arguments, since a comma may have been read, which
                    // is allowed between argtuples, but not allowed between
                    // an argtuple and the next command.
                    it = itBeforeArgtuple;
                    readArgtuples = false;
                    break;
                }
            }
            isFirstArgtuple = false;
        }

        // Add command to path data. Note that even in case of errors, we still
        // add the command if at least one argtuple was successfully read.
        if (!hasError || args.size() > 0) {
            cmds.push_back(SvgPathCommand(type, relative, std::move(args)));
        }

        // Return now in case of errors in argument parsing
        if (hasError) {
            return cmds;
        }

        // Read whitespaces and move on to the next command
        readWhitespaces(it, end);
    }
    return cmds;
}

// Parses color from string, will probably be moved to a class like CSSColor
// This implements the most of the W3 specifications
// found at https://www.w3.org/TR/SVG11/types.html#DataTypeColor
// It also extends the specifications in a few minor ways
// This includes more flexible whitespace and some CSS3 features (hsl)
//
std::optional<core::Color> parseColor(std::string_view s) {

    using std::round;

    using core::clamp;
    using core::contains;
    using core::endsWith;
    using core::parse;
    using core::split;
    using core::startsWith;
    using core::trimmed;

    // Remove excess whitespace
    s = trimmed(s);

    if (startsWith(s, "rgba") && endsWith(s, ")") && contains(s, '(')) {

        // Remove rgba()
        s.remove_prefix(4);
        s.remove_suffix(1);
        s = trimmed(s);
        s.remove_prefix(1);

        // Split into elements with comma separating them
        core::Array<std::string_view> sSplit = split(s, ',');

        // Check that there are exactly four elements
        if (sSplit.size() != 4) {
            return std::nullopt;
        }

        // RGB channels in [0, 1]
        float colors[3];

        // Handle rgb channels: either percentages in [0%-100%] or values in [0-255]
        for (int i = 0; i < 3; i++) {
            std::string_view element = sSplit[i];

            // More trimming
            element = trimmed(element);

            // Determine if it is *% or * format
            float factor = 255;
            if (endsWith(element, "%")) {
                factor = 100;
                element.remove_suffix(1); // Remove % sign
            }
            float x = parse<float>(element);
            colors[i] = clamp(0.0f, x / factor, 1.0f);
        }

        // Alpha channel is a float from 0.0 to 1.0 inclusive
        float alpha = parse<float>(sSplit[3]);
        alpha = clamp(0.0f, alpha, 1.0f);

        // Return result
        return core::Color(colors[0], colors[1], colors[2], alpha);
    }
    else if (startsWith(s, "rgb") && endsWith(s, ")") && contains(s, '(')) {

        // Remove rgb()
        s.remove_prefix(3);
        s.remove_suffix(1);
        s = trimmed(s);
        s.remove_prefix(1);

        // Split into elements with comma separating them
        core::Array<std::string_view> sSplit = split(s, ',');

        // Check that there are exactly three elements
        if (sSplit.size() != 3) {
            return std::nullopt;
        }

        // RGB channels in [0, 1]
        float colors[3];

        // Handle rgb channels: either percentages in [0%-100%] or values in [0-255]
        for (int i = 0; i < 3; i++) {
            std::string_view element = sSplit[i];

            // More trimming
            element = trimmed(element);

            // Determine if it is *% or * format
            float factor = 255;
            if (endsWith(element, "%")) {
                factor = 100;
                element.remove_suffix(1); // Remove % sign
            }
            float x = parse<float>(element);
            colors[i] = clamp(0.0f, x / factor, 1.0f);
        }

        // Return result
        return core::Color(colors[0], colors[1], colors[2]);
    }
    else if (startsWith(s, "hsla") && endsWith(s, ")") && contains(s, "(")) {

        // Remove hsla()
        s.remove_prefix(4);
        s.remove_suffix(1);
        s = trimmed(s);
        s.remove_prefix(1);

        // Split into elements with comma separating them
        core::Array<std::string_view> sSplit = split(s, ',');

        // Check that there are exactly four elements
        if (sSplit.size() != 4) {
            return std::nullopt;
        }

        // Check that saturation and lightness are percentages,
        // and remove the percentage sign
        if (!endsWith(sSplit[1], "%") || !endsWith(sSplit[2], "%")) {
            return std::nullopt;
        }
        sSplit[1].remove_suffix(1);
        sSplit[2].remove_suffix(1);

        // Hue is an angle from 0-359 inclusive
        float hue = parse<float>(sSplit[0]);

        // Saturation and lightness are read as percentages and mapped to [0, 1]
        float saturation = parse<float>(sSplit[1]) / 100.0f;
        float lightness = parse<float>(sSplit[2]) / 100.0f;

        // Alpha channel is a double from 0.0 to 1.0 inclusive
        float alpha = parse<float>(sSplit[3]);

        // Return result
        return core::Color::hsla(hue, saturation, lightness, alpha);
    }
    else if (startsWith(s, "hsl") && endsWith(s, ")") && contains(s, "(")) {

        // Remove hsl()
        s.remove_prefix(3);
        s.remove_suffix(1);
        s = trimmed(s);
        s.remove_prefix(1);

        // Split into elements with comma separating them
        core::Array<std::string_view> sSplit = split(s, ',');

        // Check that there are exactly four elements
        if (sSplit.size() != 3) {
            return std::nullopt;
        }

        // Check that saturation and lightness are percentages,
        // and remove the percentage sign
        if (!endsWith(sSplit[1], "%") || !endsWith(sSplit[2], "%")) {
            return std::nullopt;
        }
        sSplit[1].remove_suffix(1);
        sSplit[2].remove_suffix(1);

        // Hue is an angle from 0-359 inclusive
        float hue = parse<float>(sSplit[0]);

        // Saturation and lightness are read as percentages and mapped to [0, 1]
        float saturation = parse<float>(sSplit[1]) / 100.0f;
        float lightness = parse<float>(sSplit[2]) / 100.0f;

        // Return result
        return core::Color::hsl(hue, saturation, lightness);
    }
    else {
        // TODO: handle named constants and #* formats
        return core::Color();
    }
}

// https://www.w3.org/TR/SVG11/painting.html#SpecifyingPaint
SvgPaint parsePaint(std::string_view s) {
    // Remove excess whitespace
    s = core::trimmed(s);
    if (s == "none") {
        return SvgPaint();
    }
    else {
        if (std::optional<core::Color> color = parseColor(s)) {
            return SvgPaint(*color);
        }
        else {
            return SvgPaint();
        }
    }
}

class SvgPresentationAttributes {
public:
    SvgPresentationAttributes() {
        update_();
    }

    void applyChildStyle(const core::XmlStreamReader& xml);

    // Note: fill-opacity, stroke-opacity, and opacity are
    // already factored in the alpha channel of the public
    // variables `fill` and `stroke` below. Also, strokeWidth
    // is set to zero if stroke.hasColor = false.
    SvgPaint fill = core::colors::black;
    SvgPaint stroke = {}; // none
    double strokeWidth = 1.0;

private:
    // Computed values after applying inheritance rules.
    //
    // Note that fill-opacity is separately inherited from fill, so we cannot
    // just store fill-opacity inside the alpha value of fill (same for stroke
    // and stroke-opacity).
    //
    SvgPaint fill_ = core::colors::black;
    SvgPaint stroke_ = {}; // none
    double fillOpacity_ = 1.0;
    double strokeOpacity_ = 1.0;
    double strokeWidth_ = 1.0;

    // Opacity. This is not inherited but composed as a post-processing step.
    // See comment in the implementation of applyChildStyle(), and:
    // https://www.w3.org/TR/SVG11/masking.html#OpacityProperty
    // https://www.w3.org/TR/SVG11/render.html#Grouping
    double opacity_ = 1.0;

    // Update public variables
    void update_();
};

using StringViewList = core::Array<std::string_view>;
using StringViewMap = std::unordered_map<std::string_view, std::string_view>;
using OptionalStringView = std::optional<std::string_view>;

// Basic CSS style-attribute parsing. This is not fully compliant (e.g.,
// presence of comments, or semicolon within quoted strings), but should work
// in most cases, notably files generated by Inkscape. Note that units other
// than px (em, cm, %, etc.) are not properly supported and interpreted as user
// units.
//
StringViewMap parseStyleAttribute(std::string_view style) {

    using core::split;
    using core::trimmed;

    StringViewMap res;
    StringViewList declarations = split(style, ';');
    for (std::string_view d : declarations) {
        StringViewList namevalue = split(d, ':');
        if (namevalue.size() >= 2) {
            res[trimmed(namevalue[0])] = trimmed(namevalue[1]);
        }
    }
    return res;
}

std::optional<double> getNumber(
    const core::XmlStreamReader& xml,
    const StringViewMap& style,
    std::string_view property) {

    double number = 0;
    auto it = style.find(property);
    if (it != style.end() && readNumber(it->second, &number)) {
        return number;
    }
    else if (OptionalStringView s = xml.attributeValue(property)) {
        if (readNumber(*s, &number)) {
            return number;
        }
    }
    return std::nullopt;
}

std::optional<SvgPaint> getPaint(
    const core::XmlStreamReader& xml,
    const StringViewMap& style,
    std::string_view property) {

    auto it = style.find(property);
    if (it != style.end()) {
        return parsePaint(it->second);
    }
    else if (OptionalStringView s = xml.attributeValue(property)) {
        return parsePaint(*s);
    }
    return std::nullopt;
}

void SvgPresentationAttributes::applyChildStyle(const core::XmlStreamReader& xml) {

    // Style attribute. Note: styling defined via the 'style' attribute
    // takes precedence over styling defined via presentation attributes.
    StringViewMap style;
    if (OptionalStringView s = xml.attributeValue("style")) {
        style = parseStyleAttribute(*s);
    }

    // Stroke width
    if (std::optional<double> x = getNumber(xml, style, "stroke-width")) {
        strokeWidth_ = (std::max)(0.0, *x);
    }

    // Fill (color)
    if (std::optional<SvgPaint> p = getPaint(xml, style, "fill")) {
        fill_ = *p;
    }

    // Stroke (color)
    if (std::optional<SvgPaint> p = getPaint(xml, style, "stroke")) {
        stroke_ = *p;
    }

    // Fill opacity
    if (std::optional<double> x = getNumber(xml, style, "fill-opacity")) {
        fillOpacity_ = core::clamp(*x, 0.0, 1.0);
    }

    // Stroke opacity
    if (std::optional<double> x = getNumber(xml, style, "stroke-opacity")) {
        strokeOpacity_ = core::clamp(*x, 0.0, 1.0);
    }

    // Group or Element Opacity
    //
    // Note that unlike other style attributes (including `fill-opacity` and
    // `stroke-opacity`), the `opacity` attribute is not "inherited" by
    // children. Instead, children of a group are supposed to be rendered in an
    // offscreen buffer, then the buffer should be composited with the
    // background based on its opacity.
    //
    // Example 1:
    //
    // <g opacity="0.5">                            // => opacity = 0.5  fill-opacity = 1.0
    //   <circle cx="0" cy="0" r="10" fill="red">   // => opacity = 1.0  fill-opacity = 1.0
    //   <circle cx="0" cy="0" r="10" fill="green"> // => opacity = 1.0  fill-opacity = 1.0
    // </g>
    //
    // A fully-opaque green circle is drawn over a fully opaque red circle, so
    // you get a fully opaque green circle in the offscreen buffer. After
    // applying the 50% opacity of the group, you get a semi-transparent green
    // circle: rgba(0, 255, 0, 0.5).
    //
    // Example 2:
    //
    // <g fill-opacity="0.5">                       // => opacity = 1.0  fill-opacity = 0.5
    //   <circle cx="0" cy="0" r="10" fill="red">   // => opacity = 1.0  fill-opacity = 0.5
    //   <circle cx="0" cy="0" r="10" fill="green"> // => opacity = 1.0  fill-opacity = 0.5
    // </g>
    //
    // A semi-transparent green circle:               rgba(0, 255, 0, 0.5)    = Er, Eg, Eb, Ea  - Element
    // is drawn over a semi-transparent red circle:   rgba(255, 0, 0, 0.5)    = Cr, Cg, Cb, Ca  - Canvas (before blending)
    // so you get the following circle color/opacity: rgba(127, 255, 0, 0.75) = Cr',Cg',Cb',Ea' - Canvas (after blending)
    // in the offscreen buffer after applying the alpha blending rules:
    //     https://www.w3.org/TR/SVG11/masking.html#SimpleAlphaBlending
    //     Cr' = (1 - Ea) * Cr + Er
    //     Cg' = (1 - Ea) * Cg + Eg
    //     Cb' = (1 - Ea) * Cb + Eb
    //     Ca' = 1 - (1 - Ea) * (1 - Ca)
    // After applying the 100% opacity of the group (groups ignore fill-opacity),
    // you get the following circle: rgba(127, 255, 0, 0.75)
    //
    // Unfortunately, the behavior of Example 1 is impossible to capture with
    // the current simple "flattening" API of getSvgSimplePaths(), since the
    // hierarchy of groups is lost.
    //
    // Therefore, we instead compose the group opacity directly into the
    // fill/stroke-opacity of children, which is not equivalent (it gives you
    // the same result as example 2), but is at least better than ignoring the
    // property altogether.
    //
    // Nice example to test behaviour:
    // https://www.w3.org/TR/SVG11/images/masking/opacity01.svg
    //
    if (std::optional<double> x = getNumber(xml, style, "opacity")) {
        opacity_ *= core::clamp(*x, 0.0, 1.0);
    }

    update_();
}

SvgPaint applyOpacity(SvgPaint paint, double localOpacity, double opacity) {
    if (paint.paintType() == SvgPaintType::Color) {
        core::Color c = paint.color();
        float totalOpacity = core::narrow_cast<float>(localOpacity * opacity);
        c.setA(c.a() * totalOpacity);
        paint.setColor(c);
    }
    return paint;
}

void SvgPresentationAttributes::update_() {
    // Compose the different opacity attributes together. In a compliant SVG
    // renderer, we would still have this step but without the last
    // multiplication with opacity_. The opacity_ would be applied differently,
    // using an offscreen buffer.
    fill = applyOpacity(fill_, fillOpacity_, opacity_);
    stroke = applyOpacity(stroke_, strokeOpacity_, opacity_);

    // Set strokeWidth to zero if stroke = none
    strokeWidth = stroke.paintType() == SvgPaintType::None ? 0.0 : strokeWidth_;
}

// Converts path data to Curves2d.

geometry::Curves2d
pathToCurves2d(const std::vector<SvgPathCommand>& commands, const geometry::Mat3d& ctm) {

    // The current position in the path
    geometry::Vec2d currentPosition = {};

    // Previous command and last Bezier tangent control point. This is used
    // for the "smooth" bezier curveto variants, that is, S and T.
    SvgPathCommandType previousCommandType = SvgPathCommandType::MoveTo;
    geometry::Vec2d lastTangentControlPoint = {};

    // Argument tuple of current command segment
    std::vector<double> args;
    args.reserve(7);

    // Iterate over all commands
    geometry::Curves2d res;
    for (const SvgPathCommand& command : commands) {
        SvgPathCommandType commandType = command.type;
        bool isRelative = command.relative;
        size_t nargs = command.args.size();
        size_t arity = signature(commandType).size();
        size_t nargtuples = (arity == 0) ? 1 : nargs / arity;
        for (size_t k = 0; k < nargtuples; ++k) {
            args.clear();
            for (size_t i = 0; i < arity; ++i) {
                args.push_back(command.args[k * arity + i]);
            }
            const SvgPathCommandType thisCommandType = commandType;
            switch (thisCommandType) {
            case SvgPathCommandType::ClosePath: {
                res.close();
                break;
            }
            case SvgPathCommandType::MoveTo: {
                if (isRelative) {
                    currentPosition += {args[0], args[1]};
                }
                else {
                    currentPosition = {args[0], args[1]};
                }
                res.moveTo(applyTransform(ctm, currentPosition));

                // If a MoveTo is followed by multiple pairs of coords, the
                // subsequent pairs are treated as implicit LineTo commands.
                commandType = SvgPathCommandType::LineTo;
                break;
            }
            case SvgPathCommandType::LineTo:
            case SvgPathCommandType::HLineTo:
            case SvgPathCommandType::VLineTo: {
                if (isRelative) {
                    if (commandType == SvgPathCommandType::HLineTo) {
                        currentPosition += {args[0], 0.0};
                    }
                    else if (commandType == SvgPathCommandType::VLineTo) {
                        currentPosition += {0.0, args[0]};
                    }
                    else { // commandType == SvgPathCommandType::LineTo
                        currentPosition += {args[0], args[1]};
                    }
                }
                else {
                    if (commandType == SvgPathCommandType::HLineTo) {
                        currentPosition.setX(args[0]);
                    }
                    else if (commandType == SvgPathCommandType::VLineTo) {
                        currentPosition.setY(args[0]);
                    }
                    else { // commandType == SvgPathCommandType::LineTo
                        currentPosition = {args[0], args[1]};
                    }
                }
                res.lineTo(applyTransform(ctm, currentPosition));
                break;
            }
            case SvgPathCommandType::CCurveTo:
            case SvgPathCommandType::SCurveTo: {
                geometry::Vec2d q;
                geometry::Vec2d r;
                geometry::Vec2d s;
                if (commandType == SvgPathCommandType::CCurveTo) {
                    q = {args[0], args[1]};
                    r = {args[2], args[3]};
                    s = {args[4], args[5]};
                    if (isRelative) {
                        q += currentPosition;
                        r += currentPosition;
                        s += currentPosition;
                    }
                }
                else {
                    if (previousCommandType == SvgPathCommandType::CCurveTo
                        || previousCommandType == SvgPathCommandType::SCurveTo) {

                        // Symmetry of previous tangent
                        q = 2 * currentPosition - lastTangentControlPoint;
                    }
                    else {
                        q = currentPosition;
                    }
                    r = {args[0], args[1]};
                    s = {args[2], args[3]};
                    if (isRelative) {
                        r += currentPosition;
                        s += currentPosition;
                    }
                }
                lastTangentControlPoint = r;
                currentPosition = s;
                res.cubicBezierTo(
                    applyTransform(ctm, q),
                    applyTransform(ctm, r),
                    applyTransform(ctm, s));
                break;
            }
            case SvgPathCommandType::QCurveTo:
            case SvgPathCommandType::TCurveTo: {
                geometry::Vec2d q;
                geometry::Vec2d r;
                if (commandType == SvgPathCommandType::QCurveTo) {
                    q = {args[0], args[1]};
                    r = {args[2], args[3]};
                    if (isRelative) {
                        q += currentPosition;
                        r += currentPosition;
                    }
                }
                else {
                    if (previousCommandType == SvgPathCommandType::QCurveTo
                        || previousCommandType == SvgPathCommandType::TCurveTo) {

                        // Symmetry of previous tangent
                        q = 2 * currentPosition - lastTangentControlPoint;
                    }
                    else {
                        q = currentPosition;
                    }
                    r = {args[0], args[1]};
                    if (isRelative) {
                        r += currentPosition;
                    }
                }
                lastTangentControlPoint = q;
                currentPosition = r;
                res.quadraticBezierTo( //
                    applyTransform(ctm, q),
                    applyTransform(ctm, r));
                break;
            }
            case SvgPathCommandType::ArcTo: {
                // TODO
                break;
            }
            }
            previousCommandType = thisCommandType;
        }
    }
    return res;
}

} // namespace

namespace detail {

class SvgParser {
public:
    static SvgSimplePath pathToSimplePath(
        const std::vector<SvgPathCommand>& commands,
        const core::XmlStreamReader& xml,
        const SvgPresentationAttributes& pa,
        const geometry::Mat3d& ctm) {

        SvgSimplePath res;
        res.curves_ = pathToCurves2d(commands, ctm);
        res.fill_ = pa.fill;
        res.stroke_ = pa.stroke;
        res.strokeWidth_ = applyTransform(ctm, pa.strokeWidth);

        // The grammar for the value of the 'class' attribute is defined in the
        // HTML spec as a "space-separated tokens":
        //
        // https://www.w3.org/TR/SVG2/styling.html#ElementSpecificStyling
        // https://html.spec.whatwg.org/multipage/common-microsyntaxes.html#set-of-space-separated-tokens
        //
        // Like in CSS, the form feed character `\f' is here considered a
        // whitespace character, while it isn't in XML / SVG syntax.
        //
        if (OptionalStringView s = xml.attributeValue("class")) {
            std::string_view htmlWsp = " \t\r\n\f";
            res.styleClass_ = *s;
            for (std::string_view sv :
                 core::splitAnySkipEmpty(res.styleClass_, htmlWsp)) {
                res.styleClasses_.append(std::string(sv));
            }
            // Note: if we want to store res.styleClasses_ as an array of
            // string_view instead of an array of strings, we need to
            // re-implement the copy constructor of SvgSimplePath to remap the
            // string_views of the copy into the new styleClass_ string. For
            // reference, we do something similar in
            // UnparsedValue::remapPointers_() in vgc/style/value.cpp.
        }
        return res;
    }
};

} // namespace detail

namespace {

void readPath(
    core::Array<SvgSimplePath>& out,
    const core::XmlStreamReader& xml,
    SvgPresentationAttributes& pa,
    const geometry::Mat3d& ctm) {

    if (OptionalStringView d = xml.attributeValue("d")) {
        std::string error;
        std::vector<SvgPathCommand> cmds = parsePathData(*d, &error);
        if (!error.empty()) {
            VGC_ERROR(LogVgcGraphicsSvg, error);
        }

        // Import path data (up to, but not including, first invalid command)
        out.append(detail::SvgParser::pathToSimplePath(cmds, xml, pa, ctm));
    }
    else {
        // Don't output anything if no path data provided
    }
}

// TODO: readRect, readLine, etc.

} // namespace

// Note about error handling.
// -------------------------
//
// In case of errors in path data or basic shapes attributes, such as if
// rect.height < 0, the SVG specification mandates to stop processing the
// document, that is, not render any other XML element that might exist after
// the error. See:
//
//   https://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing
//
//   The document shall (ed: "MUST") be rendered up to, but not including, the
//   first element which has an error. Exceptions:
//
//   - If a ‘path’ element is the first element which has an error
//     and the only errors are in the path data specification, then
//     render the ‘path’ up to the point of the path data error.
//     For related information, see ‘path’ element implementation
//     notes.
//
//   - If a ‘polyline’ or ‘polygon’ element is the first element
//     which has an error and the only errors are within the
//     ‘points’ attribute, then render the ‘polyline’ or ‘polygon’
//     up to the segment with the error.
//
//   This approach will provide a visual clue to the user or
//   developer about where the error might be in the document.
//
// However, we purposefully violate this mandated behavior, that is, we keep
// reading subsequent XML elements. Indeed, we're not a "renderer" but an
// "importer", in which case the added value of providing a visual clue matters
// less than the ability to import whatever geometry exists in the document.
// Also, this makes the importer more robust to bugs in its implementation.
//
// Besides, this is the error handling policy that we want to use for VGC. In a
// VGC document, if an XML element is erroneous, then it should still be
// imported and marked as "in error" and not rendered (or partially rendered,
// as for SVG paths), but processing should continue for other XML elements, as
// long as it isn't an XML syntax error. This makes it much more robust to
// small bugs in user scripts or implementation which invariably happen,
// especially when approaching a deadline. When producing a movie, things are
// messy, and a broken image is much more useful than no image at all.
// Especially for geometric data, where some interpolation that overshoots
// (e.g., Catmull-Rom) might easily make height < 0 temporarily, in which case
// it is really silly not to render subsequent valid elements.
//
core::Array<SvgSimplePath> getSvgSimplePaths(std::string_view svg) {

    core::Array<SvgSimplePath> res;

    core::XmlStreamReader xml(svg);

    // Ensure that this is a SVG file
    xml.readNextStartElement();
    if (xml.name() != "svg") {
        throw core::ParseError("The root element of the given `svg` data is not <svg>");
    }

    // Initialize attribute stack
    core::Array<SvgPresentationAttributes> attributeStack;
    SvgPresentationAttributes defaultStyle;
    attributeStack.append(defaultStyle);

    // Initialize transform stack
    core::Array<geometry::Mat3d> transformStack;
    transformStack.append(geometry::Mat3d::identity);

    // Iterate over all XML tokens, including the <svg> start element
    // which may have style attributes or transforms
    while (xml.readNext()) {
        // Process start elements
        if (xml.eventType() == core::XmlEventType::StartElement) {

            // Apply child style to current style
            SvgPresentationAttributes pa = attributeStack.last();
            pa.applyChildStyle(xml);
            attributeStack.append(pa);

            // Apply child transform to CTM (= Current Transform Matrix)
            geometry::Mat3d ctm = transformStack.last();
            if (auto attr = xml.attribute("transform")) {
                std::string_view ts = attr->value();
                ctm = ctm * parseTransform(ts);
            }
            transformStack.append(ctm);

            // STRUCTURAL ELEMENTS: svg, g, defs, symbol, use
            //
            // https://www.w3.org/TR/SVG11/struct.html
            //
            if (xml.name() == "svg") {
                // https://www.w3.org/TR/SVG11/struct.html#NewDocument
                //
                // TODO: implement x, y, width, height, viewBox and preserveAspectRatio.
                // Note that SVG elements can be nested inside other SVG elements.
                //
                // Allowed children:
                //  structural elements
                //  struct-ish elements
                //  descriptive elements
                //  shape elements
                //  text-font elements
                //  styling elements
                //  interactivity elements
                //  animation elements
            }
            else if (xml.name() == "g") {
                // https://www.w3.org/TR/SVG11/struct.html#Groups
                // We support this. We just have to keep reading its children.
                // Allowed children: same as <svg>
            }
            else if (xml.name() == "defs") {
                // https://www.w3.org/TR/SVG11/struct.html#Head
                // This is an unrendered group where to define referenced
                // content such as symbols, markers, gradients, etc. Note that
                // many referenced content can in fact be defined anywhere in
                // the document, but defining them in defs is best practice.
                // We don't support <defs> yet, but we may want to support it later.
                // Allowed children: same as <svg>
                xml.skipElement();
            }
            else if (xml.name() == "symbol") {
                // https://www.w3.org/TR/SVG11/struct.html#SymbolElement
                // This is an unrendered group to be instanciated with <use>.
                // We don't support <symbol> yet, but we may want to support it later.
                // Allowed children: same as <svg>
                xml.skipElement();
            }
            else if (xml.name() == "use") {
                // https://www.w3.org/TR/SVG11/struct.html#UseElement
                // This is for instanciating a <symbol>.
                // We don't support <use> yet, but we may want to support it later.
                // Allowed children:
                //  descriptive elements
                //  animation elements
                xml.skipElement();
            }

            // STRUCT-ISH ELEMENTS: switch, image, foreignObject
            //
            // https://www.w3.org/TR/SVG11/struct.html
            // https://www.w3.org/TR/SVG11/backward.html
            // https://www.w3.org/TR/SVG11/extend.html
            //
            else if (xml.name() == "switch") {
                // https://www.w3.org/TR/SVG11/struct.html#ConditionalProcessing
                // https://www.w3.org/TR/SVG11/struct.html#SwitchElement
                // https://www.w3.org/TR/SVG11/backward.html
                // This is for selecting which child to process based on feature availability.
                // We don't support <switch> yet, but we may want to support it later.
                // Allowed children:
                //  subset of structural elements: svg, g, use
                //  struct-ish elements
                //  descriptive elements
                //  shape elements
                //  subset of text-font elements: text
                //  subset of interactivity elements: a
                //  animation elements
                xml.skipElement();
            }
            else if (xml.name() == "image") {
                // https://www.w3.org/TR/SVG11/struct.html#ImageElement
                // This is for rendering an external image (e.g.: jpg, png, svg).
                // We don't support <image> yet, but may want to support it later.
                // Allowed children:
                //  descriptive elements
                //  animation elements
                xml.skipElement();
            }
            else if (xml.name() == "foreignObject") {
                // https://www.w3.org/TR/SVG11/extend.html#ForeignObjectElement
                // This is for inline embedding of other XML documents which aren't
                // SVG documents, such as MathML (for mathematical expressions), or
                // XHML (useful for dynamically reflowing text).
                // We don't support <foreignObject> yet, but may want to support it later,
                // notably for XML formats which we support importing by themselves (e.g.,
                // if we add support for importing standalone XHTML documents, we may want
                // to support importing XHTML as foreignObject in SVG documents).
                // Allowed children: Any elements or character data.
                xml.skipElement();
            }

            // DESCRIPTIVE ELEMENTS: desc, title, metadata
            //
            // https://www.w3.org/TR/SVG11/struct.html#DescriptionAndTitleElements
            // https://www.w3.org/TR/SVG11/metadata.html
            //
            // Allowed children: any elements or character data.
            //
            // We ignore them and all their children as they don't affect
            // geometry or rendering in any ways, and can't be meaningfully
            // imported into VPaint.
            //
            else if (
                xml.name() == "desc" || xml.name() == "title"
                || xml.name() == "metadata") {
                xml.skipElement();
            }

            // SHAPE ELEMENTS: path, rect, circle, ellipse, line, polyline, polygon
            //
            // https://www.w3.org/TR/SVG11/paths.html
            // https://www.w3.org/TR/SVG11/shapes.html
            //
            // Allowed children:
            //  descriptive elements
            //  animation elements
            //
            else if (xml.name() == "path") {
                readPath(res, xml, pa, ctm);
            }
            else if (xml.name() == "rect") {
                // TODO
            }
            else if (xml.name() == "circle") {
                // TODO
            }
            else if (xml.name() == "ellipse") {
                // TODO
            }
            else if (xml.name() == "line") {
                // TODO
            }
            else if (xml.name() == "polyline") {
                // TODO
            }
            else if (xml.name() == "polygon") {
                // TODO
            }

            // TEXT-FONT ELEMENTS: text, font, font-face, altGlyphDef
            //
            // TEXT CHILD ELEMENTS:        tspan, tref, textPath, altGlyph
            // FONT CHILD ELEMENTS:        glyph, missing-glyph, hkern, vkern, font-face
            // FONT-FACE CHILD ELEMENTS:   font-face-src, font-face-uri, font-face-format, font-face-name
            // ALTGLYPHDEF CHILD ELEMENTS: glyphRef, altGlyphItem
            //
            // https://www.w3.org/TR/SVG11/text.html
            // https://www.w3.org/TR/SVG11/fonts.html
            //
            // Note: the "child elements" types listed above only include the
            // types not already listed in other categories, and they might
            // only be allowed as direct or indirect children. See the above
            // links for details on the content model.
            //
            // We don't support text-font elements for now, but we may want to
            // support them in the future.
            //
            else if (
                xml.name() == "text"         //
                || xml.name() == "font"      //
                || xml.name() == "font-face" //
                || xml.name() == "altGlyphDef") {

                xml.skipElement();
            }

            // STYLING ELEMENTS: style, marker, color-profile, linearGradient, radialGradient, pattern, clipPath, mask, filter
            //
            // GRADIENT CHILD ELEMENTS:   stop
            // LIGHT SOURCE ELEMENTS:     feDistantLight, fePointLight, feSpotLight
            // FILTER PRIMITIVE ELEMENTS: feBlend, feColorMatrix, feComponentTransfer, feComposite, feConvolveMatrix,
            //                            feDiffuseLighting, feDisplacementMap, feFlood, feGaussianBlur, feImage, feMerge,
            //                            feMorphology, feOffset, feSpecularLighting, feTile, feTurbulence
            //
            // https://www.w3.org/TR/SVG11/styling.html   style
            // https://www.w3.org/TR/SVG11/painting.html  marker
            // https://www.w3.org/TR/SVG11/color.html     color-profile
            // https://www.w3.org/TR/SVG11/pservers.html  linearGradient, radialGradient, pattern
            // https://www.w3.org/TR/SVG11/masking.html   clipPath, mask
            // https://www.w3.org/TR/SVG11/filters.html   filter
            //
            // Note: the "child elements" types listed above only include the
            // types not already listed in other categories, and they might
            // only be allowed as direct or indirect children. See the above
            // links for details on the content model.
            //
            // We don't support styling elements for now, but we may want to
            // support them in the future.
            //
            else if (
                xml.name() == "style"             //
                || xml.name() == "marker"         //
                || xml.name() == "color-profile"  //
                || xml.name() == "linearGradient" //
                || xml.name() == "radialGradient" //
                || xml.name() == "pattern"        //
                || xml.name() == "clipPath"       //
                || xml.name() == "mask"           //
                || xml.name() == "filter") {

                xml.skipElement();
            }

            // INTERACTIVITY ELEMENTS: cursor, a, view, script
            //
            // https://www.w3.org/TR/SVG11/interact.html
            // https://www.w3.org/TR/SVG11/linking.html
            // https://www.w3.org/TR/SVG11/script.html
            //
            // We ignore all of these as they make no sense in VPaint.
            // We are not planning to ever support them in the future.
            //
            else if (xml.name() == "cursor") {
                // https://www.w3.org/TR/SVG11/interact.html#CursorElement
                // This is for defining a PNG image of a cursor, e.g. to define
                // what the mouse cursor looks like when hovering some elements.
                // This is irrelevant for VPaint, so we ignore it and all its children.
                // Allowed children:
                //  descriptive elements
                xml.skipElement();
            }
            else if (xml.name() == "a") {
                // https://www.w3.org/TR/SVG11/linking.html#Links
                // This is to be redirected to another URI when clicking on
                // any graphical element containted under the <a>. We ignore
                // the clicking behavior, but we still process its children as if
                // it was a normal group <g>.
                // Allowed children: same as <svg>
            }
            else if (xml.name() == "view") {
                // https://www.w3.org/TR/SVG11/linking.html#LinksIntoSVG
                // https://www.w3.org/TR/SVG11/linking.html#ViewElement
                // This is to predefine a specific viewBox or viewTarget within
                // this SVG document, that other documents can link to, for
                // example via "MyDrawing.svg#MyView", similar to the usage
                // of id-based hashtags in HTML URLs.
                // This is irrelevant for VPaint, so we ignore it and all its children.
                // Allowed children:
                //  descriptive elements
                xml.skipElement();
            }
            else if (xml.name() == "script") {
                // https://www.w3.org/TR/SVG11/script.html#ScriptElement
                // This is for running scripts, or defining script functions to
                // be run when interacting with SVG content (clicking, hovering, etc.)
                // This is irrelevant for VPaint, so we ignore it and all its children.
                // Allowed children: Any elements or character data.
                xml.skipElement();
            }

            // ANIMATION ELEMENTS: animate, set, animateMotion, animateColor, animateTransform
            //
            // https://www.w3.org/TR/SVG11/animate.html
            //
            // Allowed children:
            //  descriptive elements
            //  mpath (only for animationMotion, and at most one)
            //
            // We don't support animation elements for now. VPaint being an
            // animation tool, we obviously may want to support them in the
            // future.
            //
            else if (
                xml.name() == "animate"          //
                || xml.name() == "set"           //
                || xml.name() == "animateMotion" //
                || xml.name() == "animateColor"  //
                || xml.name() == "animateTransform") {

                xml.skipElement();
            }

            // Unknown elements. These aren't part of SVG 1.1, such as
            // Inkscape's "sodipodi:namedview".
            else {
                xml.skipElement();
            }
        }

        // Process end elements.
        //
        // Note that we don't use "else if" since the current TokenType changes
        // from StartElement to EndElement when calling skipCurrentElement().
        //
        if (xml.eventType() == core::XmlEventType::EndElement) {
            attributeStack.pop();
            transformStack.pop();
        }
    }

    return res;
}

// https://www.w3.org/TR/SVG/coords.html#ViewBoxAttribute
//
// > The value of the ‘viewBox’ attribute is a list of four numbers <min-x>,
// > <min-y>, <width> and <height>, separated by whitespace and/or a comma.
//
// The exact grammar for viewBox is not formally specified, but we take it to be:
//
// ViewBox ::= number comma-wsp number comma-wsp number comma-wsp number
//
geometry::Rect2d getSvgViewBox(std::string_view svg) {

    core::XmlStreamReader xml(svg);

    // Ensure that this is a SVG file
    xml.readNextStartElement();
    if (xml.name() != "svg") {
        throw core::ParseError("The root element of the given `svg` data is not <svg>");
    }

    geometry::Vec2d position;
    geometry::Vec2d size;

    if (std::optional<std::string_view> s = xml.attributeValue("viewBox")) {
        bool isSignAllowed = true;
        auto it = s->cbegin();
        auto end = s->cend();
        readNumber(isSignAllowed, it, end, &position[0]);
        readCommaWhitespaces(it, end);
        readNumber(isSignAllowed, it, end, &position[1]);
        readCommaWhitespaces(it, end);
        readNumber(isSignAllowed, it, end, &size[0]);
        readCommaWhitespaces(it, end);
        readNumber(isSignAllowed, it, end, &size[1]);
    }
    return geometry::Rect2d::fromPositionSize(position, size);
}

} // namespace vgc::graphics
