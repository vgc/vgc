// Copyright 2020 The VGC Developers
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

#ifndef VGC_CORE_PERFORMANCELOG_H
#define VGC_CORE_PERFORMANCELOG_H

#include <string>
#include <vector>
#include <vgc/core/api.h>
#include <vgc/core/object.h>
#include <vgc/core/stopwatch.h>

namespace vgc {
namespace core {

VGC_CORE_DECLARE_PTRS(PerformanceLog);
VGC_CORE_DECLARE_PTRS(PerformanceLogParams);

/// \class vgc::core::PerformanceLog
/// \brief Measures and stores consecutive performance timings.
///
/// This class allows to track performance of a given task by measuring and
/// storing consecutive performance timings. While it is primarily designed as
/// a development and debugging tool, it can also be useful for end users to
/// understand what types of actions and objects affect performance, and use
/// this knowledge to optimize their scene.
///
/// Note: for now, only the last measure is stored; storing consecutive
/// measures will be implemented in the near future.
///
/// First Example
/// -------------
///
/// \code
/// PerformanceLogSharedPtr log = PerformanceLog::create("Foo");
///
/// log->start();
/// foo();
/// log->end();
///
/// std::cout << log->name() << ": " << log->lastTime() << "s" << std::end;
///
/// // Possible output:
/// // Foo: 1.2135468612s
/// \endcode
///
/// To start tracking performance, simply instanciate a PerformanceLog by
/// calling PerformanceLog::create(), then surround the piece of code you wish
/// to measure with start() and stop().
///
/// Times are stored as double-precision floating points expressed in seconds.
/// You can use vgc::core::secondsToString() from vgc/core/stringutil.h if you
/// need to pretty-print them.
///
/// Tree Hierarchy
/// --------------
///
/// A PerformanceLog may have child PerformanceLogs. For example, below is the
/// type of data that can be stored with a hierarchy made of 3 logs, each
/// storing 4 time samples. The logs "Tesselate" and "Draw" are children of the
/// log "Render":
///
/// \verbatim
/// Render:      [0.008s, 0.007s, 0.008s, 0.009s]
///   Tesselate: [0.006s, 0.005s, 0.005s, 0.006s]
///   Draw:      [0.002s, 0.002s, 0.003s, 0.003s]
/// \endverbatim
///
/// Child logs must be created using the createChild() method and cannot be
/// added as children afterwards. Child logs are not owned by their parent log,
/// but by the caller of createChild(). If the shared pointer returned by
/// createChild() goes out of scope, then the log is removed from the hierarchy
/// and destroyed. When a log is destroyed, its children are removed from the
/// hierarchy but not destroyed, that is, they become parent-less but are still
/// valid logs.
///
/// Example:
///
/// \code
/// class A {
///     PerformanceLogSharedPtr fooLog_;
///     PerformanceLogSharedPtr barLog_;
///     PerformanceLogSharedPtr bazLog_;
///
/// public:
///     A() :
///         fooLog_(PerformanceLog::create("Foo");
///         barLog_(fooLog_->createChild("Bar")),
///         bazLog_(fooLog_->createChild("Baz")) { }
///
///     void foo() {
///         fooLog_->start();
///         barLog_->start(); bar(); barLog_->stop();
///         bazLog_->start(); baz(); bazLog_->stop();
///         fooLog_->stop();
///     }
///     void bar();
///     void baz();
///
///     void printLogs() {
///         std::cout << fooLog_->name() << ": " << fooLog_->lastTime() << "s" << std::end;
///         std::cout << "  " << barLog_->name() << ": " << barLog_->lastTime() << "s" << std::end;
///         std::cout << "  " << bazLog_->name() << ": " << bazLog_->lastTime() << "s" << std::end;
///
///         // Example output:
///         // Foo: 3.2s
///         //   Bar: 2.8s
///         //   Baz: 0.4s
///     }
/// };
/// \endcode
///
/// This hierarchy is only used for organization and visualization purposes,
/// and does not affect in any way the behavior of individual logs. In
/// particular, time measurements of children are not automatically summed up
/// to provide a measure for their parent. In fact, children logs may not
/// necessarilly represent "subtasks" of their parent, but may be merely
/// related tasks which are conveniently grouped under a common parent. For
/// example a "Topology Tools" log may have children logs such as "Create
/// Vertex", "Delete Vertex", "Glue Edges", etc.
///
/// PerformanceLogTask
/// ------------------
///
/// It is possible to enable/disable performance logging from one or multiple logs
/// using the convenient class PerformanceLogTask.
///
/// Example:
///
/// \code
/// class A {
///     PerformanceLogTask fooTask_;
///     PerformanceLogTask barTask_;
///     PerformanceLogTask bazTask_;
///
/// public:
///     A() :
///         fooTask_("Foo"),
///         barTask_("Bar"),
///         bazTask_("Baz") { }
///
///     void startLoggingUnder(PerformanceLog* parent) {
///         PerformanceLog* fooLog = fooTask_.startLoggingUnder(parent);
///         barTask_.startLoggingUnder(fooLog);
///         bazTask_.startLoggingUnder(fooLog);
///     }
///
///     void stopLoggingUnder(PerformanceLog* parent) {
///         PerformanceLogSharedPtr fooLog = fooTask_.stopLoggingUnder(parent);
///         barTask_.stopLoggingUnder(fooLog.get());
///         bazTask_.stopLoggingUnder(fooLog.get());
///     }
///
///     void foo() {
///         fooTask_.start();
///         barTask_.start(); bar(); barTask_.stop();
///         bazTask_.start(); baz(); bazTask_.stop();
///         fooTask_.stop();
///     }
///     void bar();
///     void baz();
/// };
///
/// int main {
///     A a;
///
///     PerformanceLogSharedPtr log1 = PerformanceLog::create();
///     a.startLoggingUnder(log1.get());
///
///     PerformanceLogSharedPtr log2 = PerformanceLog::create();
///     a.startLoggingUnder(log2.get());
///
///     a.foo()
///
///     // [...] (example: print content of log1 and log2)
///
///     a.stopLoggingUnder(log1.get());
///     a.stopLoggingUnder(log2.get());
/// }
/// \endcode
///
class VGC_CORE_API PerformanceLog : public Object
{
private:
    VGC_CORE_OBJECT(PerformanceLog)

public:
    /// This is an implementation detail. Use create() instead.
    ///
    PerformanceLog(const ConstructorKey&, const std::string& name);

