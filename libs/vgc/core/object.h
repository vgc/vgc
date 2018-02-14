// Copyright 2017 The VGC Developers
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

#ifndef VGC_CORE_OBJECT_H
#define VGC_CORE_OBJECT_H

#include <memory>
#include <vgc/core/api.h>

#define VGC_CORE_DECLARE_PTRS(T)             \
    class T;                                 \
    using T##SharedPtr = std::shared_ptr<T>; \
    using T##WeakPtr = std::weak_ptr<T>

namespace vgc {
namespace core {

/// \class vgc::core::Object
/// \brief Object model used in the VGC codebase.
///
/// In the VGC codebase, we extensively use std::shared_ptr and std::weak_ptr for
/// memory management and thread safety. This also allows convenient implementation
/// of Qt-style signals/slots.
///
template <typename T>
class Object: public std::enable_shared_from_this<T>
{
public:
    /// Returns an std::shared_ptr to a newly created object.
    /// This method is a convenient wrapper around std::make_shared.
    ///
    template <typename... Args>
    static std::shared_ptr<T> make(Args... args) {
        return std::make_shared<T>(args...);
    }

    /// Returns an std::shared_ptr to this object.
    /// This method is a convenient wrapper around shared_from_this.
    ///
    std::shared_ptr<T> ptr() {
        return this->shared_from_this();
    }
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_OBJECT_H
