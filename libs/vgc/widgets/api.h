// Copyright 2017 The VGC Developers
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

#ifndef VGC_WIDGETS_API_H
#define VGC_WIDGETS_API_H

/// \file vgc/widgets/api.h
/// \brief Defines symbol visibility macros for defining shared library API.
///
/// See https://gcc.gnu.org/wiki/Visibility
///

#include <vgc/core/dll.h>

#if defined(VGC_WIDGETS_DLL)
#    if defined(VGC_WIDGETS_DLL_EXPORTS)
#        define VGC_WIDGETS_API VGC_CORE_DLL_EXPORT
#    else
#        define VGC_WIDGETS_API VGC_CORE_DLL_IMPORT
#    endif
#    define VGC_WIDGETS_API_HIDDEN VGC_CORE_DLL_HIDDEN
#else
#    define VGC_WIDGETS_API
#    define VGC_WIDGETS_API_HIDDEN
#endif

#endif // VGC_WIDGETS_API_H
