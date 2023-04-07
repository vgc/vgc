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

#ifndef VGC_TOPOLOGY_EXCEPTIONS_H
#define VGC_TOPOLOGY_EXCEPTIONS_H

#include <vgc/core/exceptions.h>
#include <vgc/topology/api.h>

namespace vgc::topology {

class VacNode;

namespace detail {

VGC_TOPOLOGY_API
std::string notAChildMsg(const VacNode* node, const VacNode* expectedParent);

} // namespace detail

/// \class vgc::topology::LogicError
/// \brief Raised when there is a logic error detected in vgc::topology.
///
/// This exception is raised whenever there is a logic error detected in
/// vgc::topology. This is the base class for all logic error exception classes in
/// vgc::topology.
///
class VGC_TOPOLOGY_API_EXCEPTION LogicError : public core::LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a LogicError with the given `reason`.
    ///
    explicit LogicError(const std::string& reason)
        : core::LogicError(reason) {
    }
};

/// \class vgc::topology::RuntimeError
/// \brief Raised when there is a runtime error detected in vgc::topology.
///
/// This exception is raised whenever there is a runtime error detected in
/// vgc::topology. This is the base class for all runtime error exception classes in
/// vgc::topology.
///
class VGC_TOPOLOGY_API_EXCEPTION RuntimeError : public core::RuntimeError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a RuntimeError with the given `reason`.
    ///
    explicit RuntimeError(const std::string& reason)
        : core::RuntimeError(reason) {
    }
};

/// \class vgc::topology::NotAChildError
/// \brief Raised when a given node is expected to be a child of another
///        node, but isn't.
///
/// This exception is raised when a given node is expected to be a child of
/// another node, but isn't. For example, it is raised when the `nextSibling`
/// argument of `createKeyVertex(position, parent, nextSibling)` is non-null and
/// isn't a child of `parent`.
///
class VGC_TOPOLOGY_API_EXCEPTION NotAChildError : public LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a NotAChildError, informing that the given \p object is not a
    /// child of the given \p expectedParent.
    ///
    NotAChildError(const VacNode* node, const VacNode* expectedParent)
        : LogicError(detail::notAChildMsg(node, expectedParent)) {
    }
};

} // namespace vgc::topology

#endif // VGC_TOPOLOGY_EXCEPTIONS_H
