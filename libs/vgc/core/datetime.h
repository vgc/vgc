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

#ifndef VGC_CORE_DATETIME_H
#define VGC_CORE_DATETIME_H

#include <chrono>
#include <ctime>

#include <fmt/chrono.h>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/format.h>

namespace vgc::core {

/// \enum vgc::core::TimeMode
/// \brief Indicates whether a time is meant in local time, UTC, or a given time zone.
///
/// Note: currently, time zones are not supported.
///
// TODO: Add TimeZone/OffsetFromUtc
//
enum class TimeMode : UInt8 {
    Local,
    Utc
};

/// \class vgc::core::DateTime
/// \brief Stores a date and time, in local time, UTC, or a given time zone.
///
/// Example usage:
///
/// ```cpp
/// vgc::core::DateTime utc = vgc::core::DateTime::now();
/// vgc::core::DateTime local = utc.toLocalTime();
/// std::string utcString = vgc::core::format("{:%Y-%m-%d %H:%M:%S}", utc);
/// std::string localString = vgc::core::format("{:%Y-%m-%d %H:%M:%S}", local);
///
/// // Possible output:
/// // utcString   = "2023-03-24 18:23:42"
/// // localString = "2023-03-24 19:23:42"
/// ```
///
/// Note: this class is currently very minimal, but should be improved in the
/// future, once all C++ compilers supported by VGC implement C++20 calendar
/// and time zones features (utc_clock, time_zone, etc.)
///
// Track GCC progress on C++20 utc_clock support (target milestone: GCC 13):
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104167
//
// XXX The current implementation assumes that the system time zone doesn't
// change. In order to support changing system time zones, then when the mode
// is `Local`, we should store something similar to std::tm rather than
// std::system_time, and perform the conversion between the two based on the
// current system time zone when switching from `Local` to `Utc`.
//
class VGC_CORE_API DateTime {
public:
    /// Constructs a `DateTime` representing `1970-01-01 00:00:00` in UTC.
    ///
    DateTime()
        : t_()
        , mode_(TimeMode::Utc) {
    }

    /// Returns a `DateTime` representing the current date and time in UTC.
    ///
    static DateTime now();

    /// Returns whether this `DateTime` represents local time, UTC, or a given
    /// time zone.
    ///
    /// Note that if the time mode is `Local`, then this `DateTime` does not
    /// represent a fixed point in time. For example if you choose to send a
    /// notification to all your users on 2023-03-24 at 10:00 local time, then
    /// users in Europe will receive the notification a few hours before users
    /// in North America, but all users will receive it at 10:00 in their local
    /// time.
    ///
    TimeMode mode() const {
        return mode_;
    }

    /// Returns this `DateTime` as a local time, representing the same point in
    /// time for users whose time zone is the same as the current system time
    /// zone as of calling this function.
    ///
    DateTime toLocalTime() {
        return DateTime(t_, TimeMode::Local);
    }

    /// Returns the point in time represented by this `DateTime` as an
    /// `std::chrono::system_clock::time_point`.
    ///
    /// If the `mode()` is `Local`, then the current system time zone is used
    /// for the conversion.
    ///
    std::chrono::system_clock::time_point toStdSystemTime() {
        return t_;
    }

private:
    // A time point that ignores leap seconds.
    //
    // Note that computing a difference between two SystemTime does not result
    // in a duration in "real-world seconds":
    //
    //   std::string s1 = "2016-12-31 12:00:00";
    //   std::string s2 = "2017-01-01 12:00:00";
    //   std::streamstring ss1(s1);
    //   std::streamstring ss2(s2);
    //   SystemTime st1;
    //   SystemTime st2;
    //   std::chrono::from_stream(ss1, st1);
    //   std::chrono::from_stream(ss2, st2);
    //   auto seconds = std::chrono::duration_cast<std::chrono::seconds>(st2 - st1);
    //
    //   // => seconds is 864000 (=60*60*24), despite the fact there there are
    //         actually 864001 real seconds that passed between these two time
    //         points, due to a leap seconds added on 2016-12-31 at midnight
    //
    // Conversely, this makes it easier to add "calendar days" to the time
    // point: just add 864000 seconds to get to the next UTC day at the same
    // UTC time.
    //
    std::chrono::system_clock::time_point t_;

    // Whether the formatter and/or functions such as year()/month()/hour()/etc.
    // should interpret the stored time as UTC or Local Time.
    //
    // In the future, this should be more generic and use time zones.
    //
    // Note that for now, we don't store the year_/month_/hour_/etc. as data
    // members, so this is purely just an input to the formatter, and calling
    // toLocalTime() is basically a no-op appart from changing mode_. In the
    // future, we may change this design and instead of storing a SystemTime,
    // we would store a calendar representation similar to std::tm, in which
    // case changing the value of mode_ should also change the calendar
    // representation in order to represent the same point in time.
    //
    TimeMode mode_ = TimeMode::Utc;

    DateTime(std::chrono::system_clock::time_point t, TimeMode mode)
        : t_(t)
        , mode_(mode) {
    }
};

} // namespace vgc::core

template<>
struct fmt::formatter<vgc::core::DateTime> : fmt::formatter<std::tm> {
    template<typename FormatContext>
    auto format(vgc::core::DateTime dateTime, FormatContext& ctx) {
        std::chrono::system_clock::time_point t = dateTime.toStdSystemTime();
        std::tm calendarTime;
        switch (dateTime.mode()) {
        case vgc::core::TimeMode::Local:
            // Since fmt 10.0.0, localtime(time_point) is only defined if the compiler
            // supports C++20 time zones, see: https://github.com/fmtlib/fmt/pull/3230
            // Otherwise, we need to manually convert from `time_point` to `time_t`.
#if FMT_USE_LOCAL_TIME
            calendarTime = fmt::localtime(t);
#else
            calendarTime = fmt::localtime(std::chrono::system_clock::to_time_t(t));
#endif
            break;
        case vgc::core::TimeMode::Utc:
            calendarTime = fmt::gmtime(t);
            break;
        }
        return fmt::formatter<std::tm>::format(calendarTime, ctx);
    }
};

#endif // VGC_CORE_DATETIME_H
