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

#include <gtest/gtest.h>
#include <vgc/core/datetime.h>

using vgc::core::DateTime;
using vgc::core::TimeMode;

using vgc::core::format;

// struct tm {
//     int tm_sec;   // Seconds.      [0-60] (1 leap second)
//     int tm_min;   // Minutes.      [0-59]
//     int tm_hour;  // Hours.        [0-23]
//     int tm_mday;  // Day.          [1-31]
//     int tm_mon;   // Month.        [0-11]
//     int tm_year;  // Year - 1900.
//     int tm_wday;  // Day of week.  [0-6]
//     int tm_yday;  // Days in year. [0-365]
//     int tm_isdst; // DST.          [-1/0/1]
// };
//
std::string tmToString(std::tm* t) {
    return format(
        "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
        t->tm_year + 1900,
        t->tm_mon + 1,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec);
}

TEST(TestDateTime, DefaultConstructor) {
    DateTime epochTime;
    EXPECT_EQ(epochTime.mode(), TimeMode::Utc);
    EXPECT_EQ(format("{:%Y-%m-%d %H:%M:%S}", epochTime), "1970-01-01 00:00:00");
}

TEST(TestDateTime, Mode) {
    DateTime utc = DateTime::now();
    DateTime local = utc.toLocalTime();
    EXPECT_EQ(utc.mode(), TimeMode::Utc);
    EXPECT_EQ(local.mode(), TimeMode::Local);
}

TEST(TestDateTime, Format) {

    // Format current time using our DateTime class
    DateTime utc = DateTime::now();
    DateTime local = utc.toLocalTime();
    std::string utcString = format("{:%Y-%m-%d %H:%M:%S}", utc);
    std::string localString = format("{:%Y-%m-%d %H:%M:%S}", local);

    // Format current time using c-style date and time facilities
    using std::chrono::system_clock;
    system_clock::time_point timePoint = utc.toStdSystemTime();
    std::time_t time = system_clock::to_time_t(timePoint);
    std::string gmtimeString = tmToString(std::gmtime(&time));
    std::string localtimeString = tmToString(std::localtime(&time));

    // Check that they match
    EXPECT_EQ(utcString, gmtimeString);
    EXPECT_EQ(localString, localtimeString);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
