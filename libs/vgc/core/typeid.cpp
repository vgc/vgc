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

#include <vgc/core/typeid.h>

#include <string_view>

#include <vgc/core/stringutil.h>

#if defined(VGC_COMPILER_CLANG) || defined(VGC_COMPILER_GCC)
#    include <cxxabi.h> // abi::__cxa_demangle
#endif

namespace vgc::core::detail {

namespace {

std::string demangleTypeInfoName(const char* name) {
#if defined(VGC_COMPILER_CLANG) || defined(VGC_COMPILER_GCC)
    int status = 0;
    char* demangled = abi::__cxa_demangle(name, 0, 0, &status);
    std::string res = demangled;
    free(demangled);
    return res;
#elif defined(VGC_COMPILER_MSVC)
    // Already demangled on MSVC, but prefixed by 'struct ', 'class ', or 'enum '
    using namespace std::literals::string_view_literals;
    std::string_view name_ = name;
    std::string_view res = name_;
    for (const auto& prefix : {"struct "sv, "class "sv, "enum "sv}) {
        if (startsWith(name_, prefix)) {
            res = name_.substr(prefix.size());
            break;
        }
    }
    return std::string(res);
#else
    return std::string(name);
#endif
}

// Extract unqualified type name from fully-qualified type name
std::string_view getUnqualifiedName(std::string_view fullName) {
    size_t i = fullName.rfind(':');    // index of last ':' character
    if (i == std::string_view::npos) { // if no ':' character
        return fullName;
    }
    else {
        return fullName.substr(i + 1);
    }
}

} // namespace

TypeInfo::TypeInfo(const std::type_info& info)
    : fullName(demangleTypeInfoName(info.name()))
    , name(getUnqualifiedName(fullName)) {
}

TypeId typeIdTestClass() {
    return typeId<TypeIdTestClass>();
}

TypeId typeIdInt() {
    return typeId<int>();
}

} // namespace vgc::core::detail
