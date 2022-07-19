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

#include <vgc/style/style.h>

#include <vgc/core/colors.h>
#include <vgc/style/strings.h>
#include <vgc/style/stylableobject.h>

namespace vgc::style {

StyleValue parseStyleDefault(StyleTokenIterator begin, StyleTokenIterator end)
{
    if (end == begin + 1) {
        StyleTokenType t = begin->type;
        if (t == StyleTokenType::Identifier) {
            return StyleValue::identifier(begin->codePointsValue);
        }
    }
    return StyleValue::invalid();
}

namespace internal {

// https://www.w3.org/TR/css-syntax-3/#parsing
//
// Note: we use a class with static functions (rather than free functions) to
// make it easier for the StyleSheet class (and other classes) to simply
// befriend this class, instead of befriending all the free functions.
//
class StyleParser {
    StylePropertySpecTablePtr specs_;
    bool topLevel_;
    StyleParser(const StylePropertySpecTablePtr& specs, bool topLevel)
        : specs_(specs)
        , topLevel_(topLevel) {
    }

public:
    // https://www.w3.org/TR/css-syntax-3/#parse-stylesheet
    static StyleSheetPtr parseStyleSheet(const StylePropertySpecTablePtr& specs,
                                         std::string_view styleString) {

        // Tokenize
        std::string decoded = decodeStyleString(styleString);
        StyleTokenArray tokens = tokenizeStyleString(decoded.data());

        // Parse
        bool topLevel = true;
        StyleParser parser(specs, topLevel);
        StyleTokenIterator it = tokens.begin();
        core::Array<StyleRuleSetPtr> rules = parser.consumeRuleList_(it, tokens.end());

        // Create StyleSheet
        StyleSheetPtr styleSheet = StyleSheet::create();
        styleSheet->propertySpecs_ = specs;
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
    core::Array<StyleRuleSetPtr> consumeRuleList_(StyleTokenIterator& it, StyleTokenIterator end) {
        std::ignore = topLevel_; // suppress warning
        core::Array<StyleRuleSetPtr> res;
        while (true) {
            if (it == end) {
                break;
            }
            else if (it->type == StyleTokenType::Whitespace) {
                ++it;
            }
            // Uncomment to support CDO/CDC
            // else if (it->type == StyleTokenType::CommentDelimiterOpen ||
            //          it->type == StyleTokenType::CommentDelimiterClose) {
            //     if (topLevel_) {
            //         ++it;
            //         continue;
            //     }
            //     else {
            //         StyleRuleSetPtr rule = consumeQualifiedRule_(it, end);
            //         if (rule) {
            //             res.append(rule);
            //         }
            //     }
            // }
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
    void consumeAtRule_(StyleTokenIterator& it, StyleTokenIterator end) {
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
    StyleRuleSetPtr consumeQualifiedRule_(StyleTokenIterator& it, StyleTokenIterator end) {
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
    core::Array<StyleDeclarationPtr> consumeDeclarationList_(StyleTokenIterator& it, StyleTokenIterator end, bool expectRightCurlyBracket) {
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
            else if (it->type == StyleTokenType::Identifier) {
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
    StyleDeclarationPtr consumeDeclaration_(StyleTokenIterator& it, StyleTokenIterator end) {
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
        // XXX We should probably first check for global keywords like 'inherit' and
        // only call the custom parser if it's not a global keyword.
        const StylePropertySpec* spec = specs_ ? specs_->get(declaration->property_) : nullptr;
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
    void consumeComponentValue_(StyleTokenIterator& it, StyleTokenIterator end) {
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
    void consumeSimpleBlock_(StyleTokenIterator& it, StyleTokenIterator end) {
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
    void consumeFunction_(StyleTokenIterator& it, StyleTokenIterator end) {
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
    core::Array<StyleSelectorPtr> consumeSelectorGroup_(StyleTokenIterator& it, StyleTokenIterator end) {
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
    StyleSelectorPtr consumeSelector_(StyleTokenIterator& it, StyleTokenIterator end) {
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
    bool consumeSelectorItem_(core::Array<StyleSelectorItem>& items, StyleTokenIterator& it, StyleTokenIterator end) {
        if (it == end) {
            return false;
        }
        if (it->type == StyleTokenType::Delimiter && it->codePointsValue == ".") {
            ++it;
            if (it == end || it->type != StyleTokenType::Identifier) {
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

StyleSheetPtr StyleSheet::create(const StylePropertySpecTablePtr& specs,
                                 std::string_view s)
{
    return internal::StyleParser::parseStyleSheet(specs, s);
}

StyleRuleSet::StyleRuleSet() :
    Object()
{

}

StyleRuleSetPtr StyleRuleSet::create()
{
    return StyleRuleSetPtr(new StyleRuleSet());
}

bool StyleSelector::matches(StylableObject* node)
{
    if (items_.isEmpty()) {
        // Logic error, but let's silently return false
        return false;
    }

    // For now, we only support a sequence of class selectors, that is,
    // something like ".class1.class2.class3". No combinators, pseudo-classes,
    // etc.. so the implementation is super easy : the node simply has to
    // have all classes.
    for (const StyleSelectorItem& item : items_) {
        if (!node->hasStyleClass(item.name())) {
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

} // namespace vgc::style
