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

#include <vgc/core/logging.h>

#include <iostream>

#ifdef VGC_CORE_OS_WINDOWS
#    include <Windows.h>
#endif

namespace vgc::core {

namespace {

void appendStringToLogMessage(fmt::memory_buffer& message, const char* string) {
    for (const char* it = string; *it != '\0'; ++it) {
        message.push_back(*it);
    }
}

} // namespace

namespace detail {

void appendPreambleToLogMessage(
    fmt::memory_buffer& message,
    const StringId& categoryName,
    LogLevel level) {

    if (!(categoryName == LogTmp::instance()->name())) {
        appendStringToLogMessage(message, categoryName.string().c_str());
        appendStringToLogMessage(message, ": ");
    }
    switch (level) {
    case LogLevel::Critical:
        appendStringToLogMessage(message, "Critical: ");
        break;
    case LogLevel::Error:
        appendStringToLogMessage(message, "Error: ");
        break;
    case LogLevel::Warning:
        appendStringToLogMessage(message, "Warning: ");
        break;
    default:
        break;
    }
}

void printLogMessageToStderr(fmt::memory_buffer& message) {
    message.push_back('\n');
    message.push_back('\0');
#ifdef VGC_CORE_OS_WINDOWS
    OutputDebugStringA(message.data());
    std::fflush(stderr);
#else
    std::fputs(message.data(), stderr);
    std::fflush(stderr);
#endif
}

} // namespace detail

LogCategoryRegistry::~LogCategoryRegistry() {
    for (const auto& it : map_) {
        LogCategoryBase* category = it.second;
        delete category;
    }
}

LogCategoryRegistry* LogCategoryRegistry::instance() {
    static std::unique_ptr<LogCategoryRegistry> instance_;
    if (!instance_) {
        instance_ = std::make_unique<LogCategoryRegistry>(ConstructorKey{});
    }
    return instance_.get();
}

VGC_DEFINE_LOG_CATEGORY(LogTmp, "tmp")

} // namespace vgc::core
