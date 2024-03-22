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

#include <vgc/dom/value.h>

#include <mutex>
#include <unordered_map>

#include <vgc/dom/document.h>
#include <vgc/dom/element.h>
#include <vgc/dom/exceptions.h>
#include <vgc/dom/path.h>

namespace vgc::dom {

const Value& Value::none() {
    // trusty leaky singleton
    static const Value* v = new Value();
    return *v;
}

const Value& Value::invalid() {
    // trusty leaky singleton
    static const Value* v = new Value(InvalidValue{});
    return *v;
}

namespace detail {

const ValueTypeInfo* registerValueTypeInfo(const ValueTypeInfo* info) {

    using Map = std::unordered_map<core::TypeId, const ValueTypeInfo*>;

    // trusty leaky singletons
    static std::mutex* mutex = new std::mutex();
    static Map* map = new Map();

    std::lock_guard<std::mutex> lock(*mutex);

    auto res = map->try_emplace(info->typeId, info); // => ((key, value), emplaced))
    auto it = res.first;
    auto value = it->second;

    return value;
}

} // namespace detail

} // namespace vgc::dom
