// Copyright 2023 The VGC Developers
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

#ifndef VGC_DOM_NONEOR_H
#define VGC_DOM_NONEOR_H

#include <optional>

#include <vgc/core/format.h>
#include <vgc/core/parse.h>
#include <vgc/dom/api.h>

namespace vgc::dom {

/// \class vgc::dom::NoneOr
/// \brief Used to extend a type to support "none".
///
template<typename T>
class NoneOr : public std::optional<T> {
private:
    using Base = std::optional<T>;

public:
    using Base::Base;

    template<typename U = T>
    constexpr explicit NoneOr(U&& value)
        : Base(std::forward<U>(value)) {
    }

    template<typename U>
    NoneOr& operator=(U&& x) {
        Base::operator=(std::forward<U>(x));
        return *this;
    }

    template<class U>
    friend constexpr bool operator==(const NoneOr<U>& lhs, const NoneOr<U>& rhs) {
        return operator==(
            static_cast<const std::optional<U>&>(lhs),
            static_cast<const std::optional<U>&>(rhs));
    }

    template<class U>
    friend constexpr bool operator!=(const NoneOr<U>& lhs, const NoneOr<U>& rhs) {
        return operator!=(
            static_cast<const std::optional<U>&>(lhs),
            static_cast<const std::optional<U>&>(rhs));
    }

    template<class U>
    friend constexpr bool operator<(const NoneOr<U>& lhs, const NoneOr<U>& rhs) {
        return operator<(
            static_cast<const std::optional<U>&>(lhs),
            static_cast<const std::optional<U>&>(rhs));
    }
};

template<typename OStream, typename T>
void write(OStream& out, const NoneOr<T>& v) {
    if (v.has_value()) {
        write(out, v.value());
    }
    else {
        write(out, "none");
    }
}

template<typename IStream, typename T>
void readTo(NoneOr<T>& v, IStream& in) {
    // check for "none"
    std::string peek = "????";
    Int numGot = 0;
    for (size_t i = 0; i < peek.size(); ++i) {
        if (in.get(peek[i])) {
            ++numGot;
        }
        else {
            break;
        }
    }
    if (peek == "none") {
        char c = 0;
        if (in.get(c)) {
            ++numGot;
            if (core::isWhitespace(c)) {
                core::skipWhitespaceCharacters(in);
                v.reset();
                return;
            }
        }
        else {
            v.reset();
            return;
        }
    }

    for (Int i = 0; i < numGot; ++i) {
        in.unget();
    }

    readTo(v.emplace(), in);
}

} // namespace vgc::dom

template<typename T>
struct fmt::formatter<vgc::dom::NoneOr<T>> : fmt::formatter<T> {
    template<typename FormatContext>
    auto format(const vgc::dom::NoneOr<T>& v, FormatContext& ctx) -> decltype(ctx.out()) {
        if (v.has_value()) {
            return fmt::formatter<T>::format(v.value(), ctx);
        }
        else {
            return fmt::format_to(ctx.out(), "none");
        }
    }
};

#endif // VGC_DOM_NONEOR_H
