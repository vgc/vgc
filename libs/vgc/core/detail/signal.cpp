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
std::unordered_map<TypeId, FunctionId> typesMap;
thread_local Object* emitter = nullptr;

} // namespace

FunctionId genFunctionId() {
    // XXX make this thread-safe ?
    return ++lastId;
}

FunctionId genFunctionId(TypeId ti) {
    // XXX make this thread-safe ?
    FunctionId& id = typesMap[ti];
    if (id == 0) {
        id = ++lastId;
    }
    return id;
}

// thread-local value
Object* currentEmitter() {
    return emitter;
}

// Must be called after sender `onDestroyed()` call.
void SignalHub::disconnectSignals(const Object* sender) {

    auto& hub = access(sender);
    if (!hub.emitting_) {
        // We sort connections by receiver so that we can fully disconnect each
        // receiver only once (getListenedObjectInfoRef_ is slow).
        struct {
            bool operator()(const Connection_& a, const Connection_& b) const {
                if (std::holds_alternative<ObjectSlotId>(a.to)) {
                    if (std::holds_alternative<ObjectSlotId>(b.to)) {
                        return std::get<ObjectSlotId>(a.to).first
                               < std::get<ObjectSlotId>(b.to).first;
                    }
                    return true;
                }
                return false;
            }
        } ByReceiver;
        std::sort(hub.connections_.begin(), hub.connections_.end(), ByReceiver);
    }

    Object* prevDisconnectedReceiver = nullptr;
    for (auto& c : hub.connections_) {
        if (c.pendingRemoval) {
            continue;
        }
        c.pendingRemoval = true;
        hub.pendingRemovals_ = true;
        // Let's reset the info about this object which is stored in receiver object.
        if (std::holds_alternative<ObjectSlotId>(c.to)) {
            const auto& osid = std::get<ObjectSlotId>(c.to);
            if (osid.first != prevDisconnectedReceiver) {
                prevDisconnectedReceiver = osid.first;
                auto& info = access(osid.first).getListenedObjectInfoRef_(sender);
                info.numInboundConnections = 0;
            }
        }
    }

    // If possible clear connections_ now.
    if (!hub.emitting_) {
        hub.connections_.clear();
        hub.pendingRemovals_ = false;
    }
}

ConnectionHandle SignalHub::connect(
    const Object* sender,
    SignalId from,
    SignalTransmitter&& transmitter,
    const SlotId& to) {

    auto& hub = access(sender);
    auto handle = ConnectionHandle::generate();

    if (std::holds_alternative<ObjectSlotId>(to)) {
        // Increment numInboundConnections in the receiver's info about sender.
        const auto& bsid = std::get<ObjectSlotId>(to);
        auto& info = access(bsid.first).findOrCreateListenedObjectInfo_(sender);
        info.numInboundConnections++;
    }

    hub.connections_.emplaceLast(std::move(transmitter), handle, from, to);
    return handle;
}

void SignalHub::debugInboundConnections(const Object* receiver) {
    auto& hub = access(receiver);
    for (auto& info : hub.listenedObjectInfos_) {
        if (info.numInboundConnections <= 0) {
            continue;
        }
        const Object* sender = info.object;
        auto& shub = access(sender);
        Int count = 0;
        for (auto& c : shub.connections_) {
            if (c.pendingRemoval) {
                continue;
            }
            if (std::holds_alternative<ObjectSlotId>(c.to)) {
                const auto& bsid = std::get<ObjectSlotId>(c.to);
                if (bsid.first == receiver) {
                    ++count;
                }
            }
        }
        //VGC_DEBUG_TMP(
        //    "{}/{} connections of {} are to {}",
        //    count,
        //    shub.connections_.length(),
        //    sender->className(),
        //    receiver->className());
        VGC_ASSERT(count == info.numInboundConnections);
    }
}

Int SignalHub::numOutboundConnections_() {
    if (pendingRemovals_) {
        Int count = 0;
        for (auto& c : connections_) {
            if (!c.pendingRemoval) {
                ++count;
            }
        }
        return count;
    }
    return connections_.length();
}

Int SignalHub::numOutboundConnectionsTo_(const Object* receiver) {
    if (pendingRemovals_) {
        Int count = 0;
        for (auto& c : connections_) {
            if (!c.pendingRemoval && std::holds_alternative<ObjectSlotId>(c.to)
                && std::get<ObjectSlotId>(c.to).first == receiver) {
                ++count;
            }
        }
        return count;
    }
    return connections_.length();
}

Int SignalHub::disconnectListenedObject_(
    const Object* receiver,
    ListenedObjectInfo_& info) {

    if (info.numInboundConnections <= 0) {
        return 0;
    }

    const Object* sender = info.object;
    auto& hub = access(sender);
    Int count = 0;
    for (auto& c : hub.connections_) {
        bool shouldRemove = std::holds_alternative<ObjectSlotId>(c.to)
                            && std::get<ObjectSlotId>(c.to).first == receiver;
        if (shouldRemove && !c.pendingRemoval) {
            c.pendingRemoval = true;
            ++count;
        }
    }
    if (count > 0) {
        hub.pendingRemovals_ = true;
    }

#ifdef VGC_DEBUG_BUILD
    if (count != info.numInboundConnections) {
        throw LogicError("Erased connections count != info.numInboundConnections.");
    }
#endif

    info.numInboundConnections = 0;
    return count;
}

SignalHub::ListenedObjectInfo_&
SignalHub::findOrCreateListenedObjectInfo_(const Object* object) {

    ListenedObjectInfo_* freeInfo = nullptr;
    for (ListenedObjectInfo_& x : listenedObjectInfos_) {
        if (x.object == object) {
            return x;
        }
        if (x.numInboundConnections == 0) {
            freeInfo = &x;
        }
    }
    auto& info = freeInfo ? *freeInfo : listenedObjectInfos_.emplaceLast();
    info.object = object;
    return info;
}

void SignalHub::emit_(SignalId from, const TransmitArgs& args) {
    auto& connections = connections_;
    bool outermostEmit = (emitting_ == false);
    emitting_ = true;
    // Keep weak pointer on owner to detect our own death (`this` becomes dangling).
    ObjectPtr ownerPtr(owner_);
    Object* outerEmitter = emitter;
    emitter = owner_;
    // We do it by index because connect() can happen in transmit..
    for (Int i = 0; i < connections.length(); ++i) {
        const Connection_& c = connections[i];
        if ((c.from == from) && !c.pendingRemoval) {
            c.transmitter.transmit(args);
        }
        if (owner_->isDestroyed()) {
            // We got killed.
            emitter = outerEmitter;
            return;
        }
    }
    if (outermostEmit) {
        // In a second pass if this is the outermost emit of this signal hub
        // we remove connections that have been disconnected and pending for removal.
        if (pendingRemovals_) {
            connections.removeIf([](const Connection_& c) { return c.pendingRemoval; });
            pendingRemovals_ = false;
        }
        emitting_ = false;
    }
    emitter = outerEmitter;
}

} // namespace vgc::core::detail
