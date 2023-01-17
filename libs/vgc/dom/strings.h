// Copyright 2022 The VGC Developers
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

#ifndef VGC_DOM_STRINGS_H
#define VGC_DOM_STRINGS_H

#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>

namespace vgc::dom::strings {

VGC_DOM_API extern const core::StringId name;
VGC_DOM_API extern const core::StringId id;

VGC_DOM_API extern const core::StringId vgc;
VGC_DOM_API extern const core::StringId layer;
VGC_DOM_API extern const core::StringId vertex;
VGC_DOM_API extern const core::StringId edge;

VGC_DOM_API extern const core::StringId startvertex;
VGC_DOM_API extern const core::StringId endvertex;
VGC_DOM_API extern const core::StringId position;
VGC_DOM_API extern const core::StringId positions;
VGC_DOM_API extern const core::StringId widths;
VGC_DOM_API extern const core::StringId color;

VGC_DOM_API extern const core::StringId New_Document;
VGC_DOM_API extern const core::StringId Open_Document;

VGC_DOM_API extern const core::StringId Remove_Node;
VGC_DOM_API extern const core::StringId Move_Node_in_hierarchy;
VGC_DOM_API extern const core::StringId Create_Element;
VGC_DOM_API extern const core::StringId Remove_Element;
VGC_DOM_API extern const core::StringId Move_Element_in_hierarchy;

VGC_DOM_API extern const core::StringId Set_authored_attribute;
VGC_DOM_API extern const core::StringId Clear_authored_attribute;

} // namespace vgc::dom::strings

#endif // VGC_DOM_STRINGS_H
