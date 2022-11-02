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

#ifndef VGC_CORE_SHAREDPTR_H
#define VGC_CORE_SHAREDPTR_H

#include <memory>
#include <type_traits>

#include <vgc/core/api.h>
#include <vgc/core/format.h>
#include <vgc/core/templateutil.h>

namespace vgc::core {

//template<typename T, typename U, typename SFINEA>
//struct are_equality_comparable : std::false_type {};
//template<typename T, typename U>
//struct are_equality_comparable<
//    T,
//    U,
//    MakeVoid<decltype(std::declval<const T&>() == std::declval<const U&>())>>
//    : std::true_type {};

namespace detail {

template<typename T>
class SharedConstBase {
protected:
    SharedConstBase() = delete;
    ~SharedConstBase() = default;

public:
    using ValueType = T;

    template<typename... Args>
    explicit SharedConstBase(Args&&... args)
        : value_(std::make_shared<const T>(std::forward<Args>(args)...)) {
    }

    operator const T&() const noexcept {
        return *value_;
    }

    /// Returns a const reference to the shared value.
    ///
    const T& get() const noexcept {
        return *value_;
    }

    ValueType editableCopy() const {
        return ValueType(get());
    }

private:
    std::shared_ptr<const ValueType> value_;
};

} // namespace detail

template<typename T>
class SharedConst : public detail::SharedConstBase<T> {
public:
    // SharedConst<T> is specialized at least for core::Array but not in this header.
    // It could be instanciated with an incomplete type for which the specialization
    // may exist but is not known at the time of instanciation.
    // To prevent this, we specialize SharedConst<T> in the same header where T is defined.
    // Also, the following static assert ensures T is complete at the time of
    // instantiation of SharedConst<T>.
    //
    static_assert(std::is_destructible_v<T>);

    using detail::SharedConstBase<T>::SharedConstBase;
};

static_assert(std::is_copy_constructible_v<SharedConst<char>>);
static_assert(std::is_copy_assignable_v<SharedConst<char>>);

template<typename T>
constexpr bool operator==(const SharedConst<T>& a, const SharedConst<T>& b) {
    return a.get() == b.get();
}

template<typename T>
constexpr bool operator!=(const SharedConst<T>& a, const SharedConst<T>& b) {
    return a.get() != b.get();
}

template<typename T>
constexpr bool operator<(const SharedConst<T>& a, const SharedConst<T>& b) {
    return a.get() < b.get();
}

template<typename T>
constexpr bool operator>(const SharedConst<T>& a, const SharedConst<T>& b) {
    return a.get() > b.get();
}

template<typename T>
constexpr bool operator<=(const SharedConst<T>& a, const SharedConst<T>& b) {
    return a.get() <= b.get();
}

template<typename T>
constexpr bool operator>=(const SharedConst<T>& a, const SharedConst<T>& b) {
    return a.get() >= b.get();
}

template<typename T>
constexpr bool operator==(const SharedConst<T>& a, const T& b) {
    return a.get() == b;
}

template<typename T>
constexpr bool operator==(const T& a, const SharedConst<T>& b) {
    return a == b.get();
}

template<typename T>
constexpr bool operator!=(const SharedConst<T>& a, const T& b) {
    return a.get() != b;
}

template<typename T>
constexpr bool operator!=(const T& a, const SharedConst<T>& b) {
    return a != b.get();
}

template<typename T>
constexpr bool operator<(const SharedConst<T>& a, const T& b) {
    return a.get() < b;
}

template<typename T>
constexpr bool operator<(const T& a, const SharedConst<T>& b) {
    return a < b.get();
}

template<typename T>
constexpr bool operator>(const SharedConst<T>& a, const T& b) {
    return a.get() > b;
}

template<typename T>
constexpr bool operator>(const T& a, const SharedConst<T>& b) {
    return a > b.get();
}

template<typename T>
constexpr bool operator<=(const SharedConst<T>& a, const T& b) {
    return a.get() <= b;
}

template<typename T>
constexpr bool operator<=(const T& a, const SharedConst<T>& b) {
    return a <= b.get();
}

template<typename T>
constexpr bool operator>=(const SharedConst<T>& a, const T& b) {
    return a.get() >= b;
}

template<typename T>
constexpr bool operator>=(const T& a, const SharedConst<T>& b) {
    return a >= b.get();
}

namespace detail {

template<typename T>
struct IsSharedConst : std::false_type {};

template<typename T>
struct IsSharedConst<SharedConst<T>> : std::true_type {};

template<typename T>
struct RemoveSharedConst_ {
    using type = T;
};

template<typename T>
struct RemoveSharedConst_<SharedConst<T>> {
    using type = T;
};

} // namespace detail

template<typename T>
inline constexpr bool isSharedConst = detail::IsSharedConst<T>::value;

template<typename T>
using RemoveSharedConst = typename detail::RemoveSharedConst_<T>::type;

/// Writes the shared const value to the output stream.
///
template<typename OStream, typename T>
void write(OStream& out, const SharedConst<T>& v) {
    write(out, v.get());
}

} // namespace vgc::core

template<typename T>
struct fmt::formatter<vgc::core::SharedConst<T>> : fmt::formatter<T> {
    template<typename FormatContext>
    auto format(const vgc::core::SharedConst<T>& v, FormatContext& ctx)
        -> decltype(ctx.out()) {
        return fmt::formatter<T>::format(v.get(), ctx);
    }
};

#endif // VGC_CORE_SHAREDPTR_H
