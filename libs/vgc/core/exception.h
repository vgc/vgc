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

/// \class vgc::core::Exception
/// \brief Base class for all VGC exceptions
///
/// VGC exceptions are used to report logic errors that are a consequence of
/// incorrect usage of downstream API. For example, a given method may choose
/// to throw an exception if its preconditions are not met.
///
/// The recommended programming practice when calling a method with
/// preconditions is to ensure that the preconditions are met before calling
/// the method, then call the method without checking for exceptions. Normally,
/// you should never write a try/catch block yourself to catch VGC exceptions,
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
class VGC_CORE_API Exception : public std::logic_error {
public:
    explicit Exception(const std::string& what) : std::logic_error(what) {}
    explicit Exception(const char* what) : std::logic_error(what) {}
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_EXCEPTION_H
