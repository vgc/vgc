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

#ifndef VGC_CORE_PROFILE_H
#define VGC_CORE_PROFILE_H

#include <vgc/core/compiler.h>
#include <vgc/core/logging.h>
#include <vgc/core/stopwatch.h>

namespace vgc::core::internal {

struct FormatDuration {
    double durationInSeconds;
};

struct FormatIndentation {
    int indentLevel;
};

} // namespace vgc::core::internal

template <>
struct fmt::formatter<vgc::core::internal::FormatDuration> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }
    template <typename FormatContext>
    auto format(vgc::core::internal::FormatDuration d, FormatContext& ctx) {
        double s = d.durationInSeconds;
        if      (s < 1e-6) { return format_to(ctx.out(),"{:>6.2f} ns", s * 1e9); }
        else if (s < 1e-3) { return format_to(ctx.out(),"{:>6.2f} us", s * 1e6); }
        else if (s < 1)    { return format_to(ctx.out(),"{:>6.2f} ms", s * 1e3); }
        else               { return format_to(ctx.out(),"{:>6.2f} s ", s      ); }
    }
};

template <>
struct fmt::formatter<vgc::core::internal::FormatIndentation> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }
    template <typename FormatContext>
    auto format(vgc::core::internal::FormatIndentation indent, FormatContext& ctx) {
        return format_to(ctx.out(),"{:>{}}", " ", 2 * indent.indentLevel);
    }
};

namespace vgc::core::internal {

class VGC_CORE_API VgcProfileFunction {
public:
    VgcProfileFunction(const char* function) : function_(function) {
        indent_ += 1;
    }

    ~VgcProfileFunction() {
        indent_ -= 1;
        VGC_DEBUG_TMP("{} > {}        {}",
                      FormatIndentation{indent_},
                      FormatDuration{stopwatch_.elapsed()},
                      function_);
    }

private:
    static int indent_;
    const char* function_;
    Stopwatch stopwatch_;
};

} // namespace vgc::core::internal

#define VGC_PROFILE_FUNCTION \
    ::vgc::core::internal::VgcProfileFunction vgcProfileFunction_(VGC_PRETTY_FUNCTION); \



/*
#define VGC_PROFILE_FUNCTION                                           \
    struct VgcProfileFunction_ {                                       \
        const char* function = VGC_PRETTY_FUNCTION;                    \
        ::vgc::core::Stopwatch sw;                                     \
        VgcProfileFunction_() {                                    \
            ::vgc::core::internal::VgcProfileFunctionGlobal::incrIndent(); \
        } \
        ~VgcProfileFunction_() {                                       \
            ::vgc::core::internal::VgcProfileFunctionGlobal::decrIndent(); \
            VGC_DEBUG_TMP(                                                 \
                "{}{}: {}", \
                ::vgc::core::internal::VgcProfileFunctionGlobal::indent(),\
                function,                   \
                ::vgc::core::internal::PrettyPrintDuration{sw.elapsed()}); \
        }                                                                  \
    } vgcProfileFunction_;

    */
#endif // VGC_CORE_PROFILE_H
