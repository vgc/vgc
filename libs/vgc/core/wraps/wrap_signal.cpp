// Copyright 2021 The VGC Developers
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

#include <pybind11/functional.h>

#include <vgc/core/wraps/common.h>
#include <vgc/core/signal.h>
#include <vgc/core/object.h>

// wrapping Signal as an object instead of a class requires:
//  - shared state, so that emitting on a copied signal does signal on the same connections!

// signals should live either with their owner object, or by themselves..
// -> shared_ptr to state VS shared_ptr to owner.

// emit:
// obj->signal(./->)emit()


class TestSignalsObject : vgc::core::Object
{
  
};


void wrap_signal(py::module& m)
{
    using UnsharedOwnerSignal = vgc::core::internal::Signal<>;
    py::class_<UnsharedOwnerSignal> c(m, "Signal");

    /*c.def("connect",
        [](UnsharedOwnerSignal& a, const std::function<void()>& slot_func) {
            a.connect(slot_func);
        }
    );*/

    c.def("emit",
        [](UnsharedOwnerSignal& a) {
            a();
        }
    );

    c.def(py::init([]() {
        UnsharedOwnerSignal res;
        return res;
    }));
}
