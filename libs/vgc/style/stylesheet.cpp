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

#include <vgc/style/stylesheet.h>

#include <vgc/style/logcategories.h>
#include <vgc/style/stylableobject.h>

namespace vgc::style {

namespace detail {

// https://www.w3.org/TR/css-syntax-3/#parsing
//
// Note: we use a class with static functions (rather than free functions) to
// make it easier for the StyleSheet class (and other classes) to simply
// befriend this class, instead of befriending all the free functions.
//
class StyleParser {
    bool topLevel_;
    StyleParser(bool topLevel)
        : topLevel_(topLevel) {
    }

public:
    // https://www.w3.org/TR/css-syntax-3/#parse-stylesheet
    static StyleSheetPtr parseStyleSheet(std::string_view styleString) {

        // Tokenize
        std::string decoded = decodeStyleString(styleString);
        TokenArray tokens = tokenizeStyleString(decoded.data());

        // Parse
        bool topLevel = true;
        StyleParser parser(topLevel);
        TokenIterator it = tokens.begin();
        core::Array<StyleRuleSetPtr> rules = parser.consumeRuleList_(it, tokens.end());

        // Create StyleSheet
        StyleSheetPtr styleSheet = StyleSheet::create();
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
    core::Array<StyleRuleSetPtr>
    consumeRuleList_(TokenIterator& it, TokenIterator end) {
        std::ignore = topLevel_; // suppress warning
        core::Array<StyleRuleSetPtr> res;
        while (true) {
            if (it == end) {
                break;
            }
            else if (it->type() == TokenType::Whitespace) {
                ++it;
            }
            // Uncomment to support CDO/CDC
            // else if (it->type == TokenType::CommentDelimiterOpen ||
            //          it->type == TokenType::CommentDelimiterClose) {
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
            else if (it->type() == TokenType::AtKeyword) {
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
    void consumeAtRule_(TokenIterator& it, TokenIterator end) {
        // For now, we just consume the rule without returning anything.
        // In the future, we'll return a StyleAtRule
        ++it; // skip At token
        while (true) {
            if (it == end) {
                // Parse Error: return the partially consumed AtRule
                break;
            }
            else if (it->type() == TokenType::Semicolon) {
                ++it;
                break;
            }
            else if (it->type() == TokenType::LeftCurlyBracket) {
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
    StyleRuleSetPtr
    consumeQualifiedRule_(TokenIterator& it, TokenIterator end) {
        StyleRuleSetPtr rule = StyleRuleSet::create();
        TokenIterator preludeBegin = it;
        while (true) {
            if (it == end) {
                // Parse Error: return nothing
                return StyleRuleSetPtr();
            }
            else if (it->type() == TokenType::LeftCurlyBracket) {
                TokenIterator preludeEnd = it;
                ++it;

                // Parse the prelude as a selector group
                core::Array<StyleSelectorPtr> selectors =
                    consumeSelectorGroup_(preludeBegin, preludeEnd);
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
                core::Array<StyleDeclarationPtr> declarations =
                    consumeDeclarationList_(it, end, expectRightCurlyBracket);
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
    core::Array<StyleDeclarationPtr> consumeDeclarationList_(
        TokenIterator& it,
        TokenIterator end,
        bool expectRightCurlyBracket) {

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
            else if (it->type() == TokenType::Whitespace) {
                ++it;
            }
            else if (it->type() == TokenType::Semicolon) {
                ++it;
            }
            else if (it->type() == TokenType::AtKeyword) {
                consumeAtRule_(it, end);
                // Note: for now, the at rule is simply skipped and
                // not appended to the list of declarations.
            }
            else if (it->type() == TokenType::Identifier) {
                TokenIterator declarationBegin = it;
                while (true) {
                    if (it == end                                  //
                        || it->type() == TokenType::Semicolon //
                        || (expectRightCurlyBracket                //
                            && it->type() == TokenType::RightCurlyBracket)) {

                        break;
                    }
                    else {
                        consumeComponentValue_(it, end);
                    }
                }
                TokenIterator declarationEnd = it;
                StyleDeclarationPtr declaration =
                    consumeDeclaration_(declarationBegin, declarationEnd);
                if (declaration) {
                    res.append(declaration);
                }
            }
            else if (
                expectRightCurlyBracket
                && it->type() == TokenType::RightCurlyBracket) {
                ++it;
                break;
            }
            else {
                // Parse error: throw away component values until semicolon or eof
                while (true) {
                    if (it == end || it->type() == TokenType::Semicolon
                        || (expectRightCurlyBracket
                            && it->type() == TokenType::RightCurlyBracket)) {

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
    StyleDeclarationPtr
    consumeDeclaration_(TokenIterator& it, TokenIterator end) {

        StyleDeclarationPtr declaration = StyleDeclaration::create();
        declaration->property_ = core::StringId(it->stringValue());
        declaration->value_ = Value::invalid();
        ++it;

        // Consume whitespaces
        while (it != end && it->type() == TokenType::Whitespace) {
            ++it;
        }

        // Ensure first non-whitespace token is a Colon
        if (it == end || it->type() != TokenType::Colon) {
            // Parse error: return nothing
            return StyleDeclarationPtr();
        }
        else {
            ++it;
        }

        // Consume whitespaces
        while (it != end && it->type() == TokenType::Whitespace) {
            ++it;
        }

        // Consume value components
        TokenIterator valueBegin = it;
        while (it != end) {
            consumeComponentValue_(it, end);
        }
        TokenIterator valueEnd = it;

        // Remove trailing whitespaces from value
        // TODO: also remove "!important" from value and set it as flag, see (5) in:
        //       https://www.w3.org/TR/css-syntax-3/#consume-declaration
        while (valueEnd != valueBegin
               && (valueEnd - 1)->type() == TokenType::Whitespace) {
            --valueEnd;
        }

        // Store unparsed value. Parsing is defer until the attribute is
        // actually queried, that is, until we have an appropriate SpecTable.
        //
        // XXX We might still want to check here for global keywords like 'inherit'/etc.
        //
        declaration->value_ = Value::unparsed(valueBegin, valueEnd);

        return declaration;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-component-value
    // Assumes that `it` is not end
    void consumeComponentValue_(TokenIterator& it, TokenIterator end) {
        if (it->type() == TokenType::LeftParenthesis
            || it->type() == TokenType::LeftCurlyBracket
            || it->type() == TokenType::LeftSquareBracket) {

            consumeSimpleBlock_(it, end);
            // TODO: return it
        }
        else if (it->type() == TokenType::Function) {
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
    void consumeSimpleBlock_(TokenIterator& it, TokenIterator end) {
        TokenType startToken = it->type();
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
            else if (it->type() == endToken) {
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
    void consumeFunction_(TokenIterator& it, TokenIterator end) {
        // TODO: create function object, and set its name to it->codePoints
        ++it;
        while (true) {
            if (it == end) {
                // Parse error: return the function
                break;
            }
            else if (it->type() == TokenType::RightParenthesis) {
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
    core::Array<StyleSelectorPtr>
    consumeSelectorGroup_(TokenIterator& it, TokenIterator end) {
        core::Array<StyleSelectorPtr> res;
        while (true) {
            TokenIterator selectorBegin = it;
            while (it != end && it->type() != TokenType::Comma) {
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
    StyleSelectorPtr consumeSelector_(TokenIterator& it, TokenIterator end) {
        core::Array<StyleSelectorItem> selectorItems;
        // Trim whitespaces at both ends
        while (it != end && it->type() == TokenType::Whitespace) {
            ++it;
        }
        while (it != end && (end - 1)->type() == TokenType::Whitespace) {
            --end;
        }
        if (it == end) {
            // Parse error
            return StyleSelectorPtr();
        }
        // Consume items
        while (it != end) {
            bool ok = consumeSelectorItem_(selectorItems, it, end);
            if (!ok) {
                // Parse error
                return StyleSelectorPtr();
            }
        }
        return StyleSelector::create(std::move(selectorItems));
    }

    // Consumes one item and appends it to the given array. Returns false in
    // case of parse errors, in which case the item is not appended.
    bool consumeSelectorItem_(
        core::Array<StyleSelectorItem>& items,
        TokenIterator& it,
        TokenIterator end) {

        if (it == end) {
            return false;
        }
        if (it->type() == TokenType::Delimiter && it->stringValue() == ".") {
            ++it;
            if (it == end || it->type() != TokenType::Identifier) {
                return false;
            }
            items.emplaceLast(
                StyleSelectorItemType::ClassSelector, core::StringId(it->stringValue()));
            ++it;
            return true;
        }
        else if (it->type() == TokenType::Whitespace) {
            while (it->type() == TokenType::Whitespace) {
                ++it;
                if (it == end) {
                    return false;
                }
            }
            if (it->type() == TokenType::Delimiter && it->stringValue() == ">") {
                items.emplaceLast(StyleSelectorItemType::ChildCombinator);
                ++it;
            }
            else {
                items.emplaceLast(StyleSelectorItemType::DescendantCombinator);
            }
            while (it != end && it->type() == TokenType::Whitespace) {
                ++it;
            }
            return true;
        }
        return false;
    }
};

} // namespace detail

void SpecTable::insert(
    core::StringId attributeName,
    const Value& initialValue,
    bool isInherited,
    StylePropertyParser parser) {

    if (get(attributeName)) {
        VGC_WARNING(
            LogVgcStyle,
            "Attempting to insert a property spec for the attribute '{}', which is "
            "already registered. Aborted.",
            attributeName);
        return;
    }
    StylePropertySpec spec(attributeName, initialValue, isInherited, parser);
    map_.insert({attributeName, spec});
}

bool SpecTable::setRegistered(core::StringId className) {
    auto res = registeredClassNames_.insert(className);
    return res.second;
}

StyleSheet::StyleSheet()
    : Object() {
}

StyleSheetPtr StyleSheet::create() {
    return StyleSheetPtr(new StyleSheet());
}

StyleSheetPtr StyleSheet::create(std::string_view s) {
    return detail::StyleParser::parseStyleSheet(s);
}

StyleRuleSet::StyleRuleSet()
    : Object() {
}

StyleRuleSetPtr StyleRuleSet::create() {
    return StyleRuleSetPtr(new StyleRuleSet());
}

VGC_DEFINE_ENUM(
    StyleSelectorItemType,
    (ClassSelector, "Class Selector"),
    (DescendantCombinator, "Descendant Combinator"),
    (ChildCombinator, "Child Combinator"))

namespace {

using StyleSelectorItemIterator = core::Array<StyleSelectorItem>::iterator;

// Returns whether the given StylableObject matches the given
// selector group. A selector group is a sublist of items between
// two combinators.
//
bool matchesGroup_(
    StylableObject* node,
    StyleSelectorItemIterator begin,
    StyleSelectorItemIterator end) {

    // For now, we only support a sequence of class selectors, that is,
    // something like ".class1.class2.class3". No pseudo-classes, etc... so the
    // implementation is super easy : the node simply has to have all classes.
    //
    for (StyleSelectorItemIterator it = begin; it < end; ++it) {
        if (!node->hasStyleClass(it->name())) {
            return false;
        }
    }
    return true;
}

} // namespace

bool StyleSelector::matches(StylableObject* node) {

    // TODO: Should we pre-validate the selector during parsing (thus, never
    // create an invalid StyleSelector), and in this function, raise a
    // LogicError instead of returning false when the selector isn't valid?
    // Should the whole ruleset be discarded if any of its selectors is
    // invalid?
    //
    // Should we precompute and cache the groups?
    //

    // Check that the selector item array isn't empty.
    //
    StyleSelectorItemIterator begin = items_.begin();
    StyleSelectorItemIterator end = items_.end();
    if (begin == end) {
        // Invalid selector: items is empty
        return false;
    }

    // We process the array of items by splitting it into "groups" separated by
    // a combinator, and iterating from the last group down to the first group.
    //
    StyleSelectorItemIterator groupBegin = end;
    StyleSelectorItemIterator groupEnd = end;

    // Find right-most group
    while (groupBegin != begin && !(groupBegin - 1)->isCombinator()) {
        --groupBegin;
    }
    if (groupBegin == groupEnd) {
        // Invalid selector: last item is a combinator
        return false;
    }

    // Check if the node matches the last group
    if (matchesGroup_(node, groupBegin, groupEnd)) {

        // The node matches the last group. Now we check the other constraints.
        StylableObject* currentNode = node;
        while (groupBegin != begin) {

            // No matter the combinator, if there is no parent, then it's
            // impossible to match the selector.
            //
            StylableObject* parent = currentNode->parentStylableObject();
            if (!parent) {
                return false;
            }

            // Get combinator type
            --groupBegin;
            StyleSelectorItemType combinatorType = groupBegin->type();

            // Get previous group
            groupEnd = groupBegin;
            while (groupBegin != begin && !(groupBegin - 1)->isCombinator()) {
                --groupBegin;
            }
            if (groupBegin == groupEnd) {
                // Invalid selector: two successive combinators, or
                // first item is a combinator
                return false;
            }

            // Apply combinator
            if (combinatorType == StyleSelectorItemType::ChildCombinator) {
                if (matchesGroup_(parent, groupBegin, groupEnd)) {
                    currentNode = parent;
                }
                else {
                    return false;
                }
            }
            else if (combinatorType == StyleSelectorItemType::DescendantCombinator) {
                while (parent && !matchesGroup_(parent, groupBegin, groupEnd)) {
                    parent = parent->parentStylableObject();
                }
                if (parent) {
                    currentNode = parent;
                }
                else {
                    return false;
                }
            }
            else {
                std::string message = core::format(
                    "StyleSelectorItemType {} was supposed to be a combinator but isn't.",
                    combinatorType);
                throw core::LogicError(message);
            }
        }
        return true;
    }
    else {
        // Selector doesn't match
        return false;
    }
}

StyleSelector::StyleSelector(core::Array<StyleSelectorItem>&& items)
    : Object()
    , items_(std::move(items))
    , specificity_(0) {

    // Compute specificity
    for (StyleSelectorItem& item : items_) {
        if (item.type() == StyleSelectorItemType::ClassSelector) {
            ++specificity_;
        }
    }
}

StyleSelectorPtr StyleSelector::create(core::Array<StyleSelectorItem>&& items) {
    return StyleSelectorPtr(new StyleSelector(std::move(items)));
}

StyleDeclaration::StyleDeclaration()
    : Object() {
}

StyleDeclarationPtr StyleDeclaration::create() {
    return StyleDeclarationPtr(new StyleDeclaration());
}

} // namespace vgc::style
