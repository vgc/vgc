// Copyright 2024 The VGC Developers
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

#include <vgc/canvas/debugdraw.h>

namespace vgc::canvas {

void debugDraw(core::StringId id, DebugDrawFunction function) {
    auto locked = detail::lockDebugDraws();
    detail::debugDraws().append({id, function});
}

void debugDrawClear(core::StringId id) {
    auto locked = detail::lockDebugDraws();
    detail::debugDraws().removeIf(         //
        [id](const detail::DebugDraw& x) { //
            return x.id == id;
        });
}

namespace detail {

core::Array<DebugDraw>& debugDraws() {
    static core::Array<DebugDraw> debugDraws_;
    return debugDraws_;
};

std::lock_guard<std::mutex> lockDebugDraws() {
    static std::mutex mutex;
    return std::lock_guard<std::mutex>(mutex);
}

} // namespace detail

} // namespace vgc::canvas
