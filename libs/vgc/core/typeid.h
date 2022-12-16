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

#ifndef VGC_CORE_TYPEID_H
#define VGC_CORE_TYPEID_H

#include <functional> // hash
#include <typeindex>
#include <typeinfo>

#include <vgc/core/api.h>
#include <vgc/core/format.h>
#include <vgc/core/stringid.h>

namespace vgc::core {

class TypeId;

namespace detail {

class VGC_CORE_API TypeInfo {
public:
    TypeInfo(const std::type_info& info)
        : id_(info.name()) {
    }

    bool operator==(const TypeInfo& other) const {
        return id_ == other.id_;
    }

    bool operator!=(const TypeInfo& other) const {
        return id_ != other.id_;
    }

    bool operator<(const TypeInfo& other) const {
        return id_ < other.id_;
    }

    std::string_view name() {
        return id_;
    }

private:
    friend struct std::hash<TypeId>;
    core::StringId id_;
};

} // namespace detail

/// \class vgc::core::TypeIndex
/// \brief Similar to std::type_index but works across shared library boundaries.
///
/// This is the return type of the function `vgc::core::typeId<T>()`. This type
/// has a very small footprint (one pointer), and is very fast to copy, assign,
/// and compare.
///
/// Comparison between `TypeId` makes it possible to query whether two types
/// are the same, which is guaranteed to work even across shared library
/// boundary (unlike `std::type_index`, which for example breaks across shared
/// library boundary on some versions of Clang).
///
/// Note that `TypeId` implements `operator<`, and specializes `std::hash`,
/// which makes it possible to insert in maps, sets, as well as their unordered
/// versions.
///
/// You can use `TypeId::name()` to query the name (possibly mangled) of the
/// type.
///
class VGC_CORE_API TypeId {
public:
    /// Returns the name of the type (possibly mangled).
    ///
    std::string_view name() {
        return info_->name();
    }

    /// Returns whether this `TypeIndex` is equal to `other`.
    ///
    bool operator==(const TypeId& other) const {
        return *info_ == *other.info_;
    }

    /// Returns whether this `TypeIndex` is different from `other`.
    ///
    bool operator!=(const TypeId& other) const {
        return *info_ != *other.info_;
    }

    /// Returns whether this `TypeIndex` is less than `other`.
    ///
    bool operator<(const TypeId& other) const {
        return *info_ < *other.info_;
    }

private:
    template<typename T>
    friend TypeId typeId();

    friend struct std::hash<TypeId>;

    TypeId(detail::TypeInfo* info)
        : info_(info) {
    }

    detail::TypeInfo* info_;
};

/// Returns the `TypeId` of the given type.
///
/// ```
/// TypeId id = typeId<int>();
/// ```
///
/// Using this function is preferrable over the C++ built-in function
/// `typeid()`, which does not guarantee type uniqueness across shared library
/// boundaries, under some platforms and/or compilers.
///
template<typename T>
TypeId typeId() {

    // Note: different DLLs may have their own TypeInfo instance, but they all
    // compare equal since they compare an internal StringId generated from
    // typeid(T).name(). This is fast: each StringId is only created once per
    // DLL per queried type in that DLL.
    //
    static detail::TypeInfo info(typeid(T));
    return TypeId(&info);
}

namespace detail {

// For testing TypeId comparisons accross DLLs. See tests/test_typeid.cpp.

class VGC_CORE_API TypeIdTestClass {};

VGC_CORE_API
TypeId typeIdTestClass();

VGC_CORE_API
TypeId typeIdInt();

} // namespace detail

} // namespace vgc::core

namespace std {

// Implement hash function to make TypeIndex compatible with std::unordered_map
template<>
struct hash<vgc::core::TypeId> {
    std::size_t operator()(const vgc::core::TypeId& t) const {
        return std::hash<vgc::core::StringId>()(t.info_->id_);
    }
};

} // namespace std

template<>
struct fmt::formatter<vgc::core::TypeId> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(vgc::core::TypeId t, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(t.name(), ctx);
    }
};

#endif // VGC_CORE_TYPEID_H
