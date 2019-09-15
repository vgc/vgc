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

#ifndef VGC_CORE_EXCEPTIONS_H
#define VGC_CORE_EXCEPTIONS_H

#include <stdexcept>
#include <string>

#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// \class vgc::core::LogicError
/// \brief Base class for all logic errors
///
/// This exception is raised whenever there is a logic error detected by VGC,
/// either because of a bug in a VGC function, or because the preconditions of
/// a VGC function are not met.
///
/// How to handle VGC logic errors?
/// -------------------------------
///
/// The recommended programming practice when calling a VGC function with
/// preconditions is to ensure that the preconditions are met, then call the
/// function without checking for exceptions.
///
/// Normally, you should never write a try/catch block to catch logic errors:
/// instead, the code should be fixed so that the exception is never raised.
/// However, there are at least a few rare cases where you may want to catch
/// logic errors:
///
/// 1. At a very high-level in the program architecture (for example, in
///    main()), in order to log the error, tell the user that an error occured
///    and how to recover automatically saved data, and close the program as
///    cleanly as possible.
///
/// 2. At the interface between C++ and Python, to convert C++ exceptions into
///    Python exceptions. In most cases, this should be automatically done by
///    pybind11 by registering the exception using pybind11::register_exception.
///
/// 3. You are writing a one-off Python script never to be used again, you are
///    under a very tight deadline, and you are somehow able to get the job
///    done faster by catching and ignoring the exception instead of ensuring
///    that preconditions are met. This is really not recommended: if a logic
///    error is raised, this might mean that some internal state of the program
///    is now corrupted and could result in data loss.
///
/// 4. If catching the exception is a known workaround for a bug in VGC which
///    is still not fixed, and which you cannot fix yourself. Make sure to
///    report all suspected bugs to https://github.com/vgc/vgc/issues to ensure
///    that they will get fixed in future version of VGC.
///
/// When to raise VGC logic errors?
/// -------------------------------
///
/// If you are a developer of one of the VGC libraries, then if possible, it is
/// best to design your classes such that your public API has no preconditions,
/// which means that any possible input to your public functions is valid input
/// with documented behavior. In this case, you may choose to never raise logic
/// errors.
///
/// If a function in your public API has preconditions (for example, calling
/// pop() on an empty list makes no sense, therefore a precondition is that the
/// list is not empty), then you should check for these preconditions at the
/// beginning of the function, and raise a logic error if the preconditions are
/// not met. Note that it is sometimes a tricky design decision to decide
/// whether a given input is really non-sensical and should be documented as a
/// precondition, or whether it is simply a special case that should be treated
/// as valid input with documented behavior. For example, instead of raising an
/// exception, calling pop() on an empty list might return a
/// default-constructed element and keep the list empty. Use you best judgement
/// and take inspiration from existing libraries. For example, the built-in
/// Python list raises IndexError when calling pop() on an empty list,
/// therefore keeping the same behavior for similar functions is preferred.
/// Also, prefer using exceptions if you have good reasons to think that it may
/// help upstream clients detect bugs in their code that would otherwise be
/// unnoticed, or if it allows you to provide stronger post-conditions
/// (example: "this function never returns nullptr") which make upstream code
/// simpler and more readable. Avoid exceptions if it would be hard for users
/// to enforce these preconditions, or check whether they are met. Typically,
/// preconditions are for quite obviously non-sensical operations, such as
/// out-of-bound indices or well-known constraints (example: vgc::dom::Document
/// can only have one child Element, thus attempting to insert a second one
/// raises an exception).
///
/// Besides checking for preconditions, it might also be a good idea to check
/// internal invariants and postconditions if you are writing complex
/// algorithms, for ease of debugging and/or documentation. Raising logic
/// errors in case of failure is appropriate in this case, and is preferred
/// over the use of assert(). However, checking invariants and post-conditions
/// is a double-edged sword: be mindful of false-negative! You definitely do
/// not want to upset users by incorrectly raising an exception caused by a bug
/// in the code that checks the invariant! Also, be mindful of performance:
/// avoid checking for invariants if this is an expensive operation. Here are
/// some examples to provide some guidance:
///
/// \code
/// // Good: fast and simple checks for complex, non-obvious algorithms
/// Foo* foo = nullptr;
/// int i = -1;
/// for (...) {
///   // ... many lines of code, more bug-prone than average code
/// }
/// if (!foo) {
///     throw LogicError("Foo not found! This is not supposed to be possible!");
/// }
/// if (i < 0) {
///     throw LogicError("i is negative! This is not supposed to be possible!");
/// }
///
/// // Best avoided: complex and/or expensive checks for complex, non-obvious algorithms
/// int n = graph->numConnectedComponents();
/// for (const Node& node : findNodesToRemove_(graph)) {
///     connectNeighborNodes_(node);
///     removeNode_(node);
/// }
/// if (n != graph->numConnectedComponents()) {
///     throw LogicError("The number of connected components changed! This is not supposed to be possible!");
/// }
///
/// // Bad: fast and simple checks for simple algorithms
/// int j = i+1;
/// if (j < i) {
///     throw LogicError("j smaller than i! This is not supposed to be possible!");
/// }
///
/// // Very Bad: complex and/or expensive checks for simple algorithms
/// int j = i+1;
/// if (std::sqrt(j) > std::sqrt(i)) {
///     throw LogicError("sqrt(j) smaller than sqrt(i)! This is not supposed to be possible!");
/// }
/// \endcode
///
/// Finally, keep in mind that function input may come from various sources:
///
/// 1. C++ libraries: These are expected to know what they're doing. They should
///    ensure that preconditions are met before calling any function. Failure to
///    do so or to catch exceptions may result in a program crash. Such coding
///    error should normally be catched in unit tests or beta tests before
///    shipping stable versions.
///
/// 2. Python libraries: These are also expected to know what they're doing.
///    They should ensure that preconditions are met before calling any function.
///    Failure to do so results in a thrown C++ exception converted to a Python
///    exception which may stop the execution of your Python library. Such
///    coding error should normally be catched in unit tests or beta tests
///    before shipping stable versions.
///
/// 3. Python user scripts: These may be written by users with very little
///    programming experience, and such scripts might be executed for the first
///    time within an interactive session containing valuable artistic data. It
///    is still their responsability to ensure that preconditions are met
///    before calling any function, but failure to do so should not result in a
///    program crash. The exception is typically reported to the interactive
///    Python console, and the program should return to the most recent valid
///    state before the exception was thrown.
///
/// 4. Graphical user input: This should always be sanitized by code handling
///    the user input. Graphical users should never be expected to enter "valid"
///    input satisfying some preconditions.
///
/// Why are we using exceptions for logic errors?
/// ---------------------------------------------
///
/// There is a commonly advocated guideline that using exceptions in C++ should
/// only be for runtime errors (e.g., out of memory, file permission issues,
/// etc.), and that using assert() instead should be preferred for logic errors
/// (e.g., calling mySqrt(x) where x is negative, dereferencing a null handle,
/// attempting to make a non-sensical topological operation, etc.). The
/// rationale is that these errors are in fact bugs in the code (while runtime
/// errors are not), and these should be catched and fixed in unit tests or
/// beta tests. If the bug is not fixed, then advocates of using assert() argue
/// that it is anyway unclear how we could recover from a bug we didn't know
/// about, and thus we might as well terminate the program and avoid
/// potentially corrupting more data. However, this guideline is quite
/// ill-advised in our case, because the low-level API is directly available to
/// users of the software via the Python console. Therefore, it is impossible
/// for C++ developers to fix logic errors coming from Python user code, and it
/// is a much better behavior to report the error to them as a Python exception
/// rather than crashing the application. Only them can decide whether they
/// prefer to fix the root cause of the error in their code, or use a
/// try/except/finally as a quick fix to get the work done. In fact, in Python,
/// it is much more common to use exceptions for logic errors as well as
/// runtime errors, exactly because the distinction between developer and user
/// is not always clear when the language is interpreted and can be used in an
/// interactive console, and also because a script is often written for a
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
/// reasons! In a nutshell, throwing an exception is more flexible and useful
/// than crashing the app, even if it is a logic error. It defers the decision
/// of how to handle the exception to upstream code, which generally has way
/// more information to know what is the best course of action. In some cases,
/// terminating the application might indeed be the best choice, but in many
/// other cases it is not, and more importantly it shouldn't be the
/// responsibility of the low-level library to decide.
///
class VGC_CORE_API LogicError : public std::logic_error {
public:
    /// Constructs a LogicError with the given \p reason.
    ///
    explicit LogicError(const std::string& reason) : std::logic_error(reason) {}

