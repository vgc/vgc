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

#include <unordered_map>

#include <vgc/core/object.h>

namespace vgc::core::detail {

ConnectionHandle ConnectionHandle::generate() {
    static ConnectionHandle s = {0};
    // XXX make this thread-safe ?
    return {++s.id_};
}

namespace {

FunctionId lastId = 0;
std::unordered_map<std::type_index, FunctionId> typesMap;

} // namespace

FunctionId genFunctionId() {
    // XXX make this thread-safe ?
    return ++lastId;
}

FunctionId genFunctionId(std::type_index ti) {
    // XXX make this thread-safe ?
    FunctionId& id = typesMap[ti];
    if (id == 0) {
        id = ++lastId;
    }
    return id;
}

void SignalHub::emit_(SignalId from, const TransmitArgs& args) {
    auto& connections = connections_;
    bool outermostEmit = (emitting_ == false);
    emitting_ = true;
    // Keep weak pointer on owner to detect our own death (`this` becomes dangling).
    ObjectPtr ownerPtr(owner_);
    // We do it by index because connect() can happen in transmit..
    for (Int i = 0; i < connections.length(); ++i) {
        const Connection_& c = connections[i];
        if ((c.from == from) && !c.pendingRemoval) {
            c.transmitter.transmit(args);
        }
        if (owner_->isDestroyed()) {
            // We got killed.
            return;
        }
    }
    if (outermostEmit) {
        // In a second pass if this is the outermost emit of this signal hub
        // we remove connections that have been disconnected and pending for removal.
        if (pendingRemovals_) {
            connections.removeIf([](const Connection_& c) { return c.pendingRemoval; });
        }
        emitting_ = false;
    }
}

} // namespace vgc::core::detail
