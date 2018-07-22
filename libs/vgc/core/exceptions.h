// Copyright 2018 The VGC Developers
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

#ifndef VGC_CORE_EXCEPTION_H
#define VGC_CORE_EXCEPTION_H

#include <stdexcept>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// \class vgc::core::LogicError
/// \brief Base class for all logic errors
///
/// This class should be used to report all logic errors detected by VGC code,
/// that are typically a consequence of incorrect usage of public API. For
/// example, a given method may choose to raise a LogicError if its
/// preconditions are not met.
///
/// The recommended programming practice when calling a method with
/// preconditions is to ensure that the preconditions are met before calling
/// the method, then call the method without checking for exceptions. Normally,
/// you should never write a try/catch block yourself to catch LogicErrors,
/// but instead let them be handled at a higher-level.
///
/// Keep in mind that function input may come from various sources:
///
/// 1. C++ libraries: These are expected to know what they're doing. They should
///    ensure that preconditions are met before calling any method. Failure to
///    do so or to catch exceptions may result in a program crash. Such coding
///    error should normally be catched in unit tests or beta tests before
///    shipping stable versions.
///
/// 2. Python libraries: These are also expected to know what they're doing.
///    They should ensure that preconditions are met before calling any method.
///    Failure to do so results in a thrown C++ exception converted to a Python
///    exception which may stop the execution of your Python library. Such
///    coding error should normally be catched in unit tests or beta tests
///    before shipping stable versions.
///
/// 3. Python user scripts: These may be written by users with very little
///    programming experience, and such scripts might be executed for the first
///    time within an interactive session containing valuable artistic data. It
///    is still their responsability to ensure that preconditions are met
///    before calling any method, but failure to do so should not result in a
///    program crash. The exception is typically reported to the interactive
///    Python console, and the program should return to the most recent valid
///    state before the exception was thrown.
///
/// 4. Graphical user input: This should always be satinized by code handling
///    the user input. Graphical users should never be expected to enter "valid"
///    input satisfying some preconditions.
///
/// # Why are we using exceptions for logic errors?
///
/// There is a commonly advocated guideline that using exceptions in C++ should
/// only be for runtime errors (e.g., out of memory, file permission issues,
/// etc.), and that using assert() instead should be preferred for logic errors
/// (e.g., calling mySqrt(x) where x is negative, dereferencing a null handle,
/// attempting to make a non-sensical topological operation, etc.). The
/// rationale is that these errors are in fact bugs in the code (while runtime
/// errors are not), and these should be catched and fixed in unit tests or
/// beta tests. If not fixed, it is anyway unclear how we could recover from a
/// bug we didn't know about, and thus we might as well terminate the program
/// and avoid potentially corrupting more data. However, this guideline is
/// quite ill-advised in our case, because the low-level API is directly
/// available to users of the software via the Python console. Therefore, it is
/// impossible for C++ developers to fix logic errors coming from Python user
/// code, and it is a much better behavior to report the error to them as a
/// Python exception rather than crashing the application. Only them can decide
/// whether they prefer to fix the root cause of the error in their code, or
/// use a try/except/finally as a quick fix to get the work done. In fact, in
/// Python, it is much more common to use exceptions for logic errors as well
/// as runtime errors, exactly because the distinction between developer and
/// user is not always clear, and also because a script is often written for a
/// one-time task with no unit testing. Therefore, we prefer to use C++
/// exceptions in a consistent way with how they are used in Python, which
/// makes creating bindings more straightforward.
///
/// In addition, it is quite pretentious to believe that we can catch and fix
/// all possible logic errors before shipping, and therefore throwing an
/// exception is much better: we can for example decide to catch it and send a
/// bug report automatically, so that we are aware of the problem and fix it in
/// future versions. One may argue that this is possible without using
/// exceptions: when the low-level library detect the error, it could itself
/// send the bug report just before calling abort(). However, this is in fact
/// not a good idea: first, there would be dependency issues (e.g., why should
/// vgc::dom depend on an internet protocol?), and second, the bug report
/// should only be sent if all the upstream code is VGC code and not user code,
/// and if the user agrees to do so, etc... which is information that the
/// low-level library detecting the error doesn't know about. And imagine a
/// third-party using the library for something entirely different: we
/// definitly do not want to receive bug reports for logic errors coming from
/// their code, nor they which to use a library doing so, for many obvious
/// reasons! In a nutshell, throwing an exception is infinitly more flexible
/// and useful than crashing the app, even if it is a logic error. It defers
/// the decision of how to handle the exception to upstream code, which
/// generally has way more information to know what is the best behavior. In
/// some cases, terminating the application might indeed be the best decision,
/// but in many other cases it is not, and more importantly it shouldn't be the
/// responsibility of the low-level library to decide.
///
class VGC_CORE_API LogicError : public std::logic_error {
public:
    explicit LogicError(const std::string& what) : std::logic_error(what) {}
    explicit LogicError(const char* what) : std::logic_error(what) {}
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_EXCEPTION_H