    /// Constructs a LogicError with the given \p reason.
    ///
    explicit LogicError(const char* reason) : std::logic_error(reason) {}

    /// Destructs the LogicError.
    ///
    virtual ~LogicError();
};

/// \class vgc::core::RuntimeError
/// \brief Base class for all runtime errors
///
/// This exception is raised whenever there is a runtime error detected by VGC,
/// that is, whenever a function cannot complete its task due to events that
/// could not be easily predicted.
///
/// For example, the function vgc::dom::Document::open() raises
/// vgc::dom::ParseError if the input file is not a well-formed XML file. In
/// theory, passing a well-formed XML file could be seen as a precondition of
/// the function, but in practice, it would be hard and inefficient to ask
/// client code to check this beforehand, which is why it is not considered a
/// logic error.
///
/// How to handle VGC runtime errors?
/// ---------------------------------
///
/// Function calls that may throw a RuntimeError should typically be directly
/// surrounded by a try/catch block in C++ (or a try/except block in Python),
/// and the error should be handled immediately. In a nutshell, RuntimeError
/// exceptions should be treated as if the error was reported via an error
/// code, with the advantages that exceptions cannot be ignored, and that
/// exceptions are the preferred error handling mechanism in Python.
///
/// If client code can guarantee that the conditions leading to a runtime error
/// are not possible (for example, by calling open() on a file that we know is
/// a valid file), then it is not required to surround the call by a try block.
///
/// When to raise VGC runtime errors?
/// ---------------------------------
///
/// Whenever you are implementing a function which cannot complete its task for
/// a reason that could not have been easily predicted by client code, you may
/// raise a RuntimeError exception. For example, well-formedness of an XML file
/// cannot be easily predicted, therefore these types of syntax errors should
/// be reported as runtime errors. However, checking that an index is within
/// [0, size) can easily be done by client code, therefore these types of
/// out-of-range errors should be reported as logic errors.
///
/// If you are calling a function that may itself raise a RuntimeError, and you
/// are unable to handle this error yourself, then let it propagate upstream
/// and document that your function may raise this unhandled exception.
///
class VGC_CORE_API RuntimeError : public std::runtime_error {
public:
    /// Constructs a RuntimeError with the given \p reason.
    ///
    explicit RuntimeError(const std::string& reason) : std::runtime_error(reason) {}

