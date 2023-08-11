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

#ifndef VGC_CORE_WRAPS_WRAP_SHAREDCONST_H
#define VGC_CORE_WRAPS_WRAP_SHAREDCONST_H

#include <memory>
#include <utility>

#include <vgc/core/sharedconst.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>

namespace vgc::core::wraps {

namespace detail {

// Note: it currently allows conversion to Array&.
// If we want to ensure immutability of const references we need immutable variants
// of wrappers.
// See https://github.com/pybind/pybind11/issues/717
//
template<typename ValueType>
void wrapSharedConstImplicitCast() {

    using SharedConstType = SharedConst<ValueType>;
    using SharedConstTypeCaster = py::detail::make_caster<SharedConstType>;
    using OutputCaster = py::detail::make_caster<const ValueType>;

    struct scopedFlag {
        bool& flag;
        explicit scopedFlag(bool& flag_)
            : flag(flag_) {

            flag_ = true;
        }
        ~scopedFlag() {
            flag = false;
        }
    };

    auto implicitCaster = [](PyObject* obj, PyTypeObject* /*type*/) -> PyObject* {
        static bool currentlyUsed = false;
        if (currentlyUsed) { // non-reentrant
            return nullptr;
        }

        scopedFlag flagLock(currentlyUsed);

        SharedConstTypeCaster inputCaster = {};
        if (!inputCaster.load(obj, false)) {
            return nullptr;
        }

        SharedConstType* input = reinterpret_cast<SharedConstType*>(inputCaster.value);

        py::handle h = OutputCaster::cast(
            input->get(), py::return_value_policy::reference_internal, obj);

        if (!h) {
            PyErr_Clear();
        }

        return h.ptr();
    };

    if (auto tinfo = py::detail::get_type_info(typeid(ValueType))) {
        // Note: This conversion is better than standard conversions since it
        //       does not construct a new Array. To make it higher priority we
        //       push it in front of the conversions list.
        tinfo->implicit_conversions.insert(
            tinfo->implicit_conversions.begin(), implicitCaster);
    }
    else {
        py::pybind11_fail(
            "wrapSharedConstImplicitCast: Unable to find type "
            + py::type_id<ValueType>());
    }
}

} // namespace detail

// Defines methods common to SharedConst<T> types.
//
template<typename ValueType>
void defineSharedConstCommonMethods(Class<core::SharedConst<ValueType>>& c) {

    using T = core::SharedConst<ValueType>;

    //detail::wrapSharedConstImplicitCast<ValueType>();

    //py::implicitly_convertible<ValueType, T>();

    if constexpr (std::is_copy_constructible_v<ValueType>) {
        c.def(py::init<const T&>());
        c.def("editableCopy", [](const T& a) -> std::unique_ptr<ValueType> {
            return std::make_unique<ValueType>(a.get());
        });
    }
}

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_WRAP_SHAREDCONST_H
