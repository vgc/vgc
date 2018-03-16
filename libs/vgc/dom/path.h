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

#ifndef VGC_DOM_PATH_H
#define VGC_DOM_PATH_H

#include <vgc/dom/api.h>
#include <vgc/dom/element.h>

namespace vgc {
namespace dom {

/// \class vgc::dom::Path
/// \brief A <path> element of the Document.
///
class VGC_DOM_API Path: public Element
{
public:
    VGC_CORE_OBJECT(Path)

    /// Creates a new Element.
    ///
    Path();

    /// Returns the name of the element.
    ///
    std::string name() const {
        return "path";
    }
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_PATH_H
