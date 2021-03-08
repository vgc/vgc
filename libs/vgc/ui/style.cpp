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

#include <vgc/core/colors.h>
#include <vgc/core/io.h>
#include <vgc/core/paths.h>
#include <vgc/ui/strings.h>
#include <vgc/ui/widget.h>

namespace vgc {
namespace ui {

StyleValue parseStyleDefault(StyleTokenIterator begin, StyleTokenIterator end)
{
    if (begin == end) {
        return StyleValue(std::string(""));
    }
    else {
        return StyleValue(std::string(begin->begin, (end-1)->end));
    }
}

StyleValue parseStyleColor(StyleTokenIterator begin, StyleTokenIterator end)
{
    StyleValue v = parseStyleDefault(begin, end);
    try {
        core::Color color = core::parse<core::Color>(v.string());
        return StyleValue(color);
    } catch (const core::ParseError&) {
        return StyleValue::invalid();
    } catch (const core::RangeError&) {
        return StyleValue::invalid();
    }
}

StyleValue parseStyleLength(StyleTokenIterator begin, StyleTokenIterator end)
{
    // For now, we only support a unique Dimension token with a "dp" unit
    bool isValid =
            (begin != end) &&
            (begin->type == StyleTokenType::Dimension) &&
            (begin->codePointsValue == "dp") &&
            (begin + 1 == end);

    if (isValid) {
        float v = (begin->flag == StyleTokenFlag::Integer) ?
                  static_cast<float>(begin->numericValue.integer) :
                  static_cast<float>(begin->numericValue.number);
        return StyleValue(v);
    }
    else {
        return StyleValue::invalid();
    }
}

StylePropertySpec* StylePropertySpec::get(core::StringId property)
{
    if (map_.empty()) {
        init();
    }
    auto search = map_.find(property);
    if (search == map_.end()) {
        return nullptr;
    }
    else {
        return &(search->second);
    }
}

StylePropertySpec::Table StylePropertySpec::map_;

void StylePropertySpec::init()
{
    // For reference: https://www.w3.org/TR/CSS21/propidx.html
    auto black = StyleValue(core::colors::black);
    auto transparent = StyleValue(core::colors::transparent);
    auto zero = StyleValue(0.0f);
    auto m = internal::StylePropertySpecMaker::make;
    map_ = {
        //      name                    initial   inherited     parser
        //
        m("background-color",          transparent, false, &parseStyleColor),
        m("background-color-on-hover", transparent, false, &parseStyleColor),
        m("border-radius",             zero,        false, &parseStyleLength),
        m("margin-bottom",             zero,        false, &parseStyleLength),
        m("margin-left",               zero,        false, &parseStyleLength),
        m("margin-right",              zero,        false, &parseStyleLength),
        m("margin-top",                zero,        false, &parseStyleLength),
        m("padding-bottom",            zero,        false, &parseStyleLength),
        m("padding-left",              zero,        false, &parseStyleLength),
        m("padding-right",             zero,        false, &parseStyleLength),
        m("padding-top",               zero,        false, &parseStyleLength),
        m("text-color",                black,       true,  &parseStyleColor)
    };
}

Style::Style()
{

}

StyleValue Style::cascadedValue(core::StringId property) const
{
    auto search = map_.find(property);
    if (search != map_.end()) {
        return search->second;
    }
    else {
        return StyleValue::none();
    }
}

StyleValue Style::computedValue(core::StringId property, const Widget* widget) const
{
    StylePropertySpec* spec = StylePropertySpec::get(property);
    return computedValue_(property, widget, spec);
}

// This function is a performance optimization: by passing in the spec, it
// avoids repeateadly searching for it
StyleValue Style::computedValue_(core::StringId property, const Widget* widget, StylePropertySpec* spec) const
{
    StyleValue v = cascadedValue(property);
    if (v.type() == StyleValueType::None || v.type() == StyleValueType::Inherit) {
        if (spec && v.type() == StyleValueType::None) {
            if (spec->isInherited()) {
                v = StyleValue::inherit();
            }
            else {
                v = spec->initialValue();
            }
        }
        if (v.type() == StyleValueType::Inherit) {
            Widget* parent = widget->parent();
            if (parent) {
                v = parent->style_.computedValue_(property, parent, spec);
            }
            else if (spec) {
                v = spec->initialValue();
            }
            else {
                v = StyleValue::none();
            }
        }
    }
    return v;
}

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
        std::string decoded = decodeStyleString(styleString);
        StyleTokenArray tokens = tokenizeStyleString(decoded.data());
        bool topLevel = true;
        StyleTokenIterator it = tokens.begin();
        core::Array<StyleRuleSetPtr> rules = consumeRuleList_(it, tokens.end(), topLevel);
        for (const StyleRuleSetPtr& rule : rules) {
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
    static core::Array<StyleRuleSetPtr> consumeRuleList_(StyleTokenIterator& it, StyleTokenIterator end, bool topLevel) {
        core::Array<StyleRuleSetPtr> res;
        while (true) {
            if (it == end) {
                break;
            }
            else if (it->type == StyleTokenType::Whitespace) {
                ++it;
            }
            else if (it->type == StyleTokenType::Cdo || it->type == StyleTokenType::Cdc) {
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
            else if (it->type == StyleTokenType::AtKeyword) {
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
    static void consumeAtRule_(StyleTokenIterator& it, StyleTokenIterator end) {
        // For now, we just consume the rule without returning anything.
        // In the future, we'll return a StyleAtRule
        ++it; // skip At token
        while (true) {
            if (it == end) {
                // Parse Error: return the partially consumed AtRule
                break;
            }
            else if (it->type == StyleTokenType::Semicolon) {
                ++it;
                break;
            }
            else if (it->type == StyleTokenType::LeftCurlyBracket) {
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
    static StyleRuleSetPtr consumeQualifiedRule_(StyleTokenIterator& it, StyleTokenIterator end) {
        StyleRuleSetPtr rule = StyleRuleSet::create();
        StyleTokenIterator preludeBegin = it;
        while (true) {
            if (it == end) {
                // Parse Error: return nothing
                return StyleRuleSetPtr();
            }
            else if (it->type == StyleTokenType::LeftCurlyBracket) {
                StyleTokenIterator preludeEnd = it;
                ++it;

                // Parse the prelude as a selector group
                core::Array<StyleSelectorPtr> selectors = consumeSelectorGroup_(preludeBegin, preludeEnd);
                if (selectors.isEmpty()) {
                    // Parse error
                    return StyleRuleSetPtr();
                }
                else {
                    for (const StyleSelectorPtr& selector : selectors) {
                        rule->appendChildObject_(selector.get());
                        rule->selectors_.append(selector.get());
                    }
                }

                // Consume list of declarations
                bool expectRightCurlyBracket = true;
                core::Array<StyleDeclarationPtr> declarations = consumeDeclarationList_(it, end, expectRightCurlyBracket);
                for (const StyleDeclarationPtr& declaration : declarations) {
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
    static core::Array<StyleDeclarationPtr> consumeDeclarationList_(StyleTokenIterator& it, StyleTokenIterator end, bool expectRightCurlyBracket) {
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
            else if (it->type == StyleTokenType::Whitespace) {
                ++it;
            }
            else if  (it->type == StyleTokenType::Semicolon) {
                ++it;
            }
            else if (it->type == StyleTokenType::AtKeyword) {
                consumeAtRule_(it, end);
                // Note: for now, the at rule is simply skipped and
                // not appended to the list of declarations.
            }
            else if (it->type == StyleTokenType::Ident) {
                StyleTokenIterator declarationBegin = it;
                while (true) {
                    if (it == end ||
                        it->type == StyleTokenType::Semicolon ||
                        (expectRightCurlyBracket && it->type == StyleTokenType::RightCurlyBracket)) {

                        break;
                    }
                    else {
                        consumeComponentValue_(it, end);
                    }
                }
                StyleTokenIterator declarationEnd = it;
                StyleDeclarationPtr declaration = consumeDeclaration_(declarationBegin, declarationEnd);
                if (declaration) {
                    res.append(declaration);
                }
            }
            else if (expectRightCurlyBracket && it->type == StyleTokenType::RightCurlyBracket) {
                ++it;
                break;
            }
            else {
                // Parse error: throw away component values until semicolon or eof
                while (true) {
                    if (it == end ||
                        it->type == StyleTokenType::Semicolon ||
                        (expectRightCurlyBracket && it->type == StyleTokenType::RightCurlyBracket)) {

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
    static StyleDeclarationPtr consumeDeclaration_(StyleTokenIterator& it, StyleTokenIterator end) {
        StyleDeclarationPtr declaration = StyleDeclaration::create();
        declaration->property_ = core::StringId(it->codePointsValue);
        declaration->value_ = StyleValue::invalid();
        ++it;
        // Consume whitespaces
        while (it != end && it->type == StyleTokenType::Whitespace) {
            ++it;
        }
        // Ensure first non-whitespace token is a Colon
        if (it == end || it->type != StyleTokenType::Colon) {
            // Parse error: return nothing
            return StyleDeclarationPtr();
        }
        else {
            ++it;
        }
        // Consume whitespaces
        while (it != end && it->type == StyleTokenType::Whitespace) {
            ++it;
        }
        // Consume value components
        StyleTokenIterator valueBegin = it;
        while (it != end) {
            consumeComponentValue_(it, end);
        }
        StyleTokenIterator valueEnd = it;
        // Remove trailing whitespaces from value
        // TODO: also remove "!important" from value and set it as flag, see (5) in:
        //       https://www.w3.org/TR/css-syntax-3/#consume-declaration
        while (valueEnd != valueBegin && (valueEnd-1)->type == StyleTokenType::Whitespace) {
            --valueEnd;
        }
        // Parse value
        StylePropertySpec* spec = StylePropertySpec::get(declaration->property_);
        StylePropertyParser parser = spec ? spec->parser() : &parseStyleDefault;
        declaration->value_ = parser(valueBegin, valueEnd);
        if (declaration->value_.type() == StyleValueType::Invalid) {
            // Parse error: return nothing
            return StyleDeclarationPtr();
        }
        return declaration;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-component-value
    // Assumes that `it` is not end
    static void consumeComponentValue_(StyleTokenIterator& it, StyleTokenIterator end) {
        if (it->type == StyleTokenType::LeftParenthesis ||
            it->type == StyleTokenType::LeftCurlyBracket ||
            it->type == StyleTokenType::LeftSquareBracket) {

            consumeSimpleBlock_(it, end);
            // TODO: return it
        }
        else if (it->type == StyleTokenType::Function) {
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
    static void consumeSimpleBlock_(StyleTokenIterator& it, StyleTokenIterator end) {
        StyleTokenType startToken = it->type;
        StyleTokenType endToken;
        if (startToken == StyleTokenType::LeftParenthesis) {
            endToken = StyleTokenType::RightParenthesis;
        }
        else if (startToken == StyleTokenType::LeftCurlyBracket) {
            endToken = StyleTokenType::RightCurlyBracket;
        }
        else { // startToken == TokenType::LeftSquareBracket
            endToken = StyleTokenType::RightSquareBracket;
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
    static void consumeFunction_(StyleTokenIterator& it, StyleTokenIterator end) {
        // TODO: create function object, and set its name to it->codePoints
        ++it;
        while (true) {
            if (it == end) {
                // Parse error: return the function
                break;
            }
            else if (it->type == StyleTokenType::RightParenthesis) {
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

    // https://www.w3.org/TR/selectors-3/#grouping
    // Returns an empty array if any of the selectors in the group is invalid.
    static core::Array<StyleSelectorPtr> consumeSelectorGroup_(StyleTokenIterator& it, StyleTokenIterator end) {
        core::Array<StyleSelectorPtr> res;
        while (true) {
            StyleTokenIterator selectorBegin = it;
            while (it != end && it->type != StyleTokenType::Comma) {
                ++it;
            }
            StyleSelectorPtr selector = consumeSelector_(selectorBegin, it);
            if (selector) {
                 res.append(selector);
            }
            else {
                // Syntax error
                return core::Array<StyleSelectorPtr>();
            }
            if (it == end) {
                break;
            }
            else { // it->type == TokenType::Comma
                ++it;
            }
        }
        return res;
    }

    // https://www.w3.org/TR/selectors-3/#selector-syntax
    // Returns null if the selector is invalid.
    // For now, we only accepts a single class selector as valid selector.
    static StyleSelectorPtr consumeSelector_(StyleTokenIterator& it, StyleTokenIterator end) {
        StyleSelectorPtr selector = StyleSelector::create();
        // Trim whitespaces at both ends
        while (it != end && it->type == StyleTokenType::Whitespace) {
            ++it;
        }
        while (it != end && (end-1)->type == StyleTokenType::Whitespace) {
            --end;
        }
        if (it == end) {
            // Parse error
            return StyleSelectorPtr();
        }
        // Consume items
        while (it != end) {
            bool ok = consumeSelectorItem_(selector->items_, it, end);
            if (!ok) {
                // Parse error
                return StyleSelectorPtr();
            }
        }
        return selector;
    }

    // Consumes one item and appends it to the given array. Returns false in
    // case of parse errors, in which case the item is not appended.
    static bool consumeSelectorItem_(core::Array<StyleSelectorItem>& items, StyleTokenIterator& it, StyleTokenIterator end) {
        if (it == end) {
            return false;
        }
        if (it->type == StyleTokenType::Delim && it->codePointsValue == ".") {
            ++it;
            if (it == end || it->type != StyleTokenType::Ident) {
                return false;
            }
            items.append(StyleSelectorItem(
                             StyleSelectorItemType::ClassSelector,
                             core::StringId(it->codePointsValue)));
            ++it;
            return true;
        }
        return false;
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

Style StyleSheet::computeStyle(Widget* widget) const
{
    Style style;

    // Compute which rule sets match this widget.
    //
    // TODO: improve performance by not iterating through all rule sets, but
    // instead only iterate over potential candidate rule sets (or selectors)
    // based on the widget's classes. Each per-class candidate rule sets would
    // be precomputed once after parsing the spreadsheet.
    //
    // TODO: sort rule sets by selector's specificity
    //
    for (StyleRuleSet* rule : ruleSets()) {
        for (StyleSelector* selector : rule->selectors()) {
            if (selector->matches(widget)) {
                style.ruleSets_.append(rule);
                break;
                // Note: the break is to prevent duplicate rule sets. But when
                // implementing specificity, we'll have to evaluate all
                // selectors and keep the one with largest specificity. Or
                // simply iterate over all candidate selectors regardless of
                // rule sets, and remove duplicate rule sets as a
                // post-processing step.
            }
        }
    }

    // Compute cascaded values.
    //
    for (StyleRuleSet* rule : style.ruleSets_) {
        for (StyleDeclaration* declaration : rule->declarations()) {
            style.map_[declaration->property()] = declaration->value();
        }
    }

    return style;
}

StyleRuleSet::StyleRuleSet() :
    Object()
{

}

StyleRuleSetPtr StyleRuleSet::create()
{
    return StyleRuleSetPtr(new StyleRuleSet());
}

bool StyleSelector::matches(Widget* widget)
{
    if (items_.isEmpty()) {
        // Logic error, but let's silently return false
        return false;
    }

    // For now, we only support a sequence of class selectors, that is,
    // something like ".class1.class2.class3". No combinators, pseudo-classes,
    // etc.. so the implementation is super easy : the widget simply has to
    // have all classes.
    for (const StyleSelectorItem& item : items_) {
        if (!widget->hasClass(item.name())) {
            return false;
        }
    }
    return true;
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
