// Copyright 2020 The VGC Developers
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

#ifndef VGC_CORE_LOGGING_H
#define VGC_CORE_LOGGING_H

#include <iostream>
#include <vgc/core/api.h>

// XXX TODO: Creates a Logger class deriving from Object, and allow observers
// to listen to emitted warning/error, etc., so that we can report them within
// the application.
//
// Note: I may have to use macros to automatically log in which function the
// error occurs.

namespace vgc {
namespace core {

/// Returns an output stream that can be used to issue warnings.
///
VGC_CORE_API
std::ostream& warning();

} // namespace core
} // namespace vgc

#endif // VGC_CORE_LOGGING_H
