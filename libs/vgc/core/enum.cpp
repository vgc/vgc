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

#include <vgc/core/enum.h>

namespace vgc::core::detail {

namespace {

// Extracts the fully-qualified name of the enum type (e.g., `vgc::ui::Key`)
// from the result of calling VGC_PRETTY_FUNCTION in enumData_().
//
// Example input (platform-dependent):
//
// "const class vgc::core::detail::EnumTypeData &__cdecl vgc::ui::enumData_(enum vgc::ui::Key)"
// "const ::vgc::core::detail::EnumTypeData &vgc::ui::enumData_(Key)"
//
// Desired output:
//
// "vgc::ui::Key"
//
std::string getEnumFullTypeName(std::string_view prettyFunction) {

    const auto& s = prettyFunction;

    std::string res;

    // `&__cdecl vgc::ui::enumData_(enum vgc::ui::Key)`
    //           ^        ^                       ^  ^
    //           k        l                       i  j
    //
    // `&vgc::ui::enumData_(Key)`
    //   ^        ^         ^  ^
    //   k        l         i  j

    // Find closing parenthesis
    size_t j = s.size();
    if (j == 0) {
        return "";
    }
    --j;
    while (j > 0 && s[j] != ')') {
        --j;
    }

    // Find opening parenthesis or whitespace or colon
    size_t i = j;
    while (i > 0 && s[i] != '(' && s[i] != ' ' && s[i] != ':') {
        --i;
    }

    // Set i to be the character just after found parenthese/whitespace/colon
    ++i;

    // Find opening parenthesis
    size_t l = i;
    while (l > 0 && s[l] != '(') {
        --l;
    }

    // Skip "enumData_"
    size_t enumDataSize = 9;
    if (l >= enumDataSize) {
        l -= enumDataSize;
    }
    else {
        l = 0;
    }

    // Find whitespace or ampersand
    size_t k = l;
    while (k > 0 && s[k] != ' ' && s[k] != '&') {
        --k;
    }

    // Set k to be the character just after found whitespace/ampersand
    ++k;

    // Concatenate namespace and class name
    if (l > k) {
        res += s.substr(k, l - k);
    }
    if (j > i) {
        res += s.substr(i, j - i);
    }
    return res;
}

} // namespace

EnumData::EnumData(TypeId id, std::string_view prettyFunction)
    : id(id)
    , fullTypeName(getEnumFullTypeName(prettyFunction)) {

    // Get unqualified type name from fully-qualified type name
    size_t i = fullTypeName.rfind(':'); // index of last ':' character
    if (i == std::string_view::npos) {  // if no ':' character
        shortTypeName = fullTypeName;
    }
    else {
        shortTypeName = std::string_view(fullTypeName).substr(i + 1);
    }

    // Create strings to return in case an unknown item is requested
    unknownItemShortName = "Unknown_" + shortTypeName;
    unknownItemPrettyName = "Unknown " + shortTypeName;
    unknownItemFullName = fullTypeName + unknownItemShortName;
}

void EnumData::addItem(
    UInt64 value,
    std::string_view shortName,
    std::string_view prettyName) {

    Int index = values.length();
    valueToIndex[value] = index;

    values.append(value);
    shortNames.append(std::string(shortName));
    prettyNames.append(std::string(prettyName));

    std::string fullName;
    fullName.reserve(fullTypeName.size() + 2 + shortName.size());
    fullName = fullTypeName;
    fullName.append("::");
    fullName.append(shortName);
    fullNames.append(std::move(fullName));
};

std::optional<Int> EnumData::getIndex(UInt64 value) const {
    auto search = valueToIndex.find(value);
    if (search != valueToIndex.end()) {
        return search->second;
    }
    else {
        return std::nullopt;
    }
}

} // namespace vgc::core::detail