    /// Constructs a RuntimeError with the given \p reason.
    ///
    explicit RuntimeError(const char* reason) : std::runtime_error(reason) {}

    /// Destructs the RuntimeError.
    ///
    virtual ~RuntimeError();
};

/// \class vgc::core::ParseError
/// \brief Raised when one of the read() function fails due to incorrect input.
///
/// This exception is raised whenever one of the read() function is called and
/// the input stream does not actually store characters that can be interpreted
/// as the string representation of the requested type.
///
/// Example:
///
/// \code
/// std::string str = "Hello, world!";
/// std::stringstream in(str);
/// double x;
/// read(in, x); // --> Raise ParseError!
/// \endcode
///
class VGC_CORE_API ParseError : public RuntimeError {
public:
    /// Constructs a ParseError with the given \p reason.
    ///
    ParseError(const std::string& reason) : RuntimeError(reason) {}

    /// Destructs the ParseError.
    ///
    ~ParseError();
};

/// \class vgc::core::RangeError
/// \brief Raised when the input of a conversion is outside the range of the
/// output type.
///
/// This exception is raised whenever a conversion fails because the input
/// (typically, a number) is outside the representable range of the output
/// type.
///
/// Example:
///
/// \code
/// std::string str = "1e400";
/// std::stringstream in(str);
/// double x;
/// read(in, x); // --> Raise RangeError! (1e400 is too big to be stored as a double)
/// \endcode
///
class VGC_CORE_API RangeError : public RuntimeError {
public:
    /// Constructs a ParseError with the given \p reason.
    ///
    RangeError(const std::string& reason) : RuntimeError(reason) {}

    /// Destructs the ParseError.
    ///
    ~RangeError();
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_EXCEPTIONS_H
