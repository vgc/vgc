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

#ifndef VGC_WORKSPACE_EDGEGEOMETRY_H
#define VGC_WORKSPACE_EDGEGEOMETRY_H

#include <vgc/dom/element.h>
#include <vgc/vacomplex/edgegeometry.h>
#include <vgc/workspace/api.h>

namespace vgc::workspace {

class VGC_WORKSPACE_API EdgeGeometry : public vacomplex::KeyEdgeGeometry {
public:
    std::unique_ptr<EdgeGeometry> cloneWorkspaceEdgeGeometry() {
        return std::unique_ptr<EdgeGeometry>(
            static_cast<EdgeGeometry*>(clone().release()));
    }

    virtual bool updateFromDomEdge_(dom::Element* element) = 0;
    virtual void writeToDomEdge_(dom::Element* element) const = 0;
    virtual void removeFromDomEdge_(dom::Element* element) const = 0;
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_EDGEGEOMETRY_H
