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

#ifndef VGC_UI_STYLE_H
#define VGC_UI_STYLE_H

#include <vgc/core/innercore.h>
#include <vgc/ui/api.h>

namespace vgc {
namespace ui {

VGC_DECLARE_OBJECT(StyleSheet);

/// \class vgc::ui::StyleSheet
/// \brief Parses and stores a VGCSS stylesheet.
///
class VGC_UI_API StyleSheet : public core::Object {
private:
    VGC_OBJECT(StyleSheet)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected :
    /// Constructs a StyleSheet. This constructor is an implementation detail
    /// only available to derived classes. In order to create a StyleSheet,
    /// please use the following:
    ///
    /// ```cpp
    /// StyleSheetPtr stylesheet = StyleSheet::create();
    /// ```
    ///
    StyleSheet(const std::string& s);

public:
    /// Creates an empty stylesheet.
    ///
    static StyleSheetPtr create();

    /// Creates a stylesheet from the given string.
    ///
    static StyleSheetPtr create(const std::string& s);
};

VGC_DECLARE_OBJECT(StyleRuleSet);

/// \class vgc::ui::StyleRuleSet
/// \brief One rule set of a stylesheet.
///
class VGC_UI_API StyleRuleSet : public core::Object {
private:
    VGC_OBJECT(StyleRuleSet)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

private:
    friend class StyleSheet;
    StyleRuleSet();
    static StyleRuleSetPtr create();
};

VGC_DECLARE_OBJECT(StyleSelector);

/// \class vgc::ui::StyleSelector
/// \brief One selector of a rule set of a stylesheet.
///
class VGC_UI_API StyleSelector : public core::Object {
private:
    VGC_OBJECT(StyleSelector)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

private:
    friend class StyleSheet;
    StyleSelector();
    static StyleSelectorPtr create();
};

VGC_DECLARE_OBJECT(StyleDeclaration);

/// \class vgc::ui::StyleDeclaration
/// \brief One declaration of a rule set of a stylesheet.
///
class VGC_UI_API StyleDeclaration : public core::Object {
private:
    VGC_OBJECT(StyleDeclaration)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

private:
    friend class StyleSheet;
    StyleDeclaration();
    static StyleDeclarationPtr create();
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_STYLE_H
