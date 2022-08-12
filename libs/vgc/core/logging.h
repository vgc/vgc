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

#ifndef VGC_CORE_LOGGING_H
#define VGC_CORE_LOGGING_H

#include <cstdio>   // fflush, fputc, fputs
#include <iostream> // ostream
#include <map>
#include <memory>
#include <mutex>

#include <vgc/core/api.h>
#include <vgc/core/format.h>
#include <vgc/core/os.h>
#include <vgc/core/preprocessor.h>
#include <vgc/core/stringid.h>

namespace vgc::core {

/// \enum vgc::core::LogLevel
///
/// An enumeration of the different levels of logging.
///
enum class LogLevel : uint8_t {
    Critical = 0,
    Error,
    Warning,
    Info,
    Debug
};

namespace detail {

VGC_CORE_API
void appendPreambleToLogMessage(
    fmt::memory_buffer& message,
    const StringId& categoryName,
    LogLevel level);

VGC_CORE_API
void printLogMessageToStderr(fmt::memory_buffer& message);

// Logs a message.
//
// For now, we simply print the message to `stderr`. A newline is automatically
// added, and the stream is flushed. On Windows, the message is sent to the
// debugger using `OutputDebugString()` instead of being printed to stderr.
//
// In the future, we would like to add the possibility to setup a message
// handler such that the messages are displayed to the user in different ways
// based on the log level.
//
template<typename... T>
VGC_FORCE_INLINE void
log(const StringId& categoryName,
    LogLevel level,
    fmt::format_string<T...> fmt,
    T&&... args) {

    fmt::memory_buffer message;
    appendPreambleToLogMessage(message, categoryName, level);
    fmt::format_to(std::back_inserter(message), fmt, args...);
    printLogMessageToStderr(message);
}

VGC_FORCE_INLINE void
log(const StringId& categoryName, LogLevel level, const char* cstring) {
    log(categoryName, level, "{}", cstring);
}

VGC_FORCE_INLINE void
log(const StringId& categoryName, LogLevel level, const std::string& string) {
    log(categoryName, level, "{}", string.c_str());
}

VGC_FORCE_INLINE void
log(const StringId& categoryName, LogLevel level, const StringId& stringId) {
    log(categoryName, level, "{}", stringId.string().c_str());
}

} // namespace detail

/// \class vgc::core::LogCategoryBase
/// \brief Abstract base class for all log categories
///
/// This class encapsulates run-time information required to log messages
/// related to a given log category. Note that you shouldn't instanciate
/// this directly, and instead use the macros:
///
/// ```cpp
/// VGC_DECLARE_LOG_CATEGORY(MyLogCat, Debug)       // in a *.h file
///
/// VGC_DEFINE_LOG_CATEGORY(MyLogCat, "my.log.cat") // in a *.cpp file
/// ```
///
class VGC_CORE_API LogCategoryBase {
public:
    virtual ~LogCategoryBase() = default;
    LogCategoryBase(const LogCategoryBase&) = delete;
    LogCategoryBase& operator=(const LogCategoryBase&) = delete;

    const StringId& name() const {
        return name_;
    }

protected:
    friend class LogCategoryRegistry;
    LogCategoryBase(const StringId& name)
        : name_(name) {
    }

private:
    StringId name_;
};

/// \class vgc::core::LogCategory
/// \brief Adds compile-time information to LogCategoryBase
///
template<LogLevel compileTimeEnabledLevels_>
class LogCategory : public LogCategoryBase {
protected:
    friend class LogCategoryRegistry;
    LogCategory(const StringId& name)
        : LogCategoryBase(name) {
    }

public:
    static constexpr LogLevel compileTimeEnabledLevels = compileTimeEnabledLevels_;
};

/// \class vgc::core::LogCategoryRegistry
/// \brief Stores all LogCategory instances
///
class VGC_CORE_API LogCategoryRegistry {
private:
    struct ConstructorKey {};

public:
    LogCategoryRegistry(ConstructorKey) {
    }
    ~LogCategoryRegistry();

    static LogCategoryRegistry* instance();

    template<typename Category>
    static Category* createCategory(const char* name) {
        LogCategoryRegistry* self = instance();
        std::lock_guard<std::mutex> lock(self->mutex_);
        StringId nameId(name);
        Category* instance = new Category(nameId);
        self->map_[nameId] = instance;
        return instance;
    }

private:
    std::mutex mutex_;
    std::map<StringId, LogCategoryBase*> map_;
};

} // namespace vgc::core

