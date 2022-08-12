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

#ifndef VGC_UI_EVENT_H
#define VGC_UI_EVENT_H

#include <vgc/core/innercore.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Event);

/// \class vgc::ui::Event
/// \brief Base class of all events handled in the user interface.
///
class VGC_UI_API Event : public core::Object {
private:
    VGC_OBJECT(Event, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// This is an implementation detail. please use Event::create() instead.
    ///
    Event();

public:
    /// Creates an Event.
    ///
    static EventPtr create();
};

} // namespace vgc::ui

#endif // VGC_UI_EVENT_H
