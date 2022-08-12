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

#include <vgc/core/performancelog.h>

#include <vgc/core/algorithm.h>

namespace vgc::core {

PerformanceLog::PerformanceLog(PerformanceLog* parent, const std::string& name)
    : Object()
    , params_(nullptr)
    , name_(name)
    , time_(0.0) {

    if (parent) {
        appendObjectToParent_(parent);
    }
}

/* static */
PerformanceLogPtr PerformanceLog::create(const std::string& name) {
    return PerformanceLogPtr(new PerformanceLog(nullptr, name));
}

PerformanceLog* PerformanceLog::createChild(const std::string& name) {
    return new PerformanceLog(this, name);
}

PerformanceLogParams* PerformanceLog::params() const {
    return params_.get();
}

std::string PerformanceLog::name() const {
    return name_;
}

void PerformanceLog::start() {
    stopwatch_.restart();
}

void PerformanceLog::stop() {
    log(stopwatch_.elapsed());
}

void PerformanceLog::log(double time) {
    time_ = time;
}

double PerformanceLog::lastTime() const {
    return time_;
}

PerformanceLogParams::PerformanceLogParams()
    : Object() {
}

/* static */
PerformanceLogParamsPtr PerformanceLogParams::create() {
    return PerformanceLogParamsPtr(new PerformanceLogParams());
}

PerformanceLogTask::PerformanceLogTask(const std::string& name)
    : name_(name) {
}

std::string PerformanceLogTask::name() const {
    return name_;
}

PerformanceLog* PerformanceLogTask::startLoggingUnder(PerformanceLog* parent) {
    PerformanceLog* log = getLogUnder(parent);
    if (!log) {
        logs_.push_back(parent->createChild(name_));
        log = logs_.back().get();
    }
    return log;
}

PerformanceLogPtr PerformanceLogTask::stopLoggingUnder(PerformanceLog* parent) {
    using std::swap;

    PerformanceLogPtr res;
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

PerformanceLog* PerformanceLogTask::getLogUnder(PerformanceLog* parent) const {
    for (const auto& log : logs_) {
        if (log->parent() == parent) {
            return log.get();
        }
    }
    return nullptr;
}

void PerformanceLogTask::start() {
    stopwatch_.restart();
}

void PerformanceLogTask::stop() {
    double time = stopwatch_.elapsed();
    for (const auto& log : logs_) {
        log->log(time);
    }
}

} // namespace vgc::core

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
