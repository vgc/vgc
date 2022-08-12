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

#ifndef VGC_GRAPHICS_IDGENERATOR_H
#define VGC_GRAPHICS_IDGENERATOR_H

#include <vgc/core/array.h>
#include <vgc/graphics/api.h>

namespace vgc::graphics {

/// \class vgc::graphics::IdGenerator
/// \brief Generates unique integers.
///
/// This class generates unique integer IDs by calling generate(), starting
/// from 0, and increasing sequentially (1, 2, 3, etc.). IDs that are not used
/// anymore can be manually released by calling release(id). The last released
/// ID will be used for the next call to generate().
///
/// This class is re-entrant, but not thread-safe. Please protect the calls to
/// generate and release with mutexes if you need to generate/release IDs
/// concurrently from multiple threads.
///
/// The behavior is undefined if clients call release(id) with IDs which hadn't
/// yet been generated, or if clients call release(id) multiple time without
/// having this ID re-generated yet.
///
class VGC_GRAPHICS_API IdGenerator {
public:
    IdGenerator()
        : largestGenerated_(-1) {
    }

    /// Generates and returns a new ID.
    ///
    Int generate() {
        if (released_.isEmpty()) {
            return ++largestGenerated_;
        }
        else {
            return released_.pop();
        }
    }

    /// Releases an already generated ID, so that it can be generated again.
    /// Note that released IDs are regenerated in a
    /// last-released-first-regenerated order, i.e., the released IDs are
    /// stored in a stack, and never-generated IDs only start being generated
    /// once the stack is empty.
    ///
    void release(Int id) {
        released_.append(id);
    }

private:
    Int largestGenerated_;
    core::IntArray released_;
};

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_IDGENERATOR_H