    /// Destroys the PerformanceLog, removing it from the hierarchy beforehand.
    ///
    ~PerformanceLog();

    /// Creates a PerformanceLog with the given \p name.
    ///
    static PerformanceLogSharedPtr create(const std::string& name = "");

    /// Creates a PerformanceLog with the given \p name, as a child the given
    /// \p parent log.
    ///
    /// \sa createChild()
    ///
    static PerformanceLogSharedPtr create(PerformanceLog* parent, const std::string& name = "");

    /// Creates a PerformanceLog with the given \p name, as a child the given
    /// \p parent log. This is equivalent to PerformanceLog::create(parent, name).
    ///
    /// \sa create()
    ///
    PerformanceLogSharedPtr createChild(const std::string& name = "");

    /// Returns the parameters of this log. These parameters are shared with
    /// all the children and ancestors of this log.
    ///
    PerformanceLogParams* params() const;

    /// Returns the name of this log.
    ///
    std::string name() const;

    /// Starts measuring time.
    ///
    void start();

    /// Completes the time measurement and stores it into this log.
    ///
    void stop();

    /// Manually writes a time measurement into this log.
    ///
    void log(double time);

    /// Returns the last logged time in this entry.
    ///
    double lastTime() const;

    /// Returns the parent of log entry, if any.
    ///
    PerformanceLog* parent() const;

    /// Returns the first child of this log, if any.
    ///
    PerformanceLog* firstChild() const;

    /// Returns the last child of this log, if any.
    ///
    PerformanceLog* lastChild() const;

    /// Returns the previous sibling of this log, if any.
    ///
    PerformanceLog* previousSibling() const;

    /// Returns the next sibling of this log, if any.
    ///
    PerformanceLog* nextSibling() const;

private:
    PerformanceLogParamsSharedPtr params_;
    std::string name_;
    double time_;

    PerformanceLog* parent_;
    PerformanceLog* firstChild_;
    PerformanceLog* lastChild_;
    PerformanceLog* previousSibling_;
    PerformanceLog* nextSibling_;

    Stopwatch stopwatch_;
};

/// \class vgc::core::PerformanceLogParams
/// \brief Stores the parameters of a PerformanceLog.
///
/// This class is currently empty but will be completed with useful parameters
/// in the future, such as the maximum number of time samples to store in a
/// log.
///
class VGC_CORE_API PerformanceLogParams : public Object
{
private:
    VGC_CORE_OBJECT(PerformanceLogParams)
    friend class PerformanceLog;

public:
    /// This is an implementation detail. Use create() instead.
    ///
    PerformanceLogParams(const ConstructorKey&);

    /// Creates a new PerformanceLogParams.
    ///
    static PerformanceLogParamsSharedPtr create();
};

/// \class vgc::core::PerformanceLogTask
/// \brief Creates and manages performance log entries for a given task.
///
/// See the documentation of PerformanceLog for details.
///
class VGC_CORE_API PerformanceLogTask
{
public:
    /// Creates a PerformanceLogTask with the given \p name.
    ///
    PerformanceLogTask(const std::string& name);

    /// Returns the name of this log task.
    ///
    std::string name() const;

    /// Creates and manages a new PerformanceLog called name() as a child of
    /// the given \p parent. Returns the newly created log.
    ///
    /// If this task is already managing a log whose parent is the given \p
    /// parent, then this function is a no-op and returns the already existing
    /// log.
    ///
    PerformanceLog* startLoggingUnder(PerformanceLog* parent);

    /// Releases ownership of the currently managed log whose parent is the given \p
    /// parent, if any.
    ///
    PerformanceLogSharedPtr stopLoggingUnder(PerformanceLog* parent);

    /// Returns the currently managed log whose parent is the given \p parent,
    /// if any.
    ///
    PerformanceLog* getLogUnder(PerformanceLog* parent) const;

    /// Equivalent to calling log->start() on all the managed logs.
    ///
    void start();

    /// Equivalent to calling log->stop() on all the managed logs.
    ///
    void stop();

private:
    std::string name_;
    std::vector<PerformanceLogSharedPtr> logs_;

    Stopwatch stopwatch_;
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_PERFORMANCELOG_H
