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

#ifndef VGC_CORE_WRAPS_SIGNAL_H
#define VGC_CORE_WRAPS_SIGNAL_H

#include <functional>

#include <vgc/core/wraps/common.h>
#include <vgc/core/signal.h>
#include <vgc/core/object.h>

namespace vgc::core::wraps {

class AbstractCppSlotRef {
protected:
    AbstractCppSlotRef(const core::Object* obj, std::initializer_list<std::type_index> parameters) :
        obj_(const_cast<core::Object*>(obj)), parameters_(parameters) {}

public:
    const auto& parameters() const {
        return parameters_;
    }

    core::Object* object() const {
        return obj_;
    }

    virtual py::cpp_function getPyWrappedBoundSlot() const = 0;

protected:
    std::vector<std::type_index> parameters_;
    core::Object* obj_;
};

template<typename... SlotArgs>
class CppSlotRef : public AbstractCppSlotRef {
public:
    // Note: forwarded arguments as for transmitters
    using BoundSlotSig = void(SlotArgs&&...);
    using BoundSlot = std::function<BoundSlotSig>;

    using AbstractCppSlotRef::AbstractCppSlotRef;

    BoundSlot getBoundSlot() const {
        return bind_(object());
    }

    virtual py::cpp_function getPyWrappedBoundSlot() const override {
        return py::cpp_function(getBoundSlot());
    }

private:
    virtual BoundSlot bind_(Object* obj) const = 0;
};

template<typename Obj, typename... SlotArgs>
class CppSlotRefImpl : public CppSlotRef<SlotArgs...> {
    using SuperClass = CppSlotRef<SlotArgs...>;
    using MFnT = void (Obj::*)(SlotArgs...);
    using BoundSlot = typename SuperClass::BoundSlot;

public:
    CppSlotRefImpl(const Obj* obj, MFnT mfn) :
        SuperClass(obj, { std::type_index(typeid(SlotArgs))... }),
        mfn_(mfn) {}

private:
    MFnT mfn_;

    virtual BoundSlot bind_(core::Object* obj) const override {
        // XXX can add alive check here or in the lambda..
        using MFnT = void (Obj::*)(SlotArgs...);
        return [=](SlotArgs&&... args) {
            (static_cast<Obj*>(obj)->*mfn_)(std::forward<SlotArgs>(args)...);
        };
    }
};






// to be used with connect
// should define emit(...)

class PySignal {
private:
    using ImplFn = void (*)(Object*, const py::args& args);

public:
    template<typename... Args>
    PySignal(Object* o, Signal<Args...>* s) :
        o_(o), s_(s) {
        f_ = emitImpl<Args...>;
    }

private:
    Object* o_;
    void* s_;
    ImplFn f_;

    template<typename... Args>
    static void emitImpl(Object* o, const py::args& args)
    {
        if (!o->isAlive()) {
            throw vgc::core::NotAliveError(obj);
        }

    }
};

} // namespace vgc::core::wraps

#endif // VGC_CORE_WRAPS_SIGNAL_H
