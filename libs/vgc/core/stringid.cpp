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

#include <vgc/core/stringid.h>

#include <mutex>
#include <unordered_set>

namespace vgc::core {

StringId::StringId() {
    stringPtr_ = strings::empty.stringPtr_;
}

StringId::StringId(const std::string& s) {
    using StringPool = std::unordered_set<std::string>;

    // Declare a global string pool with static storage duration. We
    // intentionally "leak" the pool (and its mutex) to allow other objects
    // with static storage duration to safely use StringId instances in their
    // destructors even if they don't use them in their destructors. This idiom
    // is called "Leaky Singleton". For more details, read:
    //
    // https://isocpp.org/wiki/faq/ctors#construct-on-first-use-v2
    // https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables
    //
    // If leaking proves to be an issue, we may change this implementation to use the
    // Nifty Counter Idiom instead:
    //
    // https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Nifty_Counter
    //
    // We could also wrap these leaks in an "Immortal" class so that its
    // leakiness doesn't interfere with memory leak detection, and we can move
    // this whole comment in a more shared/appropriate place:
    //
    // https://www.reddit.com/r/cpp/comments/7j3s46/threadsafe_leaky_singleton/dr3scmq/
    //
    static std::mutex* mutex = new std::mutex;
    static StringPool* pool = new StringPool;

    std::lock_guard<std::mutex> lock(*mutex);

    // Insert in pool. Note: insertions in an unordered_set invalidates
    // iterators but does not invalidate pointers to elements.
    auto ret = pool->insert(s);

    // Store pointer to std:string.
    auto iterator = ret.first;
    stringPtr_ = &(*iterator);
}

namespace strings {

const StringId empty("");

} // namespace strings

} // namespace vgc::core