// Bits for VGC_DECLARE_LOG_CATEGORY

#define VGC_DECLARE_LOG_CATEGORY_2(ClassName, compileTimeEnabledLevels)                  \
    class ClassName VGC_DECLARE_LOG_CATEGORY_IMPL_(ClassName, compileTimeEnabledLevels)

#define VGC_DECLARE_LOG_CATEGORY_3(API, ClassName, compileTimeEnabledLevels)             \
    class VGC_PP_EXPAND(API) ClassName VGC_DECLARE_LOG_CATEGORY_IMPL_(                   \
        ClassName, compileTimeEnabledLevels)

#define VGC_DECLARE_LOG_CATEGORY_IMPL_(ClassName, compileTimeEnabledLevels)              \
    : public ::vgc::core::LogCategory<::vgc::core::LogLevel::compileTimeEnabledLevels> { \
        friend class ::vgc::core::LogCategoryRegistry;                                   \
        static ClassName* instance_;                                                     \
        ClassName(const ::vgc::core::StringId& name)                                     \
            : ::vgc::core::LogCategory<::vgc::core::LogLevel::compileTimeEnabledLevels>( \
                name) {                                                                  \
        }                                                                                \
                                                                                         \
    public:                                                                              \
        static ClassName* instance() {                                                   \
            return instance_;                                                            \
        }                                                                                \
    };

/// Declares a log category of type `ClassName` whose enabled levels can be can
/// be controlled at compile-time. For example, if we give `Warning` to the
/// macro, then only `Warning`, `Error`, and `Critical` log messages are
/// enabled at compile-time.
///
/// ```cpp
/// VGC_DECLARE_LOG_CATEGORY(MyCat, Debug)
///
/// // in a *.cpp file
/// VGC_DEFINE_LOG_CATEGORY(MyCat, "my.cat")
/// ```
///
/// It is also possible to add import/export API declarations if the category
/// is meant to be exported and used in other DLLs:
///
/// ```cpp
/// VGC_DECLARE_LOG_CATEGORY(VGC_CORE_API, LogCore, Debug)
///
/// // in a *.cpp file
/// VGC_DEFINE_LOG_CATEGORY(MyCat, "my.cat")
/// ```
///
///
#define VGC_DECLARE_LOG_CATEGORY(...)                                                    \
    VGC_PP_EXPAND(VGC_PP_OVERLOAD(VGC_DECLARE_LOG_CATEGORY_, __VA_ARGS__)(__VA_ARGS__))

/// Defines a log category. See `VGC_DECLARE_LOG_CATEGORY` for details.
///
#define VGC_DEFINE_LOG_CATEGORY(ClassName, name)                                         \
    ClassName* ClassName::instance_ =                                                    \
        ::vgc::core::LogCategoryRegistry::createCategory<ClassName>(name);

namespace vgc::core {

VGC_DECLARE_LOG_CATEGORY(VGC_CORE_API, LogTmp, Debug)

} // namespace vgc::core

/// Logs a message associated with the given `Category` at the given `level`.
///
/// ```cpp
/// VGC_LOG(MyCat, vgc::core::Error, "The answer is not {}", 42);
/// ```
///
/// \sa VGC_CRITICAL, VGC_ERROR, VGC_WARNING, VGC_INFO, VGC_DEBUG
///
#define VGC_LOG(Category, level, ...)                                                    \
    if constexpr (                                                                       \
        static_cast<uint8_t>(level)                                                      \
        <= static_cast<uint8_t>(Category::compileTimeEnabledLevels)) {                   \
        ::vgc::core::detail::log(Category::instance()->name(), level, __VA_ARGS__);      \
    }

/// Prints a critical error message.
///
/// This should be used to notify that a critical error happened causing a
/// crash of the application. This could for example be used if an exception
/// reaches the main() function.
///
/// These are kept in release builds, and could be presented to the user via a
/// popup dialog just before closing the application.
///
#define VGC_CRITICAL(Category, ...)                                                      \
    VGC_LOG(Category, ::vgc::core::LogLevel::Critical, __VA_ARGS__)

/// Prints an error.
///
/// This should be used to notify that something wrong was detected, which
/// forced the operation to be aborted.
///
/// For example, if an exception is catched while executing an interactive user
/// action, the code catching the exception may want to undo the changes until
/// the latest known stable application state, log the error with the exception
/// message, and potentially show a popup to the user message.
///
/// These are kept in release builds, and could be presented to the user in
/// various ways, for example in a widget with a list of all the errors that
/// have occured.
///
#define VGC_ERROR(Category, ...)                                                         \
    VGC_LOG(Category, ::vgc::core::LogLevel::Error, __VA_ARGS__)

