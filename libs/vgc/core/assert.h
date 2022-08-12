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

#ifndef VGC_CORE_ASSERT_H
#define VGC_CORE_ASSERT_H

#include <cassert>

#include <vgc/core/exceptions.h>
#include <vgc/core/format.h>

// Deprecated
#define VGC_CORE_ASSERT assert

/// Throws a LogicError if the provided \p condition is false.
///
/// Use assertions whenever useful for testing pre-conditions, post-conditions,
/// invariants, and intermediate checks that should be provable at class-level
/// or file-level, but which are not immediately obvious from the few couple of
/// lines of code above. Assertions are useful as fine-grain checks during
/// initial development and unit tests, but perhaps more importantly, they are
/// useful as documentation: they inform future maintainers of intent and
/// provide them with important conditions to keep in mind when refactoring.
///
/// Example:
///
/// \code
/// // Good
/// void Foo::privateFunction_() {
///     VGC_ASSERT(p_);
///     p_->doSomething();
/// }
///
/// // Not as good
/// void Foo::privateFunction_() {
///     // Here, we know that p_ is non-null, which is why we do not check the
///     // validity of p_ before deferencing it. To future maintainers: when
///     // refactoring, make sure that p_ is valid before calling this function.
///     p_->doSomething();
/// }
///
/// // Bad (unless it is a well-known class invariant that p_ is never null)
/// void Foo::privateFunction_() {
///    p_->doSomething();
/// }
/// \endcode
///
/// A typical use case for assertions is to verify that a supposedly non-null
/// pointer is indeed non-null. Or that an index is within a specified range.
/// Or that some given loop did find an element that was supposed to be there.
///
/// Never use assertions for testing the validity of input from end users of
/// even from client code. Instead, invalid input from end users should emit a
/// user-visible warning and fail gracefully, and invalid input from client
/// code should either emit a warning and fail gracefully, or possibly throw a
/// well-defined and documented exception if recovery can only be handled at
/// higher level.
///
/// Assertions should be extremely readable and fast to compute one-liners. As
/// a rule of thumb, if implementing the assertion would take you more than
/// 10sec, then don't implement it and instead write a comment. Keep in mind
/// that while a bug in production code is bad, a bug in assertion code is even
/// worse: indeed, false positives may make a perfectly functional program
/// crash, so make sure that preconditions are actually absolutely required.
///
/// Note that by convention, in the VGC codebase, all pointers given as
/// arguments to functions are always assumed to be valid non-null pointers
/// unless specified otherwise. Therefore, in order to keep the code concise,
/// there is no need to check this as a precondition.
///
#define VGC_ASSERT(condition)                                                            \
    do {                                                                                 \
        if (!(condition)) {                                                              \
            std::string error =                                                          \
                core::format("Failed to satisfy condition `" #condition "`");            \
            throw core::LogicError(error);                                               \
        }                                                                                \
    } while (0)

#endif // VGC_CORE_ASSERT_H
