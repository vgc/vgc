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

#include <vgc/core/profile.h>

#include <chrono>
#include <sstream>
#include <thread>

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/logging.h>

namespace vgc::core::detail {

namespace {

using Clock = std::chrono::steady_clock;

// Stores the timestamp of either "entering a scope" or "leaving a scope", as
// well as the name of the scope. The correspondingIndex provides the mapping
// between "entering a scope" and "leaving a scope", and is temporarily equal
// to -1 while the scope is not closed yet.
//
struct ProfilerEntry {
    using Time = Clock::time_point;

    Time timestamp;
    const char* name;
    Int correspondingIndex;

    ProfilerEntry(Time t, const char* name, Int i)
        : timestamp(t)
        , name(name)
        , correspondingIndex(i) {
    }
};

std::string generateThreadName() {
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

struct ProfilerGlobals {
    Array<ProfilerEntry> entries;
    std::string threadName;
    std::string outputBuffer;

    ProfilerGlobals() {
        entries.reserve(1000);
        threadName = generateThreadName();
        outputBuffer.reserve(1000);
    }
};

thread_local ProfilerGlobals g;

void printDuration(std::string& out, Clock::duration d) {
    using Nanoseconds = std::chrono::duration<vgc::Int64, std::nano>;
    Int64 ns = std::chrono::duration_cast<Nanoseconds>(d).count();
    Int64 us = ns / 1000;
    ns -= us * 1000;
    Int64 ms = us / 1000;
    us -= ms * 1000;
    Int64 s = ms / 1000;
    ms -= s * 1000;
    auto out_ = std::back_inserter(out);
    if (s > 0) {
        fmt::format_to(out_, "{:>6}s {:0>3}ms {:0>3}us {:0>3}ns", s, ms, us, ns);
    }
    else if (ms > 0) {
        fmt::format_to(out_, "{:>6}  {:>3}ms {:0>3}us {:0>3}ns", ' ', ms, us, ns);
    }
    else if (us > 0) {
        fmt::format_to(out_, "{:>6}  {:>3}   {:>3}us {:0>3}ns", ' ', ' ', us, ns);
    }
    else {
        fmt::format_to(out_, "{:>6}  {:>3}   {:>3}   {:>3}ns", ' ', ' ', ' ', ns);
    }
}

void printIndent(std::string& out, Int indent) {
    for (Int j = 0; j < 4 + 2 * indent; ++j) {
        out.push_back(' ');
    }
}

void printThreadName(std::string& out, const std::string& threadName) {
    out.append("[Thread ");
    out.append(threadName);
    out.append("]\n");
}

[[maybe_unused]] void
printTimestamps(std::string& out, const Array<ProfilerEntry>& entries) {
    Int i = 0;
    Int indent = 0;
    for (const ProfilerEntry& entry : entries) {
        printDuration(out, entry.timestamp - entries.first().timestamp);
        if (i < entry.correspondingIndex) {
            printIndent(out, indent);
            out.append("BEGIN ");
            ++indent;
        }
        else {
            --indent;
            printIndent(out, indent);
            out.append("END   ");
        }
        out.append(entry.name);
        out.push_back('\n');
        ++i;
    }
}

[[maybe_unused]] void
printDurations(std::string& out, const Array<ProfilerEntry>& entries) {
    Int i = 0;
    Int indent = 0;
    for (const ProfilerEntry& entry : entries) {
        if (i < entry.correspondingIndex) {
            const ProfilerEntry& correspondingEntry = entries[entry.correspondingIndex];
            printDuration(out, correspondingEntry.timestamp - entry.timestamp);
            printIndent(out, indent);
            out.append(entry.name);
            out.push_back('\n');
            ++indent;
        }
        else {
            --indent;
        }
        ++i;
    }
}

} // namespace

ScopeProfiler::ScopeProfiler(const char* name)
    : name_(name)
    , correspondingIndex_(g.entries.length()) {

    g.entries.emplaceLast(Clock::now(), name_, -1);
}

ScopeProfiler::~ScopeProfiler() {
    constexpr bool timestampMode = false;

    Array<ProfilerEntry>& entries = g.entries;

    entries.emplaceLast(Clock::now(), name_, correspondingIndex_);
    entries[correspondingIndex_].correspondingIndex = entries.length() - 1;

    // Flush to VGC_DEBUG_TMP if we're the root scope
    if (correspondingIndex_ == 0) {
        std::string& out = g.outputBuffer;
        out.clear();
        printThreadName(out, g.threadName);
        if constexpr (timestampMode) {
            printTimestamps(out, entries);
        }
        else {
            printDurations(out, entries);
        }
        entries.clear();
        VGC_DEBUG_TMP("{}", out.data());
    }
}

} // namespace vgc::core::detail
