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

#ifndef VGC_UI_EVENT_H
#define VGC_UI_EVENT_H

#include <vgc/core/innercore.h>
#include <vgc/ui/api.h>

namespace vgc {
namespace ui {

VGC_CORE_DECLARE_PTRS(Event);

/// \class vgc::ui::Event
/// \brief Base class of all events handled in the user interface.
///
class VGC_UI_API Event : public core::Object
{  
    VGC_CORE_OBJECT(Event)

public:
    /// This is an implementation detail. please use Event::create() instead.
    ///
    Event(const ConstructorKey&);

public:
    /// Creates an Event.
    ///
    static EventSharedPtr create();
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_EVENT_H
