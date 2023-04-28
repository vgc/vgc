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

#ifndef VGC_VACOMPLEX_EXCEPTIONS_H
#define VGC_VACOMPLEX_EXCEPTIONS_H

#include <vgc/core/exceptions.h>
#include <vgc/vacomplex/api.h>

namespace vgc::vacomplex {

class Node;

namespace detail {

VGC_VACOMPLEX_API
std::string notAChildMsg(const Node* node, const Node* expectedParent);

} // namespace detail

/// \class vgc::vacomplex::LogicError
/// \brief Raised when there is a logic error detected in vgc::vacomplex.
///
/// This exception is raised whenever there is a logic error detected in
/// vgc::vacomplex. This is the base class for all logic error exception classes in
/// vgc::vacomplex.
///
class VGC_VACOMPLEX_API_EXCEPTION LogicError : public core::LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a LogicError with the given `reason`.
    ///
    explicit LogicError(const std::string& reason)
        : core::LogicError(reason) {
    }
};

/// \class vgc::vacomplex::RuntimeError
/// \brief Raised when there is a runtime error detected in vgc::vacomplex.
///
/// This exception is raised whenever there is a runtime error detected in
/// vgc::vacomplex. This is the base class for all runtime error exception classes in
/// vgc::vacomplex.
///
class VGC_VACOMPLEX_API_EXCEPTION RuntimeError : public core::RuntimeError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a RuntimeError with the given `reason`.
    ///
    explicit RuntimeError(const std::string& reason)
        : core::RuntimeError(reason) {
    }
};

/// \class vgc::vacomplex::NotAChildError
/// \brief Raised when a given node is expected to be a child of another
///        node, but isn't.
///
/// This exception is raised when a given node is expected to be a child of
/// another node, but isn't. For example, it is raised when the `nextSibling`
/// argument of `createKeyVertex(position, parent, nextSibling)` is non-null and
/// isn't a child of `parent`.
///
class VGC_VACOMPLEX_API_EXCEPTION NotAChildError : public LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a NotAChildError, informing that the given \p object is not a
    /// child of the given \p expectedParent.
    ///
    NotAChildError(const Node* node, const Node* expectedParent)
        : LogicError(detail::notAChildMsg(node, expectedParent)) {
    }
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_EXCEPTIONS_H
