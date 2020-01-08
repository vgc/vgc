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

#include <vgc/core/performancelog.h>

#include <vgc/core/algorithm.h>

namespace vgc {
namespace core {

PerformanceLog::PerformanceLog(
        const ConstructorKey&,
        const std::string& name) :

    Object(core::Object::ConstructorKey()),

    params_(nullptr),
    name_(name),
    time_(0.0),

    parent_(nullptr),
    firstChild_(nullptr),
    lastChild_(nullptr),
    previousSibling_(nullptr),
    nextSibling_(nullptr)
{

}

PerformanceLog::~PerformanceLog()
{
    // Detach all children
    PerformanceLog* child = firstChild();
    while(child) {
        PerformanceLog* nextSibling = child->nextSibling_;

        child->parent_ = nullptr;
        child->previousSibling_ = nullptr;
        child->nextSibling_ = nullptr;

        child = nextSibling;
    }
    firstChild_ = nullptr; // for documentation
    lastChild_ = nullptr;  // for documentation

    // Detach from parent
    if (parent_) {
        PerformanceLog*& forwardPtr = previousSibling_ ?
                    previousSibling_->nextSibling_ :
                    parent_->firstChild_;
        PerformanceLog*& backwardPtr = nextSibling_ ?
                    nextSibling_->previousSibling_ :
                    parent_->lastChild_;

        forwardPtr = nextSibling_;
        backwardPtr = previousSibling_;

        previousSibling_ = nullptr; // for documentation
        nextSibling_ = nullptr;     // for documentation
        parent_ = nullptr;          // for documentation
    }
}

/* static */
PerformanceLogSharedPtr PerformanceLog::create(const std::string& name)
{
    auto res = std::make_shared<PerformanceLog>(ConstructorKey(), name);
    res->params_ = PerformanceLogParams::create();
    return res;
}

/* static */
PerformanceLogSharedPtr PerformanceLog::create(PerformanceLog* parent, const std::string& name)
{
    auto res = std::make_shared<PerformanceLog>(ConstructorKey(), name);
    res->params_ = PerformanceLogParams::create();

    // Set as last child of parent
    PerformanceLog* e2 = res.get();
    if (PerformanceLog* e1 = parent->lastChild()) {
        e1->nextSibling_ = e2;
        e2->previousSibling_ = e1;
    }
    else {
        parent->firstChild_ = e2;
    }
    parent->lastChild_ = e2;

    return res;
}

PerformanceLogSharedPtr PerformanceLog::createChild(const std::string& name)
{
    return PerformanceLog::create(this, name);
}

PerformanceLogParams* PerformanceLog::params() const
{
    return params_.get();
}

std::string PerformanceLog::name() const
{
    return name_;
}

void PerformanceLog::start()
{
    stopwatch_.restart();
}

void PerformanceLog::stop()
{
    log(stopwatch_.elapsed());
}

void PerformanceLog::log(double time)
{
    time_ = time;
}

double PerformanceLog::lastTime() const
{
    return time_;
}

PerformanceLog* PerformanceLog::parent() const
{
    return parent_;
}

PerformanceLog* PerformanceLog::firstChild() const
{
    return firstChild_;
}

PerformanceLog* PerformanceLog::lastChild() const
{
    return lastChild_;
}

PerformanceLog* PerformanceLog::previousSibling() const
{
    return previousSibling_;
}

PerformanceLog* PerformanceLog::nextSibling() const
{
    return nextSibling_;
}

PerformanceLogParams::PerformanceLogParams(
        const ConstructorKey&) :

    Object(core::Object::ConstructorKey())
{

}

/* static */
PerformanceLogParamsSharedPtr PerformanceLogParams::create()
{
    return std::make_shared<PerformanceLogParams>(ConstructorKey());
}

PerformanceLogTask::PerformanceLogTask(const std::string& name) :
    name_(name)
{

}

std::string PerformanceLogTask::name() const
{
    return name_;
}

PerformanceLog* PerformanceLogTask::startLoggingUnder(PerformanceLog* parent)
{
    PerformanceLog* log = getLogUnder(parent);
    if (!log) {
        logs_.push_back(parent->createChild(name_));
        log = logs_.back().get();
    }
    return log;
}

PerformanceLogSharedPtr PerformanceLogTask::stopLoggingUnder(PerformanceLog* parent)
{
    using std::swap;

    PerformanceLogSharedPtr res;
    for (size_t i = 0; i < logs_.size(); ++i) {
        PerformanceLog* log = logs_[i].get();
        if (log->parent() == parent) {
            swap(res, logs_[i]);
            swap(logs_[i], logs_.back());
            logs_.pop_back();
            break;
        }
    }
    return res;
}

PerformanceLog* PerformanceLogTask::getLogUnder(PerformanceLog* parent) const
{
    for (const auto& log : logs_) {
        if (log->parent() == parent) {
            return log.get();
        }
    }
    return nullptr;
}

void PerformanceLogTask::start()
{
    stopwatch_.restart();
}

void PerformanceLogTask::stop()
{
    double time = stopwatch_.elapsed();
    for (const auto& log : logs_) {
        log->log(time);
    }
}

} // namespace core
} // namespace vgc

// Ideas for future work:
//
// 1. Instead of just start/stop logging, we could also have "pause". This
//    would not release ownership of the log, but instead just move it
//    from an "activeLogs_" list to a "pausedLogs_" list.
//
// 2. Maybe PerformanceLogTask could also derive from Object and have child
//    tasks, this may allow to slightly simplify the implementation of
//    A::start/stop methods, along the lines of:
//
//     A() :
//         fooTask_(PerformanceLogTask::create("Foo")),
//         barTask_(fooTask_->createChild("Bar")),
//         bazTask_(fooTask_->createChild("Baz")) { }
//
//     void startLoggingUnder(PerformanceLog* parent) {
//         fooTask_->startLoggingUnder(parent); // automatically recurse to children
//     }
//
//     void stopLoggingUnder(PerformanceLog* parent) {
//         fooTask_->stopLoggingUnder(parent); // automatically recurse to children
//     }
//
//    Unfortunately, then it becomes unclear whether A::startLoggingUnder()
//    should take as argument a PerformanceLog* or a performanceLogTask. Should
//    it take both? Maybe for the sake of conceptual simplicuty, it is better
//    to keep it as is.
//
