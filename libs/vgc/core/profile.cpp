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
    vgc::Int correspondingIndex;

    ProfilerEntry(Time t, const char* name, vgc::Int i)
        : timestamp(t), name(name), correspondingIndex(i) {}
};

std::string generateThreadName() {
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
    std::thread::id id;
}

struct ProfilerGlobals {
    vgc::core::Array<ProfilerEntry> entries;
    std::string threadName;
    std::string outputBuffer;

    ProfilerGlobals() {
        entries.reserve(1000);
        threadName = generateThreadName();
        outputBuffer.reserve(1000);
    }
};

thread_local ProfilerGlobals g;

void printDuration(vgc::Int64 ns) {
    std::string& out = g.outputBuffer;
    vgc::Int64 us = ns / 1000; ns -= us * 1000;
    vgc::Int64 ms = us / 1000; us -= ms * 1000;
    vgc::Int64  s = ms / 1000; ms -=  s * 1000;
    auto out_ = std::back_inserter(out);
    if      (s > 0)  { fmt::format_to(out_, "{:>6}s {:0>3}ms {:0>3}us {:0>3}ns", s, ms, us, ns); }
    else if (ms > 0) { fmt::format_to(out_, "{:>6}  {:>3}ms {:0>3}us {:0>3}ns", ' ', ms, us, ns); }
    else if (us > 0) { fmt::format_to(out_, "{:>6}  {:>3}   {:>3}us {:0>3}ns", ' ', ' ', us, ns); }
    else             { fmt::format_to(out_, "{:>6}  {:>3}   {:>3}   {:>3}ns", ' ', ' ', ' ', ns); }
}

void printDurations() {
    std::string& out = g.outputBuffer;
    vgc::Int i = 0;
    vgc::Int indent = 0;
    for (const ProfilerEntry& entry : g.entries) {
        if (i < entry.correspondingIndex) {
            using Nanoseconds = std::chrono::duration<vgc::Int64, std::nano>;
            auto d = std::chrono::duration_cast<Nanoseconds>(
                        g.entries[entry.correspondingIndex].timestamp -
                        entry.timestamp);
            printDuration(d.count());
            for (vgc::Int j = 0; j < 4 + 2 * indent; ++j) {
                out.push_back(' ');
            }
            out.append(entry.name);
            out.push_back('\n');
            ++indent;
        }
        else {
            --indent;
        }
        ++i;
    }
    VGC_DEBUG_TMP("{}", out.data());
}

void printThreadName() {
    std::string& out = g.outputBuffer;
    out.append("[Thread ");
    out.append(g.threadName);
    out.append("]\n");
}

void printTimestamps() {
    std::string& out = g.outputBuffer;
    vgc::Int i = 0;
    vgc::Int indent = 0;
    for (const ProfilerEntry& entry : g.entries) {
        using Nanoseconds = std::chrono::duration<vgc::Int64, std::nano>;
        auto d = std::chrono::duration_cast<Nanoseconds>(
                    entry.timestamp -
                    g.entries.first().timestamp);
        printDuration(d.count());
        if (i < entry.correspondingIndex) {
            for (vgc::Int j = 0; j < 4 + 2 * indent; ++j) { out.push_back(' '); }
            out.append("BEGIN ");
            ++indent;
        }
        else {
            --indent;
            for (vgc::Int j = 0; j < 4 + 2 * indent; ++j) { out.push_back(' '); }
            out.append("END   ");
        }
        out.append(entry.name);
        out.push_back('\n');
        ++i;
    }
    VGC_DEBUG_TMP("{}", out.data());
}

} // namespace

namespace vgc::core::internal {

ScopeProfiler::ScopeProfiler(const char* name)
    : name_(name), correspondingIndex_(g.entries.length())
{
    g.entries.emplaceLast(Clock::now(), name_, -1);
}

/*
ScopeProfiler::~ScopeProfiler()
{
    profilerEntries.emplaceLast(Clock::now(), name_, correspondingIndex_);
    Int n = profilerEntries.length();
    profilerEntries[correspondingIndex_].correspondingIndex = n - 1;

    // Flush to VGC_DEBUG_TMP if we're the root scope
    if (correspondingIndex_ == 0) {
        out.clear();
        out.append("[Thread ");
        out.append(threadName);
        out.append("]\n");
        Int i = 0;
        Int indent = 0;
        for (const ProfilerEntry& entry : profilerEntries) {
            if (i < entry.correspondingIndex) {
                using Nanoseconds = std::chrono::duration<Int64, std::nano>;
                auto d = std::chrono::duration_cast<Nanoseconds>(
                            profilerEntries[entry.correspondingIndex].timestamp -
                            entry.timestamp);
                vgc::Int64 ns = d.count();
                vgc::Int64 us = ns / 1000; ns -= us * 1000;
                vgc::Int64 ms = us / 1000; us -= ms * 1000;
                vgc::Int64  s = ms / 1000; ms -=  s * 1000;
                auto out_ = std::back_inserter(out);
                if      (s > 0)  { fmt::format_to(out_, "{:>6}s {:0>3}ms {:0>3}us {:0>3}ns", s, ms, us, ns); }
                else if (ms > 0) { fmt::format_to(out_, "{:>6}  {:>3}ms {:0>3}us {:0>3}ns", ' ', ms, us, ns); }
                else if (us > 0) { fmt::format_to(out_, "{:>6}  {:>3}   {:>3}us {:0>3}ns", ' ', ' ', us, ns); }
                else             { fmt::format_to(out_, "{:>6}  {:>3}   {:>3}   {:>3}ns", ' ', ' ', ' ', ns); }
                for (Int j = 0; j < 4 + 2 * indent; ++j) {
                    out.push_back(' ');
                }
                out.append(entry.name);
                out.push_back('\n');
                ++indent;
            }
            else {
                --indent;
            }
            ++i;
        }
        VGC_DEBUG_TMP("{}", out.data());
    }
}
*/

ScopeProfiler::~ScopeProfiler()
{
    g.entries.emplaceLast(Clock::now(), name_, correspondingIndex_);
    Int n = g.entries.length();
    g.entries[correspondingIndex_].correspondingIndex = n - 1;

    // Flush to VGC_DEBUG_TMP if we're the root scope
    if (correspondingIndex_ == 0) {
        g.outputBuffer.clear();
        printThreadName();
        //printTimestamps();
        printDurations();
        g.entries.clear();
    }
}

} // namespace vgc::core::internal
