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

#ifndef VGC_CORE_WRAPS_CLASS_H
#define VGC_CORE_WRAPS_CLASS_H

#include <memory>
#include <type_traits>

#include <vgc/core/wraps/common.h>

namespace vgc::core::wraps {

namespace detail {

template<typename CRTP, typename TClass, typename... Options>
class ClassDeclarator {
public:
    using PyClass = py::class_<TClass, Options...>;
    using Holder = typename PyClass::holder_type;

protected:
    template<typename... Extra>
    ClassDeclarator(py::handle scope, const char* name, const Extra&... extra)
        : c_(scope, name, extra...) {
    }

public:
#define VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(name)                                    \
    template<typename... Args>                                                           \
    CRTP& name(Args&&... args) {                                                         \
        c_.name(std::forward<Args>(args)...);                                            \
        return static_cast<CRTP&>(*this);                                                \
    }

    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_static)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_cast)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_buffer)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_readwrite)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_readonly)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_readwrite_static)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_readonly_static)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_property_readonly)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_property_readonly_static)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_property)
    VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD(def_property_static)

#undef VGC_CORE_WRAPS_CLASS_FWD_PYCLASS_METHOD

private:
    PyClass c_;
};

} // namespace detail

template<typename TClass, typename... Options>
class Class
    : public detail::ClassDeclarator<Class<TClass, Options...>, TClass, Options...> {

    using Base = detail::ClassDeclarator<Class<TClass, Options...>, TClass, Options...>;

public:
    template<typename... Extra>
    Class(py::handle scope, const char* name, const Extra&... extra)
        : Base(scope, name, extra...) {
    }
};

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_CLASS_H
