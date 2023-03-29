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

#ifndef VGC_UI_MACOSPERMISSIONS_H
#define VGC_UI_MACOSPERMISSIONS_H

#include <vgc/core/os.h>

#ifdef VGC_CORE_OS_MACOS

namespace vgc::ui {

/// Returns whether the user has granted accessibility permissions to your
/// your application.
///
/// Such permissions are required for some functionality to be allowed, for
/// example, `setGlobalCursorPosition()`.
///
/// This function is only available on macOS.
///
bool hasAccessibilityPermissions();

} // namespace vgc::ui

#endif // VGC_CORE_OS_MACOS

#endif // VGC_UI_MACOSPERMISSIONS_H