/// Logs a warning.
///
/// This should be used to notify that something unusual was detected, but that
/// it was locally recoverable and didn't prevent the operation from
/// continuing.
///
/// For example, if the input of a function or the output of a computation is
/// supposed to be a floating point in the range [0.0, 1.0] but isn't, the
/// function may clamp it to the expected range, issue a warning, then continue
/// its normal operation.
///
/// Note that for out-of-range integer indices, null pointers, or similar
/// integer-based preconditions/invariants which are easier to prove correct
/// via code review and/or static analysis, then it is generally preferrable to
/// document these preconditions/invariants, and raise a `LogicError` if they
/// are not met.
///
/// Warnings are typically useful for preconditions or invariants which are both:
/// 1. recoverable in a reasonable way, and
/// 2. difficult to statically prove or too slow to check at runtime beforehand
///
/// These are kept in release builds, and could be presented to the user in
/// various ways, for example in a widget with a list of all the warnings that
/// have occured.
///
#define VGC_WARNING(Category, ...)                                                       \
    VGC_LOG(Category, ::vgc::core::LogLevel::Warning, __VA_ARGS__)

/// Logs an informational message.
///
/// This should be used for general information which could be potentially
/// useful to the user. For example, during the application startup, we could
/// log information about the machine or graphics engine configuration. It
/// could also be used to log important user events, such as "New document
/// created."
///
/// These are kept in release builds, and could be presented to the user in
/// various ways, for example printed to the console if any, or in a widget
/// with a list of all available informational messages.
///
#define VGC_INFO(Category, ...)                                                          \
    VGC_LOG(Category, ::vgc::core::LogLevel::Info, __VA_ARGS__)

/// Prints a debug message.
///
/// This should be used to display information which is useful for debugging,
/// but generally not useful for users.
///
/// These are meant to be:
/// - commited in the git repository
/// - compile-time enabled (unless too time-consuming)
/// - run-time disabled by default
///
/// The full mechanism isn't implemented yet, but the idea is that users should
/// be able to enable these debug messages at run-time via a "debug tools"
/// widget.
///
/// For debug messages which are temporary and not meant to be commited (i.e.,
/// short tests when trying to fix a bug), you can use VGC_DEBUG_TMP instead.
///
#define VGC_DEBUG(Category, ...)                                                         \
    VGC_LOG(Category, ::vgc::core::LogLevel::Debug, __VA_ARGS__)

/// A convenient alias for VGC_DEBUG(vgc::core::LogTmp, ...)
///
/// This is useful for debug messages which are temporary and not meant to be
/// commited (i.e., short tests when trying to fix a bug). The advantage of
/// using this macro instead of other printing mechanism is that we can easily
/// search for `VGC_DEBUG_TMP` after the debugging session in order to remove
/// them.
///
#define VGC_DEBUG_TMP(...) VGC_DEBUG(::vgc::core::LogTmp, __VA_ARGS__)

namespace vgc::core::detail {

template<typename T, VGC_REQUIRES(!std::is_pointer_v<T>)>
const T& debugExprCast(const T& x) {
    return x;
}

template<typename T, VGC_REQUIRES(std::is_pointer_v<T>)>
const void* debugExprCast(T p) {
    return fmt::ptr(p);
}

template<typename T>
const void* debugExprCast(const std::unique_ptr<T>& p) {
    return fmt::ptr(p);
}

template<typename T>
const void* debugExprCast(const std::shared_ptr<T>& p) {
    return fmt::ptr(p);
}

} // namespace vgc::core::detail

/// Prints the result of an expression.
///
/// ```cpp
/// int x = 2;
/// int y = 40;
/// VGC_DEBUG_TMP_EXPR(x);    // Prints "x = 2"
/// VGC_DEBUG_TMP_EXPR(x+y);  // Prints "x+y = 42"
/// ```
///
/// This is essentially equivalent to `VGC_DEBUG_TMP("expr = {}", expr)`, with
/// an automatic cast to `void*` in the case where `expr` evaluates to a
/// pointer type, which is required for proper formatting of pointer types.
///
#define VGC_DEBUG_TMP_EXPR(expr)                                                         \
    VGC_DEBUG_TMP(#expr " = {}", vgc::core::detail::debugExprCast(expr))

#endif // VGC_CORE_LOGGING_H
