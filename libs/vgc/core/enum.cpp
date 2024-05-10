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

namespace vgc::core {

namespace detail {

EnumTypeInfo_::EnumTypeInfo_(TypeId id)
    : typeId(id) {
    
    unknownValueShortName = "Unknown_";
    unknownValueShortName.append(typeId.name());

    unknownValuePrettyName = "Unknown ";
    unknownValuePrettyName.append(typeId.name());
    
    unknownValueFullName = typeId.fullName();
    unknownValueFullName.append("::");
    unknownValueFullName.append(unknownValueShortName);
}

EnumTypeInfo_::~EnumTypeInfo_() {
}

void EnumTypeInfo_::addValue_(
    UInt64 value,
    std::string_view shortName,
    std::string_view prettyName) {

    std::string fullName;
    fullName.reserve(typeId.fullName().size() + 2 + shortName.size());
    fullName.append(typeId.fullName());
    fullName.append("::");
    fullName.append(shortName);

    // Note: we will be able to use make_unique in C++20:
    // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html
    //
    std::unique_ptr<EnumValueInfo> ptr(new EnumValueInfo{
        value,                  //
        std::move(fullName),    //
        std::string(shortName), //
        std::string(prettyName)});
    valueInfo.append(std::move(ptr));

    const EnumValueInfo& info = *valueInfo.last();
    Int index = valueInfo.length() - 1;

    valueToIndex[value] = index;
    shortNameToIndex[info.shortName] = index;

    enumValues.append(EnumValue(EnumType(this), value));
    fullNames.append(info.fullName);
    shortNames.append(info.shortName);
    prettyNames.append(info.prettyName);
};

std::optional<Int> EnumTypeInfo_::getIndex(UInt64 value) const {
    auto search = valueToIndex.find(value);
    if (search != valueToIndex.end()) {
        return search->second;
    }
    else {
        return std::nullopt;
    }
}

std::optional<Int>
EnumTypeInfo_::getIndexFromShortName(std::string_view shortName) const {
    auto search = shortNameToIndex.find(shortName);
    if (search != shortNameToIndex.end()) {
        return search->second;
    }
    else {
        return std::nullopt;
    }
}

const EnumTypeInfo_* getOrCreateEnumTypeInfo(TypeId typeId, EnumTypeInfoFactory factory) {

    using Map = std::unordered_map<TypeId, const EnumTypeInfo_*>;

    // trusty leaky singletons
    static std::mutex* mutex = new std::mutex();
    static Map* map = new Map();

    std::lock_guard<std::mutex> lock(*mutex);
    auto it = map->find(typeId);
    if (it == map->end()) {
        const EnumTypeInfo_* info = factory();
        it = map->try_emplace(info->typeId, info).first;
    }
    return it->second;
}

} // namespace detail

std::optional<EnumValue> EnumType::fromShortName(std::string_view shortName) const {
    if (auto index = info_->getIndexFromShortName(shortName)) {
        return info_->enumValues[*index];
    }
    else {
        return std::nullopt;
    }
}

} // namespace vgc::core
