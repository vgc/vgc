// Copyright 2023 The VGC Developers
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

#ifndef VGC_CORE_BOOLGUARD_H
#define VGC_CORE_BOOLGUARD_H

#include <vgc/core/api.h>

namespace vgc::core {

/// \class vgc::core::BoolGuard
/// \brief Sets a boolean to true on construction and restore it on destruction.
///
/// `BoolGuard` is a helper class following RAII principles for managing a
/// shared boolean status flag.
///
/// The `BoolGuard` constructor takes as input a non-const reference to a
/// boolean (i.e., a "shared boolean"), saves its current value `v`, and
/// modifies its value to `true`. Then, the `BoolGuard` destructor restores the
/// value of the shared boolean back to `v`.
///
/// Using a `BoolGuard` is often useful to protect potentially recursive or
/// mutually-recursive methods, by detecting whether there is already on the
/// call stack a call to a given method, for a given object.
///
/// Example:
///
/// ```cpp
/// class Printer {
/// public:
///     void print() {
///         if (isPrinting_) {
///             VGC_WARNING(LogPrinter, "Cannot call print(): already printing.");
///             return;
///         }
///         BoolGuard guard(isPrinting_); // BoolGuard constructor: sets isPrinting_ to true
///         [... do something ...]
///                                       // BoolGuard destructor: sets isPrinting_ to false
///     }
///
/// private:
///     bool isPrinting_ = false;
/// };
/// ```
///
class VGC_CORE_API BoolGuard {
public:
    /// Constructs a `BoolGuard` managing the shared boolean `ref`.
    ///
    /// This sets the value of `ref` to true.
    ///
    BoolGuard(bool& ref)
        : ref_(ref)
        , previousValue_(ref) {

        ref_ = true;
    }

    /// Destructs the `BoolGuard`.
    ///
    /// This restores the value of `ref` back to `previousValue()`, that is,
    /// the value that the boolean had before constructing this `BoolGuard`.
    ///
    ~BoolGuard() {
        ref_ = previousValue_;
    }

    /// Returns a non-const reference to the shared boolean managed by this
    /// `BoolGuard`.
    ///
    bool& ref() {
        return ref_;
    }

    /// Returns a const reference to the shared boolean managed by this
    /// `BoolGuard`.
    ///
    const bool& ref() const {
        return ref_;
    }

    /// Returns the value that the shared boolean had before being managed by
    /// this `BoolGuard`.
    ///
    bool previousValue() const {
        return previousValue_;
    }

private:
    bool& ref_;
    bool previousValue_;
};

} // namespace vgc::core

#endif // VGC_CORE_BOOLGUARD_H
