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

#ifndef VGC_CORE_OBJECTTYPE_H
#define VGC_CORE_OBJECTTYPE_H

#include <functional> // hash

#include <vgc/core/api.h>
#include <vgc/core/format.h>
#include <vgc/core/stringid.h>
#include <vgc/core/typeid.h>

namespace vgc::core {

namespace detail {

class ObjPtrAccess;

} // namespace detail

/// Provides meta-information about an `Object`'s type.
///
class VGC_CORE_API ObjectType {
public:
    /// Returns the `TypeId` that corresponds to this `Object`'s type.
    ///
    TypeId typeId() const {
        return typeId_;
    }

    /// Returns the unqualified name of the type.
    ///
    /// This might be non-unique and thus should only be used for
    /// human-readable printing purposes, not for type identification.
    ///
    std::string_view unqualifiedName() const {
        return unqualifiedName_;
    }

    /// Returns the unique name of the type (possibly mangled).
    ///
    // TODO: demangle
    //
    std::string_view name() const {
        return typeId_.name();
    }

    /// Returns whether this `ObjectTypeId` is equal to `other`.
    ///
    bool operator==(const ObjectType& other) const {
        return typeId_ == other.typeId_;
    }

    /// Returns whether this `ObjectTypeId` is different from `other`.
    ///
    bool operator!=(const ObjectType& other) const {
        return typeId_ != other.typeId_;
    }

    /// Returns whether this `ObjectTypeId` is less than `other`.
    ///
    bool operator<(const ObjectType& other) const {
        return typeId_ < other.typeId_;
    }

private:
    friend detail::ObjPtrAccess;

    core::TypeId typeId_;
    core::StringId unqualifiedName_;
    // TODO: other meta-info, e.g., inheritance

    ObjectType(core::TypeId typeId, std::string_view unqualifiedName)
        : typeId_(typeId)
        , unqualifiedName_(unqualifiedName) {
    }
};

} // namespace vgc::core

namespace std {

// Specializes std::hash for ObjectType
template<>
struct hash<vgc::core::ObjectType> {
    std::size_t operator()(const vgc::core::ObjectType& t) const {
        return std::hash<vgc::core::TypeId>()(t.typeId());
    }
};

} // namespace std

// Specializes fmt::formatter for ObjectType
template<>
struct fmt::formatter<vgc::core::ObjectType> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::core::ObjectType& t, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(t.name(), ctx);
    }
};

#endif // VGC_CORE_OBJECTTYPE_H
