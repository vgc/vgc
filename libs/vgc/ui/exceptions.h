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

#ifndef VGC_UI_EXCEPTIONS_H
#define VGC_UI_EXCEPTIONS_H

#include <vgc/core/exceptions.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

class Widget;

namespace detail {

VGC_UI_API std::string childCycleMsg(const Widget* parent, const Widget* child);

} // namespace detail

/// \class vgc::ui::LogicError
/// \brief Raised when there is a logic error detected in vgc::ui.
///
/// This exception is raised whenever there is a logic error detected in
/// vgc::ui. This is the base class for all logic error exception classes in
/// vgc::ui.
///
class VGC_UI_API_EXCEPTION LogicError : public core::LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a LogicError with the given \p reason.
    ///
    explicit LogicError(const std::string& reason)
        : core::LogicError(reason) {
    }
};

/// \class vgc::ui::ChildCycleError
/// \brief Raised when requested to make a Widget a child of itself or of one of
///        its descendants
///
/// The widget tree is not allowed to have cycles. Therefore, this exception is
/// raised whenever a requested operation would result in a cycle, that is,
/// when attempting to insert a Widget as a child of itself or of one of its
/// descendants.
///
class VGC_UI_API_EXCEPTION ChildCycleError : public LogicError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a ChildCycleError informing that \p parent cannot have \p
    /// child as its child because \p parent is a descendant of \p child.
    ///
    ChildCycleError(const Widget* parent, const Widget* child)
        : LogicError(detail::childCycleMsg(parent, child)) {
    }
};

/// \class vgc::ui::RuntimeError
/// \brief Raised when there is a runtime error detected in vgc::ui.
///
/// This exception is raised whenever there is a runtime error detected in
/// vgc::ui. This is the base class for all runtime error exception classes in
/// vgc::ui.
///
class VGC_UI_API_EXCEPTION RuntimeError : public core::RuntimeError {
private:
    VGC_CORE_EXCEPTIONS_DECLARE_ANCHOR

public:
    /// Constructs a LogicError with the given \p reason.
    ///
    explicit RuntimeError(const std::string& reason)
        : core::RuntimeError(reason) {
    }
};

} // namespace vgc::ui

#endif // VGC_UI_EXCEPTIONS_H
