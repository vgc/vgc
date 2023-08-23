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

#ifndef VGC_TOOLS_COPYPASTE_H
#define VGC_TOOLS_COPYPASTE_H

#include <vgc/tools/api.h>
#include <vgc/ui/command.h>

namespace vgc::tools::commands {

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(cut)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(copy)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(paste)

VGC_TOOLS_API
VGC_UI_DECLARE_COMMAND(duplicate)

} // namespace vgc::tools::commands

#endif // VGC_TOOLS_COPYPASTE_H
