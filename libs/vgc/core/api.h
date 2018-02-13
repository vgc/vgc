// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc-io/vgc/blob/master/COPYRIGHT
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

#ifndef VGC_CORE_API_H
#define VGC_CORE_API_H

#include <vgc/core/export.h>

#if defined(VGC_CORE_STATIC)
#   define VGC_CORE_API
#   define VGC_CORE_API_TEMPLATE_CLASS(...)
#   define VGC_CORE_API_TEMPLATE_STRUCT(...)
#   define VGC_CORE_LOCAL
#else
#   if defined(VGC_CORE_EXPORTS)
#       define VGC_CORE_API VGC_CORE_EXPORT
#       define VGC_CORE_API_TEMPLATE_CLASS(...) VGC_CORE_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define VGC_CORE_API_TEMPLATE_STRUCT(...) VGC_CORE_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define VGC_CORE_API VGC_CORE_IMPORT
#       define VGC_CORE_API_TEMPLATE_CLASS(...) VGC_CORE_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define VGC_CORE_API_TEMPLATE_STRUCT(...) VGC_CORE_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define VGC_CORE_LOCAL VGC_CORE_HIDDEN
#endif

#endif // VGC_CORE_API_H
